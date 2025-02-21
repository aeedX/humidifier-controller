//device-id  =  3cc0d925
#define RELAY 14

#include <GyverBME280.h>

#include <FileData.h>
#include <LittleFS.h>

#include <ESP8266WiFi.h>

#include <GyverHub.h>
#include <hub_macro.hpp>

GyverHub hub("MyDevices", "ESP8266", "");  // имя сети, имя устройства, иконка
struct Data {
  char ssid[32];
  char pass[63];
};
Data mydata;
GyverBME280 bme;

FileData data(&LittleFS, "/data.dat", 'A', &mydata, sizeof(mydata));

//String ssid = db[k::wifi_ssid];
//String pass = db[k::wifi_pass];
String ssid = "Hyper+T-2G";
String pass = "YcapMFQA789";
bool auto_state;
bool hfier_state;
float temp;
float hum;
float pres;

// билдер
void build(gh::Builder& b) {
  b.GaugeRound(&hum).label("hum"); {
    gh::Row r(b);
    b.GaugeRound(&temp).label("temp");
    b.GaugeRound(&pres).label("pres");
  } {
    gh::Row r(b);
    b.Switch(&auto_state).label("auto mode"); 
    b.Switch(&hfier_state).label("humidifier state");
  } {
    gh::Row r(b);
    b.Input(&ssid).label("ssid");
    b.Input(&pass).label("pass");
  } if (b.Button().label("confirm").click()) {
    mydata.ssid = ssid;
    mydata.pass = pass;
    data.update();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY, OUTPUT);

   // подключение к WiFi..
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  int ConnectingTimeout = 60;
  while (WiFi.status() != WL_CONNECTED and ConnectingTimeout > 0) {
    Serial.print(".");
    delay(1000);
    ConnectingTimeout--;
  }

  if (ConnectingTimeout > 0) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("ConnectionTimeout is over, installing an access point");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("H-Controller AP", "esp8266AP");
    Serial.println("access point created: \"H-Controller AP\"");
    Serial.println("password: \"espAP\"");
    Serial.println(WiFi.softAPIP());
  }
  
   // настройка MQTT/Serial/Bluetooth..
  hub.mqtt.config("m6.wqtt.ru", 17210, "device", "j5S6utsX");

  hub.onBuild(build); // подключаем билдер
  hub.begin();        // запускаем систему
  hub.sendGetAuto(true);  // для отправки в mqtt при действиях с приложения
  bme.begin();
  data.read();
}

void loop() {
  hub.tick();         // тикаем тут
  data.tick();

  if (hfier_state) {
    digitalWrite(RELAY, HIGH);
    hub.sendGet("hfier", 1);
  } else {
    digitalWrite(RELAY, LOW);
    hub.sendGet("hfier", 0);
  }

  // раз в 5 секунд отправлять значение виджета temp
  static gh::Timer tmr(5000);
  if (tmr) {
    // задать случайное значение
    temp = bme.readTemperature();
    hum = bme.readHumidity();
    pres = pressureToMmHg(bme.readPressure());
     // отправить из билдера
    hub.sendGet("temp", temp);
    hub.sendGet("hum", hum);
    hub.sendGet("pres", pres);
  }
}