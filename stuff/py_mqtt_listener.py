import json
import ssl
import time
import paho.mqtt.client as mqtt

MQTT_HOST = "57d6613d04174cecba266899377455d9.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_USER = "nomnom1"
MQTT_PASSWORD = "4cheleon^04"
TOPIC = "relay-status"
CLIENT_ID = "py-listener-1"

def on_connect(client, userdata, flags, rc):
    print("Connected, rc=", rc, "flags=", flags)
    # Subscribe after successful connect (re-subscribed on reconnect)
    client.subscribe(TOPIC, qos=1)

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode('utf-8', errors='replace')
        print(f"Received on {msg.topic} (qos={msg.qos}, retained={msg.retain}): {payload}")
        # If payload is JSON:
        try:
            doc = json.loads(payload)
            print("Parsed JSON:", doc)
        except json.JSONDecodeError:
            pass
        # Do whatever processing you need here
    except Exception as e:
        print("on_message error:", e)

def on_disconnect(client, userdata, rc):
    print("Disconnected. rc=", rc)
    # paho will auto-reconnect if you call loop_start() or loop_forever() and reconnect is enabled.

client = mqtt.Client(client_id=CLIENT_ID, clean_session=True)  # use clean_session=False if you want persistent session
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

# Configure TLS (system CA store)
client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS_CLIENT)
# If you need to disable cert verification (not recommended): client.tls_insecure_set(True)

client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect

client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)

# Option A (blocking, handles reconnects)
client.loop_forever()

# Option B (non-blocking thread; main thread can do other work)
# client.loop_start()
# try:
#     while True:
#         # your main work here
#         time.sleep(1)
# finally:
#     client.loop_stop()
#     client.disconnect()