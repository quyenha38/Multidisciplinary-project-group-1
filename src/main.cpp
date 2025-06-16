#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <HTTPClient.h>


//#define WLAN_SSID "RD-SEAI_2.4G" //uncomment to run ohstem and adafruit
#define WLAN_SSID "ESP32" //uncomment to run webserver
#define WLAN_PASS ""

//uncomment to run adafruit
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "quyenha38"
#define AIO_KEY         "aio_QSJK81LuXS8CcLafV5bXqLyePMdt"

//uncomment to run ohstem
// #define OHS_SERVER      "mqtt.ohstem.vn"
// #define OHS_SERVERPORT  1883
// #define OHS_USERNAME    "ohstem"
// #define OHS_KEY         ""

// Define ports
#define D5 GPIO_NUM_8
#define D3 GPIO_NUM_6
#define A1 GPIO_NUM_2
#define A0 GPIO_NUM_1

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe timefeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");
Adafruit_MQTT_Subscribe slider = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/slider", MQTT_QOS_1); //slider need more research
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/V1", MQTT_QOS_1);
Adafruit_MQTT_Publish sensory = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/V20");
Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish lightFeed= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/light");
Adafruit_MQTT_Publish soilMoistureFeed= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/soilmoisture");

// MQTT Feeds for ohstem 
// Adafruit_MQTT_Client mqtt(&client, OHS_SERVER, OHS_SERVERPORT, OHS_USERNAME, OHS_USERNAME, OHS_KEY); //for Ohstem
// Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V2");
// Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V3");
// Adafruit_MQTT_Publish soilMoistureFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V4");
// Adafruit_MQTT_Publish lightFeed = Adafruit_MQTT_Publish(&mqtt, OHS_USERNAME "/feeds/V5");
// Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, OHS_USERNAME "/feeds/V1", MQTT_QOS_1);

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void sendImage( String imgPath);
// Define your components here
Adafruit_NeoPixel pixels3(4, D5, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

// HTTP Server
AsyncWebServer server(80);

// void slidercallback(double x) {
//   Serial.print("Slider value: ");
//   Serial.println(x);

//   int brightness = map(x, 0, 100, 0, 255);
//   ledcWrite(0, brightness); 
// }
bool ledState = false;  // LED status
#define LIGHT_PIN GPIO_NUM_48
void onoffcallback(char *data, uint16_t len) {
  Serial.print("Button value: ");
  Serial.println(data);

  if (strcmp(data, "ON") == 0) {
      ledState = true;
      digitalWrite(LIGHT_PIN, HIGH);  // Bật đèn
  } else if (strcmp(data, "OFF") == 0) {
      ledState = false;
      digitalWrite(LIGHT_PIN, LOW);  // Tắt đèn
  }
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;
  
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);
    retries--;
    if (retries == 0) while (1);
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  Serial.begin(115200); 
  Wire.begin(GPIO_NUM_11, GPIO_NUM_12);
  dht20.begin();
  lcd.begin();
  pixels3.begin();
  ledcSetup(0, 5000, 8); 
  ledcAttachPin(GPIO_NUM_48, 0);
  WiFi.softAP(WLAN_SSID); //uncomment to run webserver
//pio run -t uploadfs to flash  data to esp32



//uncomment to run on ohstem and adafruit
  // WiFi.begin(WLAN_SSID, WLAN_PASS);
  // delay(2000);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println();
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
// mqtt end here


  //slider.setCallback(slidercallback);
  onoffbutton.setCallback(onoffcallback);
  //mqtt.subscribe(&slider);
  mqtt.subscribe(&onoffbutton);

  xTaskCreate(TaskBlink, "Task Blink", 2048, NULL, 2, NULL);
  xTaskCreate(TaskTemperatureHumidity, "Task Temperature", 2048, NULL, 2, NULL);
  xTaskCreate(TaskSoilMoistureAndRelay, "Task Soil Relay", 2048, NULL, 2, NULL);
  xTaskCreate(TaskLightAndLED, "Task Light LED", 2048, NULL, 2, NULL);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      //return;
  }
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
String generateStaticHTML() {
  File csvFile = SPIFFS.open("/data/log.csv", "r");
  if (!csvFile) {
    Serial.println("Failed to open log.csv");
    return "<!DOCTYPE html><html><body><h2>Error: CSV file not found</h2></body></html>";
  }

  String html = "<!DOCTYPE html><html><head><title>Sensor Data</title>";
  html += "<style>table{border-collapse:collapse;width:100%;font-family:Arial,sans-serif;}";
  html += "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
  html += "th{background-color:#f2f2f2;}tr:nth-child(even){background-color:#f9f9f9;}</style>";
  html += "</head><body><h2>Sensor Data Log</h2><table>";
  html += "<tr><th>Timestamp</th><th>Temperature</th><th>Humidity</th><th>Soil Moisture</th><th>Light</th></tr>";

  while (csvFile.available()) {
    String line = csvFile.readStringUntil('\n');
    html += "<tr>";
    int start = 0;
    for (int col = 0; col < 5; col++) {
      int end = line.indexOf(',', start);
      if (end == -1 && col < 4) end = line.length();
      html += "<td>" + (end == -1 ? line.substring(start) : line.substring(start, end)) + "</td>";
      start = end + 1;
    }
    html += "</tr>";
  }

  csvFile.close();
  html += "</table></body></html>";
  return html;
}
// Global login flag
bool login = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Route: Root `/`
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!login) {
          request->send(SPIFFS, "/login.html", "text/html");
    } else {
      request->send(SPIFFS, "/web.html", "text/html");
    }
  });

  // Route: Handle login POST
  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    String username, password;

    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      username = request->getParam("username", true)->value();
      password = request->getParam("password", true)->value();

      if (username == "admin1" && password == "tung") {
        login = true;
        request->redirect("/");
      } else {
        request->send(200, "text/plain", "Invalid credentials");
      }
    } else {
      request->send(400, "text/plain", "Missing login info");
    }
  });

    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = generateStaticHTML();
    request->send(200, "text/html", html);
  });

  // Endpoint to fetch temperature
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float temperature = dht20.getTemperature();
    request->send(200, "text/plain", String(temperature));
  });

  // Endpoint to fetch humidity
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    float humidity = dht20.getHumidity();
    request->send(200, "text/plain", String(humidity));
  });

  // Endpoint to fetch light intensity (using analogRead for demo)
  server.on("/light", HTTP_GET, [](AsyncWebServerRequest *request){
    float lightIntensity = analogRead(A1  );  
    request->send(200, "text/plain", String(lightIntensity));
  });

  // Endpoint to fetch Soil Moisture
  server.on("/soilmoisture", HTTP_GET, [](AsyncWebServerRequest *request){
    float soilMoisture = analogRead(A0);  // Đọc cảm biến độ ẩm đất
    request->send(200, "text/plain", String(soilMoisture));
});

  // Endpoint to control LED
  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LIGHT_PIN, HIGH); // Bật đèn
    request->send(200, "text/plain", "LED is On");
});

