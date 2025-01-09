#include <WiFi.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 4 
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define EEPROM_SIZE 512
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESET_BUTTON_PIN 5

// Khai báo các biến toàn cục
bool resetButtonPressed = false;       // Biến lưu trạng thái nút reset
unsigned long resetButtonPressTime = 0; // Biến lưu thời gian nhấn nút reset

WiFiClient espClient;
PubSubClient client(espClient);

char wifiSSID[32];
char wifiPass[64];
char *mqttServer = "192.168.217.213";
char mqttUser[32];
char *mqttPass = "";
char *mqttTopic = "v1/devices/me/telemetry";

bool wifiConnected = false;
bool mqttConnected = false;

BLECharacteristic *pCharacteristic;
BLEServer *pServer;
String rxValue = "";

void setupBLE();
void connectToWiFi(const char *ssid, const char *password);
void connectToMQTT();
void sendDHT11Data();
void onBLEReceive(String jsonData);
void saveCredentialsToEEPROM();
void loadCredentialsFromEEPROM();
void handleResetButton();
void resetDevice();

class MyBLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxData = pCharacteristic->getValue().c_str();
    if (rxData.length() > 0) {
      Serial.println("Received credentials over BLE");
      rxValue += rxData;
      Serial.println("Received Data: " + rxValue);
      if (rxValue.indexOf(';') != -1) {
        onBLEReceive(rxValue);
        Serial.println("BLECALLBACK.LOG -> " + rxValue);
        rxValue = "";
      }
    } else {
      Serial.println("Received empty data");
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  dht.begin();
  EEPROM.begin(EEPROM_SIZE);
  loadCredentialsFromEEPROM();
  connectToWiFi(wifiSSID, wifiPass);
  setupBLE();
  connectToMQTT();
}

void loop() {
  handleResetButton();
  if (WiFi.status() == WL_CONNECTED && mqttConnected) {
    sendDHT11Data();
    delay(10000);
  } else {
    Serial.print("[-]");
    delay(1000);
  }
}

void connectToWiFi(const char *ssid, const char *password) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 30000) {
      Serial.println("\nFailed to connect to WiFi");
      wifiConnected = false;
      return;
    }
  }
  Serial.println("\nWiFi connected");
  wifiConnected = true;
}

void connectToMQTT() {
  client.setServer(mqttServer, 1883);
  Serial.print("Connecting to MQTT...");
  if (client.connect("ESP32Client", mqttUser, mqttPass)) {
    Serial.println("MQTT connected");
    mqttConnected = true;
  } else {
    Serial.print("Failed to connect to MQTT, rc=");
    Serial.println(client.state());
    mqttConnected = false;
  }
}

void sendDHT11Data() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }
  String payload = "{\"humidity\": " + String(humidity) + ", \"temperature\":" + String(temperature) + "}";
  client.publish(mqttTopic, payload.c_str());
  Serial.println("Data sent to MQTT: " + payload);
}

void setupBLE() {
  BLEDevice::init("ESP32_BLE");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->setCallbacks(new MyBLECallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("BLE setup complete, waiting for credentials...");
}

void onBLEReceive(String jsonData) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }
  if (doc.containsKey("reset") && doc["reset"] == true) {
    Serial.println("Reset command received via BLE. Resetting device...");
    resetDevice();
    return;
  }
  strcpy(wifiSSID, doc["ssid"]);
  strcpy(wifiPass, doc["password"]);
  strcpy(mqttUser, doc["mqttUser"]);
  connectToWiFi(wifiSSID, wifiPass);
  if (wifiConnected) {
    connectToMQTT();
  }
  if (mqttConnected) {
    String message = "SUCCESS";
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
    Serial.println("BLE notification sent: " + message);
  }
  saveCredentialsToEEPROM();
}

void saveCredentialsToEEPROM() {
  EEPROM.writeString(0, wifiSSID);
  EEPROM.writeString(32, wifiPass);
  EEPROM.writeString(96, mqttUser);
  EEPROM.commit();
  Serial.printf("SAVE_EROM.LOG -> SSI: %s\n", wifiSSID);
  Serial.printf("SAVE_EROM.LOG -> PASSWORD: %s\n", wifiPass);
  Serial.printf("SAVE_EROM.LOG -> MQTTUSER: %s\n", mqttUser);
  Serial.println("Credentials saved to EEPROM.");
}

void loadCredentialsFromEEPROM() {
  EEPROM.readString(0, wifiSSID, 32);
  EEPROM.readString(32, wifiPass, 64);
  EEPROM.readString(96, mqttUser, 32);
}

void handleResetButton() {
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    if (!resetButtonPressed) {
      resetButtonPressTime = millis();
      resetButtonPressed = true;
    } else if (millis() - resetButtonPressTime > 10000) {
      DynamicJsonDocument doc(256);
      doc["status"] = "resetting";
      doc["message"] = "Device will reset in 10 seconds";
      String jsonMessage;
      serializeJson(doc, jsonMessage);
      client.publish("v1/devices/me/attributes", jsonMessage.c_str());
      pCharacteristic->setValue(jsonMessage.c_str());
      pCharacteristic->notify();
      Serial.println("Reset button held for 10 seconds. Resetting device...");
      resetDevice();
    }
  } else {
    resetButtonPressed = false;
  }
}

void resetDevice() {
  memset(wifiSSID, 0, sizeof(wifiSSID));
  memset(wifiPass, 0, sizeof(wifiPass));
  memset(mqttUser, 0, sizeof(mqttUser));
  saveCredentialsToEEPROM();
  String message = "RESET_SUCCESS";
  pCharacteristic->setValue(message.c_str());
  pCharacteristic->notify();
  DynamicJsonDocument doc(256);
  doc["status"] = "reset";
  doc["message"] = "Device has been reset";
  String jsonMessage;
  serializeJson(doc, jsonMessage);
  client.publish("v1/devices/me/attributes", jsonMessage.c_str());
  Serial.println("BLE notification sent: " + message);
  ESP.restart();
}