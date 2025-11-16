#include "TwistedMusic.h"

void TwistedMusic::OnLoad() {
	m_BML->RegisterCommand(new TheCommand(this));
}

void TwistedMusic::OnUnload() {
	if (mHandle) {
		CloseHandle(mHandle);
		delete mHandle;
		mHandle = nullptr;
	}
}

void TwistedMusic::OnProcess() {
	if (mShowImgui) {
		ImGui::Begin("TwistedMusic", nullptr, ImGuiWindowFlags_NoCollapse);
		if (mHandle) {
			if (ImGui::Button("断开连接")) CloseTheHandle(true);
		}
		else {
			ImGui::InputScalar("PID", ImGuiDataType_U32, &mPid);

			std::optional<const char*> error_text;
			if (ImGui::InputText("地址（16进制）", mAddrTmp, 9, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank)) {
				try {
					mAddr = std::stoul(mAddrTmp, nullptr, 16);
				}
				catch (const std::invalid_argument&) { error_text = "Invalid argument"; }
				catch (const std::out_of_range&) { error_text = "Out of range"; }
			}
			if (error_text.has_value())
				ImGui::TextColored(ImVec4{ 1, 0, 0, 1 }, *error_text);
			else
				ImGui::TextUnformatted(std::to_string(mAddr).c_str());

			if (ImGui::Button("连接")) InjectTheRustProgram();
		}

		if (ImGui::Button("关闭面板")) mShowImgui = false;
		ImGui::End();
	}

	if (mHandle) {
		DWORD tmp;
		if (GetProcessHandleCount(mHandle, &tmp) && tmp == 0) {
			CloseTheHandle(true);
			return;
		}

		double pitch = 0;
		for (auto sub : mPitchList)
			pitch += sub;
		pitch /= mPitchList.size();
		WriteProcessMemory(mHandle, (LPVOID)mAddr, &pitch, sizeof(pitch), nullptr);
	}

	if (!m_BML->IsPlaying())
		return;

	double speed = 0;
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
			speed = length / delta_time * 1000;

			auto pitch = 0.75 + speed / 30;
			mPitchList.push_back(pitch);
			if (mPitchList.size() > 20)
				mPitchList.pop_front();
			mLastPos = current_pos;
		}
	}
}

void TwistedMusic::InjectTheRustProgram() {
	if (mHandle) CloseTheHandle();

	mHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, mPid);
	if (mHandle) {
		double tmp;
		if (ReadProcessMemory(mHandle, (LPCVOID)mAddr, &tmp, sizeof(tmp), nullptr))
			m_BML->SendIngameMessage("Twisted Music connected.");
		else
			CloseTheHandle();
	}
}

void TwistedMusic::CloseTheHandle(bool sendMessage) {
	if (!mHandle) return;
	auto tmp = 1919810.0;
	WriteProcessMemory(mHandle, (LPVOID)mAddr, &tmp, sizeof(tmp), nullptr);
	CloseHandle(mHandle);
	mHandle = nullptr;
	if (sendMessage) m_BML->SendIngameMessage("Twisted Music disconnected.");
}

MOD_EXPORT IMod* BMLEntry(IBML* bml) {
	return new TwistedMusic(bml);
}

MOD_EXPORT void BMLExit(IMod* mod) {
	delete mod;
}