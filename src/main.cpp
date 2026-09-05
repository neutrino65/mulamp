#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <algorithm>

ESP8266WiFiMulti WiFiMulti;

// wifi setting
#define SSID "FTTH"
#define PASSWORD "abcd@1234"

// mqtt broker setting
#define MQTT_SERVER "57d6613d04174cecba266899377455d9.s1.eu.hivemq.cloud"
#define MQTT_USER "nomnom1"
#define MQTT_PASSWORD "4cheleon^04"
#define MQTT_PORT 8883
#define MQTT_TOPIC "relay-status"

// relay & button setting
#define RELAY_PIN D1
#define BUTTON_PIN D5

bool device_relay_status = true;  // this will store the current relay's status as high and low

#define ESP_DEVICE_ID "1"

// WiFi and MQTT client initialization
BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// SSL certificate for MQTT broker
const char *cert PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
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
-----END CERTIFICATE-----
)EOF";

bool connectToWiFi() {
    int failedAttemptsNum = 0;
    Serial.print("Connecting to WiFi");
    while ((WiFiMulti.run() != WL_CONNECTED && ++failedAttemptsNum <= 10)) {
        delay(1000);
        Serial.print(".");
    }

    if (failedAttemptsNum >= 10){
        Serial.println("Failed to connect to WiFi.");
        return false;
    }

    return true;
} 

bool waitForWiFi(unsigned long timeoutMS = 10000) {  // take 10 sec to connect to wifi, if no connecting then it will loop out so other function can execute.
  unsigned long wifi_start = millis();
  while (millis() - wifi_start < timeoutMS) {
    if (WiFiMulti.run() == WL_CONNECTED) return true;
    delay(200);   // short yield
  }
  return false;
}

// void setClock() {  // this is blocking method to configure NTP time sync.
//   configTime(0, 0, "pool.ntp.org", "time.nist.gov");

//   Serial.print("Waiting for NTP time sync: ");
//   time_t now = time(nullptr);
//   while (now < 8 * 3600 * 2) {
//       delay(500);
//       Serial.print(".");
//       now = time(nullptr);
//   }
//   Serial.println("");
//   struct tm timeinfo;
//   gmtime_r(&now, &timeinfo);
//   Serial.print("Current time: ");
//   Serial.print(asctime(&timeinfo));
// }


const unsigned long TIME_RETRY = 15000;
unsigned long timeRequestedAt = 0;
bool timeSynced = false;

void requestTimeSync() {  // this is non-blocking method to configure NTP time sync.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  timeRequestedAt = millis();
}

bool isTimeSynced() {
  time_t now = time(nullptr);
  return now > 1600000000UL;  // this NTP timestamp is equal to approx sep 2020
}

String getCurrentTime() {   // this function is to get current time 
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);      // output be like-> 2026-08-31 09:07:35
}


bool connectToMqttOnce() {
    BearSSL::X509List serverTrustedCA(cert);
    espClient.setTrustAnchors(&serverTrustedCA);

    // checking if mqtt connection is already enstablished.
    if (mqtt_client.connected()) return true;

    // doing single try to enstablish mqtt connection.
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
    if (mqtt_client.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD)) {
        Serial.println("Connected to MQTT broker");
        mqtt_client.subscribe(MQTT_TOPIC, 1);    // this 1 is the qos.
        // Publish message upon successful connection
        mqtt_client.publish(MQTT_TOPIC, "Hi SERVER I'm device 1 ^_^");  // the true is for retained message = true
        return true;
    } else {
        char err_buf[128];
        espClient.getLastSSLError(err_buf, sizeof(err_buf));
        Serial.print("Failed to connect to MQTT broker, rc=");
        Serial.println(mqtt_client.state());
        Serial.print("SSL error: ");
        Serial.println(err_buf);
        return false;
    }
}


volatile bool msgReady = false;
char latestMessage[128];

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  size_t n = std::min<size_t>((size_t)length, sizeof(latestMessage)-1);
  memcpy(latestMessage, payload, n);
  latestMessage[n] = '\0';
  msgReady = true;
}

