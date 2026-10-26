/** @file WebServer.h @brief Remaining-cycle request processing. */
#ifndef CUH_WEB_SERVER_H
#define CUH_WEB_SERVER_H
#include "Request.h"
#include <optional>

namespace cuh {

class WebServer {
public:

    explicit WebServer(std::uint64_t id);

    void assign(const Request& request);

    std::optional<Request> advance();

    void retire();

    bool busy() const;

    bool retiring() const;

    std::uint64_t id() const;
private:
    std::uint64_t id_;
    std::optional<Request> current_;
    int remainingCycles_ = 0;
    bool retiring_ = false;
};
}
#endif
