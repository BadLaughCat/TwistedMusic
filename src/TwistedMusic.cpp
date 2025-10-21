#include "TwistedMusic.h"

void TwistedMusic::OnLoad() {
	m_BML->RegisterCommand(new AddCommand(this));
	ma_engine_init(nullptr, &mAudioEngine);
}

void TwistedMusic::OnUnload() {
	for (size_t i = 0; i < mSongList.size(); i++) {
		ma_sound_stop(&std::get<1>(mSongList[i]));
		ma_sound_uninit(&std::get<1>(mSongList[i]));
	}
	mSongList.clear();
	ma_engine_uninit(&mAudioEngine);
}

void TwistedMusic::OnProcess() {
	if (m_BML->IsPlaying()) {
		auto* array = m_BML->GetArrayByName("CurrentLevel");
		if (array) {
			auto* ball = static_cast<CK3dObject*>(array->GetElementObject(0, 1));
			if (ball) {
				VxVector current_pos;
				ball->GetPosition(&current_pos);

				double dx = current_pos.x - mLastPos.x;
				double dy = current_pos.y - mLastPos.y;
				double dz = current_pos.z - mLastPos.z;
				dx *= dx;
				dy *= dy;
				dz *= dz;

				auto length = sqrt(dx + dy + dz);
				auto delta_time = m_BML->GetTimeManager()->GetLastDeltaTime();

				auto pitch = length / delta_time * 1000;
				mLastPos = current_pos;
			}
		}
	}
	ShowImGui();
	if (!mNothingSelected) {
		auto current_song = &std::get<1>(mSongList[mPlayingIndex]);
		if (ma_sound_at_end(current_song)) {
			mPlayingIndex++;
			if (mPlayingIndex >= mSongList.size())
				mPlayingIndex = 0;
			TryPlay(mPlayingIndex, true);
		}
	}
}

void TwistedMusic::ShowImGui() {
	ImGui::Begin("TwistedMusic", &mImGuiOpen, IMGUI_FLAGS);
	if (!mNothingSelected) {
		auto current_song = &std::get<1>(mSongList[mPlayingIndex]);
		float length;
		ma_sound_get_length_in_seconds(current_song, &length);
		mPlayingPos = (float)(ma_int64)ma_sound_get_time_in_milliseconds(current_song) / 1000.0f;
		if (ImGui::SliderFloat("", &mPlayingPos, 0.0f, length))
			ma_sound_seek_to_second(current_song, mPlayingPos);
	}
	ImGui::BeginChild("Scroll");
	for (size_t i = 0; i < mSongList.size(); i++)
		if (ImGui::RadioButton(std::get<0>(mSongList[i]).c_str(), !mNothingSelected && i == mPlayingIndex))
			TryPlay(i);
	ImGui::EndChild();
	ImGui::End();
}

void TwistedMusic::TryPlay(size_t index, bool forcePlay) {
	if (!mNothingSelected && !forcePlay && index == mPlayingIndex) return;
	auto current_song = &std::get<1>(mSongList[mPlayingIndex]);
	if (ma_sound_is_playing(current_song)) {
		ma_sound_stop(current_song);
		ma_sound_seek_to_pcm_frame(current_song, 0);
	}

	mPlayingIndex = index;
	auto new_song = &std::get<1>(mSongList[mPlayingIndex]);
	ma_sound_start(new_song);

	mNothingSelected = false;
}

MOD_EXPORT IMod* BMLEntry(IBML* bml) {
	return new TwistedMusic(bml);
}

MOD_EXPORT void BMLExit(IMod* mod) {
	delete mod;
}