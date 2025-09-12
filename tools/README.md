# 0x3 ESP32 Development Tools

A collection of powerful development tools designed specifically for ESP32 projects.

## 🧰 Available Tools

### 1. Memory Analyzer
Analyzes memory usage in firmware, identifying memory issues and optimizing code.

```bash
./tools/memory_analyzer.py --elf .pio/build/d1_mini/firmware.elf
```

**Key Features:**
- Memory section analysis
- Largest symbols identification
- Stack usage estimation
- RAM/Flash overview

### 2. SPIFFS Uploader
Package and upload files to ESP32 SPIFFS filesystem.

```bash
./tools/spiffs_uploader.py --data ./data --upload
```

**Key Features:**
- Automatic data directory management
- File size estimation
- OTA and serial upload support
- Sample file generation

### 3. Config Generator
Generate configuration files from templates.

```bash
./tools/config_generator.py --interactive
```

**Key Features:**
- Interactive setup mode
- Environment file support
- Secure password generation
- Template-based configuration

### 4. Serial Monitor Plus
Enhanced serial monitoring with JSON support.

```bash
./tools/serial_monitor_plus.py --port /dev/ttyUSB0 --json
```

**Key Features:**
- JSON parsing and colorization
- Filtering capabilities
- Data logging
- CSV export

### 5. WiFi Scanner
Scan available WiFi networks and analyze signal strength.

```bash
./tools/wifi_scanner.py
```

**Key Features:**
- Network scanning
- Signal quality analysis
- Multiple output formats
- Sortable results

## 📋 Unified Interface

All tools can be accessed through the main `esp32_devtools.py` script:

```bash
./tools/esp32_devtools.py [command] [options]
```

Available commands:
- `memory` - Analyze memory usage
- `spiffs` - Manage SPIFFS filesystem
- `config` - Generate configuration files
- `monitor` - Enhanced serial monitoring
- `wifi` - Scan WiFi networks
- `test` - Run unit and integration tests
- `help` - Show help message

## 📝 Usage Examples

### Memory Analysis
```bash
./tools/esp32_devtools.py memory --elf .pio/build/esp32doit-devkit-v1/firmware.elf --top 20
```

### SPIFFS Upload
```bash
./tools/esp32_devtools.py spiffs --data ./data --upload --port /dev/ttyUSB0
```

### Interactive Configuration
```bash
./tools/esp32_devtools.py config --interactive
```

### JSON Serial Monitoring
```bash
./tools/esp32_devtools.py monitor --json --log output.txt
```

### WiFi Scanning
```bash
./tools/esp32_devtools.py wifi --format json --export wifi_scan.csv
```

### Test Runner

Run unit and integration tests:

```bash
# Run all tests
./tools/esp32_devtools.py test

# Run only native tests (no hardware needed)
./tools/esp32_devtools.py test --native

# Run only hardware tests
./tools/esp32_devtools.py test --hardware

# Generate detailed test reports
./tools/esp32_devtools.py test --report

# Filter tests by name
./tools/esp32_devtools.py test --filter test_wifi
```

## 🔧 Requirements

- Python 3.6+
- PlatformIO
- pyserial (`pip install pyserial`)

## 📚 Further Documentation

Each tool has detailed built-in help accessible via:

```bash
./tools/[tool_name].py --help
```

or

```bash
./tools/esp32_devtools.py [command] --help
```
