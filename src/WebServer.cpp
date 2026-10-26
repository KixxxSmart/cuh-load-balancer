/** @file WebServer.cpp @brief Server countdown and retirement. */
#include "WebServer.h"
#include <stdexcept>

namespace cuh {
WebServer::WebServer(std::uint64_t id) : id_(id) {}

void WebServer::assign(const Request& request) {
    if (busy() || retiring_) throw std::logic_error("Server cannot accept work");
    if (request.processingTime < 1) throw std::invalid_argument("Task time must be positive");
    current_ = request;
    remainingCycles_ = request.processingTime;
}

std::optional<Request> WebServer::advance() {
    if (!current_) return std::nullopt;
    --remainingCycles_;
    if (remainingCycles_ > 0) return std::nullopt;
    auto completed = current_;
    current_.reset();
    return completed;
}

void WebServer::retire() { retiring_ = true; }
bool WebServer::busy() const { return current_.has_value(); }
bool WebServer::retiring() const { return retiring_; }
std::uint64_t WebServer::id() const { return id_; }
}
