#include "SystemUtils.h"

#include <linux/limits.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace SystemUtils {

void enableHighDPI() {}

void addToStartup(const std::string& appName) {
    std::string exePath = getExecutablePath();
    std::string autostartDir =
        std::string(std::getenv("HOME")) + "/.config/autostart/";
    std::filesystem::create_directories(autostartDir);  // 确保目录存在

    std::string desktopEntryPath = autostartDir + appName + ".desktop";

    std::ofstream out(desktopEntryPath);
    out << "[Desktop Entry]\n";
    out << "Version=1.0\n";
    out << "Type=Application\n";
    out << "Name=" << appName << "\n";
    out << "Exec=" << exePath << "\n";
    out << "Terminal=true\n";  // 打开终端运行
    out << "Hidden=false\n";
    out << "NoDisplay=false\n";
    out << "X-GNOME-Autostart-enabled=true\n";
    out.close();
}

void removeFromStartup(const std::string& appName) {
    std::string desktopEntryPath = std::string(std::getenv("HOME")) +
                                   "/.config/autostart/" + appName + ".desktop";
    std::filesystem::remove(desktopEntryPath);
}

std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0 || count == PATH_MAX) return "";
    return std::string(result, count);
}

}  // namespace SystemUtils