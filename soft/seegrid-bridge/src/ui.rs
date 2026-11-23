//! ratatui drawing only; independent of the window backend.

use std::time::{Duration, Instant};

use ratatui::layout::{Constraint, Layout, Rect};
use ratatui::style::{Color, Modifier, Style, Stylize};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Cell, Paragraph, Row, Table, TableState, Wrap};
use ratatui::Frame;

use crate::layout;
use crate::state::{Shared, Source};

/// UI-only state (selection, address input)
#[derive(Default)]
pub struct View {
    pub selected: usize,
    /// address being typed for the selected fixture
    pub edit: Option<String>,
}

const LAST_SEEN_OK: Duration = Duration::from_secs(3);
const LAST_SEEN_LOST: Duration = Duration::from_secs(10);
const BATTERY_LOW_MV: u16 = 4400;

pub fn draw(frame: &mut Frame, st: &Shared, view: &View) {
    let now = Instant::now();
    let [header, status, preview, fixtures, help_area] = Layout::vertical([
        Constraint::Length(1),
        Constraint::Length(7),
        Constraint::Length(8),
        Constraint::Min(6),
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

    draw_fixtures(frame, st, view, fixtures, now);

    let help = match &view.edit {
        Some(text) => Line::from(vec![
            Span::raw(" DMX start address: ").yellow(),
            Span::raw(format!("{text}_")).bold(),
            Span::raw("   Enter set   Esc cancel").dark_gray(),
        ]),
        None => Line::from(
            " a Art-Net  t test  b blackout   \u{2191}\u{2193} select  e address  p 8/16 bit  i identify  d discover   q quit ",
        )
        .dark_gray(),
    };
    frame.render_widget(help, help_area);
}

fn age_text(age: Duration) -> String {
    let s = age.as_secs_f32();
    if s < 60.0 { format!("{s:.1} s") } else { format!("{:.0} min", s / 60.0) }
}

fn draw_fixtures(frame: &mut Frame, st: &Shared, view: &View, area: Rect, now: Instant) {
    let dash = || Cell::from("-").dark_gray();
    let rows: Vec<Row> = st
        .fixtures
        .iter()
        .map(|(uid, f)| {
            let addr = f.dmx_address.map_or_else(dash, |a| Cell::from(a.to_string()));
            let battery = f.battery_mv.map_or_else(dash, |mv| {
                let c = Cell::from(format!("{:.2} V", mv as f32 / 1000.0));
                if mv < BATTERY_LOW_MV { c.red() } else { c }
            });
            let mode = match f.personality {
                Some(layout::PERSONALITY_8BIT) => Cell::from("8 bit"),
                Some(layout::PERSONALITY_16BIT) => Cell::from("16 bit"),
                Some(p) => Cell::from(format!("pers {p}")),
                None => dash(),
            };
            let fps = f.dmx_fps.map_or_else(dash, |v| Cell::from(v.to_string()));
            let seen = f.last_seen_age(now).map_or_else(dash, |age| {
                let c = Cell::from(age_text(age));
                if age < LAST_SEEN_OK {
                    c.green()
                } else if age < LAST_SEEN_LOST {
                    c.yellow()
                } else {
                    Cell::from(format!("lost ({})", age_text(age))).red()
                }
            });
            let identify = match f.identify {
                Some(true) => Cell::from("blinking").yellow(),
                Some(false) => Cell::from("off").dark_gray(),
                None => dash(),
            };
            let note = f.error.as_deref().map_or_else(|| Cell::from(""), |e| Cell::from(e.to_owned()).red());
            Row::new(vec![Cell::from(uid.to_string()), addr, mode, battery, fps, seen, identify, note])
        })
        .collect();

    let discovery = if st.rdm_discovering { "discovering..." } else { "" };
    let title = format!(
        " Fixtures ({})  RDM {} req, {} timeouts  {discovery} ",
        st.fixtures.len(),
        st.rdm.requests,
        st.rdm.timeouts
    );
    let table = Table::new(
        rows,
        [
            Constraint::Length(14),
            Constraint::Length(5),
            Constraint::Length(7),
            Constraint::Length(8),
            Constraint::Length(5),
            Constraint::Length(15),
            Constraint::Length(9),
            Constraint::Min(10),
        ],
    )
    .header(Row::new(["UID", "Addr", "Mode", "Battery", "DMX/s", "Last seen", "Identify", ""]).dark_gray())
    .row_highlight_style(Style::new().bg(Color::Rgb(40, 40, 70)).add_modifier(Modifier::BOLD))
    .block(Block::bordered().title(title));

    let mut ts = TableState::default();
    if !st.fixtures.is_empty() {
        ts.select(Some(view.selected.min(st.fixtures.len() - 1)));
    }
    frame.render_stateful_widget(table, area, &mut ts);
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

/// Output as the discovered fixtures see it: one coloured cell per fixture.
fn draw_preview(frame: &mut Frame, st: &Shared, area: Rect) {
    let block = Block::bordered().title(" Output preview per fixture ");
    let inner = block.inner(area);
    frame.render_widget(block, area);
    if st.fixtures.is_empty() {
        frame.render_widget(Paragraph::new("no fixtures discovered").dark_gray(), inner);
        return;
    }

    const CELL: u16 = 6; // " addr " coloured
    let per_row = (inner.width / CELL).max(1) as usize;
    let mut lines = Vec::new();
    let mut row = Vec::new();
    for (_, f) in &st.fixtures {
        let (Some(addr), personality) = (f.dmx_address, f.personality.unwrap_or(layout::PERSONALITY_16BIT)) else {
            continue;
        };
        let c = layout::read(&st.output, addr, personality);
        let to8 = |v: u16| v >> 8;
        let (w, r, g, b) = (to8(c.w), to8(c.r), to8(c.g), to8(c.b));
        let mix = |v: u16| (v + w).min(255) as u8;
        let bg = Color::Rgb(mix(r), mix(g), mix(b));
        let bright = r * 3 + g * 6 + b + w * 10 > 1100;
        let fg = if bright { Color::Black } else { Color::Gray };
        row.push(Span::styled(format!(" {addr:>3}  "), Style::new().fg(fg).bg(bg)));
        if row.len() == per_row {
            lines.push(Line::from(std::mem::take(&mut row)));
        }
    }
    if !row.is_empty() {
        lines.push(Line::from(row));
    }
    frame.render_widget(Paragraph::new(lines), inner);
}
