/** @file LoadBalancer.cpp @brief Traffic, processing, and scaling. */
#include "LoadBalancer.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace cuh {
LoadBalancer::LoadBalancer(Config config, Logger& logger, char jobType)
    : config_(std::move(config)), jobType_(jobType),
      poolName_(jobType == 'P' ? "PROCESSING" : jobType == 'S' ? "STREAMING" : ""),
      logger_(logger), firewall_(config_.blockedRanges),
      random_(config_.randomSeed) {
    config_.validate();
    if (jobType_ != '\0' && jobType_ != 'P' && jobType_ != 'S') {
        throw std::invalid_argument("Unknown server-pool job type");
    }
}

void LoadBalancer::logEvent(const std::string& type, const std::string& message) {
    logger_.event(cycle_, type, (poolName_.empty() ? "" : "pool=" + poolName_ + " ") + message);
}

void LoadBalancer::initialize(std::size_t initialServers) {
    if (initialized_) throw std::logic_error("Simulation is already initialized");
    constexpr std::size_t maxSeedAttempts = 1000000;
    if (initialServers == 0 || initialServers > maxSeedAttempts / 100) {
        throw std::invalid_argument("Starting servers must be between 1 and 10000");
    }
    for (std::size_t i = 0; i < initialServers; ++i) {
        servers_.push_back(std::make_unique<WebServer>(nextServerId_++));
    }
    stats_.initialServers = initialServers;
    const auto desiredQueue = initialServers * 100;
    std::size_t attempts = 0;
    while (queue_.size() < desiredQueue && attempts < maxSeedAttempts) {
        auto request = generateRequest(random_, nextRequestId_++, config_.serviceCyclesMin, config_.serviceCyclesMax);
        if (jobType_ != '\0') request.jobType = jobType_;
        admit(request);
        ++attempts;
    }
    if (queue_.size() != desiredQueue) {
        throw std::runtime_error("Cannot fill the starting queue; check blocked IP ranges");
    }
    stats_.initialQueue = queue_.size();
    initialized_ = true;
}

void LoadBalancer::admit(const Request& request) {
    ++stats_.generated;
    if (firewall_.isBlocked(request.ipIn)) {
        ++stats_.blocked;
        logEvent("BLOCKED", "request=" + std::to_string(request.id) +
                      " input_ip=" + ipv4ToString(request.ipIn));
        return;
    }
    queue_.push(request);
    ++stats_.accepted;
    stats_.peakQueue = std::max(stats_.peakQueue, queue_.size());
}

void LoadBalancer::advanceServers() {
    for (auto& server : servers_) {
        auto completed = server->advance();
        if (completed) {
            ++stats_.completed;
            logEvent("COMPLETE", "server=" + std::to_string(server->id()) +
                          " request=" + std::to_string(completed->id));
        }
    }
    releaseRetiredServers();
}

void LoadBalancer::releaseRetiredServers() {
    for (auto server = servers_.begin(); server != servers_.end();) {
        if ((*server)->retiring() && !(*server)->busy()) {
            logEvent("REMOVE", "server=" + std::to_string((*server)->id()));
            server = servers_.erase(server);
            ++stats_.serversRemoved;
        } else {
            ++server;
        }
    }
}

void LoadBalancer::dispatch() {
    for (auto& server : servers_) {
        if (queue_.empty()) break;
        if (server->busy() || server->retiring()) continue;
        Request request = queue_.front();
        queue_.pop();
        server->assign(request);
        logEvent("ASSIGN", "server=" + std::to_string(server->id()) +
                      " request=" + std::to_string(request.id) +
                      " service_cycles=" + std::to_string(request.processingTime) +
                      " input_ip=" + ipv4ToString(request.ipIn) +
                      " output_ip=" + ipv4ToString(request.ipOut) +
                      " job_type=" + std::string(1, request.jobType));
    }
}

void LoadBalancer::scale() {
    if (cycle_ < nextScaleCycle_) return;
    const auto active = snapshot().activeServers;
    if (queue_.size() > 80 * active) {
        servers_.push_back(std::make_unique<WebServer>(nextServerId_++));
        ++stats_.scaleUps;
        logEvent("SCALE_UP", "active_servers=" + std::to_string(active + 1) +
                      " queued=" + std::to_string(queue_.size()));
    } else if (queue_.size() < 50 * active && active > 1) {
        auto candidate = servers_.end();
        for (auto server = servers_.begin(); server != servers_.end(); ++server) {
            if ((*server)->retiring()) continue;
            candidate = server;
            if (!(*server)->busy()) break;
        }
        (*candidate)->retire();
        ++stats_.scaleDowns;
        logEvent("SCALE_DOWN", "retiring_server=" + std::to_string((*candidate)->id()) +
                      " active_servers=" + std::to_string(active - 1) +
                      " queued=" + std::to_string(queue_.size()));
        releaseRetiredServers();
    } else {
        return;
    }
    nextScaleCycle_ = cycle_ + config_.scaleCooldownCycles;
}

void LoadBalancer::step() {
    if (jobType_ != '\0') throw std::logic_error("Dedicated pools require routed arrivals");
    if (!initialized_) throw std::logic_error("Initialize the simulation first");
    std::bernoulli_distribution arrives(config_.arrivalProbability);
    std::uniform_int_distribution<int> batch(config_.arrivalBatchMin, config_.arrivalBatchMax);
    const int count = arrives(random_) ? batch(random_) : 0;
    std::vector<Request> arrivals;
    for (int i = 0; i < count; ++i) {
        arrivals.push_back(generateRequest(random_, nextRequestId_++, config_.serviceCyclesMin, config_.serviceCyclesMax));
    }
    step(arrivals);
}

void LoadBalancer::step(const std::vector<Request>& arrivals) {
    if (!initialized_) throw std::logic_error("Initialize the simulation first");
    for (const auto& request : arrivals) {
        if (request.processingTime < 1 || (request.jobType != 'P' && request.jobType != 'S') ||
            (jobType_ != '\0' && request.jobType != jobType_)) {
            throw std::invalid_argument("Request does not match the server pool");
        }
    }
    ++cycle_;
    const auto acceptedBefore = stats_.accepted;
    for (const auto& request : arrivals) admit(request);
    if (!arrivals.empty()) {
        logEvent("ARRIVAL", "generated=" + std::to_string(arrivals.size()) +
                 " accepted=" + std::to_string(stats_.accepted - acceptedBefore));
    }
    advanceServers();
    dispatch();
    scale();
}

SimulationStats LoadBalancer::snapshot() const {
    auto result = stats_;
    result.cycle = cycle_;
    result.queued = queue_.size();
    for (const auto& server : servers_) {
        if (server->retiring()) ++result.retiringServers;
        else ++result.activeServers;
        if (server->busy()) ++result.busyServers;
        else if (!server->retiring()) ++result.idleServers;
    }
    return result;
}

void LoadBalancer::writeStartingSummary() {
    logger_.summary((poolName_.empty() ? "" : poolName_ + " - ") + "STARTING SUMMARY", snapshot(), config_);
}

void LoadBalancer::writeEndingSummary() {
    logger_.summary((poolName_.empty() ? "" : poolName_ + " - ") + "ENDING SUMMARY", snapshot(), config_);
}
}
