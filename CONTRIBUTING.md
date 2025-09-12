# Contributing to 0x3 Advanced ESP8266 Project

Thank you for your interest in contributing to the 0x3 Advanced ESP8266 Project! This document provides guidelines and instructions for contributing.

## Getting Started

1. **Fork the repository** and clone it locally.
2. **Set up the development environment**:
   - Install [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
   - Install required dependencies through PlatformIO

## Development Workflow

### Setting Up

1. Run `./setup.sh` to configure your project
2. Build the project with `pio run`
3. Upload to your device with `pio run --target upload`
4. Monitor with `pio device monitor`

### Code Structure

- `src/` - Main source files
- `include/` - Header files
- `lib/` - Custom libraries (if any)
- `platformio.ini` - Project configuration

## Key Components

### Module Structure

The project follows a modular architecture:

- **System Manager** (`system_manager.h/cpp`) - Core system operations
- **WiFi Manager** (`wifi_manager.h/cpp`) - Network connectivity
- **Web Server** (`web_server.h/cpp`) - HTTP server and API
- **OTA Handler** (`ota_handler.h/cpp`) - Over-the-air updates

### Adding New Features

When adding new features:

1. **Evaluate module placement** - Determine which module should contain your feature
2. **Follow coding style** - Match existing code style and patterns
3. **Ensure ESP8266 compatibility** - Verify memory consumption and performance
4. **Add documentation** - Update README.md and relevant documentation
5. **Test thoroughly** - Test on actual hardware before submitting

## Memory Considerations

ESP8266 has limited memory (80KB RAM, 4MB Flash typical). Keep this in mind:

- Optimize string usage (use F() macro for strings in flash)
- Avoid large buffers and arrays
- Consider using static allocation where appropriate
- Test with Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

## Pull Request Process

1. Create a focused PR that addresses a specific issue or feature
2. Include a clear description of changes and testing performed
3. Update documentation if necessary
4. Ensure your code builds without warnings
5. Test on actual ESP8266 hardware

## Coding Standards

- Use clear, descriptive variable and function names
- Add comments for complex algorithms or non-obvious code
- Follow existing code style and patterns
- Use proper header guards
- Implement proper error handling

## Testing

Test your changes on actual ESP8266 hardware before submitting a PR:

1. Verify functionality works as expected
2. Verify memory usage is reasonable
3. Test edge cases and error conditions
4. Test start-up, normal operation, and recovery scenarios

## Questions?

If you have questions, please open an issue in the repository.

Thank you for contributing!