// Endpoint để tắt đèn (GPIO 48)
server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LIGHT_PIN, LOW); // Tắt đèn
    request->send(200, "text/plain", "LED is Off");
});


  // Start the server
  server.begin();

  Serial.printf("Basic Multi-Threading Arduino Example\n");
}

int pubCount = 0;
void loop() {
  MQTT_connect();
  mqtt.processPackets(10000);
  if(! mqtt.ping()) mqtt.disconnect();
}

// Task Definitions
void TaskBlink(void *pvParameters) {
  pinMode(GPIO_NUM_48, OUTPUT);
  uint32_t x=0;
//uncomment to control by button on adafruit and ohstem
  while (1) {
    if (ledState) {
        digitalWrite(LIGHT_PIN, HIGH);  // Bật đèn LED
    } else {
        digitalWrite(LIGHT_PIN, LOW);  // Tắt đèn LED
    }
    delay(5000);
}
  //uncomment to blink the LED
  // while(1) {
  //   digitalWrite(GPIO_NUM_48, HIGH);
  //   Serial.println("LED_on");
  //   delay(5000);
  //   digitalWrite(GPIO_NUM_48, LOW);
  //   Serial.println("LED_off");
  //   delay(5000);
  //   //uncomment to run addfruit
  //   if (sensory.publish(x++)) {
  //     Serial.println(F("Published successfully!!"));
  //   }
  // }
}

