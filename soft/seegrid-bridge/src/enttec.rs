//! Enttec DMX USB Pro framing, host -> widget:
//! `0x7E | label | len LSB | len MSB | data[len] | 0xE7`

use crate::state::Universe;

const SOM: u8 = 0x7E;
const EOM: u8 = 0xE7;
/// "Output Only Send DMX Packet": data = start code + channels
pub const LABEL_SEND_DMX: u8 = 6;
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
}
