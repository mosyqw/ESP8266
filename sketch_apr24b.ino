#include <ESP8266WiFi.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

// 物联设备信息
const char* sn = "1";

// WiFi连接信息
const char* ssid = "mos01.com";
const char* password = "1F8I@s6a";

// MQTT
const char* mqtt_server = "192.168.1.188";
const int mqtt_port = 1883;
const char* mqtt_username = "admin";
const char* mqtt_password = "123456";
char topic[50] = "IOT/";

WiFiClient espClient;
PubSubClient client(espClient);

// 温度传感器
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE, 15);

// 数据长度
const int msg_length = 320;
//// TCP
//const char* host = "192.168.1.9"; // 你的网点域名或IP 
//const int port = 8888;
//WiFiClient client;

// 时间戳
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// 重连时间
int reconnect_time[8] = {1,2,3,5,10,30,60,300};
int reconnect_count = 0;// 次数
int reconnect_index = 0;// 序号

// 数据间隔时间
int interval = 5000;

void setup(void)
{
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("WiFi连接中");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
 
  // Init DHT 
  dht.begin();

//  // 连接TCP
//  if (!client.connect(host, port)) {
//    Serial.println("TCP connect failed");
//    return;
//  }

  timeClient.begin();
//  timeClient.setTimeOffset(28800);

  // IGPO
  pinMode(4,INPUT);
  pinMode(14,INPUT);

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.connect(sn,mqtt_username,mqtt_password);
  strcat(topic,sn);
  strcat(topic,"/attributes/report");
//  client.subscribe("test");
//  client.setCallback(callback);
}
       
void loop() {
  //重连机制
  if (!client.connected()) {
    reconnect();
  }
  
  // 心跳  
  client.loop();

  // Reading temperature and humidity
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  boolean light = !digitalRead(4);
  boolean voice = digitalRead(14);

  // Display data
  timeClient.update();
  StaticJsonDocument<msg_length> doc;
  doc["sn"] = sn;
  doc["time"] = timeClient.getEpochTime();
  StaticJsonDocument<200> data;
  data["light"] = light;
  data["light_mpa"] = 20000/analogRead(A0);
  data["voice"] = voice;
  data["humidity"] = h;
  data["tempera"] = t;
  doc["data"] = data;
//  StaticJsonDocument<100> obj;
//  obj["m"] = "light";
//  obj["v"] = light;
//  doc["data"].add(obj);
//  obj["m"] = "light_mpa";
//  obj["v"] = 20000/analogRead(A0);
//  doc["data"].add(obj);
//  obj["m"] = "voice";
//  obj["v"] = voice;
//  doc["data"].add(obj);
//  obj["m"] = "humidity";
//  obj["v"] = h;
//  obj["u"] = "%";
//  doc["data"].add(obj);
//  obj["m"] = "tempera";
//  obj["v"] = t;
//  obj["u"] = "*C";
//  doc["data"].add(obj);
  char payload[msg_length];
  serializeJson(doc, payload);
  client.publish(topic,payload);
  
  delay(interval);
}

void reconnect(){
  Serial.println("MQTT重连中...");
  while (!client.connected()) {
    if (client.connect(sn,mqtt_username,mqtt_password)) {
      Serial.println("MQTT重连成功！");
      reconnect_index = 1;
      reconnect_count = 0;
    }else{
      Serial.println("MQTT重连失败！将于"+String(reconnect_time[reconnect_index])+"秒后重试！");
      delay(1000 * reconnect_time[reconnect_index]);
      reconnect_count += 1;
      if(reconnect_count > 4 && reconnect_index < 7){
        reconnect_index += 1;
        reconnect_count = 0;
      }
    }
  }
}
