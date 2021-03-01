//controle de lampada do jardim
//  Req-01  ligar e desligar através do html
//  Req-02  ligar e desligar via mqtt
//  Req-03  sempre iniciar desligado
//  Req-04  html tem prioridade sobre o mqtt:
//  Req-04.1   - Ligar e iniciar um timer de 10 minutos para desligar a lâmpada se o mqtt estiver comandando para desligar;
//  Req-04.2   - Desligar e iniciar um timer de 10 minutos para ligar a lâmpada se o mqtt estiver comandando para ligar;
//  Req-05  resetar o esp dois dias antes do limite do contator final
//  Req-05.1  - se o sistema estiver ligado, resetar após o sistema comandar para desligar;
//  Req-05.2  - se desligado, resetar imediatamente;
//  Req-05.3  - se estiver comando para ligar por até um dia do limte, resetar independe do tempo
//  Req-06  html botao para resetar
//  Req-07  html fazer update via OAT
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
boolean mqttButton = false; // button to enable the mqtt
boolean onoffButton = false;  // button to turn on / off the light

//dev
boolean serialPrint = false; // helps to print anything in the console
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
    ESP.reset();
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
}
 
void loop() {
  /*
   * Verifica o botao faz ligar e desligar
   */

   if (onoffButton && buttonState == false){
    //turn on the system (TBC)
    digitalWrite(relay,LOW);
    buttonState = false;
   }
   if (!onoffButton && buttonState == true){
    //turn on the system (TBC)
    digitalWrite(relay,HIGH);
    buttonState = false;
   }


  //Servidor Web
  servidorWeb.handleClient();
}
