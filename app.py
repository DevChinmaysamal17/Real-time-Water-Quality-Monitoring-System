from flask import Flask, render_template, jsonify, session, request
from datetime import datetime
import pandas as pd
import joblib

model = joblib.load("water_quality_rf_model.pkl")
label_encoder = joblib.load("label_encoder.pkl")

print("✅ ML model and LabelEncoder loaded successfully")


# import serial
# import threading

app = Flask(__name__)
app.secret_key = "asep_secret_key"

VALID_VILLAGE_ID = "village01"
VALID_PASSWORD = "1234"

SERIAL_PORT = "/dev/tty.usbserial-0001"
BAUD_RATE = 115200

latest_data = {
    "ph": None,
    "turbidity": None,
    "tds": None,
    "temperature": None,
    "status": "WAITING",
    "issues": [],
    "ml_output": None,
    "time": None
}


def check_water_quality(ph, turbidity, tds, temperature):
    issues = []

    if ph < 5 or ph > 9:
        issues.append("pH out of range")
    if turbidity > 15:
        issues.append("High turbidity")
    if tds > 500:
        issues.append("High TDS")
    if temperature < 5 or temperature > 35:
        issues.append("Temperature out of range")

    if issues:
        return "UNSAFE", issues
    return "SAFE", []


@app.route("/")
def home():
    return render_template("index.html")

@app.route("/login", methods=["POST"])
def login():
    data = request.get_json(force=True)
    if data["village_id"] == VALID_VILLAGE_ID and data["password"] == VALID_PASSWORD:
        session["logged_in"] = True
        return jsonify({"success": True})
    return jsonify({"success": False}), 401

@app.route("/api/latest")
def latest():
    print("👀 LATEST ENDPOINT CALLED")
    print("SENDING latest_data:", latest_data)
    return jsonify(latest_data)


@app.route("/api/sensor", methods=["POST"])
def sensor_data():
    data = request.json
    print("SENSOR ENDPOINT HIT")
    print("UPDATED latest_data:", latest_data)
    if not data:
        return jsonify({"error": "No data received"}), 400

    try:
        ph = float(data["ph"])
        turbidity = float(data["turbidity"])
        tds = float(data["tds"])
        temperature = float(data["temperature"])
    except (KeyError, ValueError, TypeError):
        return jsonify({"error": "Invalid sensor data"}), 400


    status, issues = check_water_quality(ph, turbidity, tds, temperature)
    
    features = pd.DataFrame([{
        "pH": ph,
        "turbidity_NTU": turbidity,
        "TDS_mg_L": tds,
        "temperature_C": temperature
    }])

    prediction_encoded = model.predict(features)
    prediction_label = label_encoder.inverse_transform(prediction_encoded)[0]
    prediction_label = str(prediction_label)

    
    prediction_label = label_encoder.inverse_transform(prediction_encoded)[0]
    prediction_label = str(prediction_label)
    

    latest_data.update({
        "ph": float(ph),
        "turbidity": float(turbidity),
        "tds": float(tds),
        "temperature": float(temperature),
        "status": str(status),
        "issues": list(issues),
        "ml_output": prediction_label,  # now safe
        "time": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    })


    return jsonify({"success": True})


if __name__ == "__main__":
    # t = threading.Thread(target=read_serial, daemon=True)
    # t.start()

    app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)



