import json
import time
import random
import signal
import sys
import paho.mqtt.client as mqtt

SITE = "site1"
NODE = "nodeA"

BROKER_HOST = "127.0.0.1"
BROKER_PORT = 1883
KEEPALIVE = 30

TOPIC_TELEMETRY = f"farm/{SITE}/{NODE}/telemetry"
TOPIC_STATUS    = f"farm/{SITE}/{NODE}/status"

INTERVAL_SEC = 10  # telemetry every 10 seconds

def make_telemetry():
    return {
        "site": SITE,
        "node": NODE,
        "soil_moisture": round(random.uniform(20.0, 70.0), 1),
        "temperature": round(random.uniform(18.0, 35.0), 1),
        "humidity": round(random.uniform(30.0, 85.0), 1),
        "ph": round(random.uniform(5.5, 7.8), 2),
        "n": random.randint(5, 30),
        "p": random.randint(5, 30),
        "k": random.randint(5, 30),
    }

def on_connect(client, userdata, flags, rc):
    client.publish(
        TOPIC_STATUS,
        json.dumps({"site": SITE, "node": NODE, "online": True}),
        qos=1,
        retain=True
    )
    print(f"[MQTT] Connected → online=true published")

def shutdown(signum, frame):
    print("[SIM] Graceful shutdown → online=false")
    client.publish(
        TOPIC_STATUS,
        json.dumps({"site": SITE, "node": NODE, "online": False}),
        qos=1,
        retain=True
    )
    client.disconnect()
    sys.exit(0)

client = mqtt.Client(client_id=f"sim-{SITE}-{NODE}")
client.will_set(
    TOPIC_STATUS,
    json.dumps({"site": SITE, "node": NODE, "online": False}),
    qos=1,
    retain=True
)

client.on_connect = on_connect

signal.signal(signal.SIGINT, shutdown)
signal.signal(signal.SIGTERM, shutdown)

print("[SIM] Connecting to MQTT broker...")
client.connect(BROKER_HOST, BROKER_PORT, KEEPALIVE)
client.loop_start()

while True:
    payload = make_telemetry()
    client.publish(TOPIC_TELEMETRY, json.dumps(payload), qos=0)
    print(f"[SIM] Telemetry sent: {payload}")
    time.sleep(INTERVAL_SEC)
