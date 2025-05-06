#include "Utils.h"

#ifdef _WIN32

void Utils::addToStartup(const std::string& appName) {
    std::string exePath = getExecutablePath();
    HKEY hKey = nullptr;
    const char* runKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegSetValueExA(hKey, appName.c_str(), 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(exePath.c_str()),
                       exePath.size() + 1);
        RegCloseKey(hKey);
    }
}

void Utils::removeFromStartup(const std::string& appName) {
    HKEY hKey = nullptr;
    const char* runKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegDeleteValueA(hKey, appName.c_str());
        RegCloseKey(hKey);
    }
}

std::string Utils::getExecutablePath() {
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length == 0 || length == MAX_PATH) return "";  // 失败或路径太长
    return std::string(path, length);
}

#endif // _WIN32