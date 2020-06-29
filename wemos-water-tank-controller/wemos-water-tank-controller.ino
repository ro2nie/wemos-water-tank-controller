#include "connectionDetails.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define echoPin D7 // Echo Pin
#define trigPin D6 // Trigger Pin

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long recordSecondsForKeepAlive = 0;
unsigned long recordSecondsForReadingInterval = 0;
int keepAliveInterval = 5000;
const char* waterTankAvailabilityTopic = "water-tank/availability";
const char* waterPumpStatusTopic = "water-pump/status";
const char* waterTankLevelTopic = "water-tank/level";
const char* waterTankIntervalNotPumpingTopic = "water-tank/interval-not-pumping";
const char* waterTankRestartTopic = "water-tank/restart";

String waterPumpStatus = "OFF";

int readingIntervalPumping = 10000;
//Default sensor reading interval when water pump is not on is set to 10 minutes, however this can be set over MQTT.
int readingIntervalNotPumping = 600000;

void setupWifi() {
  delay(10);
  // Start by connecting to a WiFi network
  Serial.println("Connecting to: " + String(ssid));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived [" + String(topic) + "]");
  Serial.println(topic);
  if (strcmp(topic, waterPumpStatusTopic) == 0) {
    waterPumpStatus = "";
    for (int i = 0; i < length; i++) {
      waterPumpStatus += (char)payload[i];
    }
    Serial.println("Received " + String(waterPumpStatusTopic) + " " + waterPumpStatus);
  } else if (strcmp(topic, waterTankIntervalNotPumpingTopic) == 0) {
    String intervalNotPumping = "";
    for (int i = 0; i < length; i++) {
      intervalNotPumping += (char)payload[i];
    }
    readingIntervalNotPumping = intervalNotPumping.toInt() * 60000;
    Serial.println("Interval not pumping set to " + String(readingIntervalNotPumping));
  } else if (strcmp(topic, waterTankRestartTopic) == 0) {
    ESP.restart();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(waterPumpStatusTopic);
      client.subscribe(waterTankIntervalNotPumpingTopic);
      client.subscribe(waterTankRestartTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  setupWifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

int measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(15);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  //Calculate the distance (in cm) based on the speed of sound.
  return duration / 58.2;
}

void keepAlive() {
  if ((millis() - recordSecondsForKeepAlive) >= keepAliveInterval) {
    Serial.println("Send MQTT keepalive");
    client.publish(waterTankAvailabilityTopic, "ON");
    recordSecondsForKeepAlive = millis();
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (waterPumpStatus == "ON") {
    if ((millis() - recordSecondsForReadingInterval) >= readingIntervalPumping) {
      client.publish(waterTankLevelTopic, String(measureDistance()).c_str());
      recordSecondsForReadingInterval = millis();
    }
  } else {
    if ((millis() - recordSecondsForReadingInterval) >= readingIntervalNotPumping) {
      client.publish(waterTankLevelTopic, String(measureDistance()).c_str());
      recordSecondsForReadingInterval = millis();
    }
  }
  keepAlive();
}
