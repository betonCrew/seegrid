//! Art-Net node: receives ArtDmx for one universe and answers ArtPoll,
//! so controllers (Resolume, QLC+, ...) list the bridge as an output.

use std::io;
use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4, UdpSocket};
use std::thread;
use std::time::{Duration, Instant};

use socket2::{Domain, Protocol, Socket, Type};

use crate::state::{SharedState, CHANNELS};

pub const PORT: u16 = 6454;
const ID: &[u8; 8] = b"Art-Net\0";
const OP_POLL: u16 = 0x2000;
const OP_POLL_REPLY: u16 = 0x2100;
const OP_DMX: u16 = 0x5000;
/// ESTA prototype manufacturer id (0x7FF0..0x7FFF), until seegrid has its own
const ESTA_MAN: u16 = 0x7FF0;

#[derive(Debug, PartialEq)]
pub enum Packet<'a> {
    Poll,
    /// `universe` is the 15 bit Port-Address (Net:SubNet:Universe)
    Dmx { universe: u16, data: &'a [u8] },
}

pub fn parse(buf: &[u8]) -> Option<Packet<'_>> {
    if buf.len() < 10 || &buf[..8] != ID {
        return None;
    }
    match u16::from_le_bytes([buf[8], buf[9]]) {
        OP_POLL => Some(Packet::Poll),
        OP_DMX if buf.len() >= 18 => {
            let universe = u16::from_le_bytes([buf[14], buf[15]]) & 0x7FFF;
            let len = (u16::from_be_bytes([buf[16], buf[17]]) as usize).min(CHANNELS);
            let data = buf.get(18..18 + len)?;
            Some(Packet::Dmx { universe, data })
        }
        _ => None,
    }
}

/// ArtPollReply (Art-Net 4, 239 bytes) for a node with one DMX output port.
pub fn poll_reply(ip: Ipv4Addr, universe: u16, report: &str) -> [u8; 239] {
    let mut r = [0u8; 239];
    r[..8].copy_from_slice(ID);
    r[8..10].copy_from_slice(&OP_POLL_REPLY.to_le_bytes());
    r[10..14].copy_from_slice(&ip.octets());
    r[14..16].copy_from_slice(&PORT.to_le_bytes());
    r[16..18].copy_from_slice(&1u16.to_be_bytes()); // firmware version
    r[18] = ((universe >> 8) & 0x7F) as u8; // NetSwitch
    r[19] = ((universe >> 4) & 0x0F) as u8; // SubSwitch
    r[20..22].copy_from_slice(&0x00FFu16.to_be_bytes()); // OEM unknown
    r[23] = 0xD0; // Status1: indicators normal, addresses set locally
    r[24..26].copy_from_slice(&ESTA_MAN.to_le_bytes());
    put_str(&mut r[26..44], "seegrid");
    put_str(&mut r[44..108], "seegrid bridge (Art-Net -> RF gateway)");
    put_str(&mut r[108..172], report);
    r[172..174].copy_from_slice(&1u16.to_be_bytes()); // NumPorts
    r[174] = 0x80; // port 0: outputs DMX512 from Art-Net
    r[182] = 0x80; // GoodOutput: data is being output
    r[190] = (universe & 0x0F) as u8; // SwOut
    r[207..211].copy_from_slice(&ip.octets()); // BindIp
    r[211] = 1; // BindIndex
    r[212] = 0x08; // Status2: 15 bit Port-Address
    r
}

fn put_str(dst: &mut [u8], s: &str) {
    let n = s.len().min(dst.len() - 1); // keep a terminating 0
    dst[..n].copy_from_slice(&s.as_bytes()[..n]);
}

/// Address this host uses to reach `peer` (to put into the ArtPollReply).
fn local_ip_towards(peer: SocketAddr) -> Ipv4Addr {
    let probe = UdpSocket::bind((Ipv4Addr::UNSPECIFIED, 0)).and_then(|s| {
        s.connect(peer)?;
        s.local_addr()
    });
    match probe {
        Ok(SocketAddr::V4(a)) if !a.ip().is_unspecified() => *a.ip(),
        _ => Ipv4Addr::LOCALHOST,
    }
}

