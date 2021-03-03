//controle de lampada do jardim - 192.168.0.122
//  Req-01  ligar e desligar através do html - botão ok
//  Req-02  ligar e desligar via mqtt
//  Req-03  sempre iniciar desligado - ok
//  Req-04  html tem prioridade sobre o mqtt:
//  Req-04.1   - Ligar e iniciar um timer de 10 minutos para desligar a lâmpada se o mqtt estiver comandando para desligar;
//  Req-04.2   - Desligar e iniciar um timer de 10 minutos para ligar a lâmpada se o mqtt estiver comandando para ligar;
//  Req-05  resetar o esp dois dias antes do limite do contator final
//  Req-05.1  - se o sistema estiver ligado, resetar após o sistema comandar para desligar;
//  Req-05.2  - se desligado, resetar imediatamente;
//  Req-05.3  - se estiver comando para ligar por até um dia do limte, resetar independe do tempo
//  Req-06  html botao para resetar - nok (não liga)
//  Req-07  html fazer update via OAT - ok
//  Req-08  funcionar como modo AP de wireless
//  Req-09  atualizar o mqtt (atualizar o mqtt server, username, password, pub/sub via html) via html e salvar na memória SPIFFS
// it should work in the sonoff single and Nodemcu v1.0 - Relay in GPIO12 - D6
// baseado no sw https://www.filipeflop.com/blog/esp8266-gravando-dados-memoria-flash/

/*
  Equivalencia das saidas Digitais entre NodeMCU e ESP8266 (na IDE do Arduino)
  NodeMCU - ESP8266
  D0 = GPIO16;
  D1 = GPIO5;
  D2 = GPIO4;
  D3 = GPIO0;
  D4 = GPIO2;
  D5 = GPIO14;
  D6 = GPIO12;
  D7 = GPIO13;
  D8 = GPIO15;
  RX = GPIO3;
  TX = GPIO1;
  S3 = GPIO10;
  S2 = GPIO9;

  Update the node mcu by FDTI
  NodeMCU pin - FDTI (pin out)
  Vin         - 3.3V
  En          - 3.3V
  GND         - GND
  D3 (GPIO0)  - GND
  TX          - RX
  RX          - TX

   ESP8266 (NODEMCU)
   Button 0 - GPIO0  - D3
   Button 1 - GPIO5  - D2
   Relay 1  - GPIO12 - D6
   Relay 2  - GPIO5  - D1


   Sonoff Dual R2
   Button 0 - GPIO0  - D3
   Button 1 - GPIO9  - S2
   Relay 1  - GPIO12 - D6
   Relay 2  - GPIO5  - D1

   Sonoff Single
   Button 0 - GPIO14 - D5
   Relay    - GPIO12 - D6
*/

#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h> //Biblioteca que cria o servico de atualizacão via wifi (ou Over The Air - OTA)
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>              //MQTT
#include <ArduinoJson.h>                 //https://github.com/bblanchon/ArduinoJson

///////////////////////////////////////////////
///////////////////////////////////////////////
/* General configuration
*/

///WiFi/WiFi Server
ESP8266HTTPUpdateServer atualizadorOTA; //Este e o objeto que permite atualizacao do programa via wifi (OTA)
ESP8266WebServer servidorWeb(80);       //Servidor Web na porta 80
WiFiClient espClient;                   //serve para o MQTT, create the WiFiClient
PubSubClient client(espClient);         //Serve para o MQTT publish/subscriber

///////////////////////////////////////////////
///////////////////////////////////////////////

int relay = D6; // relay number
boolean buttonState = false; // save the state of the button
boolean onoffButton = false;  // button to turn on / off the light

//dev
boolean serialPrint = false; // helps to print anything in the console

//MQTT
boolean mqttButton = false; // button to enable the mqtt
//configuracao
char* servidor_mqtt = "192.168.0.125";  //URL do servidor MQTT - Raspberry Pi
char* servidor_mqtt_porta = "1883";  //Porta do servidor (a mesma deve ser informada na variável abaixo)
char* servidor_mqtt_usuario = "pi1";  //Usuário
char* servidor_mqtt_senha = "12345"; //Senha

// receive message - these variable are used to receive all topics/mensages
String mqtt_topico;         //Define o topico do mqtt
String mqtt_mensagem;       //Define a mensagem mqtt

// definicao de sub/pub
const char* lamp_pub = "jardim/lamp1_pub";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* lamp_pubsupervisorio = "jardim/lamp1_supervisorio";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* lamp_sub = "jardim/lamp1";    //Topico para ler o comando do ESP8266

//MQTT
//Será usado quando atualizar via web
//String server_mqtt;
//String server_mqtt_porta;
//String server_mqtt_usuario;
//String server_mqtt_senha;

///////////////////////////////////////////////
///////////////////////////////////////////////
/* OTA Configuration
*/
//Servico OTA
void InicializaServicoAtualizacao() {
  atualizadorOTA.setup(&servidorWeb);
  servidorWeb.begin();
  if (serialPrint){
    Serial.print("O servico de atualizacao remota (OTA) Foi iniciado com sucesso! Abra http://");
    Serial.print(".local/update no seu browser para iniciar a atualizacao\n");
  }
}


void setup() {
  pinMode(relay, OUTPUT);
  Serial.begin(9600);   

  //Inicia o servidor web
  servidorWeb.on("/", handleRootInicial);
  
  servidorWeb.on("/htmlRootInicial", []() {
    
    //carrega a pagina de controle
    handleRootInicial();
  });

  //Ativa o webserver
  servidorWeb.begin();


  // OnOff Button
  servidorWeb.on("/OnOff", []() {
    if (onoffButton == true) {
      onoffButton = false;
    } else {
      onoffButton = true;
    }
    //carrega a pagina anterior
    handleRootInicial();
  });
  
  //Reset Button
  servidorWeb.on("/ESPReset", []() {
    WebReset();
    delay(1000);
    //ESP.reset();
    ESP.restart();
    
  });

  // MQTT Button
  servidorWeb.on("/MQTTButtonWeb", []() {
    if (mqttButton == true) {
      mqttButton = false;
    } else {
      mqttButton = true;
    }
    //carrega a pagina anterior
    handleRootInicial();
  });

    // OTA
  InicializaServicoAtualizacao();

  //MQTT - conect
  int portaInt = atoi(servidor_mqtt_porta); //convert string to number
  client.setServer(servidor_mqtt, portaInt);
  client.setCallback(retornoMQTTSub);
  //MQTT start the subscribers
  mqttSubscribe();
}
 
void loop() {
  /*
   * Verifica o botao faz ligar e desligar
   */

   if (onoffButton){
    //turn on the system (liga)
    digitalWrite(relay,LOW);
    buttonState = true;
   }
   if (!onoffButton){
    //turn on the system (desliga)
    digitalWrite(relay,HIGH);
    buttonState = false;
   }

  //MQTT - Reconnect
  if (!client.connected() and (mqttButton == true)) {
    // reconnect the mqqt
    //troubleshooting the next line
    reconnectMQTT();
    if (serialPrint){
      Serial.println("reconectar mqtt");
    }
    
    //countMQTT++;
    //}
    client.publish(lamp_pubsupervisorio, "delay - mqtt reconnect");
  }



  //Servidor Web
  servidorWeb.handleClient();
  
  //Wifi MQTT;
  if (mqttButton){
    client.loop();
  }
}
