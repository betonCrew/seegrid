//! RDM (ANSI E1.20) controller: discovery, polling and commands for the
//! fixtures behind the gateway. No I/O here: the link asks for the next
//! request and hands back the reply.

use std::collections::{BTreeMap, VecDeque};
use std::fmt;
use std::time::{Duration, Instant};

use crate::enttec::{LABEL_RDM_DISCOVERY, LABEL_SEND_RDM};

pub const MANUFACTURER: u16 = 0x7FF0; // ESTA prototype range, as in the gateway
const CONTROLLER_UID: Uid = Uid(((MANUFACTURER as u64) << 32) | 0x0000_0001);
const UID_ALL: Uid = Uid(0xFFFF_FFFF_FFFF);
const UID_MAX: u64 = 0xFFFF_FFFF_FFFE;

const SC_RDM: u8 = 0xCC;
const SUB_SC: u8 = 0x01;
const HEADER_LEN: usize = 24;

pub const CC_DISC: u8 = 0x10;
pub const CC_GET: u8 = 0x20;
pub const CC_SET: u8 = 0x30;
const RT_ACK: u8 = 0x00;
const RT_NACK: u8 = 0x02;

pub const PID_DISC_UNIQUE_BRANCH: u16 = 0x0001;
pub const PID_DISC_MUTE: u16 = 0x0002;
pub const PID_DISC_UN_MUTE: u16 = 0x0003;
pub const PID_DEVICE_INFO: u16 = 0x0060;
pub const PID_DMX_PERSONALITY: u16 = 0x00E0;
pub const PID_DMX_START_ADDRESS: u16 = 0x00F0;
pub const PID_SENSOR_VALUE: u16 = 0x0201;
pub const PID_IDENTIFY_DEVICE: u16 = 0x1000;
/// manufacturer specific (seegrid gateway): ms since the last heartbeat
pub const PID_LAST_SEEN: u16 = 0x8000;

const SENSOR_BATTERY: u8 = 0;
const SENSOR_DMX_RATE: u8 = 1;

const DISCOVERY_INTERVAL: Duration = Duration::from_secs(10);
/// safety net against a misbehaving responder
const DISCOVERY_MAX_STEPS: u32 = 4000;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub struct Uid(pub u64);

impl fmt::Display for Uid {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:04X}:{:08X}", self.0 >> 32, self.0 & 0xFFFF_FFFF)
    }
}

impl Uid {
    fn bytes(self) -> [u8; 6] {
        let b = self.0.to_be_bytes();
        [b[2], b[3], b[4], b[5], b[6], b[7]]
    }

    fn from_bytes(b: &[u8]) -> Uid {
        Uid(b[..6].iter().fold(0u64, |u, &x| (u << 8) | x as u64))
    }
}

/// RDM request packet, start code to checksum
pub fn encode(dest: Uid, tn: u8, cc: u8, pid: u16, pd: &[u8]) -> Vec<u8> {
    let mut p = Vec::with_capacity(HEADER_LEN + pd.len() + 2);
    p.extend_from_slice(&[SC_RDM, SUB_SC, (HEADER_LEN + pd.len()) as u8]);
    p.extend_from_slice(&dest.bytes());
    p.extend_from_slice(&CONTROLLER_UID.bytes());
    p.extend_from_slice(&[tn, 1, 0, 0, 0, cc]); // port 1, message count 0, sub device 0
    p.extend_from_slice(&pid.to_be_bytes());
    p.push(pd.len() as u8);
    p.extend_from_slice(pd);
    let sum = p.iter().fold(0u16, |s, &b| s.wrapping_add(b as u16));
    p.extend_from_slice(&sum.to_be_bytes());
    p
}

#[derive(Debug, PartialEq)]
pub struct Response {
    pub src: Uid,
    pub tn: u8,
    pub response_type: u8,
    pub cc: u8,
    pub pid: u16,
    pub pd: Vec<u8>,
}

