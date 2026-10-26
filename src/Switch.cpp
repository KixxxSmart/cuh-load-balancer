/** @file Switch.cpp @brief Shared-clock processing and streaming simulation. */
#include "Switch.h"
#include <algorithm>
#include <stdexcept>

namespace {
cuh::Config streamingSettings(cuh::Config config) {
    ++config.randomSeed;
    return config;
}
}

namespace cuh {
Switch::Switch(const Config& config, Logger& logger)
    : config_(config), logger_(logger), random_(config.randomSeed + 2U),
      processing_(config, logger, 'P'), streaming_(streamingSettings(config), logger, 'S') {}

void Switch::initialize(std::size_t initialServers) {
    if (initialized_) throw std::logic_error("Switch is already initialized");
    if (initialServers < 2 || initialServers > 10000) {
        throw std::invalid_argument("Bonus mode needs between 2 and 10000 total starting servers");
    }
    processing_.initialize((initialServers + 1) / 2);
    streaming_.initialize(initialServers / 2);
    nextRequestId_ = std::max(processing_.snapshot().generated, streaming_.snapshot().generated) + 1;
    initialized_ = true;
}

void Switch::route(const Request& request, std::vector<Request>& processing,
                   std::vector<Request>& streaming) {
    if (request.jobType == 'P') processing.push_back(request);
    else if (request.jobType == 'S') streaming.push_back(request);
    else throw std::invalid_argument("Unknown job type");
}

void Switch::step() {
    if (!initialized_) throw std::logic_error("Initialize the Switch first");
    std::bernoulli_distribution arrives(config_.arrivalProbability);
    std::uniform_int_distribution<int> batch(config_.arrivalBatchMin, config_.arrivalBatchMax);
    const int count = arrives(random_) ? batch(random_) : 0;
    std::vector<Request> processing, streaming;
    for (int i = 0; i < count; ++i) {
        route(generateRequest(random_, nextRequestId_++, config_.serviceCyclesMin,
                              config_.serviceCyclesMax), processing, streaming);
    }
    processing_.step(processing);
    streaming_.step(streaming);
    ++cycle_;
}

SimulationStats Switch::snapshot() const {
    const auto processing = processing_.snapshot();
    const auto streaming = streaming_.snapshot();
    SimulationStats total;
    total.initialServers = processing.initialServers + streaming.initialServers;
    total.initialQueue = processing.initialQueue + streaming.initialQueue;
    total.generated = processing.generated + streaming.generated;
    total.accepted = processing.accepted + streaming.accepted;
    total.blocked = processing.blocked + streaming.blocked;
    total.completed = processing.completed + streaming.completed;
    total.scaleUps = processing.scaleUps + streaming.scaleUps;
    total.scaleDowns = processing.scaleDowns + streaming.scaleDowns;
    total.serversRemoved = processing.serversRemoved + streaming.serversRemoved;
    total.cycle = cycle_;
    total.queued = processing.queued + streaming.queued;
    total.activeServers = processing.activeServers + streaming.activeServers;
    total.busyServers = processing.busyServers + streaming.busyServers;
    total.idleServers = processing.idleServers + streaming.idleServers;
    total.retiringServers = processing.retiringServers + streaming.retiringServers;
    return total;
}

SimulationStats Switch::processingStats() const { return processing_.snapshot(); }
SimulationStats Switch::streamingStats() const { return streaming_.snapshot(); }

void Switch::writeStartingSummary() {
    processing_.writeStartingSummary();
    streaming_.writeStartingSummary();
    logger_.summary("COMBINED - STARTING SUMMARY", snapshot(), config_, false);
}

void Switch::writeEndingSummary() {
    processing_.writeEndingSummary();
    streaming_.writeEndingSummary();
    logger_.summary("COMBINED - ENDING SUMMARY", snapshot(), config_, false);
}
}
