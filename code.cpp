#include <PubSubClient.h>  
#include <WiFi.h>
#include <ArduinoJson.h>  
#define WIFI_AP_NAME        "UTC-STUDENT"
#define WIFI_PASSWORD       ""


#define TOKEN               "0o3dthn7ze7rtxc6i801"
#define THINGSBOARD_SERVER  "10.37.11.220"


#define SERIAL_DEBUG_BAUD    115200


WiFiClient espClient;
PubSubClient client(espClient);
#define mqtt_server ffeaa629-ab8a-4062-a7e1-557269130999
#define LED_PIN 17
#define LIGHT_SENSOR_PIN 13 // Analog pin connected to the light sensor


bool subscribed = false;


// callback when a new message arrives
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
 
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
 
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
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("LED OFF");
    digitalWrite(LED_PIN, LOW);  
  }
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
   
    if (client.connect("ESP32Client", TOKEN, "")) {
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


void sendLightSensorData() {
  int lightValue = analogRead(LIGHT_SENSOR_PIN);
  float voltage = lightValue * (3.3 / 4095.0);  
  float lightIntensity = map(lightValue, 0, 4095, 0, 100);




  String payload = "{\"light_intensity\":" + String(lightIntensity) + "}";
  Serial.print("Publishing light intensity: ");
  Serial.println(lightIntensity);




  client.publish("v1/devices/me/telemetry", payload.c_str());
}


void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);


  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");


  client.setServer(THINGSBOARD_SERVER, 1883);
  client.setCallback(callback);
 
  pinMode(LED_PIN, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }


  client.loop();


  // Send light sensor data every second
  sendLightSensorData();
  delay(1000); // Delay 1 second between sensor data sends
}




