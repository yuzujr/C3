#include "net/CommandFetcher.h"

#include <cpr/cpr.h>

#include <format>
#include <nlohmann/json.hpp>
#include <string_view>

#include "core/Logger.h"

void CommandFetcher::fetchAndHandleCommands() {
    std::string_view url =
        std::format("{}/commands?client_id={}", serverUrl_, clientId_);
    cpr::Response r =
        cpr::Get(cpr::Url{url},
                 cpr::Header{{"User-Agent", "cpr/1.11.0"}, {"Accept", "*/*"}});

    if (r.status_code == 200) {
        try {
            nlohmann::json commands = nlohmann::json::parse(r.text);
            dispatcher_.dispatch(commands);
        } catch (const std::exception& e) {
            Logger::error(std::format("Error parsing commands: {}", e.what()));
        }
    } else {
        Logger::error(std::format("Failed to fetch commands, status code: {}",
                                  r.status_code));
    }
}