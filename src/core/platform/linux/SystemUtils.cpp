#include "core/SystemUtils.h"

#include <linux/limits.h>
#include <unistd.h>

#include <filesystem>
#include <format>
#include <fstream>

namespace SystemUtils {

void enableHighDPI() {}

void addToStartup(const std::string& appName) {
    std::string exePath = getExecutablePath();
    std::string autostartDir =
        std::format("{}/.config/autostart", std::getenv("HOME"));
    std::filesystem::create_directories(autostartDir);  // 确保目录存在

    std::string desktopEntryPath =
        std::format("{}/{}.desktop", autostartDir, appName);

    std::ofstream out(desktopEntryPath);
    out << "[Desktop Entry]\n";
    out << "Version=1.0\n";
    out << "Type=Application\n";
    out << std::format("Name={}\n", appName);
    out << std::format("Exec={}\n", exePath);
    out << "Terminal=true\n";  // 打开终端运行
    out << "Hidden=false\n";
    out << "NoDisplay=false\n";
    out << "X-GNOME-Autostart-enabled=true\n";
    out.close();
}

void removeFromStartup(const std::string& appName) {
    std::string autostartDir =
        std::format("{}/.config/autostart", std::getenv("HOME"));
    std::string desktopEntryPath =
        std::format("{}/{}.desktop", autostartDir, appName);
    std::filesystem::remove(desktopEntryPath);
}

std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0 || count == PATH_MAX) return "";
    return std::string(result, count);
}

std::string getExecutableDir() {
    return std::filesystem::path(getExecutablePath()).parent_path().string();
}

}  // namespace SystemUtils
