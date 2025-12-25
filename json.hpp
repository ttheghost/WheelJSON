#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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
}; // namespace WheelJSON
