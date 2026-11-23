//! Link to the gateway: owns the serial port, sends DMX at a fixed rate (like
//! a DMX line that is refreshed continuously) and runs RDM transactions in the
//! time between frames. Reconnects when the port goes away.

use std::io::{self, Write};
use std::thread;
use std::time::{Duration, Instant};

use serialport::SerialPort;

use crate::enttec::{self, Parser, LABEL_RDM_TIMEOUT, LABEL_RECEIVED};
use crate::pattern::test_pattern;
use crate::rdm::{self, Controller, Reply};
use crate::state::{SharedState, Source, CHANNELS};

const BAUD: u32 = 1_000_000;
const RECONNECT: Duration = Duration::from_secs(1);
const IO_TIMEOUT: Duration = Duration::from_millis(5);
/// the gateway answers from its table within a millisecond
const RDM_REPLY_TIMEOUT: Duration = Duration::from_millis(50);
/// no RDM transaction is started closer than this to the next DMX frame
const RDM_MARGIN: Duration = Duration::from_millis(4);

pub fn spawn(state: SharedState, rate_hz: f32) {
    thread::spawn(move || {
        let period = Duration::from_secs_f32(1.0 / rate_hz);
        let start = Instant::now();
        let port_name = state.lock().unwrap().serial.port.clone();
        let mut port: Option<Box<dyn SerialPort>> = None;
        let mut last_attempt: Option<Instant> = None;
        let mut parser = Parser::default();
        let mut ctrl = Controller::default();
        let mut next = Instant::now();

        loop {
            let now = Instant::now();
            if next > now {
                thread::sleep(next - now);
            } else {
                next = Instant::now(); // fell behind: don't burst to catch up
            }
            next += period;

            if port.is_none() && last_attempt.is_none_or(|t| t.elapsed() >= RECONNECT) {
                last_attempt = Some(Instant::now());
                let opened = serialport::new(&port_name, BAUD).timeout(IO_TIMEOUT).open();
                let mut st = state.lock().unwrap();
                match opened {
                    Ok(p) => {
                        port = Some(p);
                        parser = Parser::default();
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
                    Source::TestPattern => test_pattern(start.elapsed().as_secs_f32(), &st.fixtures),
                    Source::Blackout => [0; CHANNELS],
                };
                st.output
            };

            let Some(p) = port.as_mut() else { continue };
            let result = p.write_all(&enttec::dmx_packet(&frame)).and_then(|()| {
                state.lock().unwrap().serial.rate.tick(Instant::now());
                rdm_until(p.as_mut(), &mut parser, &mut ctrl, &state, next)
            });
            let mut st = state.lock().unwrap();
            match result {
                Ok(()) => st.serial.frames += 1,
                Err(e) => {
                    st.serial.connected = false;
                    st.serial.error = Some(e.to_string());
                    port = None;
                }
            }
        }
    });
}

/// RDM transactions until `deadline - RDM_MARGIN`.
fn rdm_until(
    port: &mut dyn SerialPort,
    parser: &mut Parser,
    ctrl: &mut Controller,
    state: &SharedState,
    deadline: Instant,
) -> io::Result<()> {
    while Instant::now() + RDM_MARGIN < deadline {
        for cmd in state.lock().unwrap().commands.drain(..) {
            ctrl.command(cmd);
        }
        let Some(req) = ctrl.next_request(Instant::now()) else { break };
        port.write_all(&enttec::packet(req.label, &req.packet))?;
        let reply = read_reply(port, parser)?;
        ctrl.on_reply(&req, reply, Instant::now());

        let mut st = state.lock().unwrap();
        st.fixtures = ctrl.fixtures().iter().map(|(&u, f)| (u, f.clone())).collect();
        st.rdm = ctrl.stats();
        st.rdm_discovering = ctrl.discovering();
    }
    Ok(())
}

fn read_reply(port: &mut dyn SerialPort, parser: &mut Parser) -> io::Result<Reply> {
    let deadline = Instant::now() + RDM_REPLY_TIMEOUT;
    let mut buf = [0u8; 256];
    loop {
        while let Some((label, data)) = parser.next_packet() {
            match label {
                LABEL_RECEIVED => return Ok(rdm::decode(&data)),
                LABEL_RDM_TIMEOUT => return Ok(Reply::Timeout),
                _ => {}
            }
        }
        if Instant::now() >= deadline {
            return Ok(Reply::Timeout);
        }
        match port.read(&mut buf) {
            Ok(n) => parser.push(&buf[..n]),
            Err(e) if matches!(e.kind(), io::ErrorKind::TimedOut | io::ErrorKind::WouldBlock) => {}
            Err(e) => return Err(e),
        }
    }
}
