/** @file main.cpp @brief User input and simulation loop. */
#include "Switch.h"
#include <limits>
#include <stdexcept>

namespace {
int readPositive(const std::string& prompt) {
    std::string input;
    while (true) {
        std::cout << prompt << std::flush;
        if (!std::getline(std::cin, input)) throw std::runtime_error("Input ended");
        try {
            auto value = cuh::parseUnsigned(input, "Input");
            if (value > 0 && value <= std::numeric_limits<int>::max()) {
                return static_cast<int>(value);
            }
        } catch (const std::invalid_argument&) {}
        std::cout << "Please enter a positive whole number.\n";
    }
}
}

/** Run the requested number of cycles using an optional settings file. */
int main(int argc, char* argv[]) {
    try {
        if (argc > 2) throw std::invalid_argument("Usage: ./build/cuh [config-file]");
        auto config = cuh::Config::load(argc == 2 ? argv[1] : "config/default.conf");
        int servers = readPositive("Starting servers: ");
        int cycles = readPositive("Clock cycles to run: ");
        cuh::Logger logger(config);
        if (config.bonusMode) {
            cuh::Switch router(config, logger);
            router.initialize(servers);
            router.writeStartingSummary();
            for (int elapsed = 0; elapsed < cycles; ++elapsed) router.step();
            router.writeEndingSummary();
            if (!router.processingStats().accountingConsistent() ||
                !router.streamingStats().accountingConsistent() ||
                !router.snapshot().accountingConsistent()) {
                throw std::runtime_error("Request accounting failed");
            }
        } else {
            cuh::LoadBalancer balancer(config, logger);
            balancer.initialize(servers);
            balancer.writeStartingSummary();
            for (int elapsed = 0; elapsed < cycles; ++elapsed) balancer.step();
            balancer.writeEndingSummary();
            if (!balancer.snapshot().accountingConsistent()) {
                throw std::runtime_error("Request accounting failed");
            }
        }
        std::cout << "\nLog saved to " << config.logPath << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CUH error: " << error.what() << '\n';
        return 1;
    }
}
