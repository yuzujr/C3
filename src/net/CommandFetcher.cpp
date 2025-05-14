#include <cpr/cpr.h>

#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

class CommandFetcher {
public:
    CommandFetcher(const std::string& serverUrl, const std::string& clientId)
        : serverUrl_(serverUrl), clientId_(clientId) {}

    void fetchAndHandleCommands() {
        while (true) {
            // 每隔 10 秒请求一次
            std::this_thread::sleep_for(std::chrono::seconds(10));
            cpr::Response r = cpr::Get(
                cpr::Url{serverUrl_ + "/commands?client_id=" + clientId_});

            if (r.status_code == 200) {
                try {
                    nlohmann::json commands = nlohmann::json::parse(r.text);
                    handleCommands(commands);
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing commands: " << e.what()
                              << std::endl;
                }
            } else {
                std::cerr << "Failed to fetch commands, status code: "
                          << r.status_code << std::endl;
            }
        }
    }

private:
    void handleCommands(const nlohmann::json& commands) {
        for (const auto& cmd : commands["commands"]) {
            std::string command = cmd["type"];
            if (command == "pause") {
                std::cout << "Pausing upload..." << std::endl;
                // TODO: Implement pausing logic
            } else if (command == "resume") {
                std::cout << "Resuming upload..." << std::endl;
                // TODO: Implement resume logic
            } else if (command == "update_config") {
                std::cout << "Updating configuration..." << std::endl;
                // TODO: Implement config update logic
            } else if (command == "screenshot_now") {
                std::cout << "Requesting screenshot..." << std::endl;
                // TODO: Implement screenshot capture and upload
            }
        }
    }

    std::string serverUrl_;
    std::string clientId_;
};