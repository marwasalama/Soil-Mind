#!/usr/bin/env python3
import os
import json
import time
import random
import signal

import paho.mqtt.client as mqtt

# ----------------------------
# Config
# ----------------------------
BROKER_HOST = os.getenv("MQTT_HOST", "127.0.0.1")
BROKER_PORT = int(os.getenv("MQTT_PORT", "1883"))

SITE = os.getenv("SITE", "site1")
NODE = os.getenv("NODE", "nodeA")

TELEMETRY_TOPIC = f"farm/{SITE}/{NODE}/telemetry"
STATUS_TOPIC    = f"farm/{SITE}/{NODE}/status"
CONTROL_TOPIC   = f"farm/{SITE}/{NODE}/control"
CMD_TOPIC       = f"farm/{SITE}/{NODE}/cmd"

PUBLISH_EVERY_SEC = float(os.getenv("PUBLISH_EVERY", "5"))

MIN_TH = float(os.getenv("MIN_TH", "30"))
MAX_TH = float(os.getenv("MAX_TH", "45"))

last_irrigation_state = False
manual_override = None     # None = AUTO, True/False = MANUAL
manual_reason = None

_stop = False

# ----------------------------
# Logic
# ----------------------------
def decide_irrigation(soil_moisture: float):
    global last_irrigation_state

    if soil_moisture < MIN_TH:
        irrigation = True
        decision = "ON"
        reason = "moisture below min"
    elif soil_moisture > MAX_TH:
        irrigation = False
        decision = "OFF"
        reason = "moisture above max"
    else:
        irrigation = last_irrigation_state
        decision = "HOLD"
        reason = "moisture in range"

    last_irrigation_state = irrigation
    return irrigation, decision, reason

# ----------------------------
# MQTT callbacks
# ----------------------------
def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("[MQTT] Connected")
        client.subscribe(CMD_TOPIC, qos=1)
        print(f"[MQTT] Subscribed to {CMD_TOPIC}")
    else:
        print(f"[MQTT] Connect failed: {reason_code}")

def on_message(client, userdata, msg):
    global manual_override, manual_reason, last_irrigation_state

    if msg.topic != CMD_TOPIC:
        return

    try:
        payload = json.loads(msg.payload.decode())
    except Exception as e:
        print("[MQTT] Bad CMD payload:", e)
        return

    irrigation = bool(payload.get("irrigation", False))
    manual_override = irrigation
    manual_reason = payload.get("reason", "manual override")

    last_irrigation_state = irrigation

    ack = {
        "site": SITE,
        "node": NODE,
        "irrigation": irrigation,
        "decision": "MANUAL",
        "reason": manual_reason
    }

    client.publish(CONTROL_TOPIC, json.dumps(ack), qos=1, retain=False)
    print("[MQTT] CMD received → ACK sent:", ack)

# ----------------------------
def publish_status_online(client, online: bool):
    payload = {"site": SITE, "node": NODE, "online": online}
    client.publish(STATUS_TOPIC, json.dumps(payload), qos=1)
    print(f"[MQTT] Status published: online={online}")

def handle_exit(signum=None, frame=None):
    global _stop
    _stop = True

# ----------------------------
def main():
    global _stop

    try:
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                             client_id=f"sim-{SITE}-{NODE}")
    except Exception:
        client = mqtt.Client(client_id=f"sim-{SITE}-{NODE}")

    client.on_connect = on_connect
    client.on_message = on_message

    print("[SIM] Connecting to MQTT broker...")
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    client.loop_start()

    publish_status_online(client, True)

    try:
        while not _stop:
            soil_moisture = round(random.uniform(20.0, 70.0), 1)
            temperature   = round(random.uniform(18.0, 35.0), 1)
            humidity      = round(random.uniform(40.0, 90.0), 1)
            ph            = round(random.uniform(5.5, 8.0), 2)

            telemetry_payload = {
                "site": SITE,
                "node": NODE,
                "soil_moisture": soil_moisture,
                "temperature": temperature,
                "humidity": humidity,
                "ph": ph
            }

            client.publish(TELEMETRY_TOPIC,
                           json.dumps(telemetry_payload),
                           qos=0)

            if manual_override is None:
                irrigation, decision, reason = decide_irrigation(soil_moisture)
            else:
                irrigation = manual_override
                decision = "MANUAL"
                reason = manual_reason

            control_payload = {
                "site": SITE,
                "node": NODE,
                "irrigation": irrigation,
                "decision": decision,
                "reason": reason,
                "soil_moisture": soil_moisture,
                "min_th": MIN_TH,
                "max_th": MAX_TH
            }

            client.publish(CONTROL_TOPIC,
                           json.dumps(control_payload),
                           qos=1)

            print("[SIM] Telemetry:", telemetry_payload)
            print("[SIM] Control:", control_payload)

            time.sleep(PUBLISH_EVERY_SEC)

    finally:
        publish_status_online(client, False)
        time.sleep(0.5)
        client.loop_stop()
        client.disconnect()

# ----------------------------
if __name__ == "__main__":
    signal.signal(signal.SIGINT, handle_exit)
    signal.signal(signal.SIGTERM, handle_exit)
    main()
