#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "con-sentimiento";
const char* password = "amoramor";
DNSServer dns;
AsyncWebServer server(80);

char mensaje[128] = "Mas Amor";
bool mensaje_actualizado = true;

int16_t scrollX = 0;
uint16_t textoWidth = 0;
unsigned long lastScroll = 0;
const int scrollSpeed = 30; // ms entre pasos

void calcularAnchoTexto(const char* text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(2);
  display.setTextWrap(false);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  textoWidth = w;
}

void mostrarMarquesina(const char* text) {
  display.clearDisplay();
  display.setRotation(0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextWrap(false);

  int y = (SCREEN_HEIGHT - 16) / 2;

  // Si el texto cabe, centrarlo
  if (textoWidth <= SCREEN_WIDTH) {
    int x = (SCREEN_WIDTH - textoWidth) / 2;
    display.setCursor(x, y);
    display.print(text);
  } else {
    int offset = scrollX % (textoWidth + 20);  // 20px de espacio entre repeticiones
    display.setCursor(-offset, y);
    display.print(text);
    display.print("   "); // Espacio visible entre repeticiones
    display.print(text);
  }
  display.display();
}

void actualizarScroll() {
  if (textoWidth > SCREEN_WIDTH) {
    if (millis() - lastScroll > scrollSpeed) {
      lastScroll = millis();
      scrollX++;
      if (scrollX > textoWidth + 20) scrollX = 0;  // Sin espacio negro
    }
  } else {
    scrollX = 0;
  }
}

void handleCaptive(AsyncWebServerRequest *request) {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Con-Sentimiento</title>";
  html += "<style>";
  html += "body { margin:0; font-family: 'Arial', sans-serif; background: linear-gradient(135deg, #f7c5cc, #b8dfe6); color: #333; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; }";
  html += "h1 { font-size: 2.5em; margin-bottom: 0.3em; text-align: center; }";
  html += "p { font-size: 1.2em; max-width: 90%; text-align: center; margin-bottom: 2em; }";
  html += "form { background: rgba(255,255,255,0.9); padding: 20px; border-radius: 12px; box-shadow: 0px 4px 15px rgba(0,0,0,0.2); display: flex; flex-direction: column; align-items: center; }";
  html += "textarea { width: 250px; height: 100px; margin-bottom: 15px; font-size: 1em; padding: 10px; border-radius: 8px; border: 1px solid #ccc; }";
  html += "input[type='submit'] { background: #d87ca0; color: white; border: none; padding: 12px 20px; border-radius: 8px; font-size: 1.1em; cursor: pointer; transition: background 0.3s; }";
  html += "input[type='submit']:hover { background: #bd6e8f; }";
  html += "</style></head><body>";

  html += "<h1>Con-Sentimiento</h1>";
  html += "<p>Este cuerpo no está siempre disponible.<br>Conéctate, susurra, reclama, cuida. Elige tu ritmo y tu mensaje.</p>";

  html += "<form action='/send' method='POST'>";
  html += "<textarea name='mensaje' placeholder='Escribe un mensaje bonito'></textarea>";
  html += "<input type='submit' value='Enviar mensaje'>";
  html += "</form>";

  html += "</body></html>";

  request->send(200, "text/html", html);
}


void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED no detectada"));
    while (true);
  }

  calcularAnchoTexto(mensaje);
  mostrarMarquesina(mensaje);

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
      mensaje_actualizado = true;
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
    calcularAnchoTexto(mensaje);
    scrollX = 0;
    lastScroll = millis();
    mensaje_actualizado = false;
  }

  actualizarScroll();
  mostrarMarquesina(mensaje);
  yield();
}
