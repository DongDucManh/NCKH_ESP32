#include <PubSubClient.h>  
#include <WiFi.h>
#include <ArduinoJson.h>  
#define WIFI_AP_NAME        "UTC-STUDENT"
#define WIFI_PASSWORD       ""
#define TOKEN               "0o3dthn7ze7rtxc6i801" 
#define THINGSBOARD_SERVER  "10.37.16.195" 
#define SERIAL_DEBUG_BAUD    115200

WiFiClient espClient;
PubSubClient client(espClient);
#define mqtt_server "broker.hivemq.com"
#define LED_PIN 17
#define LIGHT_SENSOR_PIN 25 // Không có chân analog, dùng chân digital

bool subscribed = false;

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
    client.publish("v1/devices/me/attributes", "LED ON");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("LED OFF");
    client.publish("v1/devices/me/attributes", "LED OFF");
    digitalWrite(LED_PIN, LOW);  
  }
  // if (String(topic) == "v1/devices/me/attributes") {
  //       Serial.print("Changing output to ");
  //       DynamicJsonDocument doc(1024);
  //       DeserializationError error = deserializeJson(doc, message);
  //       if (error) {
  //           Serial.print("deserializeJson() failed: ");
  //           Serial.println(error.c_str());
  //           return;
  //       }
  //       const char* msg = doc["msg"];
  //       if (strcmp(msg, "on") == 0) {
  //           Serial.println("on");
  //           digitalWrite(LED_PIN, HIGH);
  //       } else if (strcmp(msg, "off") == 0) {
  //           Serial.println("off");
  //           digitalWrite(LED_PIN, LOW);
  //       }
  //   }
}
void setup_wifi() {
  delay(10);
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
  float lightValue = digitalRead(LIGHT_SENSOR_PIN);
  float voltage = lightValue ;  
  float lightIntensity = map(lightValue, 0, 4095, 0, 100);
  changeLedState(digitalRead(LIGHT_SENSOR_PIN));
  String payload = "{\"light_intensity\":" + String(lightValue) + "}";
  Serial.print("Publishing light intensity: ");
  Serial.println(lightValue);


  client.publish("v1/devices/me/telemetry", payload.c_str());
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  setup_wifi();
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
