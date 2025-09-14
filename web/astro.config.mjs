// @ts-check
import { defineConfig } from 'astro/config';
import tailwindcss from '@tailwindcss/vite';
import { loadEnv } from 'vite';

// Load environment variables manually
const env = loadEnv('', process.cwd(), '');

// Multi-RTU Configuration from environment
const RTU_1_HOST = env.RTU_1_HOST || 'localhost';
const RTU_1_PORT = env.RTU_1_PORT || '80';
const RTU_1_PROTOCOL = env.RTU_1_PROTOCOL || 'http';
const RTU_1_URL = `${RTU_1_PROTOCOL}://${RTU_1_HOST}:${RTU_1_PORT}`;

const RTU_2_HOST = env.RTU_2_HOST || 'localhost';
const RTU_2_PORT = env.RTU_2_PORT || '80';
const RTU_2_PROTOCOL = env.RTU_2_PROTOCOL || 'http';
const RTU_2_URL = `${RTU_2_PROTOCOL}://${RTU_2_HOST}:${RTU_2_PORT}`;

const RTU_3_HOST = env.RTU_3_HOST || 'localhost';
const RTU_3_PORT = env.RTU_3_PORT || '80';
const RTU_3_PROTOCOL = env.RTU_3_PROTOCOL || 'http';
const RTU_3_URL = `${RTU_3_PROTOCOL}://${RTU_3_HOST}:${RTU_3_PORT}`;

// Development Settings
const DEV_PROXY_ENABLED = env.DEV_PROXY_ENABLED === 'true';
const DEV_LOG_REQUESTS = env.DEV_LOG_REQUESTS === 'true';

// Use RTU 1 as primary/default server address
const PRIMARY_SERVER_ADDRESS = RTU_1_URL;

console.log(`[Astro Config] Primary RTU Target: ${PRIMARY_SERVER_ADDRESS}`);
console.log(`[Astro Config] RTU 1 (ada-1) Target: ${RTU_1_URL}`);
console.log(`[Astro Config] RTU 2 (ada-2) Target: ${RTU_2_URL}`);
console.log(`[Astro Config] RTU 3 (ada-3) Target: ${RTU_3_URL}`);

// https://astro.build/config
export default defineConfig({
  vite: {
    plugins: [tailwindcss()],
    define: {
      // Make RTU environment variables available to client-side code
      __RTU_1_HOST__: JSON.stringify(RTU_1_HOST),
      __RTU_1_PORT__: JSON.stringify(RTU_1_PORT),
      __RTU_1_PROTOCOL__: JSON.stringify(RTU_1_PROTOCOL),
      __RTU_1_URL__: JSON.stringify(RTU_1_URL),
      __RTU_2_HOST__: JSON.stringify(RTU_2_HOST),
      __RTU_2_PORT__: JSON.stringify(RTU_2_PORT),
      __RTU_2_PROTOCOL__: JSON.stringify(RTU_2_PROTOCOL),
      __RTU_2_URL__: JSON.stringify(RTU_2_URL),
      __RTU_3_HOST__: JSON.stringify(RTU_3_HOST),
      __RTU_3_PORT__: JSON.stringify(RTU_3_PORT),
      __RTU_3_PROTOCOL__: JSON.stringify(RTU_3_PROTOCOL),
      __RTU_3_URL__: JSON.stringify(RTU_3_URL),
      __PRIMARY_SERVER_ADDRESS__: JSON.stringify(PRIMARY_SERVER_ADDRESS),
    },
    server: DEV_PROXY_ENABLED ? {
      proxy: {
        // Proxy legacy API calls to primary RTU (RTU 1)
        '/api': {
          target: PRIMARY_SERVER_ADDRESS,
          changeOrigin: true,
          secure: false,
          configure: (proxy, _options) => {
            proxy.on('error', (err, _req, _res) => {
              console.log('[Proxy Error] Primary:', err.message);
            });
            if (DEV_LOG_REQUESTS) {
              proxy.on('proxyReq', (proxyReq, req, _res) => {
                console.log(`[Proxy] Primary → ${req.method} ${req.url} → ${PRIMARY_SERVER_ADDRESS}`);
              });
              proxy.on('proxyRes', (proxyRes, req, _res) => {
                console.log(`[Proxy] Primary ← ${proxyRes.statusCode} ${req.url}`);
              });
            }
          }
        },
        // Proxy RTU 1 (ada-1) specific API calls
        '/ada-1/api': {
          target: RTU_1_URL,
          changeOrigin: true,
          secure: false,
          rewrite: (path) => path.replace(/^\/ada-1\/api/, '/api'),
          configure: (proxy, _options) => {
            proxy.on('error', (err, _req, _res) => {
              console.log('[Proxy Error] ada-1:', err.message);
            });
            if (DEV_LOG_REQUESTS) {
              proxy.on('proxyReq', (proxyReq, req, _res) => {
                const rewrittenPath = (req.url || '').replace(/^\/ada-1\/api/, '/api');
                console.log(`[Proxy] ada-1 → ${req.method} ${req.url || ''} → ${RTU_1_URL}${rewrittenPath}`);
              });
              proxy.on('proxyRes', (proxyRes, req, _res) => {
                console.log(`[Proxy] ada-1 ← ${proxyRes.statusCode} ${req.url || ''}`);
              });
            }
          }
        },
        // Proxy RTU 2 (ada-2) specific API calls
        '/ada-2/api': {
          target: RTU_2_URL,
          changeOrigin: true,
          secure: false,
          rewrite: (path) => path.replace(/^\/ada-2\/api/, '/api'),
          configure: (proxy, _options) => {
            proxy.on('error', (err, _req, _res) => {
              console.log('[Proxy Error] ada-2:', err.message);
            });
            if (DEV_LOG_REQUESTS) {
              proxy.on('proxyReq', (proxyReq, req, _res) => {
                const rewrittenPath = (req.url || '').replace(/^\/ada-2\/api/, '/api');
                console.log(`[Proxy] ada-2 → ${req.method} ${req.url || ''} → ${RTU_2_URL}${rewrittenPath}`);
              });
              proxy.on('proxyRes', (proxyRes, req, _res) => {
                console.log(`[Proxy] ada-2 ← ${proxyRes.statusCode} ${req.url || ''}`);
              });
            }
          }
        },
        // Proxy RTU 3 (ada-3) specific API calls
        '/ada-3/api': {
          target: RTU_3_URL,
          changeOrigin: true,
          secure: false,
          rewrite: (path) => path.replace(/^\/ada-3\/api/, '/api'),
          configure: (proxy, _options) => {
            proxy.on('error', (err, _req, _res) => {
              console.log('[Proxy Error] ada-3:', err.message);
            });
            if (DEV_LOG_REQUESTS) {
              proxy.on('proxyReq', (proxyReq, req, _res) => {
                const rewrittenPath = (req.url || '').replace(/^\/ada-3\/api/, '/api');
                console.log(`[Proxy] ada-3 → ${req.method} ${req.url || ''} → ${RTU_3_URL}${rewrittenPath}`);
              });
              proxy.on('proxyRes', (proxyRes, req, _res) => {
                console.log(`[Proxy] ada-3 ← ${proxyRes.statusCode} ${req.url || ''}`);
              });
            }
          }
        }
      }
    } : {}
  }
});