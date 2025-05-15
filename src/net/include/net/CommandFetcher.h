#include <nlohmann/json.hpp>
#include <string_view>

#include "core/ControlCenter.h"


class CommandFetcher {
public:
    CommandFetcher(std::string_view serverUrl, std::string_view clientId,
                   ControlCenter& controlCenter)
        : serverUrl_(serverUrl),
          clientId_(clientId),
          controlCenter_(controlCenter) {}

    void fetchAndHandleCommands();

private:
    void handleCommands(const nlohmann::json& commands);

    std::string_view serverUrl_;
    std::string_view clientId_;
    ControlCenter& controlCenter_;
};