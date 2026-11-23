//! Enttec DMX USB Pro framing, host -> widget:
//! `0x7E | label | len LSB | len MSB | data[len] | 0xE7`

use crate::state::Universe;

const SOM: u8 = 0x7E;
const EOM: u8 = 0xE7;
/// longest packet in the Enttec protocol
const MAX_LEN: usize = 600;
/// widget -> host: received packet (here: RDM response), data = status byte + packet
pub const LABEL_RECEIVED: u8 = 5;
/// "Output Only Send DMX Packet": data = start code + channels
pub const LABEL_SEND_DMX: u8 = 6;
/// RDM request: data = RDM packet from the start code to the checksum
pub const LABEL_SEND_RDM: u8 = 7;
/// RDM DISC_UNIQUE_BRANCH request
pub const LABEL_RDM_DISCOVERY: u8 = 11;
/// widget -> host: no RDM response
pub const LABEL_RDM_TIMEOUT: u8 = 12;
const START_CODE_DIMMER: u8 = 0x00;

pub fn packet(label: u8, data: &[u8]) -> Vec<u8> {
    let len = data.len() as u16;
    let mut p = Vec::with_capacity(data.len() + 5);
    p.extend_from_slice(&[SOM, label, len as u8, (len >> 8) as u8]);
    p.extend_from_slice(data);
    p.push(EOM);
    p
}

pub fn dmx_packet(channels: &Universe) -> Vec<u8> {
    let mut data = Vec::with_capacity(channels.len() + 1);
    data.push(START_CODE_DIMMER);
    data.extend_from_slice(channels);
    packet(LABEL_SEND_DMX, &data)
}

/// Splits a byte stream from the widget into (label, data) packets.
#[derive(Default)]
pub struct Parser {
    buf: Vec<u8>,
}

impl Parser {
    pub fn push(&mut self, bytes: &[u8]) {
        self.buf.extend_from_slice(bytes);
    }

    pub fn next_packet(&mut self) -> Option<(u8, Vec<u8>)> {
        loop {
            let start = self.buf.iter().position(|&b| b == SOM)?;
            self.buf.drain(..start);
            if self.buf.len() < 4 {
                return None;
            }
            let len = u16::from_le_bytes([self.buf[2], self.buf[3]]) as usize;
            if len > MAX_LEN {
                self.buf.drain(..1); // a stray SOM: don't wait for a bogus length
                continue;
            }
            if self.buf.len() < len + 5 {
                return None;
            }
            if self.buf[len + 4] != EOM {
                self.buf.drain(..1); // not a packet start: resync on the next SOM
                continue;
            }
            let label = self.buf[1];
            let data = self.buf[4..4 + len].to_vec();
            self.buf.drain(..len + 5);
            return Some((label, data));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn dmx_packet_layout() {
        let mut ch = [0u8; 512];
        ch[0] = 0x11;
        ch[511] = 0x22;
        let p = dmx_packet(&ch);
        assert_eq!(p.len(), 518);
        assert_eq!(&p[..5], &[0x7E, 6, 0x01, 0x02, 0x00]); // len 513 = 0x0201, start code 0
        assert_eq!(p[5], 0x11);
        assert_eq!(p[516], 0x22);
        assert_eq!(p[517], 0xE7);
    }

    #[test]
    fn parser_splits_and_resyncs() {
        let mut p = Parser::default();
        let mut stream = vec![0x00, 0x7E, 0x99]; // garbage, false start
        stream.extend(packet(LABEL_RDM_TIMEOUT, &[]));
        stream.extend(packet(LABEL_RECEIVED, &[0, 1, 2]));
        p.push(&stream[..6]);
        assert_eq!(p.next_packet(), None);
        p.push(&stream[6..]);
        assert_eq!(p.next_packet(), Some((LABEL_RDM_TIMEOUT, vec![])));
        assert_eq!(p.next_packet(), Some((LABEL_RECEIVED, vec![0, 1, 2])));
        assert_eq!(p.next_packet(), None);
    }
}
