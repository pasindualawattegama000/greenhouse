#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// WiFi credentials
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

// HiveMQ Cloud settings
const char* mqtt_server = "fee58338fdf74c2ca59bc0a0d5bf477c.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "your_hivemq_username";
const char* mqtt_password = "your_hivemq_password";

// Device settings
const char* device_owner = "user123"; // Should be set during device registration
const char* device_id = "esp32_1";    // Unique device ID

// Pins
const int ledPin = 2;
const int sensorPin = 34;

// Certificate for HiveMQ Cloud (TLS)
const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
float lastSensorValue = 0;
bool ledState = false;

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  String topicStr = String(topic);
  
  if (topicStr.endsWith("led/control")) {
    if (message == "ON") {
      digitalWrite(ledPin, HIGH);
      ledState = true;
      publishLedStatus();
    } else if (message == "OFF") {
      digitalWrite(ledPin, LOW);
      ledState = false;
      publishLedStatus();
    }
  }
}

void publishLedStatus() {
  String topic = String("greenhouse/") + device_owner + "/" + device_id + "/led/status";
  client.publish(topic.c_str(), ledState ? "ON" : "OFF");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(device_id, mqtt_user, mqtt_password)) {
      String subscribeTopic = String("greenhouse/") + device_owner + "/" + device_id + "/led/control";
      client.subscribe(subscribeTopic.c_str());
      
      // Publish initial status
      publishLedStatus();
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  // Configure TLS
  espClient.setCACert(ca_cert);
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long now = millis();
  if (now - lastMsg > 2000) {  // Publish every 2 seconds
    lastMsg = now;
    
    // Read sensor
    float sensorValue = analogRead(sensorPin) * (3.3 / 4095.0);  // Convert to voltage if needed
    
    // Only publish if value changed significantly
    if (abs(sensorValue - lastSensorValue) >= 0.1) {
      lastSensorValue = sensorValue;
      
      String topic = String("greenhouse/") + device_owner + "/" + device_id + "/sensor";
      client.publish(topic.c_str(), String(sensorValue).c_str());
    }
  }
}