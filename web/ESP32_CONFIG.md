# ADA ESP32 Configuration

## ESP32 Device Configuration

Update the `ESP32_IP` variable in `astro.config.mjs` with your ESP32's actual IP address.

### To find your ESP32 IP address:

1. **From ESP32 Serial Monitor:**
   ```bash
   pio device monitor
   ```
   Look for WiFi connection messages showing the assigned IP.

2. **From Router Admin Panel:**
   Check your router's DHCP client list for the ESP32 device.

3. **Using Network Scanner:**
   ```bash
   # macOS/Linux
   nmap -sn 192.168.1.0/24
   
   # or use arp-scan
   arp-scan --local
   ```

4. **Windows Command:**
   ```cmd
   ipconfig /all
   ping esp32.local
   ```

### Current Configuration
- File: `web/astro.config.mjs`
- Variable: `ESP32_IP`
- Default: `192.168.1.100` (placeholder)

### Update Steps:
1. Find your ESP32's IP address
2. Edit `web/astro.config.mjs`
3. Update the `ESP32_IP` variable
4. Restart the Astro dev server

### Example:
```javascript
const ESP32_IP = '192.168.1.155'; // Your ESP32's actual IP
```

## Development Server

The Astro development server is configured to proxy API calls to your ESP32 device, eliminating CORS issues.

### Proxy Configuration:
- `/api/*` → `http://ESP32_IP/api/*`
- `/sensors/*` → `http://ESP32_IP/sensors/*`
- `/system/*` → `http://ESP32_IP/system/*`
- `/config/*` → `http://ESP32_IP/config/*`
- `/analytics/*` → `http://ESP32_IP/analytics/*`
- `/diagnostics/*` → `http://ESP32_IP/diagnostics/*`
- `/webhooks/*` → `http://ESP32_IP/webhooks/*`
- `/ota/*` → `http://ESP32_IP/ota/*`

### Starting Development:
```bash
cd web
npm run dev
```

The web interface will be available at `http://localhost:4321`