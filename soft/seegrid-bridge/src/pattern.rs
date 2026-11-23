//! Test pattern: rotating colour wheel, white off.

use crate::layout::{self, Wrgb, PERSONALITY_16BIT};
use crate::rdm::{Fixture, Uid};
use crate::state::{Universe, CHANNELS};

const TEST_REVOLUTION_S: f32 = 15.0;

/// One colour per known fixture, at its address and in its personality.
/// Without known fixtures: a 16 bit grid (addresses 1, 9, 17, ...), the
/// default of new modules.
pub fn test_pattern(t: f32, fixtures: &[(Uid, Fixture)]) -> Universe {
    let mut u = [0u8; CHANNELS];
    let known: Vec<(u16, u8)> =
        fixtures.iter().filter_map(|(_, f)| Some((f.dmx_address?, f.personality.unwrap_or(PERSONALITY_16BIT)))).collect();
    let targets: Vec<(u16, u8)> = if known.is_empty() {
        (0..CHANNELS / layout::footprint(PERSONALITY_16BIT)).map(|i| (1 + 8 * i as u16, PERSONALITY_16BIT)).collect()
    } else {
        known
    };
    let n = targets.len() as f32;
    for (i, &(address, personality)) in targets.iter().enumerate() {
        let hue = (t / TEST_REVOLUTION_S + i as f32 / n).fract();
        layout::write(&mut u, address, personality, hue_to_wrgb(hue));
    }
    u
}

/// hue 0..1 -> fully saturated colour, white off
fn hue_to_wrgb(hue: f32) -> Wrgb {
    let h = hue * 6.0;
    let f = (h.fract() * 65535.0) as u16;
    let (r, g, b) = match h as u32 {
        0 => (0xFFFF, f, 0),
        1 => (0xFFFF - f, 0xFFFF, 0),
        2 => (0, 0xFFFF, f),
        3 => (0, 0xFFFF - f, 0xFFFF),
        4 => (f, 0, 0xFFFF),
        _ => (0xFFFF, 0, 0xFFFF - f),
    };
    Wrgb { w: 0, r, g, b }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::layout::PERSONALITY_8BIT;

    #[test]
    fn grid_without_fixtures() {
        let u = test_pattern(0.0, &[]);
        // fixture 1 at address 1, hue 0 = red, 16 bit W R B G
        assert_eq!(&u[..8], &[0, 0, 0xFF, 0xFF, 0, 0, 0, 0]);
    }

    #[test]
    fn per_fixture_layout() {
        let f8 = Fixture { dmx_address: Some(33), personality: Some(PERSONALITY_8BIT), ..Default::default() };
        let f16 = Fixture { dmx_address: Some(1), personality: Some(PERSONALITY_16BIT), ..Default::default() };
        let u = test_pattern(0.0, &[(Uid(1), f16), (Uid(2), f8)]);
        assert_eq!(&u[..4], &[0, 0, 0xFF, 0xFF]); // red, 16 bit
        assert_eq!(u[32], 0); // white off, 8 bit at 33
        assert_eq!(u[35], 0xFF); // half way round: cyan-ish, green full
        assert!(u[8..32].iter().all(|&b| b == 0));
    }
}
