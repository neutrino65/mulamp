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

bool device_relay_status = false;   // this will store the current relay's status as true -> HIGH & false -> LOW.

#define ESP_DEVICE_ID "1"

// WiFi and MQTT client initialization
BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// SSL certificate for MQTT broker (HiveMQ)
const char *cert PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
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
-----END CERTIFICATE-----
)EOF";


// -------------------- WIFI ---------------------

// bool connectToWiFi() {
//     int failedAttemptsNum = 0;
//     Serial.print("Connecting to WiFi");
//     while ((WiFiMulti.run() != WL_CONNECTED && ++failedAttemptsNum <= 10)) {
//         delay(1000);
//         Serial.print(".");
//     }

//     if (failedAttemptsNum >= 10){
//         Serial.println("Failed to connect to WiFi.");
//         return false;
//     }

//     return true;
// } 

bool waitForWiFi(unsigned long timeoutMS = 10000) {  // take 10 sec to connect to wifi, if no connecting then it will loop out so other function can execute.
  unsigned long wifi_start = millis();
  while (millis() - wifi_start < timeoutMS) {
    if (WiFiMulti.run() == WL_CONNECTED) return true;
    delay(200);   // short yield
  }
  return false;
}


// --------------- NTP and clock syncronization ------------------

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


const unsigned long NTP_TIME_RETRY = 15000;
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


// ---------------- MQTT -----------------

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
        //mqtt_client.publish(MQTT_TOPIC, "Hi SERVER I'm device 1 ^_^");  // the true is for retained message = true
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
char latestMessage[256];

// mqtt callback is a function which runs whenever esp received a message from mqtt broker on the topic the esp has subscribed to.
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  size_t n = std::min<size_t>((size_t)length, sizeof(latestMessage)-1);
  memcpy(latestMessage, payload, n);
  latestMessage[n] = '\0';
  msgReady = true;
}

// sending the device current relay's status to mqtt broker 
void sendMqttDeviceRelayStatusMsg() {      // data will be sent in form of json to the broker.
  if (mqtt_client.connected()) {
  StaticJsonDocument<256> doc;
  doc["device_id"] = ESP_DEVICE_ID;
  doc["relay_status"] = device_relay_status ? "HIGH" : "LOW";
  doc["timestamp"] = getCurrentTime();

  String mqttPayload;
  serializeJson(doc, mqttPayload);

  mqtt_client.publish(MQTT_TOPIC, mqttPayload.c_str(), true);   // the 3rd argument (true) is for making this a retained message.
  Serial.print("MQTT message sent: ");
  Serial.println(mqttPayload);    // printing data which was sent to mqtt broker server.
  }
}


// --------------- Relay & Button ----------------

const unsigned long DEBOUNSE_MS = 50;
int lastButtonRaw = HIGH;  // HIGH cuz input_pullup was used, as initial pullup state is HIGH. Thus when button get pressed this turn to LOW.
unsigned long lastDebounceTime = 0;
bool lastPressedState = false;   // after debounce button pressed state

void buttonToggleRelay() {   // this function check if the button is pressed and according to that toggle the relay on or off based on device_relay_status and also update the device_relay_status and send mqtt message to mqtt broker.
  int raw = digitalRead(BUTTON_PIN);

  if (raw != lastButtonRaw) {
    // when raw button input changes, debounce timer reset and record new raw
    lastDebounceTime = millis();
    lastButtonRaw = raw;
  }

  if (millis() - lastDebounceTime > DEBOUNSE_MS) {
    bool pressed = (raw == LOW);   // check if the button is in pressed state after debounce period ends.
    if (pressed && !lastPressedState) {   // true when button is pressed and the last button pressed state was false.
      device_relay_status = !device_relay_status;   // toggle the relay's state
  
      digitalWrite(RELAY_PIN, device_relay_status ? HIGH : LOW);

      Serial.print("Button pressed. Relay now: ");
      Serial.println(device_relay_status ? "ON" : "OFF");

      Serial.println("Sending the updated relay state mqtt message: ");
      sendMqttDeviceRelayStatusMsg();     // sending the updated device_relay_status to mqtt broker.
    }
    lastPressedState = pressed;   // updating the button's last pressed state.
  }
}


// switching device's relay and change device_relay_status as per the broker's relay status.
void updateDeviceRelay(const char* tmp_relay_status) {

  device_relay_status = (strcmp(tmp_relay_status, "HIGH") == 0);
  digitalWrite(RELAY_PIN, device_relay_status ? HIGH : LOW);

  Serial.print("Updated relay to ");
  Serial.print(digitalRead(RELAY_PIN));
  Serial.print(" & device_relay_status to :");
  Serial.println(device_relay_status);
}



void setup(){ 
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, PASSWORD);

  // // this is blocking way to sync NTP time.
  // if (connectToWiFi()) {
  //     setClock();
  // }

  Serial.println("Connecting to WiFi...");
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
    } else if (millis() - timeRequestedAt > NTP_TIME_RETRY) {
      // retry requestTimeSync() every NTP_TIME_RETRY until synced
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

    // parsing the json data from the broker
    StaticJsonDocument<256> doc;
    // here doc is the destination buffer and latestMessage contain the raw json data
    DeserializationError err = deserializeJson(doc, latestMessage);  

    if (!err){
      char relayStatusBuf[16];   // this will contain the value of key "relay_status" from json that received from mqtt server.
      char deviceIdBuf[16];        // this will contain the value of key "device_id" from json that received from mqtt server.
      strlcpy(relayStatusBuf, doc["relay_status"].as<const char*>(), sizeof(relayStatusBuf));
      strlcpy(deviceIdBuf, doc["device_id"].as<const char*>(), sizeof(deviceIdBuf));

      // updating the device_relay_status when the mqtt broker's message received has other device's id.
      if (strcmp(deviceIdBuf, ESP_DEVICE_ID) != 0) {
        updateDeviceRelay(relayStatusBuf);    // updating the device_relay_status value as per the latest received mqtt message.
        
        Serial.print("this is latest message from another device: ");   // this will print the mqtt msg received from other device.
        Serial.println(latestMessage);
      }
    }
  }
  

  yield();   // calling yield in the middle so the wifi module won't get suffocated and crashed the esp.


  // switching the relay when button is pressed and send the mqtt msg to broker.
  buttonToggleRelay();

}





