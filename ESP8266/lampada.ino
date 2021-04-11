//controle de lampada do jardim - 192.168.0.122
//  Req-01  ligar e desligar através do html - botão ok
//  Req-02  ligar e desligar via mqtt
//  Req-02.1 ter um mqtt server as backup
//  Req-03  sempre iniciar desligado - ok
//  Req-04  html tem prioridade sobre o mqtt:
//  Req-04.1   - Ligar e iniciar um timer de 10 minutos para desligar a lâmpada se o mqtt estiver comandando para desligar;
//  Req-04.2   - Desligar e iniciar um timer de 10 minutos para ligar a lâmpada se o mqtt estiver comandando para ligar;
//  Req-04.3   - Resetar as condicoes caso um comando contrario a acao ocorra durante os 10 min.
//  Req-05  resetar o esp dois dias antes do limite do contator final
//  Req-05.1  - se o sistema estiver ligado, resetar após o sistema comandar para desligar;
//  Req-05.2  - se desligado, resetar imediatamente;
//  Req-05.3  - se estiver comando para ligar por até um dia do limte, resetar independe do tempo
//  Req-06  html botao para resetar - nok (não liga)
//  Req-07  html fazer update via OAT - ok
//  Req-08  funcionar como modo AP de wireless
//  Req-09  atualizar o mqtt (atualizar o mqtt server, username, password, pub/sub via html) via html e salvar na memória SPIFFS
//  Req-10  usar um sensor de luminosidade para ligar e desligar
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

  next to button 3.3 RX TX GND GPIO14, INTERNAL LED
  before turn on, press the button
  gpio13 - is a led 
  

   Configuration
   board: Generic ESP8266 Module (sonoff)
   flash mode: DOUT
   size: 1M 64K SPIFFS
   Debug Port: Disable
   IwIP: 1.4 higher bandwidth
   Reset method: nodemcu
   cristal 26Mhz
   flash   40 Mhz
   cpu     80Mhz
   Upload speed 115200
   Erase Flash: only sketch
   
*/

#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h> //Biblioteca que cria o servico de atualizacão via wifi (ou Over The Air - OTA)
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>              //MQTT
#include <ArduinoJson.h>                 //https://github.com/bblanchon/ArduinoJson
#define sonoffLed 13 //GPIO13;
///////////////////////////////////////////////
///////////////////////////////////////////////
/* General configuration
*/

///WiFi/WiFi Server
ESP8266HTTPUpdateServer atualizadorOTA; //Este e o objeto que permite atualizacao do programa via wifi (OTA)
ESP8266WebServer servidorWeb(80);       //Servidor Web na porta 80
WiFiClient espClient;                   //serve para o MQTT, create the WiFiClient
PubSubClient client(espClient);         //Serve para o MQTT publish/subscriber
// It can only publish QoS 0 messages. It can subscribe at QoS 0 or QoS 1.
///////////////////////////////////////////////
///////////////////////////////////////////////



int relay = 12;//D6; // relay number

int buttonState = -1; // save the state of the button
int onoffButton = 0;  // button to turn on / off the light
int lampState = 0; //turn on/off the lamp
int oldlampState = -1; //turn on/off the lamp
int lampStatemqtt = -1;
int lampStatebtn = -1;

//dev
boolean serialPrint = false; // helps to print anything in the console

//MQTT
int mqttBtnSt = -1;
boolean mqttButton = true; // button to enable the mqtt
boolean mqttButtonSt = true;
//configuracao
char* servidor_mqtt = "192.168.0.124";  //URL do servidor MQTT - Raspberry Pi
char* servidor_mqtt_porta = "1883";  //Porta do servidor (a mesma deve ser informada na variável abaixo)
char* servidor_mqtt_usuario = "pi2";  //Usuário
char* servidor_mqtt_senha = "123456"; //Senha

// receive message - these variable are used to receive all topics/mensages
String mqtt_topico;         //Define o topico do mqtt
String mqtt_mensagem;       //Define a mensagem mqtt

