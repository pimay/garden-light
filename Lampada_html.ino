void handleRootInicial() {
  // SW to create the html
  String html = "<html><head><title> Lampada Jardim</title>";
  html += "<style> body ( background-color:#cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; ) </style> ";
  html += "</head><body> ";
  html += "<h1>Controle</h1> ";
  html += "<br>";
  html += "<table style=\"width:50%\">";
  html += "<tr>";
  html += "<th>Current Time:</th><th>";
  html += "timemeaure.c_str()";
  html += "<th>";
  html += "</tr><tr>";
  html += "<th>Turn On/Off the lights:</th>";
  html += "<th>";
  html += "<a href=\"OnOff\"><button>On/Off</button></a>";
  html += "</th>";
  html += "<th>";
  html += onoffButton;
  html += "</th>";
  html += "<th>";
  html += buttonState;
  html += "</th>";
  html += "</tr>";
  //html += "</table>";
  
  //html += "<table style=\"width:50%\">";
  
  html += "<tr>";
  html += "<th>Conectado no Wireless (0-desconectado, 1-conectado):</th>";
  html += "<th>";
  html += (WiFi.status() == WL_CONNECTED);
  html += "</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>Conectado ao MQTT (0-desconectado, 1-conectado):</th>";
  html += "<th>";
  html += (client.connected() and mqttButton);
  html += "</th>";
  html += "<tr>";
  html += "<th>Button status (1-on/0-off): </th>";
  html += "<th>";
  html += mqttButton;
  html += "</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>";
  html += "<a href=\"MQTTButtonWeb\"><button>MQTTButton</button></a>";
  html += "</th>";
  html += "</tr>";
  html += "</table>";
  
  
  //update ota
  html += "<p>----------------------------------------------------------</p>";
  html += "<p><a href=\"/update\">Update by OTA</a></p>";
  html += "<p>----------------------------------------------------------</p>";
  //RESET
  html += "<p><a href=\"ESPReset\"><button>Reset</button></a></p>";
  html += "<p>----------------------------------------------------------</p>";
  html += "</body></html>";

  servidorWeb.send(200, "text/html", html);

}

///////////////////////////////////////////////
/* Web Page RESET
*/
void WebReset() {
  // SW to create the html
  String html = "<html><head><title> Lampada Jardim</title>";
  html += "<style> body ( background-color:#cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; ) </style> ";
  html += "</head><body> ";
  html += "<h1>LAMPADA JARDIM in RESET MODE</h1> ";
  html += "</body></html>";

  servidorWeb.send(200, "text/html", html);
}
