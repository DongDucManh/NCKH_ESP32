#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define WIFI_AP_NAME        "Manh quy"
#define WIFI_PASSWORD       "11052005"
#define TOKEN               "yx3l0q7369xnra81fxmb"
#define THINGSBOARD_SERVER  "192.168.0.106"
#define SERIAL_DEBUG_BAUD   115200

#define LED_PIN_15 15 // Chân GPIO 15 điều khiển đèn LED 1
#define LED_PIN_17 17 // Chân GPIO 17 điều khiển đèn LED 2
#define LIGHT_SENSOR_PIN 4

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  setup_wifi();
  client.setServer(THINGSBOARD_SERVER, 1883);
  client.setCallback(callback);

  pinMode(LED_PIN_15, OUTPUT);
  pinMode(LED_PIN_17, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  digitalWrite(LED_PIN_15, LOW); // Khởi tạo đèn LED 15 tắt
  digitalWrite(LED_PIN_17, LOW); // Khởi tạo đèn LED 17 tắt
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
    if (client.connect("ESP32Client", TOKEN, "")) {
      Serial.println("connected");
      client.subscribe("v1/devices/me/attributes");
      client.subscribe("v1/devices/me/telemetry");
      client.subscribe("v1/devices/me/rpc/request/+"); // Subscribe topic RPC
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Xử lý RPC request
  if (String(topic).startsWith("v1/devices/me/rpc/request/")) {
    String requestId = String(topic).substring(26); // Lấy request ID
    String method = doc["method"]; // Lấy phương thức RPC

    if (method == "setLedState") {
      int ledPin = doc["params"]["led"]; // Lấy chân LED (15 hoặc 17)
      bool state = doc["params"]["state"]; // Lấy trạng thái (true/false)

      if (ledPin == 15) {
        digitalWrite(LED_PIN_15, state ? HIGH : LOW); // Điều khiển LED 15
      } else if (ledPin == 17) {
        digitalWrite(LED_PIN_17, state ? HIGH : LOW); // Điều khiển LED 17
      }

      // Gửi phản hồi
      String responseTopic = "v1/devices/me/rpc/response/" + requestId;
      String responsePayload = "{\"success\": true}";
      client.publish(responseTopic.c_str(), responsePayload.c_str());
    }
  }
}

void changeLedState(int ledPin, bool state) {
  if (ledPin == 15) {
    digitalWrite(LED_PIN_15, state ? HIGH : LOW);
  } else if (ledPin == 17) {
    digitalWrite(LED_PIN_17, state ? HIGH : LOW);
  }
  String payload = "{\"led\":" + String(ledPin) + ", \"state\":" + String(state) + "}";
  Serial.print("Publishing state: ");
  Serial.println(payload);
}

void sendLightSensorData() {
  float lightValue = analogRead(34);
  String payload = "{\"light_intensity\":" + String(lightValue) + "}";
  Serial.print("Publishing light intensity: ");
  Serial.println(lightValue);

  if (lightValue < 1500) {
    changeLedState(17, false); // Tắt LED 17 nếu ánh sáng yếu
  } else {
    changeLedState(17, true); // Bật LED 17 nếu ánh sáng mạnh
  }

  client.publish("v1/devices/me/telemetry", payload.c_str());
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  sendLightSensorData();
  //delay(1000); // Delay 1 second between sensor data sends
}