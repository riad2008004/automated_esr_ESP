#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// WiFi credentials
const char *ssid = "Tanvir";
const char *password = "#MTHT010isback";

WebServer server(80);
TFT_eSPI tft = TFT_eSPI();

// Data
float esrValues[10];
float temperature = 28.5;
float humidity = 65.0;

int testTime = 20; //  FIX
int menuSelection = 0;

// Button pins
const int btnPins[5] = {12, 13, 14, 27, 26};
volatile int menuIndex = 0;

TaskHandle_t displayTaskHandle;

// ===== HTML PAGE =====
String htmlPage()
{
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="refresh" content="2">
<meta charset="UTF-8">
  <title>Automatic ESR Machine</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      text-align: center;
      background: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
      color: #00ffd5;
    }

    /* Header */
    .header h1 {
      font-size: 80px;
      margin: 20px 0;
      text-shadow: 0 0 20px #00ffd5;
    }

    /* 🔥 Sensor Panel (Improved) */
    .sensor-panel {
      display: flex;
      justify-content: center;
      gap: 40px;
      margin: 20px;
    }

    .sensor {
      font-size: 32px;
      padding: 20px 30px;
      border-radius: 15px;

      background: rgba(0, 0, 0, 0.4);
      border: 2px solid #00ffd5;

      box-shadow: 0 0 15px rgba(0,255,213,0.6);
    }

    /* ESR Grid */
    .container {
      display: grid;
      grid-template-columns: repeat(5, 1fr);
      gap: 20px;
      padding: 30px;
      max-width: 1000px;
      margin: auto;
    }

    .esr {
      font-size: 26px;
      padding: 20px;
      border-radius: 12px;
      background: rgba(0,0,0,0.3);
      border: 1px solid rgba(0,255,213,0.5);
      box-shadow: 0 0 10px rgba(0,255,213,0.4);
    }

    /* Range */
    .range {
      font-size: 22px;
      margin-top: 10px;
      text-shadow: 0 0 10px #00ffd5;
    }

    /* Footer */
    .footer {
      margin-top: 10px;
      font-size: 18px;
      color: #aaa;
    }

    .credit {
      font-size: 16px;
      color: #888;
      margin-bottom: 20px;
    }

  </style>

</head>
<body>

  <div class="header">
    <h1>Automatic ESR Machine</h1>
  </div>

<div class="sensor-panel">
  <div class="sensor"> TEMP: )rawliteral";

  html += String(temperature) + " °C</div>";
  html += "<div class='sensor'> HUM: " + String(humidity) + " %</div></div>";

  html += "<div class='container'>";

  for (int i = 0; i < 10; i++)
  {
    html += "<div class='esr'>ESR ";
    html += (i + 1);
    html += ": ";
    html += String(esrValues[i]);
    html += " mm/h</div>";
  }

  html += R"rawliteral(
</div>

<p>Normal ESR Range: 1 - 10 mm/h</p>
<p>Status: Running | ESP32 Web Interface</p>
<p style="color:gray;">Developed by: Ariful Islam Riad (2008004) and Nurul Sajjad (2008006)</p>

</body>
</html>
)rawliteral";

  return html;
}

// ===== HANDLER =====
void handleRoot()
{
  server.send(200, "text/html", htmlPage());
}

