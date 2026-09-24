#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>


ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);     // WifiAPWebPortal http server running on port 80

// // wifi setting
// #define SSID "FTTH"
// #define PASSWORD "abcd@1234"

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
MIIF9DCCA9ygAwIBAgIRAPJLbRf52a18scn+p4eCaZ8wDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjYwNTEzMDAwMDAw
WhcNMzIwOTAyMjM1OTU5WjAuMQswCQYDVQQGEwJVUzENMAsGA1UEChMESVNSRzEQ
MA4GA1UEAxMHUm9vdCBZUjCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIB
ANvGJnN78CTJdWL3+eGfsLN5TrNBJs+VH9hRXqRbwxu9sGNiB0BD1fcOxbSUQCJI
M1xE13Db+5Cw1w0s0EBYsvuIP/6joF0w8cuImbgR1OGgYbSQ4OpzI+DG8SGuTlcE
873OCS+kh3srlo6vl43M5OJg4Aeo1sfHp6kTJDoIiFBNJAY+OKfX/FUvYKuhjT+n
o49lmqmupSBI5PkBQiqrEGtWU5uxU/cQWHGu8jSjFBznZqvbNPLMXMLFxCb3WTfr
JBXXjqvWG+v4bjzxjjeAtOlU7qarRDvNOyAuQYLln904M+faKx8hnLCpJ15ZqaEg
cNlY+9MMWcC5yvL2A2j3l9+2buggZX+dOE91zYmIdawTvSZuVvlbRrAlLxIB6pwM
BjneXCjYQ8+3BCCjssbSNpZU3hTcBDdhfAlEDlYr6pEatnMdmDT5BqnKC92bd0Eh
M1fbLHioLccLCuievT8ZkPhZrq7Mii7gNXAcUEAR8+lzYal+9zTg7C5DALyVOeG/
CqfRAMn1KSHCR0NSA6P8tn/mGRlnCct5rtVCLnVySVpU6H1qGg3DgTOuskf8eahT
MiYbI5ezPJmO5ertalskQ1utp74+eDy92PI4ftHKTbq9IWhH4YZKh3WnJEIt+oQv
lYZbY8tpEroKrFB6PFGzrJIDRyts4HqvuH52RFj2zv/BAgMBAAGjgeswgegwDgYD
VR0PAQH/BAQDAgEGMBMGA1UdJQQMMAoGCCsGAQUFBwMBMA8GA1UdEwEB/wQFMAMB
Af8wHQYDVR0OBBYEFN7nW2DQIm1AKH0/DQH+pLVStFGUMB8GA1UdIwQYMBaAFHm0
WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcwAoYW
aHR0cDovL3gxLmkubGVuY3Iub3JnLzATBgNVHSAEDDAKMAgGBmeBDAECATAnBgNV
HR8EIDAeMBygGqAYhhZodHRwOi8veDEuYy5sZW5jci5vcmcvMA0GCSqGSIb3DQEB
CwUAA4ICAQA8spSI95KKfn2W6GMmDpHBJSPaLbsS3W93cijJCRCYAc1fsJgL1FIL
7C0C9ecPOdcwB2fi0Dk2p94j9iTJCxmt5CFSKLRWwnXT2MMSXexVxqoVB79BdWPx
VXETkVme/qYSAuKVHh5Ps+5BixgmwS1JkjSAc+MfrUbNssVEEnH0aEiAh+rotXAV
JSP/Ye7LJPEwD9DWG72vVWbhAcuOf5OLjz57Ctk7MgQHynZ7+PlHJtajroCaIbtC
r6tcZZaAwUQm+jQyeWdV+2hv9deOYFmKeQyjjcSrN5Nadrw+L9DZJLbA1HqeNvLh
BgqpP0fvJq2N6EtD574N6eMI7uMsJTnji2UDz9el5XLSv9fqJMuDQtYVb2oTNoKp
oUqhxPVC0aq4eG5MESaIdn8b5ZGSSeAJLMHXljEdlNza+ncfkviXk1POLnnFdvx8
/gk6M374WbLWFXw8N141B/Rl/tINGfl1TxOIiqtiMYkL02RSGb1kq34BL9NPP27z
RGMuHGnzS3hFIrRTfKxrzUZ9RzQWzEG3K6fJ3r2nqSltkeytis9DIBoFY9VmVyjL
M71DMi+y1+TRSJVClEMwvA4yL++7q9XZx5r5wBRWB4kQTKH5qyoZnDw7iiuh1lID
yDFx8r7i9vIJU5HS3moZLkYWAOilMaV9N56A9Bgb6dNcHkvg3NoaYA==
-----END CERTIFICATE-----
)EOF";