void TaskTemperatureHumidity(void *pvParameters) {
  while(1) {
    dht20.read();
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();
    Serial.print("Temperature: ");
    Serial.println(dht20.getTemperature());
    // if (temperatureFeed.publish(dht20.getTemperature())) {
    //   Serial.println(F("Published successfully!!"));
    // }
    if (temperatureFeed.publish(temperature)) {
      Serial.println("Temperature Published Successfully!");
    } //for ohstem pubblish
    if (humidityFeed.publish(humidity)) {
      Serial.println("Humidity Published Successfully!");
    }//for ohstem pubblish
    Serial.print("Humidity: ");
    Serial.println(dht20.getHumidity());
    // if (humidityFeed.publish(dht20.getHumidity())) {
    //   Serial.println(F("Published successfully!!"));
    // }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20.getTemperature());
    lcd.setCursor(0, 1);
    lcd.print(dht20.getHumidity());

    delay(6000);
  }
}

void TaskSoilMoistureAndRelay(void *pvParameters) {
  pinMode(D3, OUTPUT);
  while(1) {
    float soilMoisture = analogRead(A0);
    if (soilMoistureFeed.publish(soilMoisture)) {
      Serial.println("Soil Moisture Published Successfully!");
    }//for ohstem pubblish
    Serial.println("Task Soil and Relay");
    Serial.println(analogRead(A0));

    Serial.println("IP: ");
    Serial.println(WiFi.softAPIP());
    
    if(analogRead(A0) > 500){
      digitalWrite(D3, LOW);
    }
    if(analogRead(A0) < 50){
      digitalWrite(D3, HIGH);
    }
    delay(8000);
  }
}

void TaskLightAndLED(void *pvParameters) {
  while(1) {
    Serial.println("Task Light and LED");
    Serial.println(analogRead(A1));
     float lightLevel = analogRead(A1);    
    if (lightFeed.publish(lightLevel)) {
      Serial.println("Light Level Published Successfully!");
    }//for ohstem pubblish
    if(analogRead(A1) < 1000){
      pixels3.setPixelColor(0, pixels3.Color(255,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,255));
      pixels3.setPixelColor(2, pixels3.Color(255,0,255));
      pixels3.setPixelColor(3, pixels3.Color(0,255,0));
      pixels3.show();
    } 
    else {
      pixels3.clear();
      pixels3.show();
    }
    delay(10000);
  }
}



void sendImage(String imgPath ) {
  File imageFile = SPIFFS.open(imgPath, "r");
  if (!imageFile || imageFile.isDirectory()) {
    Serial.println("Failed to open image file");
    return;
  }

  WiFiClient wifiClient;
  HTTPClient http;

  http.begin(wifiClient, "http://127.0.0.1:8000/process");  // Replace with your server URL
  http.addHeader("Content-Type", "image/png");  // Change to image/jpeg if needed

  int httpResponseCode = http.sendRequest("POST", &imageFile, imageFile.size());
  imageFile.close();  // Close before deleting

  if (httpResponseCode > 0) {
    Serial.printf("HTTP %d\nResponse: %s\n", httpResponseCode, http.getString().c_str());

    // ✅ Delete only if the request was successful
    if (SPIFFS.remove(imgPath)) {
      Serial.println("Image deleted from SPIFFS.");
    } else {
      Serial.println("Failed to delete image.");
    }

  } else {
    Serial.printf("HTTP request failed: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}
