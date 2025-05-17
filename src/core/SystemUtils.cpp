#include "core/SystemUtils.h"

#include <filesystem>


namespace SystemUtils {

std::string getExecutableDir() {
    return std::filesystem::path(getExecutablePath()).parent_path().string();
}

}  // namespace SystemUtils