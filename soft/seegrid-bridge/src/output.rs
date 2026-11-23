//! Sends the selected source to the gateway at a fixed rate, like a DMX line
//! that is refreshed continuously, and reconnects when the port goes away.

use std::io::Write;
use std::thread;
use std::time::{Duration, Instant};

use crate::enttec;
use crate::state::{SharedState, Source, Universe, CHANNELS};

const BAUD: u32 = 1_000_000;
const RECONNECT: Duration = Duration::from_secs(1);

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

pub fn spawn(state: SharedState, rate_hz: f32) {
    thread::spawn(move || {
        let period = Duration::from_secs_f32(1.0 / rate_hz);
        let start = Instant::now();
        let port_name = state.lock().unwrap().serial.port.clone();
        let mut port: Option<Box<dyn serialport::SerialPort>> = None;
        let mut last_attempt: Option<Instant> = None;
        let mut next = Instant::now();

        loop {
            next += period;
            let now = Instant::now();
            if next > now {
                thread::sleep(next - now);
            } else {
                next = now; // fell behind (e.g. blocked write): don't burst to catch up
            }

            if port.is_none() && last_attempt.is_none_or(|t| t.elapsed() >= RECONNECT) {
                last_attempt = Some(Instant::now());
                let opened = serialport::new(&port_name, BAUD).timeout(Duration::from_millis(200)).open();
                let mut st = state.lock().unwrap();
                match opened {
                    Ok(p) => {
                        port = Some(p);
                        st.serial.connected = true;
                        st.serial.error = None;
                    }
                    Err(e) => st.serial.error = Some(e.to_string()),
                }
            }

            let frame = {
                let mut st = state.lock().unwrap();
                st.output = match st.source {
                    Source::ArtNet => st.artnet_data,
                    Source::TestPattern => test_pattern(start.elapsed().as_secs_f32()),
                    Source::Blackout => [0; CHANNELS],
                };
                st.output
            };

            let Some(p) = port.as_mut() else { continue };
            let result = p.write_all(&enttec::dmx_packet(&frame));
            let mut st = state.lock().unwrap();
            match result {
                Ok(()) => {
                    st.serial.frames += 1;
                    st.serial.rate.tick(Instant::now());
                }
                Err(e) => {
                    st.serial.connected = false;
                    st.serial.error = Some(e.to_string());
                    port = None;
                }
            }
        }
    });
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
