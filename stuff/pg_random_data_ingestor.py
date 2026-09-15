import random
from datetime import datetime, timedelta, timezone

import psycopg2

# ---------------- Postgres settings (match py_mqtt_listener.py) ----------------
PG_HOST = "localhost"
PG_PORT = 5431
PG_DBNAME = "test_db"
PG_USER = "root"
PG_PASSWORD = "root"
TABLE_NAME = "readings"

# ---------------- Simulation settings ----------------
DEVICE_IDS = [1, 2]              # your two ESP8266 boards (ESP_DEVICE_ID "1" and "2")
NUM_DAYS = 60                    # how many days of history to generate
END_DATE = datetime.now(timezone.utc)   # data ends "now" (UTC, matches TIMESTAMPTZ)
MIN_SESSIONS_PER_DAY = 0         # some days: lamp never touched
MAX_SESSIONS_PER_DAY = 4         # some days: a handful of on/off cycles

# Hour-of-day weights (index 0 = midnight ... 23 = 11pm).
# Heavier in the evening, near-zero in the early morning.
HOUR_WEIGHTS = [
    1, 1, 1, 0.5, 0.5, 0.5,   # 00-05
    1, 2, 2, 1.5, 1, 1,       # 06-11
    1.5, 1.5, 1.5, 1.5, 2, 3, # 12-17
    5, 6, 6, 5, 3, 2,         # 18-23
]


def random_on_time(day_start):
    hour = random.choices(range(24), weights=HOUR_WEIGHTS, k=1)[0]
    minute = random.randint(0, 59)
    second = random.randint(0, 59)
    return day_start.replace(hour=hour, minute=minute, second=second, microsecond=0)


def random_session_minutes():
    # log-normal: median ~ e^3.6 ~= 37 min, occasional long tail sessions
    minutes = random.lognormvariate(mu=3.6, sigma=0.9)
    return max(2, min(minutes, 6 * 60))  # clamp to 2 min .. 6 hr


def generate_events():
    """Returns a list of (device_id, relay_status, timestamp) tuples."""
    events = []
    start_date = END_DATE - timedelta(days=NUM_DAYS)

    for device_id in DEVICE_IDS:
        day_cursor = start_date
        for _ in range(NUM_DAYS):
            day_cursor += timedelta(days=1)
            next_midnight = (day_cursor + timedelta(days=1)).replace(
                hour=0, minute=0, second=0, microsecond=0
            )

            num_sessions = random.randint(MIN_SESSIONS_PER_DAY, MAX_SESSIONS_PER_DAY)
            on_times = sorted(random_on_time(day_cursor) for _ in range(num_sessions))
            on_times = [t for t in on_times if t <= END_DATE]

            for i, on_time in enumerate(on_times):
                # cap this session's off_time at the next session's on_time
                # (or midnight) so same-device sessions never overlap
                ceiling = on_times[i + 1] if i + 1 < len(on_times) else next_midnight
                ceiling = min(ceiling, END_DATE)
                if ceiling <= on_time:
                    continue  # no room left for a session, skip

                duration = timedelta(minutes=random_session_minutes())
                off_time = min(on_time + duration, ceiling)
                off_time -= timedelta(seconds=30)
                if off_time <= on_time:
                    continue

                events.append((device_id, "HIGH", on_time))
                events.append((device_id, "LOW", off_time))

    events.sort(key=lambda e: e[2])
    return events


def insert_events(conn, events):
    inserted = 0
    with conn.cursor() as cur:
        for device_id, relay_status, ts in events:
            cur.execute(
                f"""
                INSERT INTO {TABLE_NAME} (device_id, relay_status, timestamp)
                VALUES (%s, %s, %s)
                ON CONFLICT ON CONSTRAINT unique_column_contraint DO NOTHING;
                """,
                (device_id, relay_status, ts),
            )
            inserted += cur.rowcount
    conn.commit()
    return inserted


def main():
    conn = psycopg2.connect(
        host=PG_HOST, port=PG_PORT, dbname=PG_DBNAME,
        user=PG_USER, password=PG_PASSWORD,
    )
    conn.autocommit = False
    try:
        events = generate_events()
        print(f"Generated {len(events)} synthetic events "
              f"across {len(DEVICE_IDS)} device(s) over {NUM_DAYS} days.")
        inserted = insert_events(conn, events)
        skipped = len(events) - inserted
        print(f"Inserted {inserted} rows into '{TABLE_NAME}' "
              f"(skipped {skipped} duplicate(s)).")
    finally:
        conn.close()


if __name__ == "__main__":
    main()