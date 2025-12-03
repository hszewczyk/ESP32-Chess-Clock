#include <Arduino.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WebServer.h>

/*
  CONFIGURATION
*/

constexpr char WIFI_SSID[] = "SSID"; // SSID of the Acces Point
constexpr char WIFI_PASSWORD[] = "PASSWORD"; // Password of the Access Point

constexpr int PIN_BUTTON_P1 = 19; // GPIO of the first button
constexpr int PIN_BUTTON_P2 = 18; // GPIO of the seconds

constexpr int LCD_COLUMNS = 16; // Number of columns of the LCD
constexpr int LCD_ROWS = 2; // Number of rows of the LCD

/*
  GLOBAL STATE
*/

enum Player { PLAYER1, PLAYER2 };
Player currentTurn = PLAYER1;

bool clockStarted = false;

int baseMinutes = -1;
int incrementSec = -1;

int timeP1 = 0;
int timeP2 = 0;

unsigned long lastTickMs = 0;

LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);
WebServer server(80);

/*
  WEB PAGE & HANDLERS
*/

String buildHtmlPage() {
  String page = "<html><body><h2>Ustaw dane:</h2>";
  page += "<form action='/set' method='GET'>";
  page += "Time (min): <input type='number' name='a'><br><br>";
  page += "Increment (s): <input type='number' name='b'><br><br>";
  page += "<input type='submit' value='Zapisz'>";
  page += "</form></body></html>";
  return page;
}

void handleRoot() {
  server.send(200, "text/html", buildHtmlPage());
}

void handleSet() {
  if (server.hasArg("a")) baseMinutes = server.arg("a").toInt();
  if (server.hasArg("b")) incrementSec = server.arg("b").toInt();
  
  clockStarted = false;

  timeP1 = baseMinutes * 60;
  timeP2 = baseMinutes * 60;

  server.send(200, "text/html",
              "<html><body><h3>Gotowe!</h3>"
              "<a href='/'>Cofnij</a></body></html>");
}

/*
  HELPER FUNCTIONS
*/

void printMMSS(int col, int row, int seconds) {
  lcd.setCursor(col, row);
  lcd.printf("%02d:%02d", seconds / 60, seconds % 60);
}

void handleTurnSwitch(Player player) {
  if (player == PLAYER1) {
    currentTurn = PLAYER2;
    if (incrementSec > 0) timeP1 += incrementSec;
  } else {
    currentTurn = PLAYER1;
    if (incrementSec > 0) timeP2 += incrementSec;
  }
}

/*
  SETUP
*/

void setup() {
  Serial.begin(115200);

  // WiFi Access Point
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();

  // Hardware
  pinMode(PIN_BUTTON_P1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_P2, INPUT_PULLUP);

  // LCD initialization
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  // Initial display
  lcd.setCursor(0, 0); lcd.print("00:00");
  lcd.setCursor(11, 0); lcd.print("00:00");
  lcd.setCursor(7, 0); lcd.print("vs");
}

/*
  LOOP
*/

void loop() {
  int btnP1 = digitalRead(PIN_BUTTON_P1);
  int btnP2 = digitalRead(PIN_BUTTON_P2);

  if (btnP1 == LOW && btnP2 && currentTurn == PLAYER1) handleTurnSwitch(PLAYER1);
  if (btnP2 == LOW && btnP1 && currentTurn == PLAYER2) handleTurnSwitch(PLAYER2);

  // Start clock on first press
  if (btnP1 == LOW || btnP2 == LOW) clockStarted = true;

  // Timer countdown
  if (clockStarted) {
    unsigned long now = millis();
    if (now - lastTickMs >= 1000) {
      lastTickMs = now;

      if (currentTurn == PLAYER1 && timeP1 > 0) timeP1--;
      else if (currentTurn == PLAYER2 && timeP2 > 0) timeP2--;
    }

    // Update LCD time displays
    printMMSS(0, 0, timeP1);
    printMMSS(11, 0, timeP2);
  }

  server.handleClient();
}