// definicao de sub/pub
const char* lamp_pub = "jardim/lamp4_pub";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* lamp_pubsupervisorio = "jardim/lamp4_supervisorio";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* lamp_sub = "jardim/lamp4";    //Topico para ler o comando do ESP8266

//time
unsigned long confTime = 0;

//MQTT
//Será usado quando atualizar via web
//String server_mqtt;
//String server_mqtt_porta;
//String server_mqtt_usuario;
//String server_mqtt_senha;

//////////////////////////////////////////////
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

//////////////////////////////////////////////
///////////////////////////////////////////////

int lampCommandmqtt(const char* mqttsub, int* mqttSt){
  //this function is to control the on off of the lamp
  //mqttsub = mqtt sub read
  //sub = sub address
  //htmlbutton = button from html
  //lampst = current status
  //boolean intercmd = *mqttSt;
  int inter = -1;
  //Serial.print("inter: " + String(intercmd)+" "+String(inter) +"\n");
  //enter in the correct sub
  if (strcmp(mqtt_topico.c_str(), mqttsub) == 0){

    //transform the mqtt message from boolean to int
    if (mqtt_mensagem.toInt() == 1){
      //turn on
      inter = 1;
    }else if (mqtt_mensagem.toInt() == 0){
      //turn off
      inter = 0;
    } else {
      //not a valid command, exit function
      return -1;
    }

    //save the status of mqtt
    //Serial.print("inter: " + String(inter)+ "/n");
    
    *mqttSt = inter;
    return inter;
  
    
  }
  return -1;
  
}

//////////////////////////////////////////////
///////////////////////////////////////////////

int lampCommandbtn( int onoffButton, int* buttonState){
  if(onoffButton != *buttonState){
    if (onoffButton == 1){
      lampState = 1;
    }  else {
      lampState = 0;
    }
    *buttonState = lampState;
    return lampState;
  }
  return -1;
}
//////////////////////////////////////////////
///////////////////////////////////////////////