fn bind() -> io::Result<UdpSocket> {
    let s = Socket::new(Domain::IPV4, Type::DGRAM, Some(Protocol::UDP))?;
    // other Art-Net software on this host may use the port as well
    s.set_reuse_address(true)?;
    s.set_broadcast(true)?;
    s.bind(&SocketAddrV4::new(Ipv4Addr::UNSPECIFIED, PORT).into())?;
    let s: UdpSocket = s.into();
    s.set_read_timeout(Some(Duration::from_millis(200)))?;
    Ok(s)
}

/// Starts the receiver thread. A bind error is reported in the shared state.
pub fn spawn(state: SharedState) {
    thread::spawn(move || {
        let sock = match bind() {
            Ok(s) => s,
            Err(e) => {
                state.lock().unwrap().artnet.error = Some(format!("bind UDP {PORT}: {e}"));
                return;
            }
        };
        state.lock().unwrap().artnet.bound = true;
        let mut buf = [0u8; 1024];
        loop {
            let (n, from) = match sock.recv_from(&mut buf) {
                Ok(r) => r,
                Err(e) if matches!(e.kind(), io::ErrorKind::WouldBlock | io::ErrorKind::TimedOut) => continue,
                Err(e) => {
                    state.lock().unwrap().artnet.error = Some(format!("receive: {e}"));
                    thread::sleep(Duration::from_millis(500));
                    continue;
                }
            };
            match parse(&buf[..n]) {
                Some(Packet::Poll) => {
                    let (universe, frames) = {
                        let mut st = state.lock().unwrap();
                        st.artnet.polls += 1;
                        (st.artnet.universe, st.artnet.frames)
                    };
                    let report = format!("#0001 [{frames:04}] seegrid OK");
                    let reply = poll_reply(local_ip_towards(from), universe, &report);
                    // controllers poll from port 6454; replying to the sender also serves tools using another port
                    let _ = sock.send_to(&reply, from);
                }
                Some(Packet::Dmx { universe, data }) => {
                    let mut st = state.lock().unwrap();
                    if universe != st.artnet.universe {
                        continue;
                    }
                    st.artnet_data[..data.len()].copy_from_slice(data);
                    st.artnet_data[data.len()..].fill(0);
                    st.artnet.source = Some(from);
                    st.artnet.frames += 1;
                    st.artnet.rate.tick(Instant::now());
                    st.artnet.error = None;
                }
                None => {}
            }
        }
    });
}

#[cfg(test)]
mod tests {
    use super::*;

    fn art_dmx(universe: u16, data: &[u8]) -> Vec<u8> {
        let mut p = ID.to_vec();
        p.extend_from_slice(&OP_DMX.to_le_bytes());
        p.extend_from_slice(&[0, 14, 1, 0]); // ProtVer 14, Sequence, Physical
        p.extend_from_slice(&universe.to_le_bytes()); // SubUni, Net
        p.extend_from_slice(&(data.len() as u16).to_be_bytes());
        p.extend_from_slice(data);
        p
    }

    #[test]
    fn parses_art_dmx() {
        let p = art_dmx(0x0123, &[1, 2, 3, 4]);
        assert_eq!(parse(&p), Some(Packet::Dmx { universe: 0x0123, data: &[1, 2, 3, 4] }));
    }

    #[test]
    fn rejects_truncated_and_foreign_packets() {
        let mut p = art_dmx(0, &[1, 2, 3, 4]);
        p.truncate(20);
        assert_eq!(parse(&p), None);
        assert_eq!(parse(b"Art-Nex\0\x00\x50"), None);
    }

    #[test]
    fn parses_poll() {
        let mut p = ID.to_vec();
        p.extend_from_slice(&OP_POLL.to_le_bytes());
        p.extend_from_slice(&[0, 14, 0, 0]);
        assert_eq!(parse(&p), Some(Packet::Poll));
    }

    #[test]
    fn poll_reply_fields() {
        let r = poll_reply(Ipv4Addr::new(192, 168, 1, 7), 0x0123, "ok");
        assert_eq!(&r[8..10], &[0x00, 0x21]);
        assert_eq!(&r[10..14], &[192, 168, 1, 7]);
        assert_eq!(&r[14..16], &[0x36, 0x19]);
        assert_eq!((r[18], r[19], r[190]), (0x01, 0x02, 0x03)); // Net 1, Sub 2, Uni 3
        assert_eq!(&r[26..33], b"seegrid");
        assert_eq!(r[174], 0x80);
    }
}
