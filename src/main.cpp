#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LIGHT_GPIO_NUM     4


const char* ssid = "SKY67NSU";
const char* password = "IUnDef45tEWU";

AsyncWebServer server(80);

void handle_jpg_stream(AsyncWebServerRequest *request) {

  AsyncWebServerResponse *response = request->beginChunkedResponse(
    "multipart/x-mixed-replace; boundary=frame",
    [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {

      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        return 0;
      }

      size_t len = 0;

      // Boundary + headers
      len += snprintf((char *)buffer, maxLen,
        "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
        fb->len);

      // Copy image data if space allows
      if (len + fb->len < maxLen) {
        memcpy(buffer + len, fb->buf, fb->len);
        len += fb->len;
        len += snprintf((char *)buffer + len, maxLen - len, "\r\n");
      }

      esp_camera_fb_return(fb);
      return len;
    }
  );

  response->addHeader("Cache-Control", "no-cache");
  response->addHeader("Pragma", "no-cache");

  request->send(response);
}
#define LIGHT_GPIO_NUM 4

void setupRoutes() {

  // Root page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html",
      "<h1>ESP32-CAM</h1>"
      "<p><a href='/stream'>View Stream</a></p>"
      "<p><a href='/light/on'>Light ON</a></p>"
      "<p><a href='/light/off'>Light OFF</a></p>"
      "<p>OTA via Arduino IDE</p>"
    );
  });

  server.on("/light/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LIGHT_GPIO_NUM, HIGH);
    request->send(200, "text/plain", "Light ON");
  });

  server.on("/light/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LIGHT_GPIO_NUM, LOW);
    request->send(200, "text/plain", "Light OFF");
  });
}


void setupOTA() {

  ArduinoOTA.setHostname("esp32cam");

  ArduinoOTA
    .onStart([]() {
      Serial.println("OTA Start");
    })
    .onEnd([]() {
      Serial.println("\nOTA End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
    });

  ArduinoOTA.begin();

  Serial.println("OTA Ready");
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-CAM Booting...");

  pinMode(LIGHT_GPIO_NUM, OUTPUT);
  digitalWrite(LIGHT_GPIO_NUM, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 15;
  config.fb_count = 3;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  IPAddress local_IP(192, 168, 1, 184);     // choose an unused IP
  IPAddress gateway(192, 168, 1, 1);        // your router
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  Serial.println("Static IP failed to configure");
}

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  WiFi.setSleep(false);
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  if (!MDNS.begin("esp32cam")) {
    Serial.println("Error starting mDNS");
  } else {
    Serial.println("mDNS started: http://esp32cam.local");
  }

  // Routes
  //setupRoutes();
  //server.on("/stream", HTTP_GET, handle_jpg_stream);

  setupOTA();

  server.begin();
  Serial.println("Server Ready");
}

void loop() {
  ArduinoOTA.handle();
}