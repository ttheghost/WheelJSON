#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <stdexcept>

namespace WheelJSON {
struct JsonValue;

using JsonVariant =
    std::variant<std::monostate, bool, double, std::string,
                 std::vector<JsonValue>, std::map<std::string, JsonValue>>;

struct JsonValue {
  JsonVariant value;

  JsonValue() : value(std::monostate{}) {}
  JsonValue(bool b) : value(b) {}
  JsonValue(double d) : value(d) {}
  JsonValue(std::string s) : value(s) {}
  JsonValue(const char *s) : value(std::string(s)) {}
  JsonValue(std::vector<JsonValue> v) : value(v) {}
  JsonValue(std::map<std::string, JsonValue> m) : value(m) {}

  bool is_null() const { return std::holds_alternative<std::monostate>(value); }
  bool is_bool() const { return std::holds_alternative<bool>(value); }
  bool is_double() const { return std::holds_alternative<double>(value); }
  bool is_string() const { return std::holds_alternative<std::string>(value); }
  bool is_array() const {
    return std::holds_alternative<std::vector<JsonValue>>(value);
  }
  bool is_object() const {
    return std::holds_alternative<std::map<std::string, JsonValue>>(value);
  }

  bool as_bool() const { return std::get<bool>(value); }
  double as_double() const { return std::get<double>(value); }
  const std::string &as_string() const { return std::get<std::string>(value); }
  const std::vector<JsonValue> &as_array() const {
    return std::get<std::vector<JsonValue>>(value);
  }
  const std::map<std::string, JsonValue> &as_object() const {
    return std::get<std::map<std::string, JsonValue>>(value);
  }
};

JsonValue parse(std::string_view json) {
    JsonParser parser(json);
    return parser.parse_value();
}

class JsonParser {
    friend JsonValue parse(std::string_view json);
private:
    std::string_view src_;
    size_t pos_;
    JsonParser(std::string_view src): src_(src), pos_(0) {}

    void skip_whitspace() {
        while (pos_ < src_.size() && std::isspace(src_[pos_]))
            pos_++;
    }

    char peek() {
        skip_whitspace();
        if (pos_ >= src_.size()) return 0;
        return src_[pos_];
    }

    bool consume(char expected) {
        skip_whitspace();
        if (pos_ < src_.size() && src_[pos_] == expected)
        {
            pos_++;
            return true;
        }
        return false;
    }

    void expect(char expected) {
        if (!consume(expected))
            throw std::runtime_error(std::string("Expected '") + expected + "' at position " + std::to_string(pos_));
    }

    JsonValue parse_value() {
        char c = peek();
        return JsonValue{};
    }
};

}; // namespace WheelJSON
