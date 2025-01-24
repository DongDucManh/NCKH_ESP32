#include <PubSubClient.h>  
#include <WiFi.h>
#include <ArduinoJson.h>  
#define WIFI_AP_NAME        "Manh quy"
#define WIFI_PASSWORD       "11052005"
#define TOKEN               "CroIT6OSkT1911EVc2Mw" 
#define THINGSBOARD_SERVER  "210.211.96.129" 
#define SERIAL_DEBUG_BAUD    115200
bool mqttConnected = false;
WiFiClient espClient;
PubSubClient client(espClient);
#define mqtt_server "210.211.96.129"
#define LED_PIN 17
#define LIGHT_SENSOR_PIN 4 
bool subscribed = false;
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  setup_wifi();
  client.setServer(THINGSBOARD_SERVER, 5883);
  client.setCallback(callback);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT); 
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println();

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }
  
  bool state = doc["state"].as<bool>();
  
  if (state) {
    Serial.println("LED ON");
    client.publish("v1/devices/me/attributes", "LED ON");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("LED OFF");
    client.publish("v1/devices/me/attributes", "LED OFF");
    digitalWrite(LED_PIN, LOW);  
  }

}
void setup_wifi() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_AP_NAME);
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP32Client", TOKEN,"")) {
      Serial.println("connected");
      client.subscribe("v1/devices/me/attributes");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
  
}

void changeLedState(bool state) {
  if (state==true) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  String payload = "{\"state\":" + String(state) + "}";
  Serial.print("Publishing state: ");
  Serial.println(payload);
  client.publish("v1/devices/me/attributes", payload.c_str());
}
void sendLightSensorData() {
  float lightValue = analogRead(34);
  float voltage = lightValue ;  
  float lightIntensity = map(lightValue, 0, 4095, 0, 100);
  //changeLedState(digitalRead(LIGHT_SENSOR_PIN));
  String payload = "{\"light_intensity\":" + String(lightValue) + "}";
  Serial.print("Publishing light intensity: ");
  Serial.println(lightValue);


  client.publish("v1/devices/me/telemetry", payload.c_str());
}



void loop() {
  if (client.connected() == false) {
    reconnect();
  }

  client.loop();

  // Send light sensor data every second
  sendLightSensorData();
  delay(1000); // Delay 1 second between sensor data sends
}
/*
 * Kết nối:
 *          VCC      -----      3.3V (ESP32)
 *          D0       -----      GPIO4 (ESP32)
 *          A0       -----      GPIO34 (ESP32 - ADC1_CH6)
 *          GND      -----      GND (ESP32)
 */

// void setup() 
// {
//   // Khởi tạo Serial Monitor với tốc độ baud 115200
//   Serial.begin(115200);
  
//   // Cấu hình chân GPIO4 là INPUT để đọc giá trị digital
//   pinMode(4, INPUT);
// }

// void loop() 
// {
//   // Đọc giá trị digital từ chân GPIO4
//   int digitalValue = digitalRead(4);
  
//   // Đọc giá trị analog từ chân GPIO34 (ADC1_CH6)
//   int analogValue = analogRead(34);
  
//   // Hiển thị giá trị digital và analog trên Serial Monitor
//   Serial.print("Giá trị Digital (D0): ");
//   Serial.print(digitalValue);
//   Serial.print("   |   Giá trị Analog (A0): ");
//   Serial.println(analogValue);
  
//   // Đợi 1 giây trước khi đọc lại
//   delay(1000);
// }