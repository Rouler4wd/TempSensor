#define ANALOG_PIN  34
#define DIGITAL_PIN 23

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

int redpin = 25;
int greenpin = 33; 
int val;

const char *ssid = "choppedchin";
const char *password = "becauseivebeen";
const char* apiKey = "2855eaab49ad41dcb33231157250102";

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 60 seconds

WebServer server(80);

String city = "New York";
String formattedCity = city;  
String temperature = "Loading...";
float kwh_price = 0.0;
float energyCost = 0.0; 
float energyCool = 0.0; 
float temperatureC = 0.0;

String abc = "Just open the windows!";
float calculateEnergyConsumption(float outsideTemp) {

    const float roomArea = 30.0; 
    const float insulationFactor = 0.5; 
    const float temperatureDifference = temperatureC - outsideTemp; 


    float energyRequired = roomArea * temperatureDifference * insulationFactor;
    return energyRequired;
}
float calculateEnergyConsumptionCool(float outsideTemp) {
    const float roomArea = 30.0; 
    const float insulationFactor = 0.5; 
    const float temperatureDifference = outsideTemp - temperatureC; // Cooling difference

    float energyRequired = roomArea * temperatureDifference * insulationFactor;
    return energyRequired;
}


void fetchWeatherData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        

        city.replace(" ", "%20");


        String apiUrl = "http://api.weatherapi.com/v1/current.json?key=" + String(apiKey) + "&q=" + city;
        Serial.println("API Request: " + apiUrl);

        http.begin(apiUrl);
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            Serial.println("Full API Response: " + payload);

            int tempIndex = payload.indexOf("\"temp_f\":");
            if (tempIndex != -1) {
                int start = tempIndex + 9;
                int end = payload.indexOf(',', start);
                temperature = payload.substring(start, end);
                
                
                float currentTemp = temperature.toFloat();
                energyCost = calculateEnergyConsumption(currentTemp);
                energyCool = calculateEnergyConsumptionCool(currentTemp);
                if (energyCool < 0) {
                  Serial.write("DAMNNN");
                }
            } else {
                temperature = "Error";
            }
        } else {
            Serial.println("Error fetching weather data");
            temperature = "Error";
        }
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
        temperature = "No WiFi";
    }
}


void handleRoot() {
    String html = "<html><head><title>ESP32 Weather</title>";
    html += "<meta charset='utf-8'>";
    html += "<meta http-equiv='refresh' content='60'>";
    html += "<style>";
    html += "body { background-color: #f0f8ff; font-family: Arial, sans-serif; margin: 0; padding: 0; }";
    html += "header { background-color: #4CAF50; color: white; padding: 20px; text-align: center; font-size: 24px; }";
    html += "h1 { font-size: 36px; }";
    html += "p { font-size: 20px; color: #333; }";
    html += "form { margin: 20px auto; text-align: center; }";
    html += "input[type='text'] { padding: 10px; font-size: 16px; width: 200px; }";
    html += "input[type='submit'] { background-color: #4CAF50; color: white; border: none; padding: 10px 20px; font-size: 16px; cursor: pointer; }";
    html += "input[type='submit']:hover { background-color: #45a049; }";
    html += ".container { width: 80%; margin: 20px auto; background-color: #fff; border-radius: 10px; padding: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.2); }";
    html += ".footer { text-align: center; padding: 10px; font-size: 14px; background-color: #4CAF50; color: white; position: fixed; bottom: 0; width: 100%; }";
    html += "</style>";
    
    html += "</head><body>";
    html += "<header><h1>ESP32 Weather Display</h1></header>";
    html += "<div class='container'>";
    html += "<p>Current Temperature in your City: " + temperature + " °F</p>";
    html += "<p>Current Temperature in your Building " + String(temperatureC, 2) + "°F <br><br>";
    html += "kWH pulled to maintain Heat: " + String(energyCost/100, 2) + " kW</p>";
    html += "<p>kWH pulled to maintain Cool: " + String(energyCool/100, 2) + " kW</p>";
    html += "<p style='font-size:9px;'>*if kW is in the negatives, you can just open your windows to achieve heat or cool respectively :)</p>";
    
    html += "<p>Cost for kWH in your city is: " + String(kwh_price) + " dollars. </p>";


    html += "<form action='/setcity' method='POST'>";
    html += "Enter City: <input type='text' name='city'><input type='submit' value='Update'>";
    html += "</form>";
    
    html += "</div>";
    html += "<div class='footer'>Nandan, Sunay, Pranauv lit Coding &copy; 2025</div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}


void handleSetCity() {
    if (server.hasArg("city")) {
        city = server.arg("city");  
        fetchWeatherData();  
    }
    server.sendHeader("Location", "/");  
    server.send(303);
}


void setup() {
    pinMode(redpin, OUTPUT);
	  pinMode(greenpin, OUTPUT);
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    

    if (MDNS.begin("esp32")) {
        Serial.println("MDNS responder started");
    }
    server.on("/", handleRoot);
    server.on("/setcity", HTTP_POST, handleSetCity);
    server.begin();
    fetchWeatherData();  // Fetch initial weather data

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    pinMode(DIGITAL_PIN, INPUT);
}

void loop() {
    analogWrite(redpin, 255);
    server.handleClient();
 
    formattedCity.replace("%20", " ");

    if (millis() - lastUpdate >= updateInterval) {
        fetchWeatherData();
        lastUpdate = millis();
    }
    int analogValue = analogRead(ANALOG_PIN);
    int digitalState = digitalRead(DIGITAL_PIN);

    
    float voltage = analogValue * (3.3 / 4095.0);

    if (energyCost > 0){
      kwh_price = (energyCost/100)*0.16;
    } else if (energyCool > 0){
      kwh_price = (energyCool/100)*0.16;
    }
    
    temperatureC = (1.3 - voltage) * 100.0; 

    Serial.print("Analog Value: "); Serial.print(analogValue);
    Serial.print(" | Voltage: "); Serial.print(voltage, 2);
    Serial.print("V | Temperature: "); Serial.print(temperatureC);
    Serial.println("°F");

    Serial.print("Digital Output: ");
    Serial.println(digitalState ? "HIGH (Threshold Exceeded)" : "LOW (Below Threshold)");

    delay(1000);
}
