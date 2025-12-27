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
		ImGui::BeginTabBar("tab_bar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton);
		if (ImGui::BeginTabItem("连接")) {
			if (mHandle) {
				if (ImGui::Button("断开连接")) CloseTheHandle(true);
			}
			else {
				if (ImGui::Button("查找程序")) SearchTheProcess();
				ImGui::Text("PID：%d", mPid);

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
					ImGui::Text("地址：%d", mAddr);

				if (ImGui::Button("连接")) InjectTheRustProgram();
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("参数")) {
			ImGui::TextDisabled("播放器写死了音调必须在[0.1, 10]范围内");
			ImGui::SliderFloat("基础音调", &mBasePitch, 0.f, 10.f, "%.2f");
			ImGui::SliderInt("除数", &mDivisor, 2, 100);
			char tmp[40];
			sprintf(tmp, "音调=%.2f+速度/%d", mBasePitch, mDivisor);
			if (ImGui::RadioButton(tmp, mMethod == CalcMethod::Linear)) mMethod = CalcMethod::Linear;
			sprintf(tmp, "音调=%.2f+(速度*速度)/(%d*%d)", mBasePitch, mDivisor, mDivisor);
			if (ImGui::RadioButton(tmp, mMethod == CalcMethod::Square)) mMethod = CalcMethod::Square;
			sprintf(tmp, "音调=%.2f+log%d(max(速度,1))", mBasePitch, mDivisor);
			if (ImGui::RadioButton(tmp, mMethod == CalcMethod::Log)) mMethod = CalcMethod::Log;
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();

		ImGui::Separator();
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

	auto* array = m_BML->GetArrayByName("CurrentLevel");
	if (array) {
		auto* ball = static_cast<CK3dObject*>(array->GetElementObject(0, 1));
		if (ball) {
			VxVector current_pos;
			ball->GetPosition(&current_pos);

			double dx = current_pos.x - mLastPos.x;
			double dy = current_pos.y - mLastPos.y;
			double dz = current_pos.z - mLastPos.z;
			auto length = sqrt(dx * dx + dy * dy + dz * dz);
			auto delta_time = m_BML->GetTimeManager()->GetLastDeltaTime();
			auto speed = length / delta_time * 1000;

			auto pitch = mBasePitch;
			switch (mMethod) {
			case CalcMethod::Linear: pitch = mBasePitch + speed / mDivisor; break;
			case CalcMethod::Square: pitch = mBasePitch + (speed * speed) / (mDivisor * mDivisor); break;
			case CalcMethod::Log: pitch = mBasePitch + log10(max(speed, 1.0)) / log10(mDivisor); break;
			}
			mPitchList.push_back(pitch);
			if (mPitchList.size() > 20)
				mPitchList.pop_front();
			mLastPos = current_pos;
		}
	}
}

bool TwistedMusic::SearchTheProcess() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return false;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	bool found = false;
	bool result = Process32First(snapshot, &pe);
	while (result) {
		if (_bstr_t(pe.szExeFile) == _bstr_t("twisted_music.exe")) {
			mPid = pe.th32ProcessID;
			found = true;
			break;
		}
		result = Process32Next(snapshot, &pe);
	}

	CloseHandle(snapshot);
	return found;
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
	mPid = 0;
	if (sendMessage) m_BML->SendIngameMessage("Twisted Music disconnected.");
}

MOD_EXPORT IMod* BMLEntry(IBML* bml) {
	return new TwistedMusic(bml);
}

MOD_EXPORT void BMLExit(IMod* mod) {
	delete mod;
}