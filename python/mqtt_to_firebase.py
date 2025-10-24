import json
import paho.mqtt.client as mqtt
import firebase_admin
from firebase_admin import credentials, firestore
from datetime import datetime, timezone, timedelta
import time
import sys
import logging

# ==========================
# 🚀 FIREBASE SUBSCRIBER (REAL MQTT MODE)
# ==========================
print("=======================================")
print("   FIREBASE SUBSCRIBER")
print("=======================================\n")

# === Kết nối Firebase ===
try:
    cred = credentials.Certificate("key.json")
    firebase_admin.initialize_app(cred)
    db = firestore.client()
    print("✅ Firebase connected successfully.\n")
except Exception as e:
    print(f"❌ Firebase connection failed: {e}")
    sys.exit(1)

# === Hàm xóa nhân viên và log liên quan ===
def delete_employee_and_logs(uid, date_vn=None):
    db.collection("employees").document(uid).delete()
    print(f"🗑️ Removed employee: {uid}")

    logs_ref = db.collection("logs_attendance").where("uid", "==", uid)
    if date_vn:
        logs_ref = logs_ref.where("date_vn", "==", date_vn)

    logs = logs_ref.stream()
    count = 0
    for log in logs:
        log.reference.delete()
        count += 1

    print(f"🧹 Deleted {count} attendance logs for {uid} ({date_vn or 'all days'})")

# === Hàm lưu log ===
def save_log(action, data):
    vn_time = datetime.now(timezone.utc) + timedelta(hours=7)
    date_str = vn_time.strftime("%d/%m/%Y")
    time_str = vn_time.strftime("%H:%M:%S")

    data["timestamp_utc"] = datetime.now(timezone.utc)
    data["date_vn"] = date_str
    data["time_vn"] = time_str
    
     # 🧱 Ngăn thêm lại log enroll hoặc delete cũ (replayed QoS=1)
    if action in ["enroll", "delete"]:
        ts = data.get("ts")
        uid = data.get("uid")
        reason = data.get("reason", "")
        if uid and ts is not None:
            # Kiểm tra xem log này đã lưu trong Firestore chưa
            existed = (
                db.collection(f"logs_{action}")
                .where("uid", "==", uid)
                .where("ts", "==", ts)
                .limit(1)
                .get()
            )
            if existed:
                print(f"⚠️ Duplicate '{action}' log ignored for {uid} (ts={ts})")
                return

    # ⚙️ Ngăn log IN/OUT trùng trong cùng ngày
    if action == "attendance" and data.get("mode") in ["IN", "OUT"]:
        uid = data.get("uid")
        mode = data.get("mode")

        if uid and mode:
            # --- Nếu là OUT, kiểm tra xem đã có IN chưa ---
            if mode == "OUT":
                in_check = (
                    db.collection("logs_attendance")
                    .where("uid", "==", uid)
                    .where("date_vn", "==", date_str)
                    .where("mode", "==", "IN")
                    .where("ok", "==", True)
                    .limit(1)
                    .get()
                )
                if not in_check:
                    print(f"🚫 Skip OUT for {uid}: no IN record found on {date_str}")
                    return

            # --- Ngăn log trùng mode trong ngày ---
            same_day = (
                db.collection("logs_attendance")
                .where("uid", "==", uid)
                .where("date_vn", "==", date_str)
                .where("mode", "==", mode)
                .limit(1)
                .get()
            )
            if same_day:
                print(f"⚠️ Skip duplicate {mode} for {uid} on {date_str}")
                return

    # ✅ Ghi log vào Firestore
    db.collection(f"logs_{action}").add(data)
    print(f"✅ Saved '{action}' log: {data}")

    # === Xử lý thêm/xóa nhân viên ===
    uid = data.get("uid")
    finger_id = data.get("fp_id")

    if action == "enroll" and uid and finger_id is not None:
        db.collection("employees").document(uid).set({
            "uid": uid,
            "fingerId": finger_id,
            "created_date": date_str,
            "created_time": time_str
        }, merge=True)
        print(f"👤 Added/Updated employee: {uid}")

    elif action == "delete" and uid:
        delete_employee_and_logs(uid, date_str)

