/** @file Config.cpp @brief Configuration parsing with line-numbered errors. */
#include "Config.h"
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}
int integerSetting(const std::string& value, const std::string& key) {
    const auto number = cuh::parseUnsigned(value, key);
    if (number > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument(key + " is too large");
    }
    return static_cast<int>(number);
}
}

namespace cuh {
std::uint64_t parseUnsigned(const std::string& text, const std::string& name) {
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::invalid_argument(name + " must be an unsigned decimal integer");
    }
    return value;
}

Config Config::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open configuration file: " + path);
    Config config;
    std::set<std::string> seen;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) continue;
        try {
            const auto equals = line.find('=');
            if (equals == std::string::npos) throw std::invalid_argument("Expected key=value");
            const auto key = trim(line.substr(0, equals));
            const auto value = trim(line.substr(equals + 1));
            if (key.empty() || value.empty()) throw std::invalid_argument("Empty key or value");
            if (key != "blocked_range" && !seen.insert(key).second) {
                throw std::invalid_argument("Duplicate setting: " + key);
            }
            if (key == "random_seed") {
                const auto number = parseUnsigned(value, key);
                if (number > std::numeric_limits<std::uint32_t>::max()) {
                    throw std::invalid_argument("random_seed exceeds 32 bits");
                }
                config.randomSeed = static_cast<std::uint32_t>(number);
            } else if (key == "arrival_probability") {
                std::size_t used = 0;
                config.arrivalProbability = std::stod(value, &used);
                if (used != value.size()) throw std::invalid_argument("Invalid probability");
            } else if (key == "arrival_batch_min") config.arrivalBatchMin = integerSetting(value, key);
            else if (key == "arrival_batch_max") config.arrivalBatchMax = integerSetting(value, key);
            else if (key == "service_cycles_min") config.serviceCyclesMin = integerSetting(value, key);
            else if (key == "service_cycles_max") config.serviceCyclesMax = integerSetting(value, key);
            else if (key == "scale_cooldown_cycles") config.scaleCooldownCycles = integerSetting(value, key);
            else if (key == "log_path") config.logPath = value;
            else if (key == "log_mode") {
                if (value == "quiet") config.logMode = LogMode::Quiet;
                else if (value == "verbose") config.logMode = LogMode::Verbose;
                else throw std::invalid_argument("log_mode must be quiet or verbose");
            } else if (key == "console_color") {
                if (value == "true") config.consoleColor = true;
                else if (value == "false") config.consoleColor = false;
                else throw std::invalid_argument("console_color must be true or false");
            } else if (key == "bonus_mode") {
                if (value == "true") config.bonusMode = true;
                else if (value == "false") config.bonusMode = false;
                else throw std::invalid_argument("bonus_mode must be true or false");
            } else if (key == "blocked_range") config.blockedRanges.push_back(parseIpRange(value));
            else throw std::invalid_argument("Unknown setting: " + key);
        } catch (const std::exception& error) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (input.bad()) throw std::runtime_error("Error reading configuration file: " + path);
    config.validate();
    return config;
}

void Config::validate() const {
    if (!std::isfinite(arrivalProbability) || arrivalProbability < 0 || arrivalProbability > 1) {
        throw std::invalid_argument("arrival_probability must be between 0 and 1");
    }
    if (arrivalBatchMin < 1 || arrivalBatchMax < arrivalBatchMin) {
        throw std::invalid_argument("Arrival batch limits must be positive and ordered");
    }
    if (serviceCyclesMin < 1 || serviceCyclesMax < serviceCyclesMin) {
        throw std::invalid_argument("Service cycle limits must be positive and ordered");
    }
    if (scaleCooldownCycles < 1) {
        throw std::invalid_argument("Scaling cooldown must be positive");
    }
    if (logPath.empty()) throw std::invalid_argument("Log path cannot be empty");
    for (const auto& range : blockedRanges) {
        if (range.first > range.last) throw std::invalid_argument("Reversed blocked IP range");
    }
}
} 
