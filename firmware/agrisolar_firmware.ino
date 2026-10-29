//PGAD - PROGRAMA DE PÓS-GRADUAÇÃO EM AGRONEGÓCIOS E DESENVOLVIMENTO
//PROJETO - DISCIPLINA INOVAÇÃO, INTELIGÊNCIA ARTIFICIAL E CULTURA ORGANIZACIONAL
//ALUNO: MARCOS VINICIUS ALVES DE OLIVEIRA

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURAÇÕES DE REDE (Wi-Fi e MQTT) ---
const char* ssid = "Markin 5G Celular";
const char* password = "teste123"; // Coloque a senha do seu wifi
const char* mqtt_server = "broker.hivemq.com"; // Broker público de testes

WiFiClient espClient;
PubSubClient client(espClient);

// --- DEFINIÇÃO DOS SENSORES INA219 ---
Adafruit_INA219 ina219_solar(0x40);
Adafruit_INA219 ina219_load(0x41);

// --- DEFINIÇÃO DOS DOIS SENSORES DHT22 ---
#define DHTPIN_DHT22_1 4  
#define DHTPIN_DHT22_2 5  

DHTesp dht22_1;
DHTesp dht22_2;

// --- PINOS DOS LEDS (SIMULANDO CARGAS) ---
const int LED_MOTOR      = 12; // Alta Prioridade
const int LED_VENTILADOR = 13; // Média Prioridade
const int LED_ILUMINACAO = 14; // Baixa Prioridade

// Variáveis de Controle e Temporização
unsigned long ultimaPublicacaoMQTT = 0;
const long intervaloMQTT = 4000; // Publica no MQTT estritamente a cada 4 segundos

unsigned long ultimaTentativaReconexao = 0;
const long intervaloReconexao = 5000; // Tentativa de reconexão não-bloqueante

float previousHumidityDHT22_1 = 0.0;
int contadorChuvaLatch = 0; 

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_MOTOR, OUTPUT);
  pinMode(LED_VENTILADOR, OUTPUT);
  pinMode(LED_ILUMINACAO, OUTPUT);

  Wire.begin(8, 9);

  if (!ina219_solar.begin()) {
    Serial.println("[ERRO] Falha ao encontrar o INA219 Solar (0x40)!");
  }
  if (!ina219_load.begin()) {
    Serial.println("[ERRO] Falha ao encontrar o INA219 Cargas (0x41)!");
  }

  dht22_1.setup(DHTPIN_DHT22_1, DHTesp::DHT22);
  dht22_2.setup(DHTPIN_DHT22_2, DHTesp::DHT22);

  setup_wifi();
  
  if (WiFi.status() == WL_CONNECTED) {
    client.setServer(mqtt_server, 1883);
  }
}

void setup_wifi() {
  delay(10);
  Serial.print("\nConectando ao Wi-Fi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 8000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado com sucesso!");
  } else {
    Serial.println("\n[AVISO] Wi-Fi não encontrado. Iniciando em Modo Offline (Edge AI ativa).");
  }
}

void reconnect_nao_bloqueante() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect("ESP32S3_AgriSolar_Marcos")) {
      Serial.println("Conectado ao Broker MQTT!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" (Tentará novamente em breve sem congelar a placa)");
    }
  }
}

void loop() {
  // 1. Gerenciamento MQTT não-bloqueante
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      unsigned long agoraRecon = millis();
      if (agoraRecon - ultimaTentativaReconexao >= intervaloReconexao) {
        ultimaTentativaReconexao = agoraRecon;
        reconnect_nao_bloqueante();
      }
    } else {
      client.loop();
    }
  }

  // --- 2. LEITURA CONTÍNUA E ATUAÇÃO INSTANTÂNEA (EDGE AI) ---
  float solar_busvoltage = ina219_solar.getBusVoltage_V();
  float solar_current_mA = ina219_solar.getCurrent_mA();
  float solar_power_mW = ina219_solar.getPower_mW();
  float solar_current_A = solar_current_mA / 1000.0;
  float solar_power_W = solar_power_mW / 1000.0;

  float load_busvoltage = ina219_load.getBusVoltage_V(); 
  float load_current_mA = ina219_load.getCurrent_mA();
  float load_power_mW = ina219_load.getPower_mW();
  float load_current_A = load_current_mA / 1000.0;
  float load_power_W = load_power_mW / 1000.0;

  float temp22_1 = dht22_1.getTemperature();
  float hum22_1 = dht22_1.getHumidity();
  float temp22_2 = dht22_2.getTemperature();
  float hum22_2 = dht22_2.getHumidity();

  String statusAlerta = "SISTEMA ESTÁVEL";
  bool riscoChuva = false;
  bool falhaPainel = false;
  bool sombreamentoSevero = false;
  bool consumoExcessivo = false;

  if (!isnan(hum22_1) && previousHumidityDHT22_1 != 0.0) {
    float deltaUmidade = hum22_1 - previousHumidityDHT22_1;
    if (deltaUmidade > 7.0) {
      contadorChuvaLatch = 12; 
    }
  }
  previousHumidityDHT22_1 = hum22_1;

  if (contadorChuvaLatch > 0) {
    riscoChuva = true;
    contadorChuvaLatch--; 
  } else {
    riscoChuva = false;
  }

  if (solar_busvoltage < 1.0 && load_current_A > 0.1) {
    falhaPainel = true; 
  } else if (solar_power_W < 0.2 && load_power_W > 0.5) {
    sombreamentoSevero = true; 
  }

  if (load_busvoltage < 6.8) {
    consumoExcessivo = true;
  }

  // Atuação imediata nas cargas (LEDs acendem/apagam sem atrasos)
  if (solar_busvoltage < 1.0) {
    statusAlerta = "EMERGÊNCIA: PAINEL DANIFICADO / DESCONECTADO";
  }
  else if (load_busvoltage < 1.0) {
    statusAlerta = "EMERGÊNCIA: BATERIAS DANIFICADAS / SUBSTITUIR";
    digitalWrite(LED_MOTOR, LOW);       
    digitalWrite(LED_VENTILADOR, LOW);   
    digitalWrite(LED_ILUMINACAO, LOW);   
  }
  else if (load_busvoltage < 4.0) {
    statusAlerta = "EMERGÊNCIA: PROTEÇÃO PROFUNDA DE BATERIA";
    digitalWrite(LED_MOTOR, HIGH);       
    digitalWrite(LED_VENTILADOR, LOW);
    digitalWrite(LED_ILUMINACAO, LOW);
  }
  else if (riscoChuva) {
    statusAlerta = "ALERTA: PREVISÃO DE CHUVA - ECONOMIA PREVENTIVA";
    digitalWrite(LED_MOTOR, HIGH);
    digitalWrite(LED_VENTILADOR, LOW);   
    digitalWrite(LED_ILUMINACAO, HIGH);
  }
  else if (consumoExcessivo || (sombreamentoSevero && load_busvoltage < 7.99)) {
    statusAlerta = "ALERTA: DÉFICIT ENERGÉTICO / BATERIA BAIXA";
    digitalWrite(LED_MOTOR, HIGH);       
    digitalWrite(LED_VENTILADOR, HIGH);   
    digitalWrite(LED_ILUMINACAO, LOW);   
  } 
  else {
    statusAlerta = "NORMAL: OPERAÇÃO PLENA";
    digitalWrite(LED_MOTOR, HIGH);
    digitalWrite(LED_VENTILADOR, HIGH);
    digitalWrite(LED_ILUMINACAO, HIGH);
  }

  // --- 3. PUBLICAÇÃO MQTT E SERIAL A CADA 4 SEGUNDOS ---
  unsigned long agora = millis();
  if (agora - ultimaPublicacaoMQTT >= intervaloMQTT) {
    ultimaPublicacaoMQTT = agora;

    // Impressão no Monitor Serial
    Serial.println("\n==================================================");
    Serial.println("       ESTAÇÃO DE ENERGIA AGRI-SOLAR (IA EMBARCADA)       ");
    Serial.println("==================================================");
    Serial.print(" [PAINEL] Tensão: "); Serial.print(solar_busvoltage); Serial.print("V | Corrente: "); Serial.print(solar_current_mA); Serial.print("mA | Potência: "); Serial.print(solar_power_W); Serial.println("W");
    Serial.print(" [BATERIA/CARGAS] Tensão: "); Serial.print(load_busvoltage); Serial.print("V | Consumo: "); Serial.print(load_current_mA); Serial.print("mA | Potência: "); Serial.print(load_power_W); Serial.println("W");
    Serial.print(" [METEORO DHT22 - 1] Temp: "); Serial.print(temp22_1); Serial.print("°C | Umidade: "); Serial.print(hum22_1); Serial.println("%");
    Serial.print(" [METEORO DHT22 - 2] Temp: "); Serial.print(temp22_2); Serial.print("°C | Umidade: "); Serial.print(hum22_2); Serial.println("%");
    Serial.println("--------------------------------------------------");
    Serial.print(" [DIAGNÓSTICO IA] Status: "); Serial.println(statusAlerta);
    Serial.println("==================================================");

    // Publicação MQTT para atualizar o painel web
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
      String payload = "{";
      payload += "\"solar_v\":" + String(solar_busvoltage) + ",";
      payload += "\"solar_w\":" + String(solar_power_W) + ",";
      payload += "\"bateria_v\":" + String(load_busvoltage) + ",";
      payload += "\"carga_w\":" + String(load_power_W) + ",";
      payload += "\"temp_dht22_1\":" + String(temp22_1) + ",";
      payload += "\"hum_dht22_1\":" + String(hum22_1) + ",";
      payload += "\"temp_dht22_2\":" + String(temp22_2) + ",";
      payload += "\"hum_dht22_2\":" + String(hum22_2) + ",";
      payload += "\"risco_chuva\":" + String(riscoChuva ? "true" : "false") + ",";
      payload += "\"falha_painel\":" + String(falhaPainel ? "true" : "false") + ",";
      payload += "\"status\":\"" + statusAlerta + "\"";
      payload += "}";

      client.publish("agrisolar/telemetria", payload.c_str());
    }
  }

  // Pequena pausa para estabilidade do ciclo sem sobrecarregar o ESP32
  delay(100);
}
