use std::{error::Error, sync::Arc, time::Duration};

use kira::{
	AudioManager, AudioManagerSettings, DefaultBackend, Tween,
	sound::{
		PlaybackState,
		static_sound::{StaticSoundData, StaticSoundHandle},
	},
};
use rfd::{AsyncFileDialog, MessageDialog};
use tokio::sync::{
	Mutex,
	broadcast::{self, Sender},
};

pub struct Background {
	pub sender: Sender<BgEvent>,

	pub song_list: Arc<Mutex<Vec<(String, StaticSoundData)>>>,
	pub current_song: Arc<Mutex<Option<SongInfo>>>,
}

#[derive(Clone)]
pub enum BgEvent {
	TryLoadSounds,
	Play(usize),
	SetVolume(f32),
	Pause,
	Resume,
}

impl Background {
	pub fn new() -> Result<Self, Box<dyn Error>> {
		let (sender, mut receiver) = broadcast::channel(100);
		let mut player = AudioManager::<DefaultBackend>::new(AudioManagerSettings::default())?;

		let pitch: f64 = 1919810.;
		let get_pitch = move || {
			if pitch == 1919810. { 1. } else { pitch.clamp(0.1, 10.) }
		};
		let mut volume: f32 = 1.;

		let song_list = Arc::new(Mutex::new(vec![]));
		let current_song = Arc::new(Mutex::new(None::<SongInfo>));

		let return_value = Self {
			sender: sender.clone(),
			song_list: song_list.clone(),
			current_song: current_song.clone(),
		};

		let song_list_ = song_list.clone();
		let current_song_ = current_song.clone();
		tokio::spawn(async move {
			while let Ok(event) = receiver.recv().await {
				let mut song_list = song_list_.lock().await;
				let mut current_song = current_song_.lock().await;
				match event {
					BgEvent::TryLoadSounds => {
						if let Some(files) = AsyncFileDialog::new()
							.set_title("添加声音")
							.add_filter("声音", &["wav", "mp3", "ogg", "flac"])
							.pick_files()
							.await
						{
							for f in &files {
								let Ok(sound) = StaticSoundData::from_file(f.path()) else {
									MessageDialog::new()
										.set_buttons(rfd::MessageButtons::Ok)
										.set_level(rfd::MessageLevel::Error)
										.set_title("错误")
										.set_description(format!("无法加载 {} ！", f.path().to_string_lossy()))
										.show();
									continue;
								};
								song_list.push((f.file_name(), sound));
							}
						}
					}
					BgEvent::Play(index) => {
						let the_sound = &song_list[index].1;
						if let Ok(mut new_song) = player.play(the_sound.clone()) {
							if let Some(old_song) = &mut *current_song {
								old_song.handle.stop(Tween::default());
							}

							new_song.set_volume(volume.log10() * 20., Tween::default());
							new_song.set_playback_rate(get_pitch(), Tween::default());
							*current_song = Some(SongInfo {
								handle: new_song,
								index_in_list: index,
								duration_secs: the_sound.unsliced_duration().as_secs_f64(),
							});
						}
					}
					BgEvent::SetVolume(vol) => {
						volume = vol;
						if let Some(song) = &mut *current_song {
							song.handle.set_volume(vol.log10() * 20., Tween::default());
						}
					}
					BgEvent::Pause => {
						if let Some(song) = &mut *current_song {
							song.handle.pause(Tween::default());
						}
					}
					BgEvent::Resume => {
						if let Some(song) = &mut *current_song {
							song.handle.resume(Tween::default());
						}
					}
				}
			}
		});

		let sender2 = sender.clone();
		tokio::spawn(async move {
			loop {
				if let Some(song) = &mut *current_song.lock().await {
					song.handle.set_playback_rate(get_pitch(), Tween::default());
					if song.handle.state() == PlaybackState::Stopped {
						let mut next_id = song.index_in_list + 1;
						if next_id >= song_list.lock().await.len() {
							next_id = 0;
						}
						let _ = sender2.send(BgEvent::Play(next_id));
					}
				}
				tokio::time::sleep(Duration::from_millis(10)).await;
			}
		});

		Ok(return_value)
	}
}

pub struct SongInfo {
	pub handle: StaticSoundHandle,
	pub index_in_list: usize,
	pub duration_secs: f64,
}
