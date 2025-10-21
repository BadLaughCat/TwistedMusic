#pragma once
#include <BML/BMLAll.h>
#include <codecvt>
#include <filesystem>
#include <imgui.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

constexpr auto IMGUI_FLAGS = //ImGuiWindowFlags_NoDecoration |
ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
ImGuiWindowFlags_NoBringToFrontOnFocus;

class TwistedMusic : public IMod {
public:
	explicit TwistedMusic(IBML* bml) : IMod(bml) {}

	const char* GetID() override { return "TwistedMusic"; }
	const char* GetVersion() override { return "1.0.0"; }
	const char* GetName() override { return "Twisted Music"; }
	const char* GetAuthor() override { return "Bad Laugh Cat"; }
	const char* GetDescription() override { return ""; }
	DECLARE_BML_VERSION;

	bool mImGuiOpen = true;
	bool mNothingSelected = true;

	ma_engine mAudioEngine;
	std::vector<std::tuple<std::string, ma_sound>> mSongList;
	size_t mPlayingIndex;
	float mPlayingPos;
	VxVector mLastPos;

	void OnLoad() override;
	void OnUnload() override;
	void OnProcess() override;
	void ShowImGui();
	void TryPlay(size_t index, bool forcePlay = false);
};

class AddCommand : public ICommand {
public:
	explicit AddCommand(TwistedMusic* _tm) : tm(_tm) {}
	std::string GetName() override { return "twistedmusic"; }
	std::string GetAlias() override { return "tm"; }
	std::string GetDescription() override { return "Load music from file or directory"; }
	bool IsCheat() override { return false; }

	void Execute(IBML* bml, const std::vector<std::string>& args) override {
		if (args.size() > 2) {
			std::string concat;
			for (size_t i = 2; i < args.size(); i++)
				concat += args[i] + " ";
			concat.pop_back();

			if (std::filesystem::is_directory(concat))
				for (const auto& entry : std::filesystem::directory_iterator(concat)) {
					auto ext = entry.path().extension();
					if (ext == ".mp3" || ext == ".wav" || ext == ".flac")
						TryLoadSound(entry.path(), bml);
					else
						continue;
				}
			else
				TryLoadSound(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(concat), bml);
		}
	}

	void TryLoadSound(std::wstring path, IBML* bml) {
		auto file_name = std::filesystem::path(path).filename();
		auto conv_name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(file_name);

		tm->mSongList.push_back(std::tuple(conv_name, ma_sound()));
		auto result = ma_sound_init_from_file_w(&tm->mAudioEngine, path.c_str(), MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, &std::get<1>(tm->mSongList.back()));
		if (result != MA_SUCCESS) {
			tm->mSongList.pop_back();
			return;
		}

		auto convert_back = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(path);
		bml->SendIngameMessage(("Loaded " + convert_back).c_str());
	}

	const std::vector<std::string> GetTabCompletion(IBML* bml, const std::vector<std::string>& args) override {
		if (args.size() == 2)
			return { "load" };
		return {};
	}

private:
	TwistedMusic* tm;
};