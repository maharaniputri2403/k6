#include <WiFi.h>
#include <WebServer.h>
#include <ThingSpeak.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <elapsedMillis.h>
#include <LittleFS.h>

// ================= SENSOR & RELAY SETUP =================
#define DHTPIN 4
#define DHTTYPE DHT21

#define RELAY_KIPAS 26
#define RELAY_POMPA 25

const int soilPin = 34;

// ================= WIFI & THINGSPEAK =================
const char* ssid = "princess";
const char* password = "12345679";

// ====== THINGSPEAK BARU ======
const char* writeAPIKey = "L0819CGH66SHDY86";
const unsigned long channelID = 3430723;
// =============================

WiFiClient client;
WebServer server(80);

// ================= INTERVAL WAKTU =================
unsigned long thingSpeakInterval = 15000;
unsigned long sensorInterval = 500;
unsigned long displayInterval = 1000;

elapsedMillis thingSpeakMillis;
elapsedMillis sensorMillis;
elapsedMillis displayMillis;

// ================= OBJEK SENSOR =================
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ================= VARIABEL GLOBAL =================
int soilValue;
int soilPercentage;
float temperature;
float humidity;

// Override manual flag
bool manualModeKipas = false;
bool manualModePompa = false;

// ================= HANDLER WEB SERVER =================
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // Disesuaikan dengan ACTIVE HIGH: HIGH berarti ON, LOW berarti OFF
  String statusJSON = "{\"kipas\":\"" +
                      String(digitalRead(RELAY_KIPAS) == HIGH ? "ON" : "OFF") +
                      "\",";
  statusJSON += "\"pompa\":\"" +
                String(digitalRead(RELAY_POMPA) == HIGH ? "ON" : "OFF") +
                "\"}";

  server.send(200, "application/json", statusJSON);
}

void handleKipasOn() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  manualModeKipas = true;
  digitalWrite(RELAY_KIPAS, HIGH); // ACTIVE HIGH: HIGH = NYALA
  server.send(200, "text/plain", "ON");
}

void handleKipasOff() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  manualModeKipas = true;
  digitalWrite(RELAY_KIPAS, LOW);  // ACTIVE HIGH: LOW = MATI
  server.send(200, "text/plain", "OFF");
}

void handlePompaOn() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  manualModePompa = true;
  digitalWrite(RELAY_POMPA, HIGH); // ACTIVE HIGH: HIGH = NYALA
  server.send(200, "text/plain", "ON");
}

void handlePompaOff() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  manualModePompa = true;
  digitalWrite(RELAY_POMPA, LOW);  // ACTIVE HIGH: LOW = MATI
  server.send(200, "text/plain", "OFF");
}

void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(3, 0);
  lcd.print("Selamat Datang!");

  lcd.setCursor(0, 1);
  lcd.print("Agroteknologi IoT");

  lcd.setCursor(3, 3);
  lcd.print("-- UG MURO --");

  delay(3000);
  lcd.clear();

  // Sensor
  pinMode(soilPin, INPUT);
  dht.begin();

  // Relay
  pinMode(RELAY_KIPAS, OUTPUT);
  pinMode(RELAY_POMPA, OUTPUT);

  // REVISI ACTIVE HIGH: Di awal sistem, pastikan semua relay MATI (LOW)
  digitalWrite(RELAY_KIPAS, LOW);
  digitalWrite(RELAY_POMPA, LOW);

  // WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address ESP32: ");
  Serial.println(WiFi.localIP());

  // Routing HTTP API
  server.on("/", handleRoot);
  server.on("/kipas/on", handleKipasOn);
  server.on("/kipas/off", handleKipasOff);
  server.on("/pompa/on", handlePompaOn);
  server.on("/pompa/off", handlePompaOff);

  server.begin();

  ThingSpeak.begin(client);

  lcd.setCursor(5, 0);
  lcd.print("Monitoring");

  lcd.setCursor(0, 1);
  lcd.print("Suhu : ");

  lcd.setCursor(0, 2);
  lcd.print("K.Udara: ");

  lcd.setCursor(0, 3);
  lcd.print("K.Tanah: ");
}

void loop() {
  server.handleClient();

  // ==================== BACA SENSOR ====================
  if (sensorMillis >= sensorInterval) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    soilValue = analogRead(soilPin);

    soilPercentage = map(soilValue, 4095, 0, 0, 100);
    soilPercentage = constrain(soilPercentage, 0, 100);

    sensorMillis = 0;
  }

  // ==================== LCD ====================
  if (displayMillis >= displayInterval) {
    lcd.setCursor(9, 1);
    lcd.print("      ");
    lcd.setCursor(9, 1);
    lcd.print(temperature);
    lcd.print(char(223));
    lcd.print("C");

    lcd.setCursor(9, 2);
    lcd.print("      ");
    lcd.setCursor(9, 2);
    lcd.print(humidity);
    lcd.print("%");

    lcd.setCursor(9, 3);
    lcd.print("      ");
    lcd.setCursor(9, 3);
    lcd.print(soilPercentage);
    lcd.print("%");

    displayMillis = 0;
  }

  // ==================== REVISI KONTROL KIPAS ====================
  if (!manualModeKipas) {
    if (temperature <= 45.0) {
      digitalWrite(RELAY_KIPAS, HIGH); 
    }
    else if (temperature >= 70.0) {
      digitalWrite(RELAY_KIPAS, LOW);  
    }
  }

  // ==================== REVISI KONTROL POMPA ====================
  if (!manualModePompa) {
    if (soilPercentage <= 0) {
      digitalWrite(RELAY_POMPA, HIGH); // Kelembapan 0% -> Pompa Menyala (HIGH)
    }
    else if (soilPercentage > 40) {
      digitalWrite(RELAY_POMPA, LOW);  // Kelembapan > 40% -> Pompa Mati (LOW)
    }
  }

  // ==================== THINGSPEAK ====================
  if (thingSpeakMillis >= thingSpeakInterval) {
    manualModeKipas = false;
    manualModePompa = false;

    if (isnan(temperature)) temperature = 0.0;
    if (isnan(humidity)) humidity = 0.0;

    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, soilPercentage);

    int x = ThingSpeak.writeFields(channelID, writeAPIKey);

    if (x == 200) {
      Serial.println("Update ThingSpeak sukses.");
    }
    else {
      Serial.println("Update gagal. HTTP error code: " + String(x));
    }

    thingSpeakMillis = 0;
  }
}
