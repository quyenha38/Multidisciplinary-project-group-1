#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>;
#include <WebServer.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//uncommment to run web server
//#define WLAN_SSID "YOLO BIT"
//uncomment to connect Wifi
#define WLAN_SSID "RD-SEAI_2.4G"
#define WLAN_PASS ""
// #define AIO_SERVER      "io.adafruit.com"
// #define AIO_SERVERPORT  1883
// #define AIO_USERNAME    "quyenha38"
// #define AIO_KEY         "aio_QSJK81LuXS8CcLafV5bXqLyePMdt"

#define OHS_SERVER      "mqtt.ohstem.vn"
#define OHS_SERVERPORT  1883
#define OHS_USERNAME    "quyen"
#define OHS_KEY         ""


// #define BKTK_SERVER      "mqttserver.tk"
// #define BKTK_SERVERPORT  1884
// #define BKTK_USERNAME    "innovation"
// #define BKTK_KEY         "Innovation_RgPQAZoA5N"
#define LED_BUILTIN 4

WiFiClient client;

//Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

// Adafruit_MQTT_Subscribe timefeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

// Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/V1", MQTT_QOS_1);

// Adafruit_MQTT_Publish sensory = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/V20");
// Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
// Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
// Adafruit_MQTT_Publish lightFeed= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/light");
// Adafruit_MQTT_Publish soilMoistureFeed= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/soilmoisture");

//Adafruit_MQTT_Subscribe slider = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/slider", MQTT_QOS_1);

// MQTT Feeds for ohstem 
Adafruit_MQTT_Client mqtt(&client, OHS_SERVER, OHS_SERVERPORT, OHS_USERNAME, OHS_USERNAME, OHS_KEY); //for Ohstem
Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V2");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V3");
Adafruit_MQTT_Publish soilMoistureFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V4");
Adafruit_MQTT_Publish lightFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V5");
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, OHS_USERNAME "/feeds/V1", MQTT_QOS_1);

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndPump(void *pvParameters);
void TaskLightAndLED(void *pvParameters);

