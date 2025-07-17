//MEDIBOX PROJECT......................................................


#include <WiFi.h> //to connecting the wifi connection
#include <PubSubClient.h> //to sen the temp via mqtt
#include "DHTesp.h"//to take the temperature value and send it to the dashboard
#include <NTPClient.h> //to take time to our application 
#include <WiFiUdp.h> //to initiate the ntp client library, we need that library
#include <ESP32Servo.h>//import servo motor

//defining pins for components
#define DHT_PIN 15
#define BUZZER 18
#define LDR_PIN 34 
#define SERVO_PIN 5

// Setting up components and variables
WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

char tempAr[6];

DHTesp dhtSensor;

bool isScheduledON = false;
unsigned long scheduledOnTime;

// Variables for LDR
unsigned long samplingInterval = 5000;   // ts in milliseconds (default 5s)
unsigned long sendingInterval = 120000;  // tu in milliseconds (default 2 mins)

#define MAX_SAMPLES 300
float ldrSamples[MAX_SAMPLES];
int sampleCount = 0;

unsigned long lastSampleTime = 0;
unsigned long lastSendTime = 0;

//Variables for Servo
Servo servo;

float thetaOffset = 30.0;
float gammaFactor = 0.75;
float Tmed = 30.0;


// Functions defined 

//Setting up wifi
void setupWifi(){
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST","");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

}
// buzzer operation
void buzzerOn(bool on){
  if(on){
    tone(BUZZER, 18);
  }else{
    noTone(BUZZER);
  }
}
// get data from mqtt server
void receiveCallback(char* topic,byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");


  // ------- DEBUG BLOCK -------------------
  Serial.print("Raw bytes: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.write(payload[i]);        // print as characters
  }
  Serial.print(" | Length = ");
  Serial.println(length);
  // -----------------------------------------------


  char payloadCharAr[length];
  for(int i = 0;i<length;i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }

  Serial.println();

  if(strcmp(topic, "ENTC-ADMIN-MAIN-ON-OFF") ==0){
    buzzerOn(payloadCharAr[0] == '1'); 
  }else if (strcmp(topic, "ENTC-ADMIN-SCH-ON-OFF") ==0){
    if(payloadCharAr[0] == 'N'){
      isScheduledON= false;
    }else{
      isScheduledON = true;
      scheduledOnTime = atol(payloadCharAr);

       // ------- DEBUG LINE ----------------
      Serial.print("Scheduler ON   |  Epoch parsed = ");
      Serial.println(scheduledOnTime);
      // ------------------------------------------
    }
  }else if (strcmp(topic, "ENTC-LIGHT-ts") == 0) {
  samplingInterval = atol(payloadCharAr) * 1000;
  Serial.print("Updated ts to: "); Serial.println(samplingInterval);
  }else if (strcmp(topic, "ENTC-LIGHT-tu") == 0) {
    sendingInterval = atol(payloadCharAr) * 1000;
    Serial.print("Updated tu to: "); Serial.println(sendingInterval);
  }else if (strcmp(topic, "ENTC-WINDOW-THETA") == 0) {
    thetaOffset = atof(payloadCharAr);
    Serial.print("Updated thetaOffset: "); Serial.println(thetaOffset);

  }else if (strcmp(topic, "ENTC-WINDOW-GAMMA") == 0) {
      gammaFactor = atof(payloadCharAr);
      Serial.print("Updated gammaFactor: "); Serial.println(gammaFactor);

  }else if (strcmp(topic, "ENTC-WINDOW-TEMP-MED") == 0) {
      Tmed = atof(payloadCharAr);
      Serial.print("Updated Tmed: "); Serial.println(Tmed);
  }

}
//setting up mqtt 
void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);

  mqttClient.subscribe("ENTC-ADMIN-MAIN-ON-OFF");
  mqttClient.subscribe("ENTC-ADMIN-SCH-ON-OFF");
  mqttClient.subscribe("ENTC-LIGHT-ts");
  mqttClient.subscribe("ENTC-LIGHT-tu");

  mqttClient.subscribe("ENTC-WINDOW-THETA");
  mqttClient.subscribe("ENTC-WINDOW-GAMMA");
  mqttClient.subscribe("ENTC-WINDOW-TEMP-MED");

}
// setup initials
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setupWifi();

  setupMqtt();
  
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  servo.attach(SERVO_PIN);
  servo.write(thetaOffset);  // Start at minimum angle

}
// temperature reading
void updateTemperature() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (isnan(data.temperature)) {
    Serial.println("DHT read failed!");
    return;
  }
  String(data.temperature, 2).toCharArray(tempAr, 6);

}

