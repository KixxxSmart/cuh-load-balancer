/** @file Logger.h @brief Console and file logging. */
#ifndef CUH_LOGGER_H
#define CUH_LOGGER_H
#include "Config.h"
#include "SimulationStats.h"
#include <fstream>
#include <iostream>

namespace cuh {

class Logger {
public:

    explicit Logger(const Config& config, std::ostream& console = std::cout);

    void event(std::uint64_t cycle, const std::string& type, const std::string& message);

    void summary(const std::string& label, const SimulationStats& stats, const Config& config, bool includePeak = true);
private:
    LogMode mode_;
    bool color_;
    std::ofstream file_;
    std::ostream& console_;
};
}
#endif
