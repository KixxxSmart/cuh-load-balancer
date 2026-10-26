/** @file Config.h @brief Simulation settings. */
#ifndef CUH_CONFIG_H
#define CUH_CONFIG_H
#include "Firewall.h"
#include <string>
#include <vector>

namespace cuh {

enum class LogMode { Quiet, Verbose };

struct Config {
    std::uint32_t randomSeed = 412;
    double arrivalProbability = 0.5;
    int arrivalBatchMin = 1;
    int arrivalBatchMax = 3;
    int serviceCyclesMin = 10;
    int serviceCyclesMax = 100;
    int scaleCooldownCycles = 50;
    LogMode logMode = LogMode::Quiet;
    std::string logPath = "logs/cuh-run.txt";
    bool consoleColor = true;
    bool bonusMode = true;
    std::vector<IpRange> blockedRanges;

    static Config load(const std::string& path);

    void validate() const;
};

std::uint64_t parseUnsigned(const std::string& text, const std::string& name);
}
#endif