impl Response {
    fn nack_reason(&self) -> Option<u16> {
        (self.response_type == RT_NACK && self.pd.len() >= 2).then(|| u16::from_be_bytes([self.pd[0], self.pd[1]]))
    }
}

#[derive(Debug, PartialEq)]
pub enum Reply {
    Timeout,
    Response(Response),
    /// DISC_UNIQUE_BRANCH: exactly one responder
    Discovered(Uid),
    /// DISC_UNIQUE_BRANCH: several responders (corrupt response)
    Collision,
    /// something unusable
    Invalid,
}

/// Decodes the data of an Enttec label 5 packet (status byte + received packet).
pub fn decode(data: &[u8]) -> Reply {
    let Some((_status, p)) = data.split_first() else { return Reply::Invalid };
    match p.first() {
        Some(0xFE) | Some(0xAA) => decode_discovery(p),
        Some(&SC_RDM) => decode_response(p),
        _ => Reply::Invalid,
    }
}

fn decode_discovery(p: &[u8]) -> Reply {
    let Some(sep) = p.iter().position(|&b| b == 0xAA) else { return Reply::Collision };
    let e = &p[sep + 1..];
    if e.len() < 16 {
        return Reply::Collision;
    }
    let uid: Vec<u8> = (0..6).map(|i| e[2 * i] & e[2 * i + 1]).collect();
    let checksum = ((e[12] & e[13]) as u16) << 8 | (e[14] & e[15]) as u16;
    let sum = e[..12].iter().fold(0u16, |s, &b| s.wrapping_add(b as u16));
    if sum == checksum { Reply::Discovered(Uid::from_bytes(&uid)) } else { Reply::Collision }
}

fn decode_response(p: &[u8]) -> Reply {
    if p.len() < HEADER_LEN + 2 || p[1] != SUB_SC {
        return Reply::Invalid;
    }
    let len = p[2] as usize;
    let pdl = p[23] as usize;
    if len < HEADER_LEN || len + 2 > p.len() || len != HEADER_LEN + pdl {
        return Reply::Invalid;
    }
    let sum = p[..len].iter().fold(0u16, |s, &b| s.wrapping_add(b as u16));
    if sum != u16::from_be_bytes([p[len], p[len + 1]]) {
        return Reply::Invalid;
    }
    Reply::Response(Response {
        src: Uid::from_bytes(&p[9..15]),
        tn: p[15],
        response_type: p[16],
        cc: p[20],
        pid: u16::from_be_bytes([p[21], p[22]]),
        pd: p[24..24 + pdl].to_vec(),
    })
}

/// What the monitor shows per fixture.
#[derive(Clone, Debug, Default)]
pub struct Fixture {
    pub dmx_address: Option<u16>,
    pub footprint: Option<u16>,
    pub personality: Option<u8>,
    pub firmware: Option<u32>,
    pub battery_mv: Option<u16>,
    pub dmx_fps: Option<u16>,
    pub identify: Option<bool>,
    /// last heartbeat age as reported by the gateway, and when that was asked
    pub last_seen: Option<(Duration, Instant)>,
    pub error: Option<String>,
}