// change the device relay status as per the broker's relay status
void updateRelayStatus(const char* tmp_relay_status) {
  if (device_relay_status && (strcmp(tmp_relay_status, "HIGH") == 0)) {
    return;
  } else if (!device_relay_status && (strcmp(tmp_relay_status, "LOW") == 0)) {
    return;
  } else {
    device_relay_status = (strcmp(tmp_relay_status, "HIGH") == 0);   // strcmp when both str same then they return 0. thus this will make the device_relay_status as true if broker relay_status be HIGH.
    return;
  }
}


void setup(){ 
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, PASSWORD);

  // // this is blocking way to sync NTP time.
  // if (connectToWiFi()) {
  //     setClock();
  // }

  // connecting to wifi (blocking for 5 sec)
  if (!waitForWiFi(5000)) {
    Serial.println("WiFi timed out, continuing offline");
  }

  requestTimeSync(); // this is non-blocking way to sync NTP time.

  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(mqttCallback);
}

void loop() {
  // keep wifi stack running (non-blocking)
  if (WiFi.status() != WL_CONNECTED) {
    WiFiMulti.run();  // non-blocking wifi connect.
  }

  // configuring ntp time sync (non-blocking)
  if (!timeSynced) {
    if (isTimeSynced()) {
      timeSynced = true;
      Serial.println("NTP synced: " + getCurrentTime());
    } else if (millis() - timeRequestedAt > TIME_RETRY) {
      // retry requestTimeSync() every TIME_RETRY until synced
      Serial.println("NTP not ready, retyring requestTimeSync()");
      requestTimeSync();
    }
  }

  // non-breaking mqtt connection check and re-enstablishment
  static unsigned long lastMqttAttempt = 0;  // latest mqtt connection try time
  const unsigned long mqttRetryTime = 5000; // every 5 sec
  if (WiFi.status() == WL_CONNECTED && timeSynced) {  // mqtt connectiong occur after wifi is connected and NTP time is synced.
    if (!mqtt_client.connected()) {
      unsigned long mqttNow = millis();
      if (mqttNow - lastMqttAttempt >= mqttRetryTime) {
        lastMqttAttempt = mqttNow;
        connectToMqttOnce();
      }
    } else {
      mqtt_client.loop(); // connected, thus keep the mqtt connection alive
    }
  }

  // checking if there is any new message from mqtt broker and acting accordingly.
  if (msgReady) {
    msgReady = false;
    Serial.println(latestMessage);

    // parsing the json data from the broker
    StaticJsonDocument<256> doc;
    // here doc is the destination buffer and latestMessage contain the raw json data
    DeserializationError err = deserializeJson(doc, latestMessage);  
    if (!err){
      // json responce is like {"device_id":"1","relay_status":"LOW","timestamp":"2026-08-31 19:10:37"}
      const char* device_id = doc["device_id"] | nullptr;   // using nullptr as if the key is not found then excvaddr error will occur causing "null dereference" and crashing the esp8266.
      const char* relay_status = doc["relay_status"] | nullptr;
      if (device_id) Serial.println(device_id);
      if (relay_status) {
        Serial.println(relay_status);
        /*
        First retrieve the mqtt broker's relay_status and compare that with device's relay_status.
        if different then update the device's relay_status as per broker's relay_status.
        when relay is turned on via button then publish the latest relay status to the broker server.
        */
        updateRelayStatus(relay_status);
      }
    }
  }

  if (device_relay_status) Serial.println("RELAY ON!");

  // sending json data to mqtt server
  if (mqtt_client.connected()) {
    // making json to send to mqtt broker server (testing rn).
    JsonDocument doc;
    doc["device_id"] = ESP_DEVICE_ID;
    if (device_relay_status) doc["relay_status"] = "HIGH";
    else if (!device_relay_status) doc["relay_status"] = "LOW";
    doc["timestamp"] = getCurrentTime();

    String mqttPayload;
    serializeJson(doc, mqttPayload);

    mqtt_client.publish(MQTT_TOPIC, mqttPayload.c_str(), true);
  }


}





