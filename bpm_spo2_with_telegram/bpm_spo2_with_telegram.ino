#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

const char* ssid = "project";
const char* password = "projectp";

String BOTtoken = "8422357507:AAEOcCOgRsccsU6GnUzFFjoFFqEZK4eskQM";
String chatId = "6124419234";

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

MAX30105 particleSensor;

#define BUFFER_SIZE 100

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;

int32_t heartRate;
int8_t validHeartRate;

byte bufferIndex = 0;

unsigned long lastTelegram = 0;

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected");

  client.setInsecure();

  Wire.begin(4,5);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30102 not found");
    while (1);
  }

  Serial.println("Sensor Ready");

  particleSensor.setup(
      60,
      4,
      2,
      100,
      411,
      4096
  );
}

void loop()
{
  particleSensor.check();

  while (particleSensor.available())
  {
    redBuffer[bufferIndex] = particleSensor.getRed();
    irBuffer[bufferIndex] = particleSensor.getIR();

    particleSensor.nextSample();

    bufferIndex++;

    if(bufferIndex == BUFFER_SIZE)
    {
      maxim_heart_rate_and_oxygen_saturation(
          irBuffer,
          BUFFER_SIZE,
          redBuffer,
          &spo2,
          &validSPO2,
          &heartRate,
          &validHeartRate);

      if(irBuffer[BUFFER_SIZE-1] < 5000)
      {
        Serial.println("No finger");

        if(millis() - lastTelegram > 15000)
        {
          bot.sendMessage(chatId, "No finger detected", "");
          lastTelegram = millis();
        }
      }
      else
      {
        if(validHeartRate && validSPO2)
        {
          Serial.print("BPM: ");
          Serial.print(heartRate);
          Serial.print(" SpO2: ");
          Serial.println(spo2);

          if(millis() - lastTelegram > 15000)
          {
            String message =
            "Finger Detected\n"
            "BPM: " + String(heartRate) + "\n"
            "SpO2: " + String(spo2) + "%";

            bot.sendMessage(chatId, message, "");

            lastTelegram = millis();
          }
        }
      }

      bufferIndex = 0;
    }
  }
}