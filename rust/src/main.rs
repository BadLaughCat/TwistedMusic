#![windows_subsystem = "windows"]
use std::{error::Error, sync::Arc};

use eframe::egui::{self, RichText, Sense};
use egui_extras::{Column, TableBuilder};
use kira::sound::PlaybackState;
use windows::Win32::System::Threading::GetCurrentProcessId;
mod bg_play;
use crate::bg_play::{Background, BgEvent};

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
	let native_options = eframe::NativeOptions {
		viewport: egui::ViewportBuilder {
			inner_size: Some(egui::vec2(400., 300.)),
			..Default::default()
		},
		centered: true,
		..Default::default()
	};

	let audio_bg = Background::new()?;
	eframe::run_native("Twisted Music", native_options, Box::new(|cc| Ok(Box::new(MyEguiApp::new(cc, audio_bg)))))?;
	Ok(())
}

struct MyEguiApp {
	selected_row: usize,
	pid: String,

	audio_bg: Background,
	song_pos: f64,
	song_volume: f32,
}

impl MyEguiApp {
	fn new(cc: &eframe::CreationContext<'_>, audio_bg: Background) -> Self {
		let mut fonts = egui::FontDefinitions::default();
		fonts.font_data.insert(
			"my_font".to_owned(),
			Arc::new(egui::FontData::from_static(include_bytes!("../LXGWNeoXiHeiScreen.ttf"))),
		);
		fonts
			.families
			.entry(egui::FontFamily::Proportional)
			.or_default()
			.insert(0, "my_font".to_owned());
		cc.egui_ctx.set_fonts(fonts);

		let pid = format!("PID {}", unsafe { GetCurrentProcessId() });
		// unsafe {
		// let handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid).unwrap();
		// let mut entry = MODULEENTRY32 {
		// dwSize: size_of::<MODULEENTRY32>() as u32,
		// ..Default::default()
		// };
		// Module32First(handle, &mut entry).unwrap();
		// let addr = format!("{:p}", entry.modBaseAddr);
		// CloseHandle(handle).unwrap();
		// }

		Self {
			selected_row: 0,
			pid,
			audio_bg,
			song_pos: 0.,
			song_volume: 1.0,
		}
	}
}

impl eframe::App for MyEguiApp {
	fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
		if let Ok(song) = self.audio_bg.current_song.try_lock()
			&& let Some(song) = &*song
			&& song.handle.state() == PlaybackState::Playing
		{
			self.song_pos = song.handle.position();
		}

		egui::TopBottomPanel::bottom("the_bottom").show(ctx, |ui| {
			if let Ok(mut song) = self.audio_bg.current_song.try_lock()
				&& let Some(song) = &mut *song
			{
				ui.horizontal(|ui| {
					ui.spacing_mut().slider_width = ui.available_width() - 112.;
					match song.handle.state() {
						PlaybackState::Playing => {
							if ui.button("暂停").clicked() {
								let _ = self.audio_bg.sender.send(BgEvent::Pause);
							}
						}
						PlaybackState::Paused => {
							if ui.button("播放").clicked() {
								let _ = self.audio_bg.sender.send(BgEvent::Resume);
							}
						}
						_ => (),
					}
					if ui
						.add(
							egui::Slider::new(&mut self.song_pos, 0.0..=song.duration_secs)
								.handle_shape(egui::style::HandleShape::Circle)
								.custom_formatter(|n, _| {
									let milli = (n * 100.) as u32 % 100;
									let n = n as u32;
									format!("{:02}:{:02}.{:02}", n / 60, n % 60, milli)
								})
								.step_by(0.01),
						)
						.dragged()
					{
						song.handle.seek_to(self.song_pos);
					}
				});
			}
			ui.horizontal(|ui| {
				if ui.button("添加声音").clicked() {
					let _ = self.audio_bg.sender.send(BgEvent::TryLoadSounds);
				}
				ui.label("音量");
				if ui
					.add(
						egui::Slider::new(&mut self.song_volume, 0.001..=1.5)
							.handle_shape(egui::style::HandleShape::Circle)
							.fixed_decimals(2)
							.step_by(0.01),
					)
					.changed()
				{
					let _ = self.audio_bg.sender.send(BgEvent::SetVolume(self.song_volume));
				}
				ui.label(&self.pid);
			});
		});
		egui::CentralPanel::default()
			.frame(egui::Frame::new().fill(ctx.style().visuals.panel_fill))
			.show(ctx, |ui| {
				TableBuilder::new(ui)
					.striped(true)
					.column(Column::remainder())
					.sense(Sense::click())
					.cell_layout(egui::Layout::left_to_right(egui::Align::Center))
					.body(|body| {
						let song_list = self.audio_bg.song_list.clone();
						let Ok(song_list) = song_list.try_lock() else {
							return;
						};

						body.rows(24., song_list.len(), |mut row| {
							let index = row.index();
							row.set_selected(self.selected_row == index);
							row.col(|ui| {
								let mut the_text = RichText::new(&song_list[index].0);
								if self
									.audio_bg
									.current_song
									.try_lock()
									.is_ok_and(|o| o.as_ref().is_some_and(|s| s.index_in_list == index))
								{
									the_text = the_text.size(20.);
								}
								ui.label(the_text);
							});
							if row.response().clicked() {
								self.selected_row = index;
							}
							if row.response().double_clicked() {
								let _ = self.audio_bg.sender.send(BgEvent::Play(index));
							}
						});
					});
			});
	}
}
