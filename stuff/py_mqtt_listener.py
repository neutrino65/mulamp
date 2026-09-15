# this is an ingestion script.

import json
import ssl
import time
import paho.mqtt.client as mqtt
import psycopg2

# ---------------- MQTT settings ------------------
MQTT_HOST = "57d6613d04174cecba266899377455d9.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_USER = "nomnom1"
MQTT_PASSWORD = "4cheleon^04"
TOPIC = "relay-status"
CLIENT_ID = "py-listener-1"

# --------------- postgres settings -----------------------
conn = psycopg2.connect(
    host="localhost",
    port=5431,
    dbname="test_db",
    user="root",
    password="root"
)
conn.autocommit = True
cur = conn.cursor()
TABLE_NAME = "readings"


# ------------------- postgres functions ---------------------

# checking if the table exist in the database.
def is_table_exist(conn, table_name, schema_name='public') -> bool:
    with conn.cursor() as cur:
        cur.execute(f"SELECT to_regclass('{schema_name}.{table_name}') IS NOT NULL;")
        return cur.fetchone()[0]

def create_table(conn, table_name) -> None:
    with conn.cursor() as cur:
        cur.execute(f"""
            CREATE TABLE {table_name} (
                reading_no SERIAL PRIMARY KEY,
                device_id INT NOT NULL,
                relay_status TEXT NOT NULL,
                timestamp TIMESTAMPTZ NOT NULL
            );
        """)
        print("Table created successfully!!")

        # adding contraint so that duplicate rows (combination of device_id, relay_status & timestamp) wont be added.
        cur.execute(f"""
            ALTER TABLE {table_name}
            ADD CONSTRAINT unique_column_contraint
            UNIQUE (device_id, relay_status, timestamp);
        """)
        print("Unique column contraint added successfully!!")

# this function will insert the value retrieved from the mqtt response json doc to table as new row.
def insert_into_table(conn, table_name, doc : dict) -> None:
    with conn.cursor() as cur:
        cur.execute(f"""
            INSERT INTO {table_name} (device_id, relay_status, timestamp)
            VALUES (%s, %s, %s);
        """, (doc["device_id"], doc["relay_status"], doc["timestamp"])
        )

    print(f"values inserted to {TABLE_NAME} successfully!!")

# Creating 'readings' table.
if not is_table_exist(conn, TABLE_NAME):
        create_table(conn, TABLE_NAME)


# ----------------- MQTT functions -------------------------
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
        # Inserting the data retrieved from mqtt broker's response to postgres table
        insert_into_table(conn, TABLE_NAME, doc)

    except Exception as e:
        print("on_message error:", e)

def on_disconnect(client, userdata, rc):
    print("Disconnected. rc=", rc)
    # paho will auto-reconnect if you call loop_start() or loop_forever() and reconnect is enabled.



def main():
    # #setting up postgres table
    # if not is_table_exist(conn, TABLE_NAME):
    #     create_table(conn, TABLE_NAME)

    client = mqtt.Client(client_id=CLIENT_ID, clean_session=False)  # use clean_session=False if you want persistent session
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


if __name__ == "__main__":
    main()