impl Fixture {
    /// time since the gateway last heard from the fixture
    pub fn last_seen_age(&self, now: Instant) -> Option<Duration> {
        self.last_seen.map(|(age, at)| age + now.duration_since(at))
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Command {
    SetAddress(Uid, u16),
    SetPersonality(Uid, u8),
    Identify(Uid, bool),
    Discover,
}

#[derive(Clone, Debug, PartialEq)]
enum Op {
    UnmuteAll,
    Branch(u64, u64),
    Mute(Uid),
    Get(Uid, u16, Vec<u8>),
    Set(Uid, u16, Vec<u8>),
}

/// A request handed to the link.
pub struct Request {
    op: Op,
    pub tn: u8,
    pub label: u8,
    pub packet: Vec<u8>,
}

#[derive(Default, Clone, Copy)]
pub struct Stats {
    pub requests: u64,
    pub timeouts: u64,
    pub invalid: u64,
    pub discoveries: u64,
}

#[derive(Default)]
pub struct Controller {
    tn: u8,
    fixtures: BTreeMap<Uid, Fixture>,
    commands: VecDeque<Command>,
    /// open ranges of the running discovery (None: idle)
    discovery: Option<Vec<(u64, u64)>>,
    discovery_ops: VecDeque<Op>,
    discovery_steps: u32,
    last_discovery: Option<Instant>,
    polls: VecDeque<Op>,
    stats: Stats,
}

impl Controller {
    pub fn fixtures(&self) -> &BTreeMap<Uid, Fixture> {
        &self.fixtures
    }

    pub fn stats(&self) -> Stats {
        self.stats
    }

    pub fn discovering(&self) -> bool {
        self.discovery.is_some()
    }

    pub fn command(&mut self, cmd: Command) {
        self.commands.push_back(cmd);
    }

    pub fn next_request(&mut self, now: Instant) -> Option<Request> {
        let op = self.next_op(now)?;
        self.tn = self.tn.wrapping_add(1);
        let (dest, cc, pid, pd, label) = match &op {
            Op::UnmuteAll => (UID_ALL, CC_DISC, PID_DISC_UN_MUTE, vec![], LABEL_SEND_RDM),
            Op::Branch(lo, hi) => {
                let mut pd = Uid(*lo).bytes().to_vec();
                pd.extend_from_slice(&Uid(*hi).bytes());
                (UID_ALL, CC_DISC, PID_DISC_UNIQUE_BRANCH, pd, LABEL_RDM_DISCOVERY)
            }
            Op::Mute(uid) => (*uid, CC_DISC, PID_DISC_MUTE, vec![], LABEL_SEND_RDM),
            Op::Get(uid, pid, pd) => (*uid, CC_GET, *pid, pd.clone(), LABEL_SEND_RDM),
            Op::Set(uid, pid, pd) => (*uid, CC_SET, *pid, pd.clone(), LABEL_SEND_RDM),
        };
        self.stats.requests += 1;
        Some(Request { packet: encode(dest, self.tn, cc, pid, &pd), tn: self.tn, label, op })
    }

    fn next_op(&mut self, now: Instant) -> Option<Op> {
        // UI commands first
        while let Some(cmd) = self.commands.pop_front() {
            match cmd {
                Command::SetAddress(uid, a) => return Some(Op::Set(uid, PID_DMX_START_ADDRESS, a.to_be_bytes().to_vec())),
                Command::SetPersonality(uid, p) => return Some(Op::Set(uid, PID_DMX_PERSONALITY, vec![p])),
                Command::Identify(uid, on) => return Some(Op::Set(uid, PID_IDENTIFY_DEVICE, vec![on as u8])),
                Command::Discover => self.start_discovery(),
            }
        }
        // discovery
        if self.discovery.is_none() && self.last_discovery.is_none_or(|t| now.duration_since(t) >= DISCOVERY_INTERVAL) {
            self.start_discovery();
        }
        if let Some(ranges) = &mut self.discovery {
            if let Some(op) = self.discovery_ops.pop_front() {
                return Some(op);
            }
            match ranges.last() {
                Some(&(lo, hi)) if self.discovery_steps < DISCOVERY_MAX_STEPS => {
                    self.discovery_steps += 1;
                    return Some(Op::Branch(lo, hi));
                }
                _ => {
                    self.discovery = None;
                    self.last_discovery = Some(now);
                    self.stats.discoveries += 1;
                }
            }
        }
        // polling, round robin over the fixtures
        if self.polls.is_empty() {
            for &uid in self.fixtures.keys() {
                self.polls.extend([
                    Op::Get(uid, PID_DEVICE_INFO, vec![]),
                    Op::Get(uid, PID_SENSOR_VALUE, vec![SENSOR_BATTERY]),
                    Op::Get(uid, PID_SENSOR_VALUE, vec![SENSOR_DMX_RATE]),
                    Op::Get(uid, PID_LAST_SEEN, vec![]),
                    Op::Get(uid, PID_IDENTIFY_DEVICE, vec![]),
                ]);
            }
        }
        self.polls.pop_front()
    }

    fn start_discovery(&mut self) {
        self.discovery = Some(vec![(0, UID_MAX)]);
        self.discovery_ops = VecDeque::from([Op::UnmuteAll]);
        self.discovery_steps = 0;
    }

    pub fn on_reply(&mut self, req: &Request, reply: Reply, now: Instant) {
        match &reply {
            Reply::Timeout => self.stats.timeouts += 1,
            Reply::Invalid => self.stats.invalid += 1,
            Reply::Response(r) if r.tn != req.tn => {
                self.stats.invalid += 1;
                return;
            }
            _ => {}
        }
        match (&req.op, reply) {
            (Op::Branch(lo, hi), reply) => {
                let ranges = self.discovery.get_or_insert_with(Vec::new);
                match reply {
                    // mute it; the same range is asked again for the others
                    Reply::Discovered(uid) => self.discovery_ops.push_back(Op::Mute(uid)),
                    Reply::Collision if lo < hi => {
                        let mid = lo + (hi - lo) / 2;
                        ranges.pop();
                        ranges.push((mid + 1, *hi));
                        ranges.push((*lo, mid));
                    }
                    _ => {
                        ranges.pop();
                    }
                }
            }
            (Op::Mute(uid), Reply::Response(r)) if r.response_type == RT_ACK => {
                self.fixtures.entry(*uid).or_default();
            }
            (Op::Get(uid, pid, pd), Reply::Response(r)) => {
                let Some(fix) = self.fixtures.get_mut(uid) else { return };
                if r.response_type != RT_ACK {
                    fix.error = Some(format!("GET {pid:#06x}: NACK {:#06x}", r.nack_reason().unwrap_or(0)));
                    return;
                }
                let d = &r.pd;
                match *pid {
                    PID_DEVICE_INFO if d.len() >= 19 => {
                        fix.firmware = Some(u32::from_be_bytes([d[6], d[7], d[8], d[9]]));
                        fix.footprint = Some(u16::from_be_bytes([d[10], d[11]]));
                        fix.personality = Some(d[12]);
                        fix.dmx_address = Some(u16::from_be_bytes([d[14], d[15]]));
                    }
                    PID_SENSOR_VALUE if d.len() >= 3 => {
                        let v = u16::from_be_bytes([d[1], d[2]]);
                        match pd.first() {
                            Some(&SENSOR_BATTERY) => fix.battery_mv = Some(v),
                            Some(&SENSOR_DMX_RATE) => fix.dmx_fps = Some(v),
                            _ => {}
                        }
                    }
                    PID_LAST_SEEN if d.len() >= 4 => {
                        let ms = u32::from_be_bytes([d[0], d[1], d[2], d[3]]);
                        fix.last_seen = Some((Duration::from_millis(ms as u64), now));
                    }
                    PID_IDENTIFY_DEVICE if !d.is_empty() => fix.identify = Some(d[0] != 0),
                    _ => {}
                }
            }
            (Op::Set(uid, pid, pd), reply) => {
                let Some(fix) = self.fixtures.get_mut(uid) else { return };
                match reply {
                    Reply::Response(r) if r.response_type == RT_ACK => {
                        fix.error = None;
                        match *pid {
                            PID_DMX_START_ADDRESS => fix.dmx_address = Some(u16::from_be_bytes([pd[0], pd[1]])),
                            PID_IDENTIFY_DEVICE => fix.identify = Some(pd[0] != 0),
                            PID_DMX_PERSONALITY => {
                                fix.personality = Some(pd[0]);
                                fix.footprint = Some(crate::layout::footprint(pd[0]) as u16);
                            }
                            _ => {}
                        }
                    }
                    Reply::Response(r) => {
                        let what = match *pid {
                            PID_DMX_START_ADDRESS => "address",
                            PID_DMX_PERSONALITY => "personality",
                            _ => "identify",
                        };
                        fix.error = Some(match r.nack_reason() {
                            Some(0x0006) => format!("{what}: out of range"),
                            other => format!("{what}: NACK {:#06x}", other.unwrap_or(0)),
                        });
                    }
                    _ => fix.error = Some("no response".to_owned()),
                }
            }
            _ => {}
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Simulated gateway: proxies `fixtures`, answers like the firmware.
    struct FakeGateway {
        fixtures: Vec<(Uid, bool, u16)>, // uid, muted, address
    }

    impl FakeGateway {
        fn respond(&mut self, req: &[u8]) -> Reply {
            let dest = Uid::from_bytes(&req[3..9]);
            let cc = req[20];
            let pid = u16::from_be_bytes([req[21], req[22]]);
            let pd = &req[24..24 + req[23] as usize];
            let ack = |src: Uid, pd: Vec<u8>| {
                Reply::Response(Response { src, tn: req[15], response_type: RT_ACK, cc: cc + 1, pid, pd })
            };
            match (cc, pid) {
                (CC_DISC, PID_DISC_UN_MUTE) => {
                    self.fixtures.iter_mut().for_each(|f| f.1 = false);
                    Reply::Timeout
                }
                (CC_DISC, PID_DISC_UNIQUE_BRANCH) => {
                    let (lo, hi) = (Uid::from_bytes(&pd[..6]).0, Uid::from_bytes(&pd[6..]).0);
                    let hits: Vec<Uid> =
                        self.fixtures.iter().filter(|f| !f.1 && f.0 .0 >= lo && f.0 .0 <= hi).map(|f| f.0).collect();
                    match hits.len() {
                        0 => Reply::Timeout,
                        1 => decode(&discovery_bytes(hits[0])),
                        _ => Reply::Collision,
                    }
                }
                (CC_DISC, PID_DISC_MUTE) => match self.fixtures.iter_mut().find(|f| f.0 == dest) {
                    Some(f) => {
                        f.1 = true;
                        ack(dest, vec![0, 0])
                    }
                    None => Reply::Timeout,
                },
                (CC_SET, PID_DMX_START_ADDRESS) => {
                    let f = self.fixtures.iter_mut().find(|f| f.0 == dest).unwrap();
                    f.2 = u16::from_be_bytes([pd[0], pd[1]]);
                    ack(dest, vec![])
                }
                (CC_GET, PID_SENSOR_VALUE) => ack(dest, vec![pd[0], 0x14, 0x31, 0, 0, 0, 0, 0, 0]),
                _ => Reply::Timeout,
            }
        }
    }

    /// discovery response bytes as the gateway sends them (status byte first)
    fn discovery_bytes(uid: Uid) -> Vec<u8> {
        let mut d = vec![0, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xAA];
        let mut sum = 0u16;
        for b in uid.bytes() {
            for x in [b | 0xAA, b | 0x55] {
                d.push(x);
                sum = sum.wrapping_add(x as u16);
            }
        }
        for b in sum.to_be_bytes() {
            d.extend([b | 0xAA, b | 0x55]);
        }
        d
    }

    fn run(ctrl: &mut Controller, gw: &mut FakeGateway, steps: usize) {
        let now = Instant::now();
        for _ in 0..steps {
            let Some(req) = ctrl.next_request(now) else { break };
            let reply = gw.respond(&req.packet);
            ctrl.on_reply(&req, reply, now);
        }
    }

    #[test]
    fn encode_layout_and_checksum() {
        let p = encode(Uid(0x7FF0_D8A8_6BD0), 5, CC_GET, PID_DEVICE_INFO, &[]);
        assert_eq!(p.len(), 26);
        assert_eq!(&p[..3], &[0xCC, 0x01, 24]);
        assert_eq!(&p[3..9], &[0x7F, 0xF0, 0xD8, 0xA8, 0x6B, 0xD0]);
        assert_eq!((p[15], p[20], p[21], p[22], p[23]), (5, CC_GET, 0x00, 0x60, 0));
        let sum = p[..24].iter().fold(0u16, |s, &b| s.wrapping_add(b as u16));
        assert_eq!(u16::from_be_bytes([p[24], p[25]]), sum);
    }

    #[test]
    fn decodes_discovery_response_and_collision() {
        let uid = Uid(0x7FF0_D8A8_6BD0);
        assert_eq!(decode(&discovery_bytes(uid)), Reply::Discovered(uid));
        let mut broken = discovery_bytes(uid);
        broken[20] ^= 0x01;
        assert_eq!(decode(&broken), Reply::Collision);
    }

    #[test]
    fn decodes_response_from_gateway() {
        // MUTE response as captured from the gateway
        let hex = "00cc011a7ff0000000017ff0d8a86bd003000000001100020200000699";
        let data: Vec<u8> = (0..hex.len()).step_by(2).map(|i| u8::from_str_radix(&hex[i..i + 2], 16).unwrap()).collect();
        let Reply::Response(r) = decode(&data) else { panic!("not a response") };
        assert_eq!((r.src, r.tn, r.response_type, r.cc, r.pid), (Uid(0x7FF0_D8A8_6BD0), 3, RT_ACK, 0x11, PID_DISC_MUTE));
        assert_eq!(r.pd, vec![0, 0]);
    }

    #[test]
    fn discovery_finds_all_fixtures_through_collisions() {
        let uids = [0x7FF0_0000_0010u64, 0x7FF0_0000_0011, 0x7FF0_8000_0000, 0x7FF0_FFFF_FFFF];
        let mut gw = FakeGateway { fixtures: uids.iter().map(|&u| (Uid(u), false, 1)).collect() };
        let mut ctrl = Controller::default();
        let now = Instant::now();
        while let Some(req) = ctrl.next_request(now) {
            let reply = gw.respond(&req.packet);
            ctrl.on_reply(&req, reply, now);
            if !ctrl.discovering() {
                break;
            }
        }
        let found: Vec<u64> = ctrl.fixtures().keys().map(|u| u.0).collect();
        assert_eq!(found, uids);
        assert!(ctrl.stats().requests < 400, "{} requests", ctrl.stats().requests);
    }

    #[test]
    fn polls_and_applies_commands() {
        let uid = Uid(0x7FF0_0000_0042);
        let mut gw = FakeGateway { fixtures: vec![(uid, false, 1)] };
        let mut ctrl = Controller::default();
        run(&mut ctrl, &mut gw, 200);
        assert_eq!(ctrl.fixtures()[&uid].battery_mv, Some(0x1431));

        ctrl.command(Command::SetAddress(uid, 33));
        run(&mut ctrl, &mut gw, 1);
        assert_eq!(gw.fixtures[0].2, 33);
        assert_eq!(ctrl.fixtures()[&uid].dmx_address, Some(33));
    }

    #[test]
    fn ignores_reply_with_wrong_transaction_number() {
        let uid = Uid(0x7FF0_0000_0042);
        let mut ctrl = Controller::default();
        ctrl.fixtures.insert(uid, Fixture::default());
        ctrl.discovery = None;
        ctrl.last_discovery = Some(Instant::now());
        let req = ctrl.next_request(Instant::now()).unwrap();
        let stale = Response { src: uid, tn: req.tn.wrapping_sub(1), response_type: RT_ACK, cc: 0x21, pid: PID_DEVICE_INFO, pd: vec![0; 19] };
        ctrl.on_reply(&req, Reply::Response(stale), Instant::now());
        assert_eq!(ctrl.fixtures()[&uid].dmx_address, None);
        assert_eq!(ctrl.stats().invalid, 1);
    }
}