float readNormalizedLDR() {
  int raw = analogRead(LDR_PIN); // 0-4095
  return float(raw) / 4095.0;
}
// connect to mqtt broker
void connectToBroker(){
  while (!mqttClient.connected()){
    Serial.print("Attempting MQTT connection....");
    if (mqttClient.connect("ESP32-12345645454")){
      Serial.println("connected");
      mqttClient.subscribe("ENTC-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("ENTC-ADMIN-SCH-ON-OFF");
      mqttClient.subscribe("ENTC-LIGHT-ts");
      mqttClient.subscribe("ENTC-LIGHT-tu");


    }else{
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}
// get the time now
unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

//checking shedule
void checkSchedule(){
  if(isScheduledON ){
    unsigned long currentTime = getTime();
    if(currentTime > scheduledOnTime){
      buzzerOn(true);
      isScheduledON = false;
      mqttClient.publish("ENTC-ADMIN-MAIN-ON-OFF-ESP", "1");
      mqttClient.publish("ENTC-ADMIN-SCH-ESP-ON", "0");
      Serial.println("Scheduled ON");
    }

  }
}

// Maintain window according to light intencity
void updateServoAngle(float lightIntensity, float temperature) {
  // Avoid divide-by-zero or invalid log input
  unsigned long ts = samplingInterval;
  unsigned long tu = sendingInterval;

  
  if (tu == 0 || ts == 0 || lightIntensity < 0 || lightIntensity > 1 || temperature <= 0) return;

  float logRatio = log((float)ts / (float)tu);  // natural log
  float factor = lightIntensity * gammaFactor * logRatio * (temperature / Tmed);

  float angle = thetaOffset + (180.0 - thetaOffset) * factor;

  // Clamp angle between 0 and 180
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;

  servo.write((int)angle);
  Serial.print("Servo angle set to: ");
  Serial.println((int)angle);
}


// running the functions accordingly
void loop() {
  // put your main code here, to run repeatedly:
  if (!mqttClient.connected()){
    connectToBroker();
  }
  mqttClient.loop();
  
  updateTemperature();
  Serial.println(tempAr);
  mqttClient.publish("ENTC-ADMIN-TEMP",tempAr);
  
  checkSchedule();

  unsigned long now = millis();

  // Sample every ts seconds
  if (now - lastSampleTime >= samplingInterval) {
    lastSampleTime = now;
    if (sampleCount < MAX_SAMPLES) {
      ldrSamples[sampleCount++] = readNormalizedLDR();
    }
  }

  // Average and send every tu seconds
  if (now - lastSendTime >= sendingInterval) {
    lastSendTime = now;

    if (sampleCount > 0) {
      float sum = 0;
      for (int i = 0; i < sampleCount; i++) {
        sum += ldrSamples[i];
      }
      float avg = sum / sampleCount;

      char ldrMsg[8];
      dtostrf(avg, 1, 4, ldrMsg);
      mqttClient.publish("ENTC-LIGHT-AVG", ldrMsg);
      Serial.print("LDR Avg Sent: "); Serial.println(ldrMsg);

      // Read temperature and update servo
      TempAndHumidity data = dhtSensor.getTempAndHumidity();
      float tempNow = data.temperature;

      updateServoAngle(avg, tempNow);  // 🔄 Adjust sliding window

      sampleCount = 0;  // Reset buffer
    }
  }

  delay(1000);
}

