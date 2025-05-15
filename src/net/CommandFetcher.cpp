#include "net/CommandFetcher.h"

#include <cpr/cpr.h>

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "core/Logger.h"

void CommandFetcher::fetchAndHandleCommands() {
    std::string url = std::string(serverUrl_) +
                      "/commands?client_id=" + std::string(clientId_);
    cpr::Response r =
        cpr::Get(cpr::Url{url},
                 cpr::Header{{"User-Agent", "cpr/1.11.0"}, {"Accept", "*/*"}});

    if (r.status_code == 200) {
        try {
            nlohmann::json commands = nlohmann::json::parse(r.text);
            handleCommands(commands);
        } catch (const std::exception& e) {
            Logger::error(std::format("Error parsing commands: {}", e.what()));
        }
    } else {
        Logger::error(std::format("Failed to fetch commands, status code: {}",
                                  r.status_code));
    }
}

void CommandFetcher::handleCommands(const nlohmann::json& commands) {
    for (const auto& cmd : commands["commands"]) {
        std::string command = cmd["type"];
        if (command == "pause") {
            Logger::info("Pausing upload...");
            // TODO: Implement pausing logic
        } else if (command == "resume") {
            Logger::info("Resuming upload...");
            // TODO: Implement resume logic
        } else if (command == "update_config") {
            Logger::info("Updating configuration...");
            // TODO: Implement config update logic
        } else if (command == "screenshot_now") {
            Logger::info("Requesting screenshot...");
            // TODO: Implement screenshot capture and upload
        }
    }
}
