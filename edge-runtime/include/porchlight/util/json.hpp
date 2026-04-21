#pragma once

#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace porchlight::json {

std::string escape(const std::string& value);
std::string quote(const std::string& value);
std::string bool_value(bool value);
std::string number(double value, int precision = 3);
std::string integer(std::int64_t value);
std::string string_array(const std::vector<std::string>& values);
std::string number_array(const std::vector<double>& values, int precision = 3);

}  // namespace porchlight::json
