/** @file Request.cpp @brief Strict IPv4 parsing and formatting. */
#include "Request.h"

#include <charconv>
#include <stdexcept>
#include <limits>

namespace cuh {
Request generateRequest(std::mt19937& random, std::uint64_t id, int minimumCycles, int maximumCycles) {
    if (minimumCycles < 1 || maximumCycles < minimumCycles) {
        throw std::invalid_argument("Task times must be positive and ordered");
    }
    std::uniform_int_distribution<IPv4> address(0, std::numeric_limits<IPv4>::max());
    std::uniform_int_distribution<int> duration(minimumCycles, maximumCycles);
    std::bernoulli_distribution processing(0.5);
    Request request;
    request.id = id;
    request.ipIn = address(random);
    request.ipOut = address(random);
    request.processingTime = duration(random);
    request.jobType = processing(random) ? 'P' : 'S';
    return request;
}

IPv4 parseIPv4(const std::string& text) {
    IPv4 address = 0;
    std::size_t begin = 0;
    for (int part = 0; part < 4; ++part) {
        const auto end = text.find('.', begin);
        if ((part < 3 && end == std::string::npos) ||
            (part == 3 && end != std::string::npos)) {
            throw std::invalid_argument("Invalid IPv4 address: " + text);
        }
        const auto stop = end == std::string::npos ? text.size() : end;
        unsigned int octet = 0;
        const auto parsed = std::from_chars(text.data() + begin,
                                            text.data() + stop, octet);
        if (begin == stop || parsed.ec != std::errc{} ||
            parsed.ptr != text.data() + stop || octet > 255) {
            throw std::invalid_argument("Invalid IPv4 address: " + text);
        }
        address = (address << 8U) | octet;
        begin = stop + 1;
    }
    return address;
}

std::string ipv4ToString(IPv4 address) {
    return std::to_string((address >> 24U) & 255U) + "." +
           std::to_string((address >> 16U) & 255U) + "." +
           std::to_string((address >> 8U) & 255U) + "." +
           std::to_string(address & 255U);
}
} 
