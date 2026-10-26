/** @file Logger.cpp @brief Human-readable simulation events and reconciled summaries. */
#include "Logger.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace cuh {
Logger::Logger(const Config& config, std::ostream& console)
    : mode_(config.logMode), color_(config.consoleColor), console_(console) {
    const std::filesystem::path path(config.logPath);
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    file_.open(path);
    if (!file_) throw std::runtime_error("Cannot create log file: " + config.logPath);
    file_.exceptions(std::ios::badbit | std::ios::failbit);
}
void Logger::event(std::uint64_t cycle, const std::string& type, const std::string& message) {
    if (mode_ == LogMode::Quiet) return;
    std::ostringstream line;
    line << "[cycle " << cycle << "] " << type << " | " << message << '\n';
    file_ << line.str();
    if (!color_) {
        console_ << line.str();
        return;
    }
    const char* color = "\033[36m";
    if (type == "BLOCKED" || type == "SCALE_DOWN") color = "\033[33m";
    else if (type == "COMPLETE" || type == "SCALE_UP") color = "\033[32m";
    console_ << color << line.str() << "\033[0m";
}
void Logger::summary(const std::string& label, const SimulationStats& stats, const Config& config, bool includePeak) {
    std::ostringstream out;
    out << "\nCUH - CLOUD UTILITY HUB\n" << label << "\n"
        << "------------------------------------------------------------\n";
    auto field = [&out](const std::string& name, const auto& value) {
        out << std::left << std::setw(36) << name << ": " << value << '\n';
    };
    field("Clock cycles elapsed", stats.cycle);
    field("Initial servers", stats.initialServers);
    field("Starting queue size", stats.initialQueue);
    field("Current / ending queue size", stats.queued);
    field("Task time range (cycles)", std::to_string(config.serviceCyclesMin) + " to " + std::to_string(config.serviceCyclesMax));
    field("Random seed", config.randomSeed);
    field("Arrival probability per cycle", config.arrivalProbability);
    field("Arrival batch range", std::to_string(config.arrivalBatchMin) + " to " + std::to_string(config.arrivalBatchMax));
    field("Scaling cooldown (cycles)", config.scaleCooldownCycles);
    field("Blocked IP ranges", config.blockedRanges.size());
    field("Logging mode", config.logMode == LogMode::Quiet ? "quiet" : "verbose");
    field("Generated request attempts", stats.generated);
    field("Accepted requests", stats.accepted);
    field("Blocked / discarded requests", stats.blocked);
    field("Completed requests", stats.completed);
    field("In-flight requests", stats.busyServers);
    field("Active pool servers (busy + idle)", stats.activeServers);
    field("Busy servers (includes retiring)", stats.busyServers);
    field("Idle active servers", stats.idleServers);
    field("Retiring servers", stats.retiringServers);
    field("Servers added", stats.scaleUps);
    field("Servers retired", stats.scaleDowns);
    field("Server objects removed", stats.serversRemoved);
    if (includePeak) field("Peak waiting queue", stats.peakQueue);
    field("Work accounting", stats.accountingConsistent() ? "PASS" : "FAIL");
    out << "Accepted = completed + queued + in-flight\n"
        << "Generated = accepted + blocked (includes initial seeding)\n"
        << "Stop policy: unfinished work is counted; no extra drain cycles.\n";
    file_ << out.str();
    console_ << out.str();
    file_.flush();
}
}
