//wiehack(animal fencing system)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClientSecure.h>
#include "SparkFun_SGP30_Arduino_Library.h"
#include "esp_camera.h"

// Wi-Fi credentials
const char* ssid = "your_ssid";
const char* password = "your_password";

// Server details
const char* serverAddress = "your_server_address";
const int serverPort = 80;

// Pin configuration
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

// OLED display setup
Adafruit_SSD1306 display(-1);

// Camera configuration
camera_config_t config;

// Initialize SGP30 sensor
SGP30 airSensor;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Initialize I2C communication for OLED display
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // Initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");
  
  // Initialize camera
  esp_camera_init(&config);
  
  // Initialize SGP30 sensor
  airSensor.begin();
  
     // Print IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Capture image from camera
  camera_fb_t *fb = esp_camera_fb_get();
  
  if (fb) {
    // Connect to server
    WiFiClientSecure client;
    if (client.connect(serverAddress, serverPort)) {
      // Create HTTP POST request
      HTTPClient http;
      http.begin(client, serverAddress, serverPort, "/upload");
      
      // Create JSON payload
      StaticJsonDocument<200> doc;
      doc["sensor"] = "camera";
      
      // Convert image to base64 and add to payload
      size_t encodedSize = base64_enc_len(fb->len);
      char* base64Image = (char*)malloc(encodedSize);
      base64_encode(base64Image, (char*)fb->buf, fb->len);
      doc["image"] = base64Image;
      
      // Serialize JSON document
      String payload;
      serializeJson(doc, payload);
      
      // Send POST request with payload
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(payload);
      
      if (httpResponseCode == 200) {
        Serial.println("Image uploaded successfully");
        
        // Parse server response
        String response = http.getString();
        DynamicJsonDocument jsonDoc(200);
        deserializeJson(jsonDoc, response);
        
        // Get response values
        bool fenceStatus = jsonDoc["fence_status"];
        int animalCount = jsonDoc["animal_count"];
        
        // Display fence status and animal count on OLED display
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print("Fence: ");
        display.setTextColor(fenceStatus ? WHITE : BLACK);
        display.println(fenceStatus ? "OK" : "BROKEN");
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.print("Animals: ");
        display.setTextColor(WHITE);
        display.println(animalCount);
        display.display();
        
        // Print response to serial monitor
        Serial.println(response);
      } else {
        Serial.print("Error uploading image. HTTP response code: ");
        Serial.println(httpResponseCode);
      }
      
      // End HTTP request
      http.end();
    } else {
      Serial.println("Failed to connect to server");
    }
    
    // Free memory
    free(base64Image);
    esp_camera_fb_return(fb);
  }
  
  // Delay before next image capture
  delay(5000);
}
