#include <cctype>
#include <charconv>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
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
  JsonValue(const std::string &s) : value(s) {}
  JsonValue(const char *s) : value(std::string(s)) {}
  JsonValue(const std::vector<JsonValue> &v) : value(v) {}
  JsonValue(const std::map<std::string, JsonValue> &m) : value(m) {}

  JsonValue(const JsonValue &) = default;
  JsonValue &operator=(const JsonValue &) = default;

  JsonValue(JsonValue &&) = default;
  JsonValue &operator=(JsonValue &&) = default;

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

class JsonParser {
public:
  static JsonValue parse(std::string_view json) {
    JsonParser parser(json);
    return parser.start_parse();
  }

private:
  std::string_view src_;
  size_t pos_;
  JsonParser(std::string_view src) : src_(src), pos_(0) {}

  void skip_whitspace() {
    while (pos_ < src_.size() &&
           std::isspace(static_cast<unsigned char>(src_[pos_])))
      pos_++;
  }

  char peek() {
    skip_whitspace();
    if (pos_ >= src_.size())
      return 0;
    return src_[pos_];
  }

  bool consume(char expected) {
    skip_whitspace();
    if (pos_ < src_.size() && src_[pos_] == expected) {
      pos_++;
      return true;
    }
    return false;
  }

  void expect(char expected) {
    if (!consume(expected))
      throw std::runtime_error(std::string("Expected '") + expected +
                               "' at position " + std::to_string(pos_));
  }

  JsonValue start_parse() {
    char c = peek();
    switch (c) {
    case '{':
      return parse_object();
    case '[':
      return parse_array();
    default:
      throw std::runtime_error("Invalid JSON input");
    }
  }

  JsonValue parse_value() {
    char c = peek();
    switch (c) {
    case '{':
      return parse_object();
    case '[':
      return parse_array();
    case '"':
      return parse_string();
    case '-':
      return parse_number();
    case 't':
      return parse_literal("true", true);
    case 'f':
      return parse_literal("false", false);
    case 'n':
      return parse_literal("null", JsonValue{});
    }
    if (std::isdigit(c)) {
      return parse_number();
    }
    throw std::runtime_error("Invalid JSON input");
  }

  JsonValue parse_object() {
    expect('{');
    std::map<std::string, JsonValue> obj;
    if (consume('}'))
      return obj;
    while (true) {
      std::string key = parse_string_raw();
      expect(':');
      JsonValue val = parse_value();
      obj[std::move(key)] = std::move(val);
      if (consume('}'))
        break;
      expect(',');
    }
    return obj;
  }

  JsonValue parse_array() {
    expect('[');
    std::vector<JsonValue> arr;
    if (consume(']'))
      return arr;
    while (true) {
      arr.push_back(parse_value());
      if (consume(']'))
        break;
      expect(',');
    }
    return arr;
  }

  std::string parse_string_raw() {
    expect('"');
    std::string str;
    while (pos_ < src_.size()) {
      {
        char c = src_[pos_++];
        if (c == '"') {
          return str;
        }
        if (c == '\\') {
          if (pos_ >= src_.size())
            throw std::runtime_error("Unexpected end of input in string");
          char escape = src_[pos_++];
          switch (escape) {
          case '"':
            str += '"';
            break;
          case '\\':
            str += '\\';
            break;
          case '/':
            str += '/';
            break;
          case 'b':
            str += '\b';
            break;
          case 'f':
            str += '\f';
            break;
          case 'n':
            str += '\n';
            break;
          case 'r':
            str += '\r';
            break;
          case 't':
            str += '\t';
            break;
          case 'u': {
            uint32_t codepoint = parse_hex4();

            if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                if (pos_ + 2 < src_.size() && src_[pos_] == '\\' && src_[pos_ + 1] == 'u') {
                    pos_ += 2;
                    uint32_t low = parse_hex4();
                    if (low >= 0xDC00 && low <= 0xDFFF) {
                        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                    } else {
                        throw std::runtime_error("Invalid low surrogate in \\u sequence");
                    }
                } else {
                    throw std::runtime_error("Lone high surrogate in \\u sequence");
                }
            }else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                throw std::runtime_error("Unexpected low surrogate in \\u sequence");
            }

            encode_utf8(str, codepoint);
            break;
          }
          default:
            str += escape;
            break;
          }
        } else {
          str += c;
        }
      }
    }
    throw std::runtime_error("Unterminated string");
  }

  JsonValue parse_string() { return parse_string_raw(); }

  JsonValue parse_number() {
    size_t start = pos_;
    if (src_[pos_] == '-') {
      pos_++;
    }
    while (pos_ < src_.size() &&
           (std::isdigit(src_[pos_]) || src_[pos_] == '.' ||
            src_[pos_] == 'e' || src_[pos_] == 'E' || src_[pos_] == '+' ||
            src_[pos_] == '-')) {
      pos_++;
    }

    double val;
    auto res = std::from_chars(src_.data() + start, src_.data() + pos_, val);

    if (res.ec != std::errc()) {
      throw std::runtime_error("Failed to parse number");
    }

    return val;
  }

  template <typename T> T parse_literal(std::string expected, T value) {
    for (char c : expected) {
      if (pos_ >= src_.size() || src_[pos_] != c) {
        throw std::runtime_error("Invalid literal");
      }
      pos_++;
    }
    return value;
  }

  uint16_t parse_hex4() {
    uint16_t val = 0;
    for (int i = 0; i < 4; ++i) {
      if (pos_ >= src_.size())
        throw std::runtime_error("Unexpected end of input in \\u sequence");

      char c = src_[pos_++];
      val <<= 4;

      if (c >= '0' && c <= '9')
        val |= (c - '0');
      else if (c >= 'a' && c <= 'f')
        val |= (c - 'a' + 10);
      else if (c >= 'A' && c <= 'F')
        val |= (c - 'A' + 10);
      else
        throw std::runtime_error("Invalid hex digit in \\u sequence");
    }
    return val;
  }

  void encode_utf8(std::string &out, uint32_t codepoint) {
    if (codepoint <= 0x7F) {
      out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
      out += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
      out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
      out += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
      out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
      out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
  }
};

}; // namespace WheelJSON
