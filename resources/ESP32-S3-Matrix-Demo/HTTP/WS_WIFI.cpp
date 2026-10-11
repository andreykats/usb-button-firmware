#include "WS_WIFI.h"

// The name and password of the WiFi access point
const char *ssid = APSSID;                
const char *password = APPSK;               
IPAddress apIP(10, 10, 10, 1);    // Set the IP address of the AP

char ipStr[16];
WebServer server(80);                               

void handleRoot() {
  String myhtmlPage =
    String("") +
    "<html>"+
    "<head>"+
    "    <meta charset=\"utf-8\">"+
    "    <title>ESP32-S3-Matrix</title>"+
    "    <style>" +
    "        body {" +
    "            font-family: Arial, sans-serif;" +
    "            background-color: #f0f0f0;" +
    "            margin: 0;" +
    "            padding: 0;" +
    "        }" +
    "        .header {" +
    "            text-align: center;" +
    "            padding: 20px 0;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            margin-bottom: 20px;" +
    "        }" +
    "        .container {" +
    "            max-width: 600px;" +
    "            margin: 0 auto;" +
    "            padding: 20px;" +
    "            background-color: #fff;" +
    "            border-radius: 5px;" +
    "            box-shadow: 0 0 5px rgba(0, 0, 0, 0.3);" +
    "        }" +
    "        .input-container {//" +
    "            display: flex;" +
    "            align-items: center;" +
    "            margin-bottom: 10px;" +
    "        }" +
    "        .input-container label {" +
    "            width: 80px;" + 
    "            margin-right: 10px;" +
    "        }" +
    "        .input-container input[type=\"text\"] {" +
    "            flex: 1;" +
    "            padding: 5px;" +
    "            border: 1px solid #ccc;" +
    "            border-radius: 3px;" +
    "            margin-right: 10px; "+ 
    "        }" +
    "        .input-container button {" +
    "            padding: 5px 10px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container {" +
    "            margin-top: 20px;" +
    "            text-align: center;" +
    "        }" +
    "        .button-container button {" +
    "            margin: 0 5px;" +
    "            padding: 10px 15px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container button:hover {" +
    "            background-color: #555;" +
    "        }" +
    "    </style>" +
    "</head>"+
    "<body>"+
    "    <script defer=\"defer\">"+
    "        function ledSwitch(ledNumber) {"+
    "            var xhttp = new XMLHttpRequest();" +
    "            xhttp.onreadystatechange = function() {" +
    "                if (this.readyState == 4 && this.status == 200) {" +
    "                    console.log('LED ' + ledNumber + ' state changed');" +
    "                }" +
    "            };" +
    "            if (ledNumber < 2) {" +
    "             xhttp.open('GET', '/SendData', true);" +
    "            }" +
    "            else if(ledNumber == 2){" +
    "            xhttp.open('GET', '/RGBOn', true);" +
    "            }" +
    "            else if(ledNumber == 3){" +
    "            xhttp.open('GET', '/RGBOff', true);" +
    "            }" +
    "            xhttp.send();" +
    "        }" +
    "        function updateData() {"
    "            var xhr = new XMLHttpRequest();"
    "            xhr.open('GET', '/getData', true);"
    "            xhr.onreadystatechange = function() {"
    "              if (xhr.readyState === 4 && xhr.status === 200) {"
    "                var dataArray = xhr.responseText;"
    "                document.getElementById('Text').value = dataArray;"
    // "                // Remove the button's disabled attribute to make it clickable"+
    // "                document.getElementById('btn1').removeAttribute(\'disabled\');"+
    // "                document.getElementById('btn2').removeAttribute(\'disabled\');"+
    // "                document.getElementById('btn3').removeAttribute(\'disabled\');"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "          }"+
    "        function sendData() {"+
    "            var textData = document.getElementById('Text').value;"+
    "            var xhr = new XMLHttpRequest();"+
    "            xhr.open('GET', '/SendData?data=' + textData, true);"+
    "            xhr.send();"+
    "          }"+
    "    </script>" +
    "</head>"+
    "<body>"+
    "    <div class=\"header\">"+
    "        <h1>ESP32-S3-Matrix</h1>"+
    "    </div>"+
    "    <div class=\"container\">"+
    "        <div class=\"input-container\" style=\"margin-left: 140px;\">"+
    "            <label for=\"input1\">Enter text</label>"+
    "            <input type=\"text\" id=\"Text\" />"+
    "            <button value=\"SendData\" id=\"btn1\" onclick=\"sendData()\">Send Data</button>"+
    "        </div>"+
    "        <div class=\"button-container\">"+
    "            <button value=\"RGBOn\" id=\"btn2\" onclick=\"ledSwitch(2)\">RGB On</button>"+
    "            <button value=\"RGBOff\" id=\"btn3\" onclick=\"ledSwitch(3)\">RGB Off</button>"+
    "        </div>"+
    "    </div>"+
    "</body>"+
    "</html>";
    
  server.send(200, "text/html", myhtmlPage); 
  printf("The user visited the home page\r\n");
  
}

void handleGetData() {
  String json = "";
  for (int i = 0; i < sizeof(Text) / sizeof(Text[0]); i++) {
    json += String(Text[i]);
  }
  json += "";
  server.send(200, "application/json", json);
}

void handleSwitch(uint8_t ledNumber) {
  switch(ledNumber){
    case 1:
      if (server.hasArg("data")) {
        String newData = server.arg("data");
        newData.toCharArray(Text, sizeof(Text));
      }
      printf("Text=%s.\r\n",Text);
      break;
    case 2:
      colorWipe(Matrix.Color(0, 255, 0), 0);
      printf("RGB On.\r\n");
      Flow_Flag = 0;
      break;
    case 3:
      colorWipe(Matrix.Color(0, 0, 0), 0);
      printf("RGB Off.\r\n");
      Flow_Flag = 1;
      break;
  }
  server.send(200, "text/plain", "OK");
}
void handleSendData()  { handleSwitch(1); }
void handleRGBOn()     { handleSwitch(2); }
void handleRGBOff()    { handleSwitch(3); }


void WIFI_Init()
{

  WiFi.mode(WIFI_AP); 
  while(!WiFi.softAP(ssid, password)) {
    printf("Soft AP creation failed.\r\n");
    printf("Try setting up the WIFI again.\r\n");
  } 
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Set the IP address and gateway of the AP
  
  IPAddress myIP = WiFi.softAPIP();
  uint32_t ipAddress = WiFi.softAPIP();
  printf("AP IP address: ");
  sprintf(ipStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  printf("%s\r\n", ipStr);

  server.on("/", handleRoot);
  server.on("/getData"  , handleGetData);
  server.on("/SendData" , handleSendData);
  server.on("/RGBOn"       , handleRGBOn);
  server.on("/RGBOff"      , handleRGBOff);

  server.begin(); 
  printf("Web server started\r\n");
}

void WIFI_Loop()
{
  server.handleClient(); // Processing requests from clients
}
















