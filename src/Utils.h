#ifndef UTILS_H
#define UTILS_H

#include <string>
#ifdef _WIN32
#include <windows.h>
#endif  // _WIN32

class Utils {
public:
#ifdef _WIN32
    static void addToStartup(const std::string& appName);
    static void removeFromStartup(const std::string& appName);
    static std::string getExecutablePath();
#endif  // _WIN32
};

#endif  // UTILS_H