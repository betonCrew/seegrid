//! DMX layout of a seegrid fixture: 4 channels W R B G, 8 bit (personality 1)
//! or 16 bit high byte first (personality 2), as in the firmware (seeGridRF).

use crate::state::{Universe, CHANNELS};

pub const PERSONALITY_8BIT: u8 = 1;
pub const PERSONALITY_16BIT: u8 = 2;

/// Colour on the 16 bit scale.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Wrgb {
    pub w: u16,
    pub r: u16,
    pub g: u16,
    pub b: u16,
}

pub fn footprint(personality: u8) -> usize {
    if personality == PERSONALITY_16BIT { 8 } else { 4 }
}

/// channel order on the wire
fn channels(c: &Wrgb) -> [u16; 4] {
    [c.w, c.r, c.b, c.g]
}

/// Writes `c` at the 1-based start address; channels beyond 512 are dropped.
pub fn write(u: &mut Universe, address: u16, personality: u8, c: Wrgb) {
    let wide = personality == PERSONALITY_16BIT;
    let mut i = address as usize - 1;
    for v in channels(&c) {
        let bytes: &[u8] = if wide { &v.to_be_bytes() } else { &[(v >> 8) as u8] };
        for &b in bytes {
            if i < CHANNELS {
                u[i] = b;
            }
            i += 1;
        }
    }
}

/// Reads the colour at the 1-based start address (8 bit values scaled to 16 bit).
pub fn read(u: &Universe, address: u16, personality: u8) -> Wrgb {
    let wide = personality == PERSONALITY_16BIT;
    let at = |i: usize| u.get(i).copied().unwrap_or(0) as u16;
    let ch = |n: usize| {
        let i = address as usize - 1 + if wide { 2 * n } else { n };
        if wide { at(i) << 8 | at(i + 1) } else { at(i) * 257 }
    };
    Wrgb { w: ch(0), r: ch(1), b: ch(2), g: ch(3) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn layouts_round_trip() {
        let c = Wrgb { w: 0x0102, r: 0xABCD, g: 0x7F80, b: 0xFFFF };
        let mut u = [0u8; CHANNELS];
        write(&mut u, 9, PERSONALITY_16BIT, c);
        assert_eq!(&u[8..16], &[0x01, 0x02, 0xAB, 0xCD, 0xFF, 0xFF, 0x7F, 0x80]); // W R B G, high byte first
        assert_eq!(read(&u, 9, PERSONALITY_16BIT), c);

        let mut u = [0u8; CHANNELS];
        write(&mut u, 1, PERSONALITY_8BIT, c);
        assert_eq!(&u[..4], &[0x01, 0xAB, 0xFF, 0x7F]);
        assert_eq!(read(&u, 1, PERSONALITY_8BIT), Wrgb { w: 0x0101, r: 0xABAB, g: 0x7F7F, b: 0xFFFF });
    }

    #[test]
    fn write_clips_at_universe_end() {
        let mut u = [0u8; CHANNELS];
        write(&mut u, 509, PERSONALITY_16BIT, Wrgb { w: 0xFFFF, r: 0xFFFF, g: 0xFFFF, b: 0xFFFF });
        assert!(u[508..].iter().all(|&b| b == 0xFF));
    }
}
