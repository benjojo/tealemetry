#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <Wire.h>

#define MLX90614_TA 0x06
#define MLX90614_TOBJ1 0x07


const char* ssid = "XXXXXXXXXXXXXXXXXXXX";
const char* password = "XXXXXXXXXXXXXXXXXXxx";

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from tea coaster!");
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}


uint16_t read16(uint8_t addr, uint8_t i2c_addr){
uint16_t ret;

Wire.beginTransmission(i2c_addr); // start transmission to device 
Wire.write(addr); // sends register address to read from
Wire.endTransmission(false); // end transmission

Wire.requestFrom(i2c_addr, (uint8_t)3);// send data n-bytes read
ret = Wire.read(); // receive DATA
ret |= Wire.read() << 8; // receive DATA

uint8_t pec = Wire.read();

return ret;
}


float readTemp(uint8_t reg, uint8_t i2c_addr) {
float temp;

temp = read16(reg, i2c_addr);
temp *= .02;
temp -= 273.15;
return temp;

}

double readObjectTempC(uint8_t i2c_addr) {
return readTemp(MLX90614_TOBJ1, i2c_addr);
}


double readAmbientTempC(uint8_t i2c_addr) {
return readTemp(MLX90614_TA, i2c_addr);
}

uint32_t noquerycount;


void setup(void){
  Wire.begin(4,5);
  Wire.setClock(100000);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/metrics", [](){
    digitalWrite(led, 1);
    char page[512];
    sprintf(page, "# HELP tea_mug_temp The point temp on the coaster.\n# TYPE tea_mug_temp gauge\ntea_mug_temp %f\n# HELP tea_ambient_temp the temp of the sensor.\n# TYPE tea_ambient_temp gauge\ntea_ambient_temp %f\n",readObjectTempC(0x5A), readAmbientTempC(0x5A));
    server.send(200, "text/plain", page);
    noquerycount = 0;
    digitalWrite(led, 0);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
  delay(10);
  noquerycount++;
  if (WiFi.status() != WL_CONNECTED || noquerycount == 3000) {
    ESP.restart();
  }
}

