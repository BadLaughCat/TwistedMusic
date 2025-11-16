#pragma once
#include <BML/BMLAll.h>
#include <imgui.h>

#include <deque>
#include <optional>
#include <stdexcept>

#include <Windows.h>
#include <processthreadsapi.h>
#include <handleapi.h>
#include <memoryapi.h>

class TwistedMusic : public IMod {
public:
	explicit TwistedMusic(IBML* bml) : IMod(bml) {}

	const char* GetID() override { return "TwistedMusic"; }
	const char* GetVersion() override { return "1.0.0"; }
	const char* GetName() override { return "Twisted Music"; }
	const char* GetAuthor() override { return "Bad Laugh Cat"; }
	const char* GetDescription() override { return ""; }
	DECLARE_BML_VERSION;

	VxVector mLastPos;
	std::deque<double> mPitchList;

	bool mShowImgui = true;
	HANDLE mHandle = nullptr;
	DWORD mPid;
	char mAddrTmp[9];
	uint32_t mAddr;

	void OnLoad() override;
	void OnUnload() override;
	void OnProcess() override;
	void InjectTheRustProgram();
	void CloseTheHandle(bool sendMessage = false);
};

class TheCommand : public ICommand {
public:
	explicit TheCommand(TwistedMusic* _tm) : tm(_tm) {}
	std::string GetName() override { return "twistedmusic"; }
	std::string GetAlias() override { return "tm"; }
	std::string GetDescription() override { return "Open settings."; }
	bool IsCheat() override { return false; }

	void Execute(IBML* bml, const std::vector<std::string>& args) override {
		tm->mShowImgui = true;
	}

	const std::vector<std::string> GetTabCompletion(IBML* bml, const std::vector<std::string>& args) override {
		return {};
	}

private:
	TwistedMusic* tm;
};