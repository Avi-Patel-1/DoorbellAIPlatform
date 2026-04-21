#include "porchlight/util/json.hpp"

#include <cmath>
#include <iomanip>

namespace porchlight::json {

std::string escape(const std::string& value) {
    std::ostringstream out;
    for (char c : value) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    out << c;
                }
        }
    }
    return out.str();
}

std::string quote(const std::string& value) { return "\"" + escape(value) + "\""; }
std::string bool_value(bool value) { return value ? "true" : "false"; }

std::string number(double value, int precision) {
    if (!std::isfinite(value)) return "0";
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

std::string integer(std::int64_t value) { return std::to_string(value); }

std::string string_array(const std::vector<std::string>& values) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ",";
        out << quote(values[i]);
    }
    out << "]";
    return out.str();
}

std::string number_array(const std::vector<double>& values, int precision) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ",";
        out << number(values[i], precision);
    }
    out << "]";
    return out.str();
}

}  // namespace porchlight::json
