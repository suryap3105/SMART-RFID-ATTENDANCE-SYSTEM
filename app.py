import streamlit as st
import serial
import time
import pandas as pd
import threading
import csv
from datetime import datetime

# === CONFIG ===
BAUD = 9600
CSV_FILE = "attendance_log.csv"
log_data = []

st.set_page_config(page_title="Attendance Logger", layout="wide")
st.title("📡 Smart Attendance Logger")
port = st.text_input("Enter COM port (e.g., COM3 or /dev/ttyUSB0):", "COM3")
start_button = st.button("Start Logging")
stop_button = st.button("Stop Logging")
status_placeholder = st.empty()
log_display = st.empty()

# === Start serial thread ===
running = False
ser = None

def read_serial():
    global ser, running, log_data
    try:
        ser = serial.Serial(port, BAUD, timeout=1)
        time.sleep(2)
        status_placeholder.success(f"🔌 Connected to {port}")
    except:
        status_placeholder.error(f"❌ Could not connect to {port}")
        return

    with open(CSV_FILE, mode='a', newline='') as file:
        writer = csv.writer(file)
        if file.tell() == 0:
            writer.writerow(['Timestamp', 'UID', 'Fingerprint ID', 'Distance (cm)', 'Access'])

        while running:
            try:
                line = ser.readline().decode('utf-8').strip()
                if not line:
                    continue

                if line.startswith("LOG,"):
                    parts = line.split(',')
                    if len(parts) == 5:
                        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        uid = parts[1]
                        fid = parts[2]
                        dist = parts[3]
                        status = parts[4]
                        log_data.append([timestamp, uid, fid, dist, status])
                        writer.writerow([timestamp, uid, fid, dist, status])
                        file.flush()

                elif "Ultrasonic" in line:
                    log_display.info(f"📶 {line}")

            except Exception as e:
                status_placeholder.error(f"⚠️ Error: {e}")
                break

    if ser:
        ser.close()

if start_button and not running:
    running = True
    log_data = []
    threading.Thread(target=read_serial, daemon=True).start()

if stop_button and running:
    running = False
    status_placeholder.warning("🛑 Logging stopped.")

if log_data:
    df = pd.DataFrame(log_data, columns=['Timestamp', 'UID', 'Fingerprint ID', 'Distance (cm)', 'Access'])
    st.subheader("📋 Attendance Log")
    st.dataframe(df)

    csv_dl = df.to_csv(index=False).encode('utf-8')
    st.download_button("⬇️ Download CSV", csv_dl, file_name=CSV_FILE, mime='text/csv')
