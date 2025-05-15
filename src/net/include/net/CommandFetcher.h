#include <nlohmann/json.hpp>
#include <string_view>
#include "core/ControlCenter.h"

class CommandFetcher {
public:
    CommandFetcher(std::string_view serverUrl, std::string_view clientId)
        : serverUrl_(serverUrl), clientId_(clientId) {}

    void fetchAndHandleCommands();

private:
    void handleCommands(const nlohmann::json& commands);

    std::string_view serverUrl_;
    std::string_view clientId_;
};