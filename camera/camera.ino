#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

const char* ssid = "project";
const char* password = "projectp";

String BOTtoken = "8422357507:AAEOcCOgRsccsU6GnUzFFjoFFqEZK4eskQM";
String chatId = "6124419234";

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

long lastTimeBotRan;
int botRequestDelay = 1000;

// ESP32-CAM AI Thinker Pins
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void setup() {

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  clientTCP.setInsecure();

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

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_camera_init(&config);

  bot.sendMessage(chatId,"ESP32 Camera Ready","");
}

void sendPhoto() {

  camera_fb_t * fb = esp_camera_fb_get();

  if(!fb){
    Serial.println("Camera capture failed");
    return;
  }

  if (clientTCP.connect("api.telegram.org", 443)) {

    String head = "--ESP32CAM\r\nContent-Disposition: form-data; name=\"chat_id\";\r\n\r\n" + chatId +
                  "\r\n--ESP32CAM\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"frame.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--ESP32CAM--\r\n";

    uint32_t totalLen = fb->len + head.length() + tail.length();

    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: api.telegram.org");
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=ESP32CAM");
    clientTCP.println();
    clientTCP.print(head);
    clientTCP.write(fb->buf, fb->len);
    clientTCP.print(tail);

    esp_camera_fb_return(fb);
  }

  clientTCP.stop();
}

void recordVideo10sec(){

  bot.sendMessage(chatId,"Recording 10 sec video...","");

  unsigned long startTime = millis();

  while(millis() - startTime < 10000){

    sendPhoto();

    delay(500); // 2 frames per second
  }

  bot.sendMessage(chatId,"Video finished","");
}

void handleNewMessages(int numNewMessages){

  for (int i = 0; i < numNewMessages; i++){

    String text = bot.messages[i].text;

    if(text == "/video"){
      recordVideo10sec();
    }
  }
}

void loop(){

  if (millis() > lastTimeBotRan + botRequestDelay){

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages){
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }
}