void setup() {
  Serial.begin(9600);   
  pinMode(relay, OUTPUT);
  pinMode(sonoffLed, OUTPUT);
  
  //WIFI
  WiFiManager wifiManager;                // start the wifi manager
  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //Default 180 seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP_ESP", "senha1234")) {
    delay(3000);
  }
  
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
    if (onoffButton == 1) {
      onoffButton = 0;
    } else {
      onoffButton = 1;
    }
    //carrega a pagina anterior
    handleRootInicial();
  });
  
  //Reset Button
  servidorWeb.on("/ESPReset", []() {
    WebReset();
    delay(1000);
    //ESP.reset();
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

  // Updage page
  servidorWeb.on("/updatePage", []() {
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
//////////////////////////////////////////////
/////////////////////////////////////////////// 
void loop() {
  /*
   * Verifica o botao faz ligar e desligar
   */
  if (mqttButton and client.connected()){
    lampStatemqtt = lampCommandmqtt(lamp_sub, &mqttBtnSt);
    lampStatebtn = lampCommandbtn(onoffButton,&buttonState);
    //Serial.print("lampStatemqtt: " + String(lampStatemqtt) + " lampCommandbtn: " + String(lampCommandbtn) + "\n");
    
    if (confTime > 0 and (lampStatebtn>-1)){
      confTime = 0;
      Serial.print("confTime confTime reset: " + String(confTime)+"\n");
      String pubsupervisorio = "lampStatemqtt: " + String(lampStatemqtt) + " lampStatebtn: " + String(lampStatebtn);
      client.publish(lamp_pubsupervisorio, pubsupervisorio.c_str());
      // Update the button to pre-activation condition
        if (lampStatebtn == 0){ 
          onoffButton = 0;
          buttonState = 0;
        } else {
          onoffButton = 1;
          buttonState = 1;
        }
        
        //reset condition
        
    } else if (oldlampState == 0 and ( lampStatebtn == 1 )  and confTime == 0){
      //lamp off, mqtt off and button on -> turn on for a period
      //start time
      confTime = millis();
      Serial.print("confTime oldLampState = 0, mqtt = 0 btn =1: " + String(confTime)+"\n");
      //turn on relay
      lampState = 1;
    } else if (oldlampState == 1 and (lampStatebtn == 0) and confTime == 0){
      //lamp on, mqtt on and button off -> turn off for a period
      //start time
      confTime = millis();
      Serial.print("confTime oldLampState = 1, mqtt = 1 btn =0: " + String(confTime)+"\n");
      //turn off relay
      lampState = 0;
    } else if (oldlampState == 0 and lampStatemqtt == 1 and confTime == 0) {
      //lamp off, mqtt on, does not depend the button off -> turn on 
      Serial.print("confTime oldLampState = 0, mqtt = 1 btn =X: " + String(confTime)+"\n");
      //turn on relay
      lampState = 1;
      //update the button to avoid to click twice to turn on for a period 
      onoffButton = 1;
      buttonState = 1;
      
    } else if (oldlampState == 1 and lampStatemqtt == 0 and confTime == 0) {
      //lamp on, mqtt off, does not depend the button -> off
      Serial.print("confTime oldLampState = 1, mqtt = 0 btn =X: " + String(confTime)+"\n");
      //turn off relay
      lampState = 0;

      //update the button to avoid to click twice to turn on for a period 
      onoffButton = 0;
      buttonState = 0;
    }

    //if the confTime is > 0 continues the lampstate until the time finished
    if (confTime>0){
      //test the time
      //confTime > current time
      if (confTime + (1*5*1000) < millis()){
        //reset the time
        confTime = 0;

        // return the button to pre-activation condition
        if (lampState == 1){ 
          onoffButton = 0;
          buttonState = 0;
        } else {
          onoffButton = 1;
          buttonState = 1;
        }

        //update the mqtt to pre-activation conditoin
        
        //update the status
        lampState = !oldlampState;
        Serial.print("new lampState\n");
      }
    }
  } else {
    //only button
    lampState = lampCommandbtn(onoffButton,&buttonState);;
  }
  
  //client.publish(lamp_pubsupervisorio, String(lampState).c_str());
  //turn on/off the relay
  //String confT = "confTime: " + String(confTime) + " missing time: " + String((millis() - confTime)*1000) + "[s]";
  //client.publish(lamp_pubsupervisorio, confT.c_str());
  if ((lampState != oldlampState) and (lampState >=0 )){
    //Serial.print("lampState update: " + String(lampState)+" "+String(oldlampState) +"\n");
    if (lampState){
      //turn on the system (liga)
      digitalWrite(relay,HIGH);
      digitalWrite(sonoffLed,HIGH);
    }else{
      //turn on the system (desliga)
      digitalWrite(relay,LOW);
      digitalWrite(sonoffLed,LOW);
      
      }
    //publish the state
    if (mqttButton == true){
      if (lampState ){
        client.publish(lamp_pub, String(true).c_str());
      }else{
        client.publish(lamp_pub, String(false).c_str());
      }
    }
    //only update if the command is different
    oldlampState=lampState;
  }


  //MQTT - Reconnect
  if (!client.connected() and (mqttButton)) {
    // reconnect the mqqt
    //troubleshooting the next line
    reconnectMQTT();
    if (serialPrint){
      Serial.println("reconectar mqtt");
    }
    
    //countMQTT++;
    //}
    client.publish(lamp_pubsupervisorio, "delay - mqtt reconnect");
    //get the last status
    client.publish(lamp_pub, "reboot");

    mqttButtonSt = mqttButton;
  }
  
  if(mqttButtonSt == false and mqttButton){
    client.publish(lamp_pub, "reboot");
    mqttButtonSt = mqttButton;
  }  
  


  //Servidor Web
  servidorWeb.handleClient();
  
  //Wifi MQTT;
  if (mqttButton){
    client.loop();
  }
}

/*Tests:
1) start in off
2) turn on through html
3) turn off through html

*/
