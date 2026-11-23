//! Test pattern for the seegrid fixtures.

use crate::state::{Universe, CHANNELS};

/// Fixture layout of the seegrid modules (4 channels W R B G, 8 bit).
pub const FIXTURE_CHANNELS: usize = 4;
const TEST_REVOLUTION_S: f32 = 15.0;

/// Rotating colour wheel over all fixtures, white off.
pub fn test_pattern(t: f32) -> Universe {
    let mut u = [0u8; CHANNELS];
    let fixtures = CHANNELS / FIXTURE_CHANNELS;
    for (i, fix) in u.chunks_exact_mut(FIXTURE_CHANNELS).enumerate() {
        let hue = (t / TEST_REVOLUTION_S + i as f32 / fixtures as f32).fract();
        let (r, g, b) = hue_to_rgb(hue);
        fix.copy_from_slice(&[0, r, b, g]);
    }
    u
}

/// hue 0..1 -> fully saturated RGB
fn hue_to_rgb(hue: f32) -> (u8, u8, u8) {
    let h = hue * 6.0;
    let f = ((h.fract()) * 255.0) as u8;
    match h as u32 {
        0 => (255, f, 0),
        1 => (255 - f, 255, 0),
        2 => (0, 255, f),
        3 => (0, 255 - f, 255),
        4 => (f, 0, 255),
        _ => (255, 0, 255 - f),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_pattern_layout() {
        let u = test_pattern(0.0);
        // fixture 0: hue 0 = red; layout W R B G
        assert_eq!(&u[..4], &[0, 255, 0, 0]);
        // fixture 42 (hue 0.328, just before pure green): G full, B off, R almost off
        let fix = &u[42 * 4..42 * 4 + 4];
        assert_eq!((fix[0], fix[2], fix[3]), (0, 0, 255));
        assert!(fix[1] < 16, "red {}", fix[1]);
    }

    #[test]
    fn hue_sectors() {
        assert_eq!(hue_to_rgb(0.0), (255, 0, 0));
        assert_eq!(hue_to_rgb(1.0 / 3.0 + 1e-6).1, 255);
        assert_eq!(hue_to_rgb(2.0 / 3.0 + 1e-6).2, 255);
    }
}
