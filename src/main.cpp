#include <Wire.h>
#include <WiFi.h>
#include <OneWire.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <driver/adc.h>
#include <DHT.h>
#include "EmonLib.h"
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define WIFI_PASSWORD "NMKYJS12345"
#define WIFI_SSID "QUANTUM-2"
#define DATA_BASE_URL "https://espdata-96140-default-rtdb.europe-west1.firebasedatabase.app/"
#define DATA_BASE_API "AIzaSyAYWvganNqHKDgka8ciI0NSztVehStWWQA"
#define DHT_PIN 4
#define CT_INPUT 34      // the GPIO pin where the CT sensor is connected
#define VOLTAGE_INPUT 33 // The GPIO pin where the voltege sensor is connected
// #define DHT_TYPE DHT22
// DHT dht(DHT_PIN, DHT_TYPE); // Initialize DHT sensor
EnergyMonitor emon1;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
const unsigned long SEND_INTERVAL = 15000; // time after which the data will be updated in the database e.g= 15 sec
unsigned long lastDataSendTime = 0;
const float vCalibration = 41.5;
const float currCalibration = 0.15;
bool signupOK = false;
void connectToWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  lcd.setCursor(0, 0);
  lcd.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED && retries < 15)
  {
    delay(500);
    lcd.print(".");
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("CONNECTED WITH IP: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
  }
  else
  {
    Serial.println("WiFi connection failed");
    lcd.clear();
    lcd.print("WiFi connection Failed");
  }
}
// Function to connect to FIREBASE data base
void connectToDatabase()
{
  // Assign the dataBase API_KEY
  config.api_key = DATA_BASE_API;
  // Assign the dataBase URL
  config.database_url = DATA_BASE_URL;
  // Using the provided configuration and authintication credientials the firebase connection is established while the user name and pass field are kept empty because of annonymous database
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.print("Connected to DataBase");
    lcd.clear();
    lcd.print("DB Connected");
    signupOK = true;
  }
  else
  { // this line print a formated string to the serial monitor
    // c_srt is a function used to manipulate the c type string (a null terminated array of characters  )
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
    Serial.print("DB connection failed ");
    lcd.clear();
    lcd.print("DB FAILED");
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
// Main function
void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  // Initialize the DHT values
  //  calling the connect to wifi function
  connectToWifi();
  // Initialize the current sensor
  emon1.voltage(VOLTAGE_INPUT, vCalibration, 1.7);
  emon1.current(CT_INPUT, currCalibration);
  // Connect to DataBase by calling the function
  connectToDatabase();
}

void loop()
{
  if (Firebase.ready() && signupOK && (millis() - lastDataSendTime > SEND_INTERVAL || lastDataSendTime == 0))
  { // assign value of the millis to  the lastDataSendTime
    lastDataSendTime = millis();
    // This method of emonlib is used to calculate the RMS of using 20(samples) over (2000) 2 seconds
    emon1.calcVI(20, 2000);
    double voltage = emon1.Vrms;
    double amps = emon1.Irms;
    double watt = amps * voltage;
    lcd.clear();
    lcd.print("V:" + String(voltage) + "  C:" + String(amps));
    lcd.setCursor(0, 1);
    lcd.print("P:" + String(watt));

    // WRITE data of current to the path :path/current
    if (Firebase.RTDB.setDouble(&fbdo, "path/current", amps))
    {
      Serial.println("Passed");
      // lcd.print("Passed");
      Serial.println("PATH:" + fbdo.dataPath());
      // lcd.print("PATH:" + fbdo.dataPath());
      Serial.println("TYPE :" + fbdo.dataType());
      // lcd.print("TYPE :" + fbdo.dataType());
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CURRENT FAILED");
      Serial.println("FAILED TO WRITE CURRENT DATA");
      Serial.println("REASON :" + fbdo.errorReason());
    }
    // WRITE data of power to the path :path/power
    if (Firebase.RTDB.setDouble(&fbdo, "path/power", watt))
    {
      Serial.println("Passed");
      // lcd.print("Passed");
      Serial.println("PATH:" + fbdo.dataPath());
      // lcd.print("PATH:" + fbdo.dataPath());
      Serial.println("TYPE :" + fbdo.dataType());
      // lcd.print("TYPE :" + fbdo.dataType());
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("POWER FAILED");
      Serial.println("FAILED To WRITE POWER DATA");
      Serial.println("REASON :" + fbdo.errorReason());
    }
    // WRITE data of voltage to the path :path/temprature
    if (Firebase.RTDB.setDouble(&fbdo, "path/voltage", voltage))
    {
      Serial.println("Passed");
      // lcd.print("Passed");
      Serial.println("PATH:" + fbdo.dataPath());
      // lcd.print("PATH:" + fbdo.dataPath());
      Serial.println("TYPE :" + fbdo.dataType());
      // lcd.print("TYPE :" + fbdo.dataType());
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("VOLTAGE FAILED");
      Serial.println("FAILED TO WRITE VOLTAGE DATA");
      Serial.println("REASON :" + fbdo.errorReason());
    }
  }
}