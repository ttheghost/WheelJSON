#include "json.hpp"
#include <iostream>

int main() {
  std::string json = R"({"name": "John", "age": 30, "scores": [95, 87, 92]})";

  try {
    auto value = WheelJSON::JsonParser::parse(json);

    if (value.is_object()) {
      const auto &obj = value.as_object();
      if (obj.count("name")) {
        std::cout << "Name: " << obj.at("name").as_string() << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Parse error: " << e.what() << std::endl;
  }
}
