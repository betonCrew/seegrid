//! ratatui drawing only; independent of the window backend.

use std::time::Instant;

use ratatui::layout::{Constraint, Layout, Rect};
use ratatui::style::{Color, Modifier, Style, Stylize};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Paragraph, Wrap};
use ratatui::Frame;

use crate::output::FIXTURE_CHANNELS;
use crate::state::{Shared, Source};

pub fn draw(frame: &mut Frame, st: &Shared) {
    let now = Instant::now();
    let [header, status, preview, fixtures, help] = Layout::vertical([
        Constraint::Length(1),
        Constraint::Length(7),
        Constraint::Min(6),
        Constraint::Length(4),
        Constraint::Length(1),
    ])
    .areas(frame.area());

    frame.render_widget(Line::from(" seegrid bridge ".bold()).centered(), header);

    let [serial, artnet, output] =
        Layout::horizontal([Constraint::Ratio(1, 3); 3]).areas(status);
    frame.render_widget(serial_box(st, now), serial);
    frame.render_widget(artnet_box(st, now), artnet);
    frame.render_widget(output_box(st), output);

    draw_preview(frame, st, preview);

    frame.render_widget(
        Paragraph::new("No fixture data yet: needs the RF control protocol (heartbeat, battery, address).")
            .wrap(Wrap { trim: true })
            .dark_gray()
            .block(Block::bordered().title(" Fixtures ")),
        fixtures,
    );

    frame.render_widget(
        Line::from(" a Art-Net   t test pattern   b blackout   q quit ").dark_gray(),
        help,
    );
}

fn ok_err(ok: bool, ok_text: &str, err_text: &str) -> Span<'static> {
    if ok {
        Span::styled(ok_text.to_owned(), Style::new().fg(Color::Green))
    } else {
        Span::styled(err_text.to_owned(), Style::new().fg(Color::Red))
    }
}

fn kv(k: &str, v: impl Into<Span<'static>>) -> Line<'static> {
    Line::from(vec![Span::raw(format!("{k:<9}")).dark_gray(), v.into()])
}

fn serial_box(st: &Shared, now: Instant) -> Paragraph<'static> {
    let s = &st.serial;
    let mut lines = vec![
        kv("port", s.port.clone()),
        kv("state", ok_err(s.connected, "connected", "disconnected")),
        kv("sent", format!("{} frames", s.frames)),
        kv("rate", format!("{:.1} fps", s.rate.rate(now))),
    ];
    if let Some(e) = &s.error {
        lines.push(Line::from(e.clone()).red());
    }
    Paragraph::new(lines).wrap(Wrap { trim: true }).block(Block::bordered().title(" Gateway (serial) "))
}

fn artnet_box(st: &Shared, now: Instant) -> Paragraph<'static> {
    let a = &st.artnet;
    let age = match a.rate.last_event() {
        Some(t) => format!("{:.1} s ago", now.duration_since(t).as_secs_f32()),
        None => "never".to_owned(),
    };
    let universe = format!("{} ({}:{}:{})", a.universe, a.universe >> 8, (a.universe >> 4) & 0xF, a.universe & 0xF);
    let mut lines = vec![
        kv("listen", ok_err(a.bound, "UDP 6454", "not bound")),
        kv("universe", universe),
        kv("source", a.source.map_or("-".to_owned(), |s| s.ip().to_string())),
        kv("rate", format!("{:.1} fps, last {age}", a.rate.rate(now))),
        kv("polls", a.polls.to_string()),
    ];
    if let Some(e) = &a.error {
        lines.push(Line::from(e.clone()).red());
    }
    Paragraph::new(lines).wrap(Wrap { trim: true }).block(Block::bordered().title(" Art-Net "))
}

fn output_box(st: &Shared) -> Paragraph<'static> {
    let item = |s: Source, name: &str| {
        let style = if st.source == s {
            Style::new().fg(Color::Black).bg(Color::Yellow).add_modifier(Modifier::BOLD)
        } else {
            Style::new().dark_gray()
        };
        Line::from(Span::styled(format!(" {name} "), style))
    };
    Paragraph::new(vec![
        item(Source::ArtNet, "Art-Net"),
        item(Source::TestPattern, "Test pattern"),
        item(Source::Blackout, "Blackout"),
    ])
    .block(Block::bordered().title(" Output "))
}

/// Output as fixtures (4 channels W R B G each), one coloured cell per fixture.
fn draw_preview(frame: &mut Frame, st: &Shared, area: Rect) {
    let block = Block::bordered().title(" Output preview: fixtures, 4 ch W R B G, 8 bit ");
    let inner = block.inner(area);
    frame.render_widget(block, area);

    const CELL: u16 = 4; // "nnn " fixture number, coloured
    let per_row = (inner.width / CELL).max(1) as usize;
    let mut lines = Vec::new();
    let mut row = Vec::new();
    for (i, fix) in st.output.chunks_exact(FIXTURE_CHANNELS).enumerate() {
        let (w, r, b, g) = (fix[0] as u16, fix[1] as u16, fix[2] as u16, fix[3] as u16);
        let mix = |c: u16| (c + w).min(255) as u8;
        let bg = Color::Rgb(mix(r), mix(g), mix(b));
        let bright = r * 3 + g * 6 + b + w * 10 > 1100;
        let fg = if bright { Color::Black } else { Color::Gray };
        row.push(Span::styled(format!("{:>3} ", i + 1), Style::new().fg(fg).bg(bg)));
        if row.len() == per_row {
            lines.push(Line::from(std::mem::take(&mut row)));
        }
    }
    if !row.is_empty() {
        lines.push(Line::from(row));
    }
    frame.render_widget(Paragraph::new(lines), inner);
}
