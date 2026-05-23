/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: k_suwatchai@hotmail.com
 *
 * Github: https://github.com/mobizt/Firebase-ESP8266
 *
 * Copyright (c) 2023 mobizt
 *
 */

/** This example will show how to authenticate using
 * the legacy token or database secret with the new APIs (using config and auth data).
 */

#include <Wire.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

#include <Arduino.h>
#include <FirebaseESP32.h>

// Display OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Sensor de Temperatura e Umidade
#include <Adafruit_SHT31.h>

// Sensor de Gestos
#include <Adafruit_APDS9960.h>

// Acelerômetro
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>


// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "uaifai-brum"
#define WIFI_PASSWORD "bemvindoaocesar"

/* 2. If work with RTDB, define the RTDB URL and database secret */
#define DATABASE_URL "https://integracaoplaca-f611e-default-rtdb.firebaseio.com/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define DATABASE_SECRET "aRmz3uAad6WOEYlxiWKWPAE1POAg1KqVWoUHRkLc"

// ---- Bar Graph (progress bar de temperatura) ----
#define BAR_1  13   // 1º LED (mais baixo)
#define BAR_2   4   // 2º LED
#define BAR_3  16   // 3º LED
#define BAR_4  17   // 4º LED (mais alto)

// ---- LED RGB ----
#define LED_R  19
#define LED_G  23
#define LED_B  18

// ---- Botão ----
#define BTN    27

// ---- Limites Temperatura ----
#define TEMP_IDEAL_MAX   1
#define TEMP_ATENCAO_MAX 1
#define TEMP_IDEAL_MIN  1

// ---- Limites Umidade ----
#define UMID_IDEAL_MIN   40.0
#define UMID_IDEAL_MAX   60.0
#define UMID_ATENCAO_MAX 70.0
#define UMID_ATENCAO_MIN 30.0

// ---- Limites Vibração (acelerômetro) ----
#define VIBRACAO_THRESHOLD  2.5  // m/s² acima da gravidade

// ---- Intervalo de envio ao Firebase ----
#define INTERVALO_FIREBASE  3000

// =============================================

Adafruit_SSD1306  display(128, 64, &Wire, -1);
Adafruit_SHT31    sht31;
Adafruit_APDS9960 apds;
Adafruit_MPU6050  mpu;

/*FirebaseData   fbData;
FirebaseAuth   auth;
FirebaseConfig config;*/

int           contPessoas  = 0;
unsigned long ultimoEnvio  = 0;
bool          vibracaoAtiva = false;

/* 3. Define the Firebase Data object */
FirebaseData fbdo;

/* 4, Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* Define the FirebaseConfig data for config data */
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;

String statusTemp(float t) {
  if (t > TEMP_ATENCAO_MAX || t < TEMP_IDEAL_MIN) return "CRITICO";
  if (t > TEMP_IDEAL_MAX)                          return "ATENCAO";
  return "IDEAL";
}

String statusUmid(float u) {
  if (u > UMID_ATENCAO_MAX || u < UMID_ATENCAO_MIN) return "CRITICO";
  if (u > UMID_IDEAL_MAX   || u < UMID_IDEAL_MIN)   return "ATENCAO";
  return "IDEAL";
}

bool temAlerta(String sT, String sU) {
  return (sT != "IDEAL" || sU != "IDEAL");
}

void setLED(int r, int g, int b) {
  // LED RGB é anodo comum: LOW = aceso, HIGH = apagado
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

void atualizarLED(String sT, String sU) {
  if (sT == "CRITICO" || sU == "CRITICO") {
    setLED(1, 0, 0);  // Vermelho — crítico
  } else if (sT == "ATENCAO" || sU == "ATENCAO") {
    setLED(1, 1, 0);  // Amarelo — atenção
  } else {
    setLED(0, 1, 0);  // Verde — ideal
  }
}



void setup() {
  Serial.begin(115200);

  // GPIOs de saída
  pinMode(BAR_1, OUTPUT); pinMode(BAR_2, OUTPUT);
  pinMode(BAR_3, OUTPUT); pinMode(BAR_4, OUTPUT);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);

  // I2C
  Wire.begin(21, 22);

   // Display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro: Display OLED não encontrado!");
    while (true);
  }
}

void loop() {
  // 1. Lê temperatura e umidade
  float temp = sht31.readTemperature();
  float umid = sht31.readHumidity();

  if (isnan(temp) || isnan(umid)) {
    Serial.println("Erro ao ler SHT31!");
    setLED(1, 0, 1); // Roxo = erro de sensor
    delay(1000);
    return;
  }

  String sT = statusTemp(temp);
  String sU = statusUmid(umid);

  // 2. Verifica gestos (entrada/saída de pessoas)
 // verificarGestos();

  // 3. Verifica vibração
//  verificarVibracao();

  // 4. Atualiza alertas visuais
  atualizarLED(sT, sU);
  //atualizarBarGraph(temp);

  // 5. Atualiza display OLED
  /*atualizarDisplay(temp, umid, contPessoas,
                   sT, sU, vibracaoAtiva);*/

  // 6. Log no Monitor Serial
  /*Serial.println("================================");
  Serial.print("Temp: ");    Serial.print(temp, 1);
  Serial.print("°C [");      Serial.print(sT);
  Serial.print("] | Umid: "); Serial.print(umid, 1);
  Serial.print("% [");       Serial.print(sU); Serial.println("]");
  Serial.print("Pessoas: "); Serial.println(contPessoas);
  Serial.print("Vibracao: ");Serial.println(vibracaoAtiva ? "SIM" : "Não");*/

  // 7. Envia ao Firebase a cada 3 segundos
  if (millis() - ultimoEnvio >= INTERVALO_FIREBASE) {
    ultimoEnvio = millis();
   // enviarFirebase(temp, umid, sT, sU);
  }

  delay(500);
}

