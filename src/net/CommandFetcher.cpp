#include "net/CommandFetcher.h"

#include <cpr/cpr.h>

#include <format>

#include "core/Logger.h"

std::optional<nlohmann::json> CommandFetcher::fetchCommands(
    const std::string& serverUrl, const std::string& clientId) {
    std::string url =
        std::format("{}/commands?client_id={}", serverUrl, clientId);
    cpr::Response r =
        cpr::Get(cpr::Url{url},
                 cpr::Header{{"User-Agent", "cpr/1.11.0"}, {"Accept", "*/*"}});

    if (r.status_code == 200) {
        try {
            nlohmann::json commands = nlohmann::json::parse(r.text);
            return commands;
        } catch (const std::exception& e) {
            Logger::error(std::format("Error parsing commands: {}", e.what()));
        }
    } else {
        Logger::error(std::format("Failed to fetch commands, status code: {}",
                                  r.status_code));
    }

    return std::nullopt;  // 明确表示“失败”
}