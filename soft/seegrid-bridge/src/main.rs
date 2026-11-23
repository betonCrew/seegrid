//! seegrid bridge: Art-Net in, Enttec DMX USB Pro framing out to the RF gateway,
//! with a ratatui monitor rendered into its own window.

mod artnet;
mod enttec;
mod output;
mod state;
mod ui;

use std::time::Duration;

use clap::Parser;
use eframe::egui;
use egui_ratatui::RataguiBackend;
use ratatui::Terminal;
use soft_ratatui::embedded_graphics_unicodefonts::{mono_9x18_atlas, mono_9x18_bold_atlas};
use soft_ratatui::{EmbeddedGraphics, SoftBackend};

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
    output::spawn(state.clone(), args.rate);

    let backend = SoftBackend::<EmbeddedGraphics>::new(100, 40, mono_9x18_atlas(), Some(mono_9x18_bold_atlas()), None);
    let mut terminal = Terminal::new(RataguiBackend::new("seegrid", backend)).expect("terminal");

    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default().with_title("seegrid bridge").with_inner_size([960.0, 720.0]),
        ..Default::default()
    };
    eframe::run_ui_native("seegrid bridge", options, move |ui, _frame| {
        let ctx = ui.ctx().clone();
        let pressed = |k| ctx.input(|i| i.key_pressed(k));
        let select = if pressed(egui::Key::A) {
            Some(Source::ArtNet)
        } else if pressed(egui::Key::T) {
            Some(Source::TestPattern)
        } else if pressed(egui::Key::B) {
            Some(Source::Blackout)
        } else {
            None
        };
        if pressed(egui::Key::Q) {
            ctx.send_viewport_cmd(egui::ViewportCommand::Close);
        }

        {
            let mut st = state.lock().unwrap();
            if let Some(s) = select {
                st.source = s;
            }
            terminal.draw(|f| ui::draw(f, &st)).expect("draw");
        }
        egui::CentralPanel::default()
            .frame(egui::Frame::NONE)
            .show_inside(ui, |ui| ui.add(terminal.backend_mut()));
        ctx.request_repaint_after(Duration::from_millis(50));
    })
}
