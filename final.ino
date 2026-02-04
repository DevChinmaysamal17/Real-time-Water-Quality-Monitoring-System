#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"


LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "Your Wifi Name";
const char* password = "Wifi Password";
const char* serverUrl = "http://<ip>:5000/api/sensor";


float calibration_value = 21.34 - 0.55;
unsigned long int avgval;
int buffer_arr[10], temp;
float ph_act;


#define PH_SENSOR_PIN 34   
#define TU_SENSOR_PIN 35
#define DPIN 4
#define DTYPE DHT11
#define TdsSensorPin 32
#define VREF 3.3
#define SCOUNT  15  
DHT dht(DPIN,DTYPE);

#define PH_MIN 6.5
#define PH_MAX 8.5
#define TDS_MAX 500
#define TURB_MAX 50
bool waterSafe = true;


int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float tc = 0.0;
int turbidity = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation
// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 30000; // 30 seconds

void setup() {
  analogReadResolution(12);     // 0–4095
  Wire.begin();
  Serial.begin(115200);   
  dht.begin();


  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  delay(2000);
  lcd.clear();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

}

void loop() {

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();
    Serial.println("⏱️ 30 seconds elapsed — sending data");


    // ---- DHT ----
    float tempRead = dht.readTemperature(false);
  if (isnan(tempRead)) {
    Serial.println("❌ DHT read failed, using last value");
    tempRead = temperature;
  }

    tc = tempRead;
    temperature = tc;

    // ---- pH ----
    for (int i = 0; i < 10; i++) {
      buffer_arr[i] = analogRead(PH_SENSOR_PIN);
      delay(30);
    }
    for (int i = 0; i < 9; i++) {
      for (int j = i + 1; j < 10; j++) {
        if (buffer_arr[i] > buffer_arr[j]) {
          temp = buffer_arr[i];
          buffer_arr[i] = buffer_arr[j];
          buffer_arr[j] = temp;
        }
      }
    }
        avgval = 0;
    for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
    float volt = avgval * 3.3 / 4095.0 / 6;
    ph_act = -5.70 * volt + calibration_value;

    // ---- Turbidity ----
    turbidity = map(analogRead(TU_SENSOR_PIN), 0, 4095, 0, 100);

    // ---- TDS ----
    for (int i = 0; i < SCOUNT; i++)
      analogBufferTemp[i] = analogRead(TdsSensorPin);

    float avg = 0;
    for (int i = 0; i < SCOUNT; i++)
      avg += analogBufferTemp[i];
    avg /= SCOUNT;

    float voltage = avg * (VREF / 4095.0);
    float compV = voltage / (1.0 + 0.02 * (temperature - 25.0));

    tdsValue = (133.42 * compV * compV * compV
               -255.86 * compV * compV
               +857.39 * compV) * 0.5;

    // ---- LCD ----
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("pH:");
    lcd.print(ph_act,2);
    lcd.print(" T:");
    lcd.print(turbidity);
    lcd.setCursor(0,1);
    lcd.print("Temp:");
    lcd.print((int)tc);
    lcd.print(" TDS:");
    lcd.print(tdsValue);

    // ---- HTTP ----
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("📡 WiFi OK, sending HTTP POST");

      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      String payload = "{";
      payload += "\"ph\":" + String(ph_act,2) + ",";
      payload += "\"turbidity\":" + String(turbidity) + ",";
      payload += "\"tds\":" + String(tdsValue,2) + ",";
      payload += "\"temperature\":" + String(tc);
      payload += "}";

      Serial.println("➡ Payload:");
      Serial.println(payload);

      int code = http.POST(payload);
      Serial.print("⬅ HTTP response code: ");
      Serial.println(code);

      http.end();
    } else {
      Serial.println("❌ WiFi disconnected");
    }

  }
}


