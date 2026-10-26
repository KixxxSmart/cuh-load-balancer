/** @file SimulationStats.h @brief Run counters and server status. */
#ifndef CUH_SIMULATION_STATS_H
#define CUH_SIMULATION_STATS_H
#include <cstddef>
#include <cstdint>

namespace cuh {

struct SimulationStats {
    std::size_t initialServers = 0;
    std::size_t initialQueue = 0;
    std::uint64_t generated = 0;
    std::uint64_t accepted = 0;
    std::uint64_t blocked = 0;
    std::uint64_t completed = 0;
    std::uint64_t scaleUps = 0;
    std::uint64_t scaleDowns = 0;
    std::uint64_t serversRemoved = 0;
    std::size_t peakQueue = 0;
    std::uint64_t cycle = 0;
    std::size_t queued = 0;
    std::size_t activeServers = 0;
    std::size_t busyServers = 0;
    std::size_t idleServers = 0;
    std::size_t retiringServers = 0;

    bool accountingConsistent() const {
        return generated == accepted + blocked &&
               accepted == completed + queued + busyServers;
    }
};
}
#endif
