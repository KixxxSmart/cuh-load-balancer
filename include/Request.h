/** @file Request.h @brief Request data and IPv4 conversion. */
#ifndef CUH_REQUEST_H
#define CUH_REQUEST_H
#include <cstdint>
#include <string>
#include <random>

namespace cuh {
using IPv4 = std::uint32_t;

struct Request {
    std::uint64_t id = 0;
    IPv4 ipIn = 0;
    IPv4 ipOut = 0;
    int processingTime = 1;
    char jobType = 'P';
};

Request generateRequest(std::mt19937& random, std::uint64_t id, int minimumCycles, int maximumCycles);

IPv4 parseIPv4(const std::string& text);

std::string ipv4ToString(IPv4 address);
}
#endif
