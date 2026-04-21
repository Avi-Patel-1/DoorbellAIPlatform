#pragma once

#include <string>

namespace porchlight {

class HttpClient {
public:
    explicit HttpClient(std::string base_url, std::string auth_token);
    bool post_json(const std::string& endpoint, const std::string& body, int timeout_ms = 1500) const;
    const std::string& base_url() const { return base_url_; }

private:
    std::string base_url_;
    std::string auth_token_;
};

}  // namespace porchlight