# === Callback khi nhận message MQTT ===
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode().strip()
        if not payload:
            print(f"⚠️ Empty message on topic {msg.topic}")
            print(f"🟢 Full payload: {payload}")  
            return

        data = json.loads(payload)

        # 🔹 Log Attendance
        if msg.topic == "esp32/attendance/logs":
            log_type = data.get("type")
            if log_type:
                if log_type == "management_add":
                    action = "enroll"
                elif log_type == "management_delete":
                    action = "delete"
                elif log_type == "attendance":
                    action = "attendance"
                else:
                    print(f"⚠️ Unknown log type: {log_type}")
                    return
                save_log(action, data)
            else:
                print("⚠️ Field 'type' is missing in the data:", data)

        # 🔹 Log mở cửa
        elif msg.topic == "esp32/door/status":
            ok = data.get("ok", False)
            by = data.get("by", "unknown")
            status = data.get("status", "")
            vn_time = datetime.now(timezone.utc) + timedelta(hours=7)
            time_str = vn_time.strftime("%H:%M:%S")

            door_log = {
                "ok": ok,
                "by": by,
                "status": status,
                "time_vn": time_str,
                "timestamp_utc": datetime.now(timezone.utc)
            }

            db.collection("logs_door").add(door_log)
            if ok:
                print(f"🚪✅ Door opened by {by} at {time_str}")
            else:
                print(f"🚪❌ Door open failed by {by} at {time_str}")

        else:
            print(f"⚠️ Unknown topic received: {msg.topic}")

    except json.JSONDecodeError:
        print(f"⚠️ Invalid JSON data on topic {msg.topic}: {msg.payload.decode()}")
    except Exception as e:
        print(f"❌ An error occurred: {e}")

# === Callback khi kết nối MQTT ===
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT broker successfully.")
        client.subscribe("esp32/attendance/logs", qos=1)
        client.subscribe("esp32/door/status", qos=1)
        print("📡 Subscribed to topics: esp32/attendance/logs, esp32/door/status")
    else:
        print(f"❌ MQTT connection failed with code {rc}")

# === Callback khi ngắt kết nối ===
def on_disconnect(client, userdata, rc):
    if rc != 0:
        print(f"⚠️ Disconnected unexpectedly (rc={rc})")
        reconnect_mqtt(client)

# === Hàm reconnect tự động ===
def reconnect_mqtt(client):
    delay = 3
    while True:
        try:
            print(f"⏳ Trying to reconnect in {delay}s ...")
            time.sleep(delay)
            client.reconnect()
            print("✅ Reconnected successfully.")
            break
        except Exception as e:
            print(f"⚠️ Reconnect failed: {e}")
            delay = min(delay + 2, 15)

# === MQTT setup ===
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [MQTT] %(message)s",
)
client = mqtt.Client(client_id="firebase_subscriber", clean_session=False)
client.username_pw_set("u1", "p1")

# ⚡ Bật log MQTT thật — hiện gói CONNECT, CONNACK, SUBSCRIBE, PUBLISH, v.v.
client.enable_logger()

client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message
client.reconnect_delay_set(min_delay=1, max_delay=30)

BROKER_IP = "10.235.197.90"
PORT = 1883
KEEPALIVE = 60

print(f"🔗 Connecting to MQTT broker at {BROKER_IP}:{PORT} ...\n")
client.connect(BROKER_IP, PORT, KEEPALIVE)

print("🚀 Listening for logs from ESP32... (persistent session ON, QoS=1)\n")
try:
    client.loop_forever()
except KeyboardInterrupt:
    print("\n🛑 KeyboardInterrupt detected, disconnecting cleanly...")
    client.disconnect()
    client.loop_stop()
    sys.exit(0)
