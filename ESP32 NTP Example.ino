#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ThingSpeak.h>

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     -10800
#define UTC_OFFSET_DST 0

#define myChannelNumber 
#define myWriteAPIKey  
#define myReadAPIKey  

const int RELE = 19; // Pino do relé
const int LED1 = 23;   // Pino do LED

unsigned long ultimaAtivacao = 0; // Tempo desde a ultima ativacao
const unsigned long intervalo = 10000; // Intervalo de ativação em ms (10000 apenas para fins de teste altere para 86400000 -> um dia)
bool ativado = false; // Estado da ativação
unsigned long inicioAtivacao = 0; // Tempo de início ativação

WiFiClient client;

void spinner() {
  static int8_t counter = 0;
  const char* glyphs = "\xa1\xa5\xdb";
  LCD.setCursor(15, 0);
  LCD.print(glyphs[counter++]);
  if (counter == strlen(glyphs)) {
    counter = 0;
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    LCD.setCursor(0, 1);
    LCD.println("Connection Err");
    return;
  }

  LCD.setCursor(8, 0);
  LCD.println(&timeinfo, "%H:%M:%S");

  LCD.setCursor(0, 1);
  LCD.println(&timeinfo, "%d/%m/%Y   %Z");

  // Verifica se a hora registrada é maior que 0 segundos e se já passou o intervalo da ativação
  /*O trecho abaixo faz a irrigação ativar sempre que a hora for maior que zero segundos apenas para testes.
  Para que funcione corretamente, inclua [timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 &&] no lugar de [timeinfo.tm_sec > 0]*/
  if (timeinfo.tm_sec > 0 && millis() - ultimaAtivacao >= intervalo && !ativado) {
    ativarSistema();
    gravarAtivacao(timeinfo);
  }
}

void ativarSistema() {
  ativado = true;
  inicioAtivacao = millis();
  digitalWrite(RELE, HIGH); // Ativa o relé
  digitalWrite(LED1, HIGH);   // Acende o LED
}

void checarSistema() {
  if (ativado && millis() - inicioAtivacao >= 10000) { // 10 Segundos apenas para teste -> tempo de irrigação
    ativado = false;
    digitalWrite(RELE, LOW);  // Desativa o relé
    digitalWrite(LED1, LOW);    // Apaga o LED
    ultimaAtivacao = millis(); 
  }
}



void gravarAtivacao(struct tm timeinfo) {
  String horaAtivacao = String(formato(timeinfo.tm_mday)) + "/" +                        
                          String(formato(timeinfo.tm_mon + 1)) + "/" + 
                          String(timeinfo.tm_year + 1900) + " às " +
                          String(formato(timeinfo.tm_hour)) + ":" + 
                          String(formato(timeinfo.tm_min)) + ":" + 
                          String(formato(timeinfo.tm_sec));
  
  ThingSpeak.setField(1, horaAtivacao);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

// Função para formatar os valores com menos de dois digitos
String formato(int value) {
  if (value < 10) {
    return "0" + String(value);
  } else {
    return String(value);
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(RELE, OUTPUT);
  pinMode(LED1, OUTPUT);

  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Conectando wifi");

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    spinner();
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.println("Online");
  LCD.setCursor(0, 1);
  LCD.println("Atualizando hora");

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  ultimaAtivacao = millis(); 

  // Configura ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  printLocalTime();
  checarSistema();
  delay(250);
}
