//! seegrid bridge: Art-Net in, Enttec DMX USB Pro framing out to the RF gateway,
//! with a ratatui monitor rendered into its own window.

mod artnet;
mod enttec;
mod link;
mod pattern;
mod rdm;
mod state;
mod ui;

use std::time::Duration;

use clap::Parser;
use eframe::egui;
use egui_ratatui::RataguiBackend;
use ratatui::Terminal;
use soft_ratatui::embedded_graphics_unicodefonts::{mono_9x18_atlas, mono_9x18_bold_atlas};
use soft_ratatui::{EmbeddedGraphics, SoftBackend};

use rdm::Command;
use state::Source;

#[derive(Parser)]
#[command(version, about)]
struct Args {
    /// Serial port of the gateway (J-Link VCOM of the XMC1400 boot kit)
    #[arg(short, long, default_value = "/dev/ttyACM0")]
    port: String,
    /// Art-Net universe (15 bit Port-Address)
    #[arg(short, long, default_value_t = 0)]
    universe: u16,
    /// Frames per second sent to the gateway
    #[arg(short, long, default_value_t = 30.0)]
    rate: f32,
}

fn main() -> eframe::Result {
    let args = Args::parse();
    let state = state::new_shared(&args.port, args.universe);
    artnet::spawn(state.clone());
    link::spawn(state.clone(), args.rate);

    let backend = SoftBackend::<EmbeddedGraphics>::new(100, 40, mono_9x18_atlas(), Some(mono_9x18_bold_atlas()), None);
    let mut terminal = Terminal::new(RataguiBackend::new("seegrid", backend)).expect("terminal");

    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default().with_title("seegrid bridge").with_inner_size([960.0, 720.0]),
        ..Default::default()
    };
    let mut view = ui::View::default();
    eframe::run_ui_native("seegrid bridge", options, move |ui, _frame| {
        let ctx = ui.ctx().clone();
        let events = ctx.input(|i| i.events.clone());
        {
            let mut st = state.lock().unwrap();
            if handle_input(&events, &mut view, &mut st) {
                ctx.send_viewport_cmd(egui::ViewportCommand::Close);
            }
            terminal.draw(|f| ui::draw(f, &st, &view)).expect("draw");
        }
        egui::CentralPanel::default()
            .frame(egui::Frame::NONE)
            .show_inside(ui, |ui| ui.add(terminal.backend_mut()));
        ctx.request_repaint_after(Duration::from_millis(50));
    })
}

/// Keyboard -> actions; true to quit.
fn handle_input(events: &[egui::Event], view: &mut ui::View, st: &mut state::Shared) -> bool {
    use egui::Key;
    for ev in events {
        match ev {
            egui::Event::Text(text) => {
                if let Some(buf) = &mut view.edit {
                    for c in text.chars().filter(char::is_ascii_digit) {
                        if buf.len() < 3 {
                            buf.push(c);
                        }
                    }
                }
            }
            egui::Event::Key { key, pressed: true, .. } => {
                let selected = st.fixtures.get(view.selected).map(|(uid, f)| (*uid, f.clone()));
                if let Some(buf) = &mut view.edit {
                    match key {
                        Key::Backspace => {
                            buf.pop();
                        }
                        Key::Escape => view.edit = None,
                        Key::Enter => {
                            if let (Ok(addr), Some((uid, _))) = (buf.parse::<u16>(), selected)
                                && (1..=512).contains(&addr)
                            {
                                st.commands.push_back(Command::SetAddress(uid, addr));
                            }
                            view.edit = None;
                        }
                        _ => {}
                    }
                    continue;
                }
                match key {
                    Key::A => st.source = Source::ArtNet,
                    Key::T => st.source = Source::TestPattern,
                    Key::B => st.source = Source::Blackout,
                    Key::Q => return true,
                    Key::ArrowUp => view.selected = view.selected.saturating_sub(1),
                    Key::ArrowDown => view.selected = (view.selected + 1).min(st.fixtures.len().saturating_sub(1)),
                    Key::D => st.commands.push_back(Command::Discover),
                    Key::E => {
                        if let Some((_, f)) = &selected {
                            view.edit = Some(f.dmx_address.map(|a| a.to_string()).unwrap_or_default());
                        }
                    }
                    Key::I => {
                        if let Some((uid, f)) = selected {
                            st.commands.push_back(Command::Identify(uid, !f.identify.unwrap_or(false)));
                        }
                    }
                    _ => {}
                }
            }
            _ => {}
        }
    }
    false
}
