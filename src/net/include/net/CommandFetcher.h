#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>

class CommandFetcher {
public:
    // 禁止创建实例
    CommandFetcher() = delete;

    static std::optional<nlohmann::json> fetchCommands(
        const std::string& serverUrl, const std::string& clientId);
};