#include "porchlight/comms/http_client.hpp"

#include <cstring>
#include <sstream>
#include <string>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace porchlight {
namespace {

struct ParsedUrl {
    std::string host{"127.0.0.1"};
    std::string port{"80"};
    std::string path_prefix;
};

ParsedUrl parse_url(std::string url) {
    ParsedUrl parsed;
    const std::string http = "http://";
    if (url.rfind(http, 0) == 0) url = url.substr(http.size());
    const auto slash = url.find('/');
    std::string hostport = slash == std::string::npos ? url : url.substr(0, slash);
    parsed.path_prefix = slash == std::string::npos ? "" : url.substr(slash);
    const auto colon = hostport.find(':');
    if (colon != std::string::npos) {
        parsed.host = hostport.substr(0, colon);
        parsed.port = hostport.substr(colon + 1);
    } else {
        parsed.host = hostport;
    }
    return parsed;
}

bool send_all(int fd, const std::string& data) {
    const char* ptr = data.data();
    std::size_t left = data.size();
    while (left > 0) {
        const ssize_t sent = ::send(fd, ptr, left, 0);
        if (sent <= 0) return false;
        ptr += sent;
        left -= static_cast<std::size_t>(sent);
    }
    return true;
}

}  // namespace

HttpClient::HttpClient(std::string base_url, std::string auth_token)
    : base_url_(std::move(base_url)), auth_token_(std::move(auth_token)) {}

bool HttpClient::post_json(const std::string& endpoint, const std::string& body, int timeout_ms) const {
    const auto url = parse_url(base_url_);
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* result = nullptr;
    if (getaddrinfo(url.host.c_str(), url.port.c_str(), &hints, &result) != 0) return false;

    int fd = -1;
    for (auto* rp = result; rp != nullptr; rp = rp->ai_next) {
        fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) continue;
        timeval tv{};
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(result);
    if (fd == -1) return false;

    const std::string path = (url.path_prefix.empty() ? "" : url.path_prefix) + endpoint;
    std::ostringstream req;
    req << "POST " << (path.empty() ? "/" : path) << " HTTP/1.1\r\n"
        << "Host: " << url.host << "\r\n"
        << "Connection: close\r\n"
        << "Content-Type: application/json\r\n"
        << "X-Device-Token: " << auth_token_ << "\r\n"
        << "Content-Length: " << body.size() << "\r\n\r\n"
        << body;

    bool ok = send_all(fd, req.str());
    char buffer[256];
    std::string response;
    if (ok) {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            response.assign(buffer);
        }
    }
    ::close(fd);
    return ok && (response.find("HTTP/1.1 2") != std::string::npos || response.find("HTTP/1.0 2") != std::string::npos);
}

}  // namespace porchlight
