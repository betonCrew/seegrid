//! State shared between the Art-Net receiver, the serial output and the UI.

use std::net::SocketAddr;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

pub const CHANNELS: usize = 512;
pub type Universe = [u8; CHANNELS];

/// What the bridge sends to the gateway.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Source {
    ArtNet,
    TestPattern,
    Blackout,
}

/// Events per second, measured over the last full second.
#[derive(Default)]
pub struct RateMeter {
    window_start: Option<Instant>,
    count: u32,
    rate: f32,
    last_event: Option<Instant>,
}

impl RateMeter {
    pub fn tick(&mut self, now: Instant) {
        let start = *self.window_start.get_or_insert(now);
        let elapsed = now.duration_since(start);
        if elapsed >= Duration::from_secs(1) {
            self.rate = self.count as f32 / elapsed.as_secs_f32();
            self.window_start = Some(now);
            self.count = 0;
        }
        self.count += 1;
        self.last_event = Some(now);
    }

    pub fn rate(&self, now: Instant) -> f32 {
        match self.last_event {
            Some(t) if now.duration_since(t) < Duration::from_secs(2) => self.rate,
            _ => 0.0,
        }
    }

    pub fn last_event(&self) -> Option<Instant> {
        self.last_event
    }
}

#[derive(Default)]
pub struct ArtNetStatus {
    pub universe: u16,
    pub bound: bool,
    pub error: Option<String>,
    pub source: Option<SocketAddr>,
    pub frames: u64,
    pub rate: RateMeter,
    pub polls: u64,
}

#[derive(Default)]
pub struct SerialStatus {
    pub port: String,
    pub connected: bool,
    pub error: Option<String>,
    pub frames: u64,
    pub rate: RateMeter,
}

pub struct Shared {
    pub source: Source,
    /// latest DMX data received via Art-Net
    pub artnet_data: Universe,
    /// what was last sent to the gateway (shown in the preview)
    pub output: Universe,
    pub artnet: ArtNetStatus,
    pub serial: SerialStatus,
}

pub type SharedState = Arc<Mutex<Shared>>;

pub fn new_shared(port: &str, universe: u16) -> SharedState {
    Arc::new(Mutex::new(Shared {
        source: Source::ArtNet,
        artnet_data: [0; CHANNELS],
        output: [0; CHANNELS],
        artnet: ArtNetStatus { universe, ..Default::default() },
        serial: SerialStatus { port: port.to_owned(), ..Default::default() },
    }))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rate_meter_counts_per_second_and_goes_stale() {
        let t0 = Instant::now();
        let mut m = RateMeter::default();
        for i in 0..=40 {
            m.tick(t0 + Duration::from_millis(25 * i));
        }
        let t = t0 + Duration::from_millis(1000);
        assert!((m.rate(t) - 40.0).abs() < 0.5, "rate {}", m.rate(t));
        assert_eq!(m.rate(t + Duration::from_secs(3)), 0.0);
    }
}
