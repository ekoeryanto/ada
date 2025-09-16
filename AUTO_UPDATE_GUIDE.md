# Auto-Update Server Setup Guide

## 🚀 ESP32 Auto-Update System Implementation Complete!

Sistem auto-update telah berhasil diimplementasikan dengan fitur-fitur berikut:

### ✅ Features Implemented:

1. **Configurable Auto-Update System**
   - Server URL dapat dikonfigurasi
   - Check interval dapat diatur (default: 1 jam)
   - Enable/disable auto-update
   - Maximum retry dengan delay

2. **Version Management**
   - Version comparison (semantic versioning)
   - Metadata lengkap (build date, git hash, release notes)
   - Firmware size verification
   - Checksum validation (opsional)

3. **API Endpoints**
   - `GET /api/auto-update/status` - Status dan informasi version
   - `GET /api/auto-update/config` - Konfigurasi saat ini
   - `POST /api/auto-update/config` - Update konfigurasi
   - `POST /api/auto-update/check` - Manual check for updates
   - `POST /api/auto-update/install` - Manual install update
   - `POST /api/auto-update/enable` - Enable auto-update
   - `POST /api/auto-update/disable` - Disable auto-update

### 📊 Firmware Stats:
- **Size**: 88.4% (1,390,877 bytes) - naik ~16KB untuk fitur auto-update
- **RAM**: 21.3% (69,896 bytes)
- **Build**: ✅ Success

---

## 🔧 Server-Side Implementation

Anda perlu membuat server yang menyediakan 2 endpoint:

### 1. Version Check Endpoint
**URL**: `{SERVER_URL}/version`  
**Method**: POST  
**Content-Type**: application/json

**Request Body:**
```json
{
  "current_version": "1.0.0",
  "device_id": "AA:BB:CC:DD:EE:FF",
  "hostname": "ada-1",
  "project": "ada-1"
}
```

**Response Body:**
```json
{
  "version": "1.0.1",
  "build_date": "Sep 15 2025",
  "build_time": "14:30:00",
  "git_hash": "a1b2c3d4",
  "firmware_size": 1400000,
  "checksum": "md5_hash_here",
  "download_url": "https://your-server.com/firmware/download/ada-1-v1.0.1.bin",
  "release_notes": "Bug fixes and improvements"
}
```

### 2. Firmware Download Endpoint
**URL**: `{SERVER_URL}/download`  
**Method**: GET  
**Response**: Binary firmware file

---

## 🎯 Usage Examples

### 1. Konfigurasi Auto-Update via API

```bash
# Update server URL
curl -X POST http://esp32-ip/api/auto-update/config \
  -H "Content-Type: application/json" \
  -d '{"server_url": "https://your-server.com/firmware"}'

# Set check interval to 30 minutes
curl -X POST http://esp32-ip/api/auto-update/config \
  -H "Content-Type: application/json" \
  -d '{"check_interval": 1800000}'

# Enable auto-update
curl -X POST http://esp32-ip/api/auto-update/enable
```

### 2. Manual Update Check

```bash
# Check for updates now
curl -X POST http://esp32-ip/api/auto-update/check

# Get status
curl http://esp32-ip/api/auto-update/status
```

### 3. Get Current Configuration

```bash
curl http://esp32-ip/api/auto-update/config
```

---

## 🔧 ESP32 Configuration

Default konfigurasi di `config.h`:

```cpp
#define AUTO_UPDATE_ENABLED true
#define AUTO_UPDATE_SERVER_URL "https://your-server.com/firmware"
#define AUTO_UPDATE_CHECK_INTERVAL 3600000         // 1 hour
#define AUTO_UPDATE_VERSION_ENDPOINT "/version"
#define AUTO_UPDATE_FIRMWARE_ENDPOINT "/download"
#define AUTO_UPDATE_MAX_RETRY 3
#define AUTO_UPDATE_RETRY_DELAY 300000             // 5 minutes
```

---

## 📋 Server Implementation Example (Node.js/Express)

```javascript
const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();

app.use(express.json());

// Version database (bisa diganti dengan database)
const versions = {
  "ada-1": {
    version: "1.0.1",
    build_date: "Sep 15 2025",
    build_time: "14:30:00",
    git_hash: "a1b2c3d4",
    firmware_size: 1400000,
    checksum: "md5_hash_here",
    download_url: "https://your-server.com/firmware/download/ada-1-v1.0.1.bin",
    release_notes: "Auto-update system implemented"
  }
};

// Version check endpoint
app.post('/firmware/version', (req, res) => {
  const { current_version, project } = req.body;
  const latest = versions[project];
  
  if (!latest) {
    return res.status(404).json({ error: "Project not found" });
  }
  
  // Simple version comparison
  if (latest.version !== current_version) {
    res.json(latest);
  } else {
    res.json({ version: current_version, message: "No update available" });
  }
});

// Firmware download endpoint
app.get('/firmware/download/:filename', (req, res) => {
  const filename = req.params.filename;
  const filePath = path.join(__dirname, 'firmware', filename);
  
  if (fs.existsSync(filePath)) {
    res.download(filePath);
  } else {
    res.status(404).json({ error: "File not found" });
  }
});

app.listen(3000, () => {
  console.log('Firmware server running on port 3000');
});
```

---

## 🧪 Testing Flow

1. **Upload firmware ke server** dengan version baru
2. **Update metadata** di server dengan version info
3. **ESP32 akan otomatis check** setiap interval yang dikonfigurasi
4. **Download dan install** otomatis jika ada update
5. **Restart** dan konfirmasi version baru

---

## 🔒 Security Considerations

1. **HTTPS wajib** untuk production
2. **Firmware signing** untuk verifikasi
3. **Authentication** untuk server access
4. **Rate limiting** untuk prevent abuse
5. **Rollback mechanism** jika update gagal

---

## 🎉 Ready to Use!

Auto-update system sudah siap digunakan! Tinggal:
1. Setup server sesuai contoh di atas
2. Update `AUTO_UPDATE_SERVER_URL` di config.h
3. Deploy firmware ke ESP32
4. Upload firmware baru ke server untuk testing

ESP32 akan otomatis check dan update firmware sesuai konfigurasi interval yang diset.