BearSSL::X509List serverTrustedCA(cert);


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
  JsonDocument doc;
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


// ------------------ WifiAPWebPortal -----------------------

// this function convert the enum value it get from WiFi.encryptionType() into human readable.
String getWifiSecurityType(int i) {
    if (i == 5) return "WEP";
    else if (i == 2) return "WPA/PSK";
    else if (i == 4) return "WPA2/PSK";
    else if (i == 7) return "Open";
    else if (i == 8) return "WPA/WPA2/PSK";
    else return "Unknown";
}


//return true if file opened properly and if it didn't then false.
bool didFileOpenProperly(File &file) {
  if (!file) {Serial.println("File didn't opened properly"); return false;}
  return true;
}


// returns true if wifi cred is added/updated in wifi_cred.json file, else return false.
bool addToWifiCredsFile(String tmp_ssid, String tmp_pw) {
  /*
  check if wifi_creds.json file exist in littleFS or not.
  if no then make the file and save the given creds.
  if yes the add/update the given creds to the file.
  */

  // checking if wifi_creds.json file exist or not.
  File file = LittleFS.open("/wifi_creds.json", "r");
  if (!file) {
    // create the json doc (which will contain all the wifi creds)
    /*
      doc
      └── networks[]
          ├── ssid1
          └── password1
          └── ssid2
          └── password2
    */
    JsonDocument doc;
    JsonArray listedNetworks = doc["networks"].to<JsonArray>();
    // saving the given ssid and password to the network 
    JsonObject tmp_nw_json_obj = listedNetworks.add<JsonObject>();
    tmp_nw_json_obj["ssid"] = tmp_ssid;
    tmp_nw_json_obj["password"] = tmp_pw;
    //creating wifi_creds.json file and saving the json doc we have made.
    file = LittleFS.open("/wifi_creds.json", "w");
    if (!didFileOpenProperly(file)) return false;   // checking if file opened properly or not.
    // serializing the json data structure directly into the wifi_creds.json file. ("JsonDocument" is a data structure in RAM only, to make it into json file we have to serialize it).
    size_t bytes_written = serializeJson(doc, file);
    // checking the data is written to file
    if (bytes_written > 0) {
      Serial.println("wifi_creds.json file created.");
      Serial.print("Bytes written: ");
      Serial.println(bytes_written);
      file.close();
      return true;
    } else {
      Serial.println("Write failed!");
      file.close();
      return false;
    }
  } else {   // file exist already, so add/update wifi creds.
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    if (!err) {
      JsonArray listedNetworks = doc["networks"];
      // iterating throught the saved networks in the file.
      for (JsonVariant p : listedNetworks){      // here p represent one complete object in the networks array. thus looping through objects in the array.
        Serial.print("wifi creds are updated >> ");
        Serial.println(p.as<String>());
        String listed_ssid = p["ssid"].as<String>();         // extracting the value associated with the key in the given json object which is listed inside the network list in wifi_creds.json file.
        // checking if the given ssid are same as the ssid of json object.
        if(strcmp(listed_ssid.c_str(), tmp_ssid.c_str()) == 0){   // if both ssid same then update the password.
          p["password"] = tmp_pw;
          Serial.println("wifi ssid found in saved file. Updating password");
          // writing the json with updated password
          // opening the file in write mode
          file.close();
          file = LittleFS.open("/wifi_creds.json", "w");
          if (!didFileOpenProperly(file)) return false;
          serializeJson(doc, file);
          Serial.print("updated pw saved file -> ");
          Serial.println(doc.as<String>());
          file.close();
          return true;
        }
      }
      //wifi cred not present in wifi_creds.json file. thus appending json object which contain ssid and password.
      JsonObject tmp_obj = listedNetworks.add<JsonObject>();
      tmp_obj["ssid"] = tmp_ssid;
      tmp_obj["password"] = tmp_pw;
      // opening the file here in write mode again cuz we need to write the json doc with appended ssid and pw.
      file.close();
      file = LittleFS.open("/wifi_creds.json", "w");
      if (!didFileOpenProperly(file)) return false;
      serializeJson(doc, file);
      file.close();
      Serial.print("wifi creds are appended >> ");
      Serial.println(doc.as<String>());
      return true;
    }

    return false;
  }
}


