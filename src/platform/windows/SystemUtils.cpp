#include <shellscalingapi.h>
#include <windows.h>

#include "SystemUtils.h"

namespace SystemUtils {

void addToStartup(const std::string& appName) {
    std::string exePath = getExecutablePath();
    HKEY hKey = nullptr;
    const char* runKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegSetValueExA(hKey, appName.c_str(), 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(exePath.c_str()),
                       static_cast<DWORD>(exePath.size() + 1));
        RegCloseKey(hKey);
    }
}

void removeFromStartup(const std::string& appName) {
    HKEY hKey = nullptr;
    const char* runKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegDeleteValueA(hKey, appName.c_str());
        RegCloseKey(hKey);
    }
}

std::string getExecutablePath() {
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length == 0 || length == MAX_PATH) return "";  // 失败或路径太长
    return std::string(path, length);
}

void enableHighDPI() {
    // 启用高 DPI 感知
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

}  // namespace SystemUtils