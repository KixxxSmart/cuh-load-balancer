/** @file Firewall.h @brief Source-IP filtering. */
#ifndef CUH_FIREWALL_H
#define CUH_FIREWALL_H
#include "Request.h"
#include <vector>

namespace cuh {

struct IpRange {
    IPv4 first = 0;
    IPv4 last = 0;
};

IpRange parseIpRange(const std::string& text);

class Firewall {
public:

    explicit Firewall(std::vector<IpRange> ranges = {});

    bool isBlocked(IPv4 address) const;
private:
    std::vector<IpRange> ranges_;
};
}
#endif
