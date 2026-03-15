#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define BUFFER_SIZE 100

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;

int32_t heartRate;
int8_t validHeartRate;

byte bufferIndex = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("MAX30102 Initializing...");

  Wire.begin(4,5);   // SDA=GPIO4  SCL=GPIO5

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("Sensor not detected");
    while(1);
  }

  Serial.println("Sensor detected");

  particleSensor.setup(
      60,   // LED brightness
      4,    // sample average
      2,    // LED mode (Red + IR)
      100,  // sample rate
      411,  // pulse width
      4096  // ADC range
  );
}

void loop()
{
  particleSensor.check();

  while (particleSensor.available())
  {
    redBuffer[bufferIndex] = particleSensor.getRed();
    irBuffer[bufferIndex]  = particleSensor.getIR();

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
        Serial.println("No finger detected");
      }
      else
      {
        if(validHeartRate)
        {
          Serial.print("BPM: ");
          Serial.print(heartRate);
        }

        if(validSPO2)
        {
          Serial.print("   SpO2: ");
          Serial.print(spo2);
          Serial.println("%");
        }
      }

      bufferIndex = 0;
    }
  }
}