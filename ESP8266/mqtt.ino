//MQTT

///////////////////////////////////////////////
///////////////////////////////////////////////
/* MQTT - Sub - Indica os subscribes para monitorar
   adicionar todos os subscribes
*/
void mqttSubscribe() {
  // add sub topicos to read the command from the server
    client.subscribe(lamp_sub, 1); //QoS 1
}


///////////////////////////////////////////////
///////////////////////////////////////////////
/* MQTT - Reconectar
    Serve para reconectar o sistema de MQTT, tenta reconectar 3 vez por ciclo
*/
void reconnectMQTT() {
  String textretorno = "reconnect MQTT";
  //client.publish("cerveja/supervisorio", textretorno.c_str());
  //Try to reconect to MQTT without the while loop, try 3 times, before leave
  //for (int iI = 0; iI <= 3; iI++) {
  if (!client.connected()) {
    //Tentativa de conectar. Se o MQTT precisa de autenticação, será chamada a função com autenticação, caso contrário, chama a sem autenticação.
    bool conectado = strlen(servidor_mqtt_usuario) > 0 ?
                     client.connect("ESP8266Client", servidor_mqtt_usuario, servidor_mqtt_senha) :
                     client.connect("ESP8266Client");

    //Conectado com senha
    if (conectado) {
      if (serialPrint){
        Serial.println("connected");
      }
      // Once Connected, read the subscribes
      mqttSubscribe();
    } else {
      if (serialPrint){
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 1 seconds");
      }
      // Wait 1 seconds before retrying
      client.publish(lamp_pubsupervisorio, "delay reconnect mqtt");
      delay(1000);
    }
  }
  /* else {
    //leave loop
    break;
    }
    } //for loop
  */
}

///////////////////////////////////////////////
///////////////////////////////////////////////
/* MQTT - Sub - monitora as mensagens do servidor
   Função que será chamada ao monitar as mensagens do servidor MQTT
   Retorna todas as mensagens lidas
*/
void retornoMQTTSub(char* topico, byte* mensagem, unsigned int tamanho) {
  //A mensagem vai ser On ou Off
  //Convertendo a mensagem recebida para string
  mensagem[tamanho] = '\0';
  String strMensagem = String((char*)mensagem);
  strMensagem.toLowerCase();
  //float f = s.toFloat();

  //informa o topico e mensagem
  mqtt_topico = String(topico);
  mqtt_mensagem = strMensagem;


//  String textretorno = "Retorno topico: ";
//  textretorno += String(topico).c_str();
//  textretorno += " in mqtt_topico: ";
//  textretorno += mqtt_topico.c_str();
//  textretorno += " msg: ";
//  textretorno += mqtt_mensagem.c_str();
//  textretorno += " \n\n";
//  client.publish("geladeira/supervisorio", textretorno.c_str());

}
///////////////////////////////////////////////
///////////////////////////////////////////////
