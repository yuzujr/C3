#ifndef CONFIG_H
#define CONFIG_H

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#include "Logger.h"

class Config {
public:
    bool load(const std::string& path);
    void list() const;

    std::string upload_url = "";
    int interval_seconds = 60;
    int max_retries = 3;
    int retry_delay_ms = 1000;

private:
    nlohmann::json json_data;
};

#endif  // CONFIG_H