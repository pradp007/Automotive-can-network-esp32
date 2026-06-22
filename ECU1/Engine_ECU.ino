#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS 5
#define POT_PIN 34

MCP_CAN CAN(CAN_CS);

volatile int potValue = 0;
volatile int speed = 0;
volatile int rpm = 800;
volatile int battery = 126;

// --------------------o
// Sensor Task
// --------------------
void SensorTask(void *pvParameters)
{
  while(1)
  {
    potValue = analogRead(POT_PIN);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// --------------------
// Calculation Task
// --------------------
void CalculationTask(void *pvParameters)
{
  while(1)
  {
    speed = map(potValue, 0, 4095, 0, 120);

    if(speed == 0)
      rpm = 800;
    else
      rpm = 800 + (speed * 43);

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --------------------
// CAN Task
// --------------------
void CANTask(void *pvParameters)
{
  while(1)
  {
    byte speedData[1];
    speedData[0] = speed;

    byte rpmData[2];
    rpmData[0] = rpm >> 8;
    rpmData[1] = rpm & 0xFF;

    byte battData[1];
    battData[0] = battery;

    CAN.sendMsgBuf(0x100, 0, 1, speedData);

    CAN.sendMsgBuf(0x101, 0, 2, rpmData);

    CAN.sendMsgBuf(0x102, 0, 1, battData);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// --------------------
// Debug Task
// --------------------
void DebugTask(void *pvParameters)
{
  while(1)
  {
    Serial.print("Speed: ");
    Serial.print(speed);

    Serial.print(" km/h  RPM: ");
    Serial.print(rpm);

    Serial.print("  BAT: ");
    Serial.print(battery / 10.0);

    Serial.println(" V");

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);

  while (CAN.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) != CAN_OK)
  {
    Serial.println("CAN Init Failed");
    delay(1000);
  }

  Serial.println("CAN Init OK");

  CAN.setMode(MCP_NORMAL);

  xTaskCreate(
    SensorTask,
    "SensorTask",
    2048,
    NULL,
    1,
    NULL);

  xTaskCreate(
    CalculationTask,
    "CalculationTask",
    2048,
    NULL,
    1,
    NULL);

  xTaskCreate(
    CANTask,
    "CANTask",
    4096,
    NULL,
    1,
    NULL);

  xTaskCreate(
    DebugTask,
    "DebugTask",
    2048,
    NULL,
    1,
    NULL);
}

void loop()
{
}
