#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>

class CommandFetcher {
public:
    CommandFetcher(std::string_view serverUrl, std::string_view clientId)
        : serverUrl_(serverUrl), clientId_(clientId) {}

    std::optional<nlohmann::json> fetchCommands();

private:
    std::string_view serverUrl_;
    std::string_view clientId_;
};