// check for wifi_creds.json file in LittleFS, if present then load its wifi creds into WiFiMulti's AP.
int loadWifiCredsToAP(){
  // loading wifi_creds.json file.
  File file = LittleFS.open("/wifi_creds.json", "r");
  // if file not found then return 0 (as in 0 wifi creds added to AP).
  if (!file) {Serial.println("File not Found."); return 0;}
  // looping through the file.
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();    // closing the file as we already got the data read from file into json document into the RAM.
  if (!err) {
    JsonArray listedNetworks = doc["networks"];
    for (JsonVariant p : listedNetworks) {
      String tmp_ssid = p["ssid"];
      String tmp_pw = p["password"];
      // adding the ssid & password to wifiMulti's AP.
      WiFiMulti.addAP(tmp_ssid.c_str(), tmp_pw.c_str());
    }
    Serial.print("No. of wifi creds present in already saved wifi_creds file: ");
    Serial.println(listedNetworks.size());
    return (listedNetworks.size());
  } else {
    Serial.println("Error deserializing the wifi_creds file.");
    return -1;    // return -1 to let the person/program know that this function didn't worked as inteded and encountered some sort of error.
  }
}




// =============================================

void setup(){ 
  Serial.begin(115200);

  LittleFS.begin();                       // mounting the LittleFS filesystem.

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);

  WiFi.mode(WIFI_AP_STA);                     // running the wifi in AP + STA dual mode.
  WiFi.softAP("mulamp-wifi");                 // setting up ESP's wifi ssid.

  // WiFiMulti.addAP(SSID, PASSWORD);

  // // this is blocking way to sync NTP time.
  // if (connectToWiFi()) {
  //     setClock();
  // }

  // adding the wifi creds which are saved in ESP's flash memory (LittleFS) to WiFiMulti's AP.
  loadWifiCredsToAP();

  // connecting to wifi (blocking for 5 sec).
  if (!waitForWiFi(5000)) {
    Serial.println("WiFi timed out, continuing offline");
  }

  if (WiFi.status() ==  WL_CONNECTED)  Serial.println(WiFi.SSID());

  Serial.print("STA IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());   // starting the esp's wifi n/w
  
  // Doing a Non-blocking way to sync NTP time.
  requestTimeSync();

  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(mqttCallback);

  // Setting HTTP server routes for WifiAPWebPortal.
  server.on("/", []() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "index.html not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/css/style.css", []() {
    File file = LittleFS.open("/css/style.css", "r");
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/js/api.js", []() {
    File file = LittleFS.open("/js/api.js", "r");
    server.streamFile(file, "application/javascript");
    file.close();
  });

  server.on("/js/app.js", []() {
    File file = LittleFS.open("/js/app.js", "r");
    server.streamFile(file, "application/javascript");
    file.close();
  });

  // API routes
  server.on("/api/device", HTTP_GET, []() {
    File file = LittleFS.open("/device.json", "r");
    if (!file) {
        server.send(404, "text/plain", "device.json file not found");
        return;
    }
    server.streamFile(file, "application/json");
    file.close();
  });

  // sends array of jsons containing scanned networks details.
  server.on("/api/wifi/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();

    //allocating json doc memory pool.
    JsonDocument doc;
    // creating json array inside the document's element called "networks". this is due to the required output to be {"networks": [{n/w1}, {n/w2},...]}.
    // thus here we telling esp that make "networks" which is a member of doc and make this member an array, and we gave that array a name "scannedNetwork" for easier handling. 
    JsonArray scannedNetwork = doc["networks"].to<JsonArray>();
    // creating json objects and adding them to array.
    for (int i = 0; i < n; ++i) {
        JsonObject tmp_json_obj = scannedNetwork.add<JsonObject>();
        tmp_json_obj["ssid"] = WiFi.SSID(i);
        tmp_json_obj["rssi"] = WiFi.RSSI(i);
        tmp_json_obj["security"] = getWifiSecurityType(WiFi.encryptionType(i));
        tmp_json_obj["channel"] = WiFi.channel(i);
    }
    // converting json into string (serializing) to send over internet.
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    // sending the array of json objects which contain all the scanned network's details.
    server.send(200, "application/json", jsonResponse);

    //Serial.println(jsonResponse);
  });

  // receiving ssid & password cred and checking if the wifi connects and send appropriate response.
  server.on("/api/wifi/connect", HTTP_POST, [](){
    String body = server.arg("plain");                     // "plain" let us extract the request body from the HTTP POST REQUEST.

    // parsing the json data from the POST request.
    JsonDocument doc;
    // here doc is the destination buffer and body is the raw json data.
    DeserializationError err = deserializeJson(doc, body);
    // extracting the ssid and password from the POST request.
    if(!err){
      String sent_ssid;
      String sent_password;

      sent_ssid = doc["ssid"].as<String>();                 // used .as<String>() allows us to get the value which we want "as string".
      sent_password = doc["password"].as<String>();

      // attempting to connect to the received wifi creds.
      // trying to connect every 1 sec for 10 sec duration, if not connect then send unsuccessful as response else successful.
      WiFi.begin(sent_ssid, sent_password);
      unsigned long wifi_connect_start_time = millis();
      while (millis() - wifi_connect_start_time < 10000 && WiFi.status() != WL_CONNECTED){
        delay(1000);
      }
      if (WiFi.status() != WL_CONNECTED) {    // making json response to send to the POST request for unsucessful wifi connection attempt to given creds.
        JsonDocument doc;
        doc["success"] = false;
        doc["message"] = "Failed. Please check credentials.";
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        server.send(200, "application/json", jsonResponse);
        Serial.printf("Failed connection due to incorrect ssid or password | ssid -> %s | password -> %s \n", sent_ssid.c_str(), sent_password.c_str());
      } else {                           // wifi got connected thus sending response to browser and adding that to wifiMulti's AP list and saving these cred to wifi_creds file and littleFS.
        // adding the wifi cred to wifiMulti's AP list.
        WiFiMulti.addAP(sent_ssid.c_str(), sent_password.c_str());
        // adding the wifi cred to wifi_creds.json file (which is in LittleFS).
        Serial.print("ESP is currently connected to: ");
        Serial.println(WiFi.SSID());
        addToWifiCredsFile(sent_ssid, sent_password);

        // sending the response
        JsonDocument doc;
        doc["success"] = true;
        doc["message"] = "Successful! ESP connected to wifi.";
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        server.send(200, "application/json", jsonResponse);
      }

    }
  });

  // sends ESP's wifi status, if the esp is currently connected to wifi or not.
  server.on("/api/wifi/status", HTTP_GET, []() {
    //check if wifi connection currently established
    //if yes then send the according json response
    //if not then send the according json response

    // making response json doc.
    JsonDocument doc;
    if (WiFi.status() == WL_CONNECTED) {
      doc["connected"] = true;
      doc["ssid"] = WiFi.SSID();
      doc["rssi"] = WiFi.RSSI();
    } else {
      doc["connected"] = false;
      doc["ssid"] = nullptr;
      doc["rssi"] = nullptr;
    }
    //sending the response.
    String responseJson;
    serializeJson(doc, responseJson);
    //sending the response to browser.
    server.send(200, "application/json", responseJson);
    Serial.print("Sent ESP's current wifi connection status to browser >> ");
    Serial.println(responseJson.c_str());
  });

  // starting HTTP server
  server.begin();
}

void loop() {
  // running the WiFiAPWebPortal's HTTP server's callback function. this webserver run on local n/w created by esp and does not require internet connectivity and used for setting up wifi creds to enable the esp to connect to wifi for internet.
  server.handleClient();
  
  // keep wifi stack running (non-blocking)
  if (WiFi.status() != WL_CONNECTED) {
    WiFiMulti.run();  // non-blocking wifi connect.
  }

  // configuring ntp time sync (non-blocking)
  if (!timeSynced && WiFi.status() == WL_CONNECTED) {
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
    JsonDocument doc;
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





