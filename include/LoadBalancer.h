/** @file LoadBalancer.h @brief Request queue and server management. */
#ifndef CUH_LOAD_BALANCER_H
#define CUH_LOAD_BALANCER_H
#include "Logger.h"
#include "WebServer.h"
#include <memory>
#include <queue>
#include <random>
#include <vector>

namespace cuh {

class LoadBalancer {
public:

    LoadBalancer(Config config, Logger& logger, char jobType = '\0');

    void initialize(std::size_t initialServers);

    void step();

    void step(const std::vector<Request>& arrivals);

    SimulationStats snapshot() const;

    void writeStartingSummary();

    void writeEndingSummary();
private:
    Config config_;
    char jobType_;
    std::string poolName_;
    Logger& logger_;
    Firewall firewall_;
    std::mt19937 random_;
    std::queue<Request> queue_;
    std::vector<std::unique_ptr<WebServer>> servers_;
    SimulationStats stats_;
    std::uint64_t cycle_ = 0;
    std::uint64_t nextScaleCycle_ = 0;
    std::uint64_t nextServerId_ = 1;
    std::uint64_t nextRequestId_ = 1;
    bool initialized_ = false;

    void logEvent(const std::string& type, const std::string& message);
    void admit(const Request& request);
    void advanceServers();
    void dispatch();
    void scale();
    void releaseRetiredServers();
};
}
#endif