void MQTT_connect() {
  int8_t ret;

  if (mqtt.connected()) {
    Serial.println("MQTT connected");
    return;
  }
Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);
    retries--;
    if (retries == 0) {
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

//Define your components here
Adafruit_NeoPixel pixels3(4, P2, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);
WebServer server(80);

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Sensor Dashboard</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    .data-box { margin: 20px; padding: 20px; border: 1px solid #ddd; display: inline-block; width: 300px; }
    .bar-container {
      width: 100%;
      background-color: #ddd;
      height: 20px;
      border-radius: 10px;
      overflow: hidden;
      margin-top: 10px;
    }
    .bar {
      height: 100%;
      background-color: lightblue; /* Changed to light blue */
    }
    .button {
      padding: 10px 20px;
      margin-top: 20px;
      background-color: #4CAF50;
      color: white;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    .button:hover {
      background-color: #45a049;
    }
  </style>
  <script>
    async function fetchData() {
      const response = await fetch('/data');
      const data = await response.json();

      // Update numerical values
      document.getElementById('temperature').innerText = data.temperature + ' °C';
      document.getElementById('humidity').innerText = data.humidity + ' %';
      document.getElementById('light').innerText = data.light + ' lx';
      document.getElementById('soil').innerText = data.soil + ' %';
      document.getElementById('led').innerText = data.led ? 'ON' : 'OFF';

      // Update progress bars
      updateBar('temp-bar', data.temperature, 50); // Assuming max temp is 50 °C
      updateBar('humidity-bar', data.humidity, 100); // Max humidity is 100%
      updateBar('light-bar', data.light, 5000); // Max light intensity (example)
      updateBar('soil-bar', data.soil, 5000); // Max soil moisture is 5000
    }

    function updateBar(id, value, maxValue) {
      const bar = document.getElementById(id);
      const percentage = (value / maxValue) * 100;
      bar.style.width = percentage + '%';
      bar.innerText = ''; // Remove the text inside the bar
    }

    async function toggleLED() {
      const ledStatus = document.getElementById('led').innerText === 'OFF' ? 'ON' : 'OFF';
      await fetch(`/toggle_led?state=${ledStatus}`);
      fetchData(); // Re-fetch the data to update the LED status
    }

    setInterval(fetchData, 1000);
    window.onload = fetchData;
  </script>
</head>
<body>
  <h1>ESP32 Sensor Dashboard</h1>
  <div class="data-box">
    <h2>Temperature</h2>
    <p id="temperature">--</p>
    <div class="bar-container">
      <div id="temp-bar" class="bar" style="width: 0%"></div> <!-- No text inside -->
    </div>
  </div>
  <div class="data-box">
    <h2>Humidity</h2>
    <p id="humidity">--</p>
    <div class="bar-container">
      <div id="humidity-bar" class="bar" style="width: 0%"></div> <!-- No text inside -->
    </div>
  </div>
  <div class="data-box">
    <h2>Light Intensity</h2>
    <p id="light">--</p>
    <div class="bar-container">
      <div id="light-bar" class="bar" style="width: 0%"></div> <!-- No text inside -->
    </div>
  </div>
  <div class="data-box">
    <h2>Soil Moisture</h2>
    <p id="soil">--</p>
    <div class="bar-container">
      <div id="soil-bar" class="bar" style="width: 0%"></div> <!-- No text inside -->
    </div>
  </div>
  <div class="data-box">
    <h2>LED Status</h2>
    <p id="led">--</p>
    <button class="button" onclick="toggleLED()">Toggle LED</button>
  </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}
void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(dht20.getTemperature()) + ",";
  json += "\"humidity\":" + String(dht20.getHumidity()) + ",";
  json += "\"light\":" + String(analogRead(P1)) + ",";
  json += "\"soil\":" + String(analogRead(P0)) + ","; //get data trực tiếp từ sensor không cần thời gian delay trong while
  json += "\"led\":" + String(digitalRead(LED_BUILTIN));
  json += "}";
  server.send(200, "application/json", json);
}
// void slidercallback(double x) {
//   Serial.print("Hey we're in a slider callback, the slider value is: ");
//   Serial.println(x);
// }
void onoffcallback(char *data, uint16_t len) {
  Serial.print("Button value received from MQTT: ");
  Serial.println(data);

  if (strcmp(data, "ON") == 0) {
      digitalWrite(P3, HIGH);  
      Serial.println("Turning ON the pump");
  } else if (strcmp(data, "OFF") == 0) {
      digitalWrite(P3, LOW);  
      Serial.println("Turning OFF the pump");
  } else {
      Serial.println("Invalid command received");
  }

}



  uint8_t retries = 3;
  


void setup() {
   pinMode(P13, OUTPUT);
digitalWrite(P13, LOW); // Default turn of pump
  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200); 
   //WiFi.softAP(WLAN_SSID);
  
  Wire.begin(P20, P19);  
  dht20.begin();

  lcd.begin(33,16,2);
lcd.backlight(); 

  pixels3.begin();

  // Connect to Wi-Fi
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT Subscription
  onoffbutton.setCallback(onoffcallback);
  mqtt.subscribe(&onoffbutton);

  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
 

  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();
  Serial.println("HTTP server started");
  // slider.setCallback(slidercallback);
  onoffbutton.setCallback(onoffcallback);
  // mqtt.subscribe(&slider);
  mqtt.subscribe(&onoffbutton);

  
  //xTaskCreate( TaskBlink, "Task Blink" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature and Humidity" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndPump, "Task Soild Pump" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  
  
  //Now the task scheduler is automatically started.
  Serial.printf("Basic Multi Threading Arduino Example\n");
}
int pubCount = 0;
void loop() {
  MQTT_connect();
  mqtt.processPackets(10000);
  if (!mqtt.ping()) {
    mqtt.disconnect();
  }

   server.handleClient();
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

uint32_t x=0;
void TaskBlink(void *pvParameters) {  // This is a task.
  uint32_t blink_delay = *((uint32_t *)pvParameters);

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  

  while(1) {                          
    digitalWrite(LED_BUILTIN, HIGH); 
    Serial.println("LED_on"); // turn the LED ON
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED OFF
    Serial.println("LED_off");
    delay(5000);
    // if (sensory.publish(x++)) {
    //   Serial.println(F("Published successfully!!"));
    // }
  }
}


void TaskTemperatureHumidity(void *pvParameters) {  // This is a task.
  uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {          
    dht20.read();
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();
    Serial.println("Temperature: ");
    Serial.println(dht20.getTemperature());
    // if (temperatureFeed.publish(dht20.getTemperature())) {
    //   Serial.println(F("Published successfully!!"));
    // }
        if (temperatureFeed.publish(temperature)) {
      Serial.println("Temperature Published Successfully!");
    } //for ohstem pubblish
    Serial.println("Humidity: ");
    Serial.println(dht20.getHumidity());
    // if (humidityFeed.publish(dht20.getHumidity())) {
    //   Serial.println(F("Published successfully!!"));
    // }
    if (humidityFeed.publish(humidity)) {
      Serial.println("Humidity Published Successfully!");
    }//for ohstem pubblish

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20.getTemperature());
    lcd.setCursor(0, 1);
    lcd.print(dht20.getHumidity());

    delay(5000);
  }
}

void TaskSoilMoistureAndPump(void *pvParameters) {  // This is a task.
  uint32_t blink_delay = *((uint32_t *)pvParameters);

pinMode(P13, OUTPUT);

  while(1) {               
    float soilMoisture = analogRead(P0);           
    Serial.println("Task Soild and Pump");
    Serial.println(analogRead(P0));
      if (soilMoistureFeed.publish(soilMoisture)) {
      Serial.println("Soil Moisture Published Successfully!");
    }//for ohstem pubblish
    if(analogRead(P0) > 1850){
      digitalWrite(P13, LOW);
    }
    if(analogRead(P0) < 1850){
      digitalWrite(P13, HIGH);
    }
    delay(7000);
  }
}


void TaskLightAndLED(void *pvParameters) {  // This is a task.
  uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {   
    float lightLevel = analogRead(P1);                       
    Serial.println("Task Light and LED");
    Serial.println(analogRead(P1));
        if (lightFeed.publish(lightLevel)) {
      Serial.println("Light Level Published Successfully!");
    }//for ohstem pubblish
    if(analogRead(P1) < 1000){
      pixels3.setPixelColor(0, pixels3.Color(255,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,255));
      pixels3.setPixelColor(2, pixels3.Color(255,0,255));
      pixels3.setPixelColor(3, pixels3.Color(0,255,0));
      pixels3.show();
    }
    if(analogRead(P1) > 1000){
      pixels3.setPixelColor(0, pixels3.Color(0,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,0));
      pixels3.setPixelColor(2, pixels3.Color(0,0,0));
      pixels3.setPixelColor(3, pixels3.Color(0,0,0));
      pixels3.show();
    }
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    delay(9000);
  }
}