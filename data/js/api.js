/*
  API adapter.

  Set USE_MOCK_API to false when your ESP8266 firmware is ready.
  The browser should load this page from the ESP8266 itself whenever
  possible, so relative API paths work automatically.
*/

const USE_MOCK_API = false;
const API_BASE = "";

const mockNetworks = [
  { ssid: "WiFi-Name-1", rssi: -42, security: "WPA2", channel: 6 },
  { ssid: "WiFi-Name-2", rssi: -56, security: "WPA2", channel: 11 },
  { ssid: "NINI_HOME", rssi: -63, security: "WPA/WPA2", channel: 1 },
  { ssid: "Guest_Network", rssi: -71, security: "Open", channel: 3 },
  { ssid: "Office_2.4G", rssi: -78, security: "WPA2", channel: 9 },
  { ssid: "Weak_Signal", rssi: -86, security: "WPA2", channel: 13 }
];

const mockDevice = {
  device: "ESP8266 NodeMCU 1.0",
  owner: "nini",
  initialization_date: "20 Sep '26",
  firmware_version: "1.0.0"
};

function wait(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function requestJson(path, options = {}) {
  const response = await fetch(`${API_BASE}${path}`, {
    headers: {
      "Content-Type": "application/json",
      ...(options.headers || {})
    },
    ...options
  });

  if (!response.ok) {
    throw new Error(`Request failed: ${response.status}`);
  }

  return response.json();
}

async function getDeviceInfo() {
  if (USE_MOCK_API) {
    await wait(250);
    return mockDevice;
  }

  return requestJson("/api/device");
}

async function scanNetworks() {
  if (USE_MOCK_API) {
    await wait(650);
    return {
      networks: [...mockNetworks].sort((a, b) => b.rssi - a.rssi)
    };
  }

  return requestJson("/api/wifi/scan");
}

async function getInternetStatus() {
  if (USE_MOCK_API) {
    await wait(500);
    return {
      connected: true,
      ssid: "NINI_HOME",
      rssi: -48
    };
  }

  return requestJson("/api/wifi/status");
}

async function submitWifiCredentials(ssid, password) {
  if (USE_MOCK_API) {
    await wait(800);

    if (!ssid || !password) {
      return {
        success: false,
        message: "SSID and password are required."
      };
    }

    return {
      success: true,
      message: "Demo success. Credentials were not sent to hardware."
    };
  }

  return requestJson("/api/wifi/connect", {
    method: "POST",
    body: JSON.stringify({ ssid, password })
  });
}
