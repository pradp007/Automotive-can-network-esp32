#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define CAN_CS 5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

MCP_CAN CAN(CAN_CS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

volatile int speed = 0;
volatile int rpm = 0;
volatile float battery = 12.6;

volatile bool headlightOn = false;

volatile bool showHeadlightMsg = false;
volatile unsigned long headlightMsgTime = 0;

// ---------------------------
// CAN RECEIVE TASK
// ---------------------------
void canTask(void *pvParameters)
{
  unsigned long rxId;
  unsigned char len;
  unsigned char buf[8];

  while (1)
  {
    if (CAN_MSGAVAIL == CAN.checkReceive())
    {
      CAN.readMsgBuf(&rxId, &len, buf);

      if (rxId == 0x100)
      {
        speed = buf[0];
      }

      else if (rxId == 0x101)
      {
        rpm = (buf[0] << 8) | buf[1];
      }

      else if (rxId == 0x102)
      {
        battery = buf[0] / 10.0;
      }

      else if (rxId == 0x200)
      {
        headlightOn = buf[0];

        showHeadlightMsg = true;
        headlightMsgTime = millis();

        Serial.print("HEADLIGHT: ");

        if (headlightOn)
          Serial.println("ON");
        else
          Serial.println("OFF");
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ---------------------------
// OLED TASK
// ---------------------------
void oledTask(void *pvParameters)
{
  while (1)
  {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    if (showHeadlightMsg)
    {
      display.setTextSize(2);
      display.setCursor(5, 8);

      if (headlightOn)
      {
        display.println("HL ON");
      }
      else
      {
        display.println("HL OFF");
      }

      if (millis() - headlightMsgTime > 1000)
      {
        showHeadlightMsg = false;
      }
    }
    else
    {
      display.setTextSize(1);

      display.setCursor(0, 0);
      display.print("SPD:");
      display.print(speed);
      display.print("km/h");

      display.setCursor(65, 0);
      display.print("BAT:");
      display.print(battery, 1);
      display.print("V");

      display.setCursor(0, 16);
      display.print("RPM:");
      display.print(rpm);
    }

    display.display();

    vTaskDelay(100 / portTICK_PERIOD_MS);
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

  CAN.setMode(MCP_NORMAL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    while (1);
  }

  display.clearDisplay();
  display.display();

  xTaskCreatePinnedToCore(
      canTask,
      "CAN Task",
      4096,
      NULL,
      2,
      NULL,
      0);

  xTaskCreatePinnedToCore(
      oledTask,
      "OLED Task",
      4096,
      NULL,
      1,
      NULL,
      1);
}

void loop()
{
  // FreeRTOS handles everything
}
