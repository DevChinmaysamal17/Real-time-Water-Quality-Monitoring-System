IoT-Based Water Quality Monitoring System

Real-time water quality monitoring system with daily ML risk prediction.

Features

Real-time Monitoring: ESP32 sends sensor data every 3 hours
Daily ML Prediction: Automatic risk assessment using pre-trained Random Forest model
REST API: FastAPI backend with SQLAlchemy ORM
Web Dashboard: Real-time visualization of sensor data and predictions
Project Structure

water-quality-monitoring/
├── app/                    # Backend application
├── ml_artifacts/           # Pre-trained ML models
├── static/                 # Frontend dashboard
└── data/                   # SQLite database (auto-created)
Prerequisites

Python 3.10+
Pre-trained ML models:
water_quality_rf_model.pkl
label_encoder.pkl
Installation

Clone the repository:
git clone <your-repo-url>
cd water-quality-monitoring
Create virtual environment:
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
Install dependencies:
pip install -r requirements.txt
Place ML artifacts in ml_artifacts/ directory
Running the Application

Start the backend server:
uvicorn app.main:app --reload
Access the dashboard:
http://localhost:8000/static/index.html
API documentation:
http://localhost:8000/docs
API Endpoints

Submit Sensor Reading (ESP32)

POST /api/readings
Content-Type: application/json

{
    "ph": 7.2,
    "tds": 350.5,
    "turbidity": 2.8,
    "temperature": 25.3
}
Get Latest Reading

GET /api/readings/latest
Get Latest Prediction

GET /api/predictions/latest
Get Dashboard Data

GET /api/dashboard
How It Works

Daily Prediction Pipeline

Data Collection: ESP32 sends sensor readings every 3 hours
Storage: Each reading is stored in SQLite database
Aggregation: At midnight (00:00), system computes daily averages
ML Inference: Averages are fed to pre-trained Random Forest model
Storage: Prediction is stored with metadata
Display: Dashboard shows latest prediction
Scheduler Architecture

APScheduler runs background task at midnight
Task aggregates previous day's data (yesterday, not today)
Uses SQLAlchemy for efficient aggregation queries
ML model loaded once at startup for performance
ML Model Loading

Model is loaded once during FastAPI startup:

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Load ML artifacts at startup
    app.state.ml_service = MLService(...)
    app.state.ml_service.load_artifacts()
    yield
    # Cleanup on shutdown
ESP32 Configuration

Update the following in ESP32 code:

WiFi SSID and password
Backend server IP address
Sensor pin configurations
Database Schema

SensorReading

id, ph, tds, turbidity, temperature, timestamp
DailyPrediction

id, date, avg_ph, avg_tds, avg_turbidity, avg_temperature
prediction, prediction_confidence, reading_count, created_at
Development

Testing Daily Prediction Manually

POST /api/predictions/trigger
Viewing Logs

Backend logs show:

Sensor reading submissions
Daily prediction execution
ML inference results
Production Deployment

For production:

Use PostgreSQL instead of SQLite
Configure proper CORS origins
Add authentication/authorization
Use environment variables for configuration
Deploy with Gunicorn + Nginx
Enable HTTPS
License

MIT License
