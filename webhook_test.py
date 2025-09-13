#!/usr/bin/env python3
"""
ESP32 Webhook Test Server
Simple HTTP server to receive and display webhook notifications from ESP32
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import time
from datetime import datetime
import threading
import requests

class WebhookHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Get content length
        content_length = int(self.headers.get('Content-Length', 0))
        
        # Read the POST data
        post_data = self.rfile.read(content_length)
        
        try:
            # Parse JSON payload
            webhook_data = json.loads(post_data.decode('utf-8'))
            
            # Print webhook received
            print(f"\n{'='*60}")
            print(f"🔔 WEBHOOK RECEIVED at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"{'='*60}")
            
            # Print headers
            print("📋 HEADERS:")
            for header, value in self.headers.items():
                if header.lower().startswith('x-'):
                    print(f"  {header}: {value}")
            
            # Print payload
            print("\n📦 PAYLOAD:")
            print(json.dumps(webhook_data, indent=2))
            
            # Log to file
            log_entry = {
                "timestamp": datetime.now().isoformat(),
                "headers": dict(self.headers),
                "payload": webhook_data
            }
            
            with open("webhook_log.json", "a") as f:
                f.write(json.dumps(log_entry) + "\n")
            
            # Send success response
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status": "success", "message": "Webhook received"}')
            
        except json.JSONDecodeError as e:
            print(f"❌ JSON Decode Error: {e}")
            print(f"Raw data: {post_data}")
            
            self.send_response(400)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status": "error", "message": "Invalid JSON"}')
        
        except Exception as e:
            print(f"❌ Error processing webhook: {e}")
            
            self.send_response(500)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status": "error", "message": "Internal server error"}')
    
    def do_GET(self):
        # Simple health check endpoint
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        response = {
            "status": "running",
            "message": "Webhook test server is active",
            "timestamp": datetime.now().isoformat()
        }
        self.wfile.write(json.dumps(response).encode())
    
    def log_message(self, format, *args):
        # Suppress default request logging
        pass

def run_webhook_server(port=8080):
    """Run the webhook test server"""
    server_address = ('', port)
    httpd = HTTPServer(server_address, WebhookHandler)
    
    print(f"🚀 Webhook Test Server Starting...")
    print(f"📍 Listening on: http://localhost:{port}")
    print(f"📁 Logs will be saved to: webhook_log.json")
    print(f"🛑 Press Ctrl+C to stop the server")
    print(f"{'='*60}")
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\n🛑 Server stopped by user")
        httpd.server_close()

def test_esp32_webhook_endpoints(esp32_ip="192.168.1.100"):
    """Test ESP32 webhook configuration endpoints"""
    base_url = f"http://{esp32_ip}"
    
    print(f"\n🧪 Testing ESP32 Webhook Endpoints")
    print(f"🎯 Target: {base_url}")
    print(f"{'='*60}")
    
    test_endpoints = [
        ("GET", "/api/webhooks/status", "Webhook Status"),
        ("GET", "/api/webhooks", "List Webhooks"),
        ("GET", "/api/webhooks/statistics", "Webhook Statistics"),
        ("GET", "/api/webhooks/queue", "Webhook Queue Status"),
    ]
    
    for method, endpoint, description in test_endpoints:
        try:
            url = base_url + endpoint
            print(f"\n📡 Testing: {description}")
            print(f"   {method} {url}")
            
            response = requests.get(url, timeout=10)
            
            if response.status_code == 200:
                data = response.json()
                print(f"   ✅ SUCCESS")
                print(f"   📄 Response: {json.dumps(data, indent=6)}")
            else:
                print(f"   ❌ FAILED (HTTP {response.status_code})")
                print(f"   📄 Response: {response.text}")
                
        except requests.exceptions.RequestException as e:
            print(f"   ❌ CONNECTION ERROR: {e}")
        except json.JSONDecodeError as e:
            print(f"   ❌ JSON ERROR: {e}")

def configure_esp32_webhook(esp32_ip="192.168.1.100", webhook_url="http://localhost:8080"):
    """Configure ESP32 to send webhooks to test server"""
    base_url = f"http://{esp32_ip}"
    
    print(f"\n⚙️  Configuring ESP32 Webhook")
    print(f"🎯 ESP32: {base_url}")
    print(f"🔗 Webhook URL: {webhook_url}")
    print(f"{'='*60}")
    
    try:
        # Add webhook configuration
        config_url = f"{base_url}/api/webhooks"
        data = {
            "url": webhook_url,
            "secret": "test-secret-123",
            "auth_token": ""
        }
        
        print(f"📤 Adding webhook configuration...")
        response = requests.post(config_url, data=data, timeout=10)
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Webhook configured successfully!")
            print(f"📋 Response: {json.dumps(result, indent=2)}")
            
            # Test the webhook
            test_url = f"{base_url}/api/webhooks/test"
            if "webhook_id" in result:
                test_data = {"webhook_id": result["webhook_id"]}
                print(f"\n🧪 Sending test webhook...")
                test_response = requests.post(test_url, data=test_data, timeout=10)
                
                if test_response.status_code == 200:
                    test_result = test_response.json()
                    print(f"✅ Test webhook sent!")
                    print(f"📋 Response: {json.dumps(test_result, indent=2)}")
                else:
                    print(f"❌ Test webhook failed (HTTP {test_response.status_code})")
            
        else:
            print(f"❌ Failed to configure webhook (HTTP {response.status_code})")
            print(f"📄 Response: {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ CONNECTION ERROR: {e}")
    except json.JSONDecodeError as e:
        print(f"❌ JSON ERROR: {e}")

def main():
    print("🔗 ESP32 Webhook Test Utility")
    print("=" * 50)
    
    import sys
    
    if len(sys.argv) > 1:
        command = sys.argv[1].lower()
        
        if command == "server":
            port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080
            run_webhook_server(port)
            
        elif command == "test":
            esp32_ip = sys.argv[2] if len(sys.argv) > 2 else "192.168.1.100"
            test_esp32_webhook_endpoints(esp32_ip)
            
        elif command == "configure":
            esp32_ip = sys.argv[2] if len(sys.argv) > 2 else "192.168.1.100"
            webhook_url = sys.argv[3] if len(sys.argv) > 3 else "http://localhost:8080"
            configure_esp32_webhook(esp32_ip, webhook_url)
            
        else:
            print("❌ Unknown command")
            print_usage()
    else:
        print_usage()

def print_usage():
    print("\n📖 Usage:")
    print("  python webhook_test.py server [port]          - Start webhook test server")
    print("  python webhook_test.py test [esp32_ip]        - Test ESP32 webhook endpoints")
    print("  python webhook_test.py configure [esp32_ip] [webhook_url] - Configure ESP32 webhook")
    print("\nExamples:")
    print("  python webhook_test.py server 8080")
    print("  python webhook_test.py test 192.168.1.100")
    print("  python webhook_test.py configure 192.168.1.100 http://your-server.com/webhook")

if __name__ == "__main__":
    main()