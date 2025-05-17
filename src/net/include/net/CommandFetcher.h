#include <string_view>

#include "core/CommandDispatcher.h"

class CommandFetcher {
public:
    CommandFetcher(std::string_view serverUrl, std::string_view clientId,
                   CommandDispatcher& dispatcher)
        : serverUrl_(serverUrl), clientId_(clientId), dispatcher_(dispatcher) {}

    void fetchAndHandleCommands();

private:
    std::string_view serverUrl_;
    std::string_view clientId_;
    CommandDispatcher& dispatcher_;
};