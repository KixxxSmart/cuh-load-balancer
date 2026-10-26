/** @file Firewall.cpp @brief Numeric range comparisons for the simulated firewall. */
#include "Firewall.h"
#include <stdexcept>
#include <utility>

namespace cuh {
IpRange parseIpRange(const std::string& text) {
    const auto separator = text.find('-');
    if (separator == std::string::npos || text.find('-', separator + 1) != std::string::npos) {
        throw std::invalid_argument("Expected an IPv4 range: first.address-last.address");
    }
    IpRange range{parseIPv4(text.substr(0, separator)),
                  parseIPv4(text.substr(separator + 1))};
    if (range.first > range.last) {
        throw std::invalid_argument("Blocked range begins after it ends: " + text);
    }
    return range;
}

Firewall::Firewall(std::vector<IpRange> ranges) : ranges_(std::move(ranges)) {
    for (const auto& range : ranges_) {
        if (range.first > range.last) {
            throw std::invalid_argument("Blocked range begins after it ends");
        }
    }
}

bool Firewall::isBlocked(IPv4 address) const {
    for (const auto& range : ranges_) {
        if (address >= range.first && address <= range.last) {
            return true;
        }
    }
    return false;
}
} 
