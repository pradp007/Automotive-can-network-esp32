#include <SPI.h>
#include <mcp_can.h>
#include <FastLED.h>

#define CAN_CS 5

#define LED_PIN 4
#define NUM_LEDS 64

#define BTN1 25
#define BTN2 27

MCP_CAN CAN(CAN_CS);
CRGB leds[NUM_LEDS];

volatile int speed = 0;
volatile int rpm = 0;
volatile float battery = 12.6;

volatile int mode = 0;
volatile bool headlightOn = false;

volatile unsigned long lastMessageTime = 0;

// -------------------------
// CAN RECEIVE TASK
// -------------------------
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

      lastMessageTime = millis();

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
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// -------------------------
// BUTTON TASK
// -------------------------
void buttonTask(void *pvParameters)
{
  bool lastBtn1 = HIGH;
  bool lastBtn2 = HIGH;

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  while (1)
  {
    bool btn1 = digitalRead(BTN1);
    bool btn2 = digitalRead(BTN2);

    // Mode Cycle
    if (lastBtn1 == HIGH && btn1 == LOW)
    {
      mode++;

      if (mode > 4)
        mode = 0;

      Serial.print("MODE = ");
      Serial.println(mode);

      vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    // Headlight Toggle
    if (lastBtn2 == HIGH && btn2 == LOW)
    {
      headlightOn = !headlightOn;

      unsigned char txBuf[1];

      if (headlightOn)
      {
        txBuf[0] = 1;
        CAN.sendMsgBuf(0x200, 0, 1, txBuf);

        Serial.println("HEADLIGHT ON");
      }
      else
      {
        txBuf[0] = 0;
        CAN.sendMsgBuf(0x200, 0, 1, txBuf);

        Serial.println("HEADLIGHT OFF");
      }

      vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    lastBtn1 = btn1;
    lastBtn2 = btn2;

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// -------------------------
// RGB TASK
// -------------------------
void rgbTask(void *pvParameters)
{
  while (1)
  {
    if (millis() - lastMessageTime > 2000)
    {
      static bool flash = false;

      flash = !flash;

      fill_solid(
          leds,
          NUM_LEDS,
          flash ? CRGB::Red : CRGB::Black);

      FastLED.show();

      vTaskDelay(250 / portTICK_PERIOD_MS);

      continue;
    }

    if (headlightOn)
    {
      fill_solid(leds, NUM_LEDS, CRGB::White);
    }
    else
    {
      switch (mode)
      {
        case 0:

          if (battery < 11.5)
          {
            fill_solid(leds, NUM_LEDS, CRGB::Blue);
          }
          else if (speed > 100)
          {
            fill_solid(leds, NUM_LEDS, CRGB::Red);
          }
          else if (speed > 80)
          {
            fill_solid(leds, NUM_LEDS, CRGB::Yellow);
          }
          else
          {
            fill_solid(leds, NUM_LEDS, CRGB::Green);
          }

          break;

        case 1:
          fill_solid(leds, NUM_LEDS, CRGB::Orange);
          break;

        case 2:
          fill_solid(leds, NUM_LEDS, CRGB::Purple);
          break;

        case 3:
          fill_solid(leds, NUM_LEDS, CRGB::Blue);
          break;

        case 4:
          fill_solid(leds, NUM_LEDS, CRGB::Cyan);
          break;
      }
    }

    FastLED.show();

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

  Serial.println("Diagnostic ECU RTOS Ready");

  CAN.setMode(MCP_NORMAL);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(20);

  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();

  delay(1000);

  FastLED.clear();
  FastLED.show();

  lastMessageTime = millis();

  xTaskCreatePinnedToCore(
      canTask,
      "CAN Task",
      4096,
      NULL,
      2,
      NULL,
      0);

  xTaskCreatePinnedToCore(
      buttonTask,
      "Button Task",
      2048,
      NULL,
      2,
      NULL,
      1);

  xTaskCreatePinnedToCore(
      rgbTask,
      "RGB Task",
      4096,
      NULL,
      1,
      NULL,
      1);
}

void loop()
{
  // RTOS handles everything
}
