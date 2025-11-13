#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "con-sentimiento";
const char* password = "amoramor";
DNSServer dns;
AsyncWebServer server(80);

// Buffers robustos
char mensaje[64] = "Mas Amor";
bool mensaje_actualizado = true; // bandera para actualizar OLED

void mostrarMensaje(const char* text) {
  display.clearDisplay();
  display.setRotation(2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;
  
  display.setCursor(x, y);
  display.println(text);
  display.display();
}

void handleCaptive(AsyncWebServerRequest *request) {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>";
  html += "<title>Con-Sentimiento</title></head><body style='text-align:center;font-family:sans-serif'>";
  html += "<h2>Envía un mensaje</h2>";
  html += "<form action='/send' method='POST'>";
  html += "<textarea name='mensaje' rows='4' cols='30'></textarea><br>";
  html += "<input type='submit' value='Enviar'></form></body></html>";
  request->send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED no detectada"));
    while (true);
  }
  mostrarMensaje(mensaje);

  WiFi.softAP(ssid, password);
  delay(100);
  IPAddress myIP = WiFi.softAPIP();
  dns.start(53, "*", myIP);

  server.on("/generate_204", HTTP_GET, handleCaptive);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);
  server.on("/", HTTP_GET, handleCaptive);

  server.on("/send", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("mensaje", true)) {
      String nuevo = request->getParam("mensaje", true)->value();
      nuevo.trim();
      nuevo.toCharArray(mensaje, sizeof(mensaje));
      mensaje_actualizado = true;  // solo cambiamos bandera
      Serial.println("Nuevo mensaje recibido!");
    }
    request->redirect("/");
  });

  server.begin();
  Serial.println("Servidor listo en IP: " + myIP.toString());
}

void loop() {
  dns.processNextRequest();

  if (mensaje_actualizado) {
    mostrarMensaje(mensaje);  // actualizar fuera del contexto HTTP
    mensaje_actualizado = false;
    Serial.println("OLED actualizado");
  }
  yield();
}
