/** @file Switch.h @brief Job-type routing to dedicated load balancers. */
#ifndef CUH_SWITCH_H
#define CUH_SWITCH_H
#include "LoadBalancer.h"

namespace cuh {
class Switch {
public:
    Switch(const Config& config, Logger& logger);
    void initialize(std::size_t initialServers);
    void step();
    SimulationStats snapshot() const;
    SimulationStats processingStats() const;
    SimulationStats streamingStats() const;
    void writeStartingSummary();
    void writeEndingSummary();
private:
    Config config_;
    Logger& logger_;
    std::mt19937 random_;
    LoadBalancer processing_;
    LoadBalancer streaming_;
    std::uint64_t cycle_ = 0;
    std::uint64_t nextRequestId_ = 1;
    bool initialized_ = false;

    void route(const Request& request, std::vector<Request>& processing,
               std::vector<Request>& streaming);
};
}
#endif
