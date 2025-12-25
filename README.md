# WheelJSON

> "Why use a library when you can spend a day writing your own?"

**WheelJSON** is a lightweight, high-performance JSON parser written from scratch in modern C++ (C++17).

It was built specifically to tackle Advent of Code 2015, Day 12, avoiding the temptation to simply #include `<nlohmann/json.hpp>` and instead opting to understand the internal mechanics of serialization, recursive descent parsing, and memory management.

## Why Reinvent the Wheel?

Yes, I know `RapidJSON` and `nlohmann/json` exist. They are battle-tested and excellent. But using them teaches me how to use an API, not how to build one.

### Features

1. **Single File**: Easy to drop into any C++ project.
2. **Type Safe**: Uses `std::variant` to handle dynamic JSON types without unsafe `void*` casting.
3. **Standard Compliant-ish**: Handles Objects, Arrays, Strings, Numbers (via fast `std::from_chars`), and Literals.
4. **Zero-Copy Lexing**: Uses `std::string_view` to scan the JSON buffer without creating temporary string objects.

## License

MIT. Feel free to reinvent your own wheel using this as a blueprint.
