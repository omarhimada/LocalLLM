#include "LocalLLM.HelperFunctions.h"

std::wstring getExeDir() {
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, MAX_PATH);

	const std::wstring exePath(path);
	const size_t pos = exePath.find_last_of(L"\\/");
	return exePath.substr(0, pos);
}

QString fromExe(const wchar_t* relative) {
	std::wstring full = getExeDir() + L"\\" + relative;
	return QString::fromWCharArray(full.c_str());
}

std::string wideToUtf8(const std::wstring& w) {
	if (w.empty()) return {};

	const int size = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
	std::string result(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), result.data(), size, nullptr, nullptr);
	return result;
}

std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return {};
	
	const int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), wstrTo.data(), size_needed);
	return wstrTo;
}