// ===== TASKS =====
void TaskWebServer(void *pvParameters)
{
  while (1)
  {
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskDisplay(void *pvParameters)
{
  tft.fillScreen(TFT_BLACK);

  // ===== HEADER =====
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(40, 5);
  tft.println("Automatic ESR");

  tft.setCursor(73, 25);
  tft.println("Machine");

  // ===== SENSOR PANEL =====
  tft.drawRoundRect(5, 50, 226, 35, 8, TFT_GREEN);

  // ===== ESR GRID STATIC =====
  int xStart = 1;
  int yStart = 95;
  int boxW = 44;
  int boxH = 45;

  for (int i = 0; i < 10; i++)
  {
    int col = i % 5;
    int row = i / 5;

    int x = xStart + col * (boxW + 3);
    int y = yStart + row * (boxH + 3);

    tft.drawRoundRect(x, y, boxW, boxH, 5, TFT_CYAN);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(x + 5, y + 5);
    tft.print("E");
    tft.print(i + 1);
  }

  while (1)
  {
    // ===== TEMP =====
    tft.fillRect(10, 60, 90, 20, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(10, 60);
    tft.print("T:");
    tft.print(temperature, 1);

    // ===== HUM =====
    tft.fillRect(120, 60, 90, 20, TFT_BLACK);
    tft.setTextColor(TFT_BLUE);
    tft.setCursor(120, 60);
    tft.print("H:");
    tft.print(humidity, 1);

    // ===== ESR VALUES =====
    for (int i = 0; i < 10; i++)
    {
      int col = i % 5;
      int row = i / 5;

      int x = xStart + col * (boxW + 3);
      int y = yStart + row * (boxH + 5);

      // Clear value area
      tft.fillRect(x + 2, y + 25, 40, 15, TFT_BLACK);

      // Color warning
      if (esrValues[i] > 10)
        tft.setTextColor(TFT_RED);
      else
        tft.setTextColor(TFT_WHITE);

      tft.setCursor(x + 5, y + 25);
      tft.print(esrValues[i], 1);
    }

    // ===== RANGE =====
    tft.fillRect(0, 200, 240, 15, TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(20, 200);
    tft.print("Range: 1-10 mm/h");

    // ===== IP =====
    tft.fillRect(0, 220, 240, 20, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(20, 220);
    tft.print("IP:");
    tft.println(WiFi.localIP());

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TaskButtons(void *pvParameters)
{
  int testTimes[3] = {20, 30, 40};

  while (1)
  {
    // ===== ENTER MENU (wait for press + release) =====
    if (digitalRead(13) == HIGH)
    {
      while (digitalRead(13) == HIGH)
        ; // wait release

      vTaskSuspend(displayTaskHandle); // pause display
      tft.fillScreen(TFT_BLACK);

      bool inMenu = true;

      while (inMenu)
      {
        tft.fillScreen(TFT_BLACK);

        // ===== TITLE =====
        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(2);
        tft.setCursor(80, 10);
        tft.println("MENU");

        // ===== UP (27) =====
        if (digitalRead(27) == HIGH)
        {
          while (digitalRead(27) == HIGH)
            ; // wait release

          menuSelection--;
          if (menuSelection < 0)
            menuSelection = 2;
        }

        // ===== DOWN (12) =====
        if (digitalRead(12) == HIGH)
        {
          while (digitalRead(12) == HIGH)
            ;

          menuSelection++;
          if (menuSelection > 2)
            menuSelection = 0;
        }

        // ===== SELECT (26) =====
        if (digitalRead(26) == HIGH)
        {
          while (digitalRead(26) == HIGH)
            ;

          testTime = testTimes[menuSelection];
        }

        // ===== EXIT MENU (13) =====
        if (digitalRead(13) == HIGH)
        {
          inMenu = false;
          vTaskDelay(200 / portTICK_PERIOD_MS); // debounce
          break;
        }

        // ===== DISPLAY OPTIONS =====
        for (int i = 0; i < 3; i++)
        {
          int y = 60 + i * 40;

          if (i == menuSelection)
          {
            // highlight box
            tft.fillRoundRect(10, y - 5, 220, 30, 5, TFT_YELLOW);
            tft.setTextColor(TFT_BLACK);
          }
          else
          {
            tft.setTextColor(TFT_WHITE);
          }

          tft.setCursor(20, y);
          tft.print("Test Time: ");
          tft.print(testTimes[i]);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
      }

      // ===== EXIT CLEANUP =====
      vTaskResume(displayTaskHandle);
      tft.fillScreen(TFT_BLACK);

      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void TaskESR(void *pvParameters)
{
  String data = "";

  while (1)
  {
    while (Serial.available())
    {
      char c = Serial.read();

      if (c == '\n') // full line received
      {
        // ===== PARSE DATA =====

        // TEMP
        int tIndex = data.indexOf("TEMP=");
        if (tIndex != -1)
        {
          temperature = data.substring(tIndex + 5).toFloat();
        }

        // HUM
        int hIndex = data.indexOf("HUM=");
        if (hIndex != -1)
        {
          humidity = data.substring(hIndex + 4).toFloat();
        }

        // ESR values
        for (int i = 0; i < 10; i++)
        {
          String key = "ESR" + String(i + 1) + "=";
          int index = data.indexOf(key);

          if (index != -1)
          {
            esrValues[i] = data.substring(index + key.length()).toFloat();
          }
        }

        data = ""; // clear buffer
      }
      else
      {
        data += c; // build string
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ===== SETUP =====
void setup()
{
  Serial.begin(9600);

  for (int i = 0; i < 5; i++)
  {
    pinMode(btnPins[i], INPUT);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting...");
  }

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();

  xTaskCreatePinnedToCore(TaskWebServer, "Web", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskButtons, "Buttons", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskESR, "ESR", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskDisplay, "Display", 4096, NULL, 1, &displayTaskHandle, 1);
}

void loop() {}