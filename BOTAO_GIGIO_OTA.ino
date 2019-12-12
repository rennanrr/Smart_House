#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

const char* ssid = "Gigio Network";
const char* password = "giovanis@2018";

WiFiClient wifiClient;   


uint8_t relayOn[] = {0xA0, 0x01, 0x01, 0xA2};
uint8_t relayOff[] = {0xA0, 0x01, 0x00, 0xA1};

int botao = 3;

int statusLampada = -1;
bool acabouDeEnviar = false;
int estadoBotao;
int estadoAnteriorBotao;                       
 
//MQTT Server
const char* BROKER_MQTT = "tailor.cloudmqtt.com"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 13896;                      // Porta do Broker MQTT

#define ID_MQTT  "030610004802002OFFICEESP"             //Informe um ID unico e seu. Caso sejam usados IDs repetidos a ultima conexão irá sobrepor a anterior. 
#define TOPIC "030610004802002OFFICE"   //Informe um Tópico único. Caso sejam usados tópicos em duplicidade, o último irá eliminar o anterior.
#define MQTT_USER "munqymnj"
#define MQTT_PASSWORD "kzcBouTvF2_1"
PubSubClient MQTT(wifiClient);        // Instancia o Cliente MQTT passando o objeto espClient

//Declaração das Funções
void mantemConexoes();
void recebePacote(char* topic, byte* payload, unsigned int length);



 
void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Conexao falhou! Reiniciando...");
    delay(5000);
    ESP.restart();
  }
 
  // Porta padrao do ESP8266 para OTA eh 8266 - Voce pode mudar ser quiser, mas deixe indicado!
  // ArduinoOTA.setPort(8266);
 
  // O Hostname padrao eh esp8266-[ChipID], mas voce pode mudar com essa funcao
  // ArduinoOTA.setHostname("nome_do_meu_esp8266");
 
  // Nenhuma senha eh pedida, mas voce pode dar mais seguranca pedindo uma senha pra gravar
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    Serial.println("Inicio...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("nFim!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Autenticacao Falhou");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Inicio");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na Conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na Recepcao");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim");
  });
  ArduinoOTA.begin();
  Serial.println("Pronto");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());



  
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);  
  MQTT.setCallback(recebePacote); 
  MQTT.connect(ID_MQTT);
  MQTT.subscribe(TOPIC);
  
  pinMode(botao, INPUT);
}
 
void loop() {
  ArduinoOTA.handle();

  mantemConexoes();
  leituraBotao();
  MQTT.loop();
}

void mantemConexoes() 
{    
    if (WiFi.status() != WL_CONNECTED) 
    {
      delay(100);
    }
    else
    {
      if (!MQTT.connected()) 
      {
        MQTT.connect(ID_MQTT, MQTT_USER, MQTT_PASSWORD);
        if (MQTT.connected()) 
        {
          MQTT.subscribe(TOPIC);
          if(statusLampada == 1)
          {
            MQTT.publish(TOPIC, "1");
          }
          else
          {
            MQTT.publish(TOPIC, "-1");
          }
        }
      }
    }
}

void leituraBotao() 
{  
  if(digitalRead(botao) == HIGH)
  {
    estadoBotao = 1;
  }
  else
  {
    estadoBotao = -1;
  }
  if(estadoBotao != estadoAnteriorBotao)
  {
    if(!acabouDeEnviar)
    {
      estadoAnteriorBotao = estadoBotao;
      statusLampada = statusLampada*-1;
    
      if(statusLampada == 1)
      {
        Serial.write(relayOn, 4); 
        MQTT.publish(TOPIC, "1");
      }
      else
      {
        Serial.write(relayOff, 4);
        MQTT.publish(TOPIC, "-1");
      }
      acabouDeEnviar = true;  
    }
    else
    {
    acabouDeEnviar = false;
    }
  }
}  

void recebePacote(char* topic, byte* payload, unsigned int length) 
{
  if(!acabouDeEnviar)
  {
    String msg;

    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }

    if (msg == "-1") 
    {
      statusLampada = -1;
       Serial.write(relayOff, 4);
    }
    if (msg == "1") 
    {
      statusLampada = 1;
       Serial.write(relayOn, 4); 
    }
   }
   else
   {
    acabouDeEnviar = false;
   }
    
}  
