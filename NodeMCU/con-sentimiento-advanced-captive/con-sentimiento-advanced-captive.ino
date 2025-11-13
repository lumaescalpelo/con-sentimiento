// Proyecto: Con-Sentimiento COMPLETO con frases, scroll, captive portal y robusta aleatorización
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

extern "C" uint32_t system_get_time(void);

// Frases OLED
const char* frases_oled[] = {
  "Mas amor, menos prisa.", "Este cuerpo elige cuando.", "Hoy solo recibo dulzura.",
  "Disponibilidad es un regalo.", "Aqui mando yo o no.", "Con-Sentimiento: obedecer se siente bien.",
  "A tu ritmo, a mi pulso.", "Cadena invisible, deseo visible.", "Hoy obedezco a mis ganas.",
  "Me entrego, pero no me rindo.", "Controla mi cuello, no mi vida.", "Dame ordenes suaves.",
  "Mi cuerpo responde al afecto.", "Te leo, no te sigo.", "Si me sujetas, hazlo bonito.",
  "Soy mia, pero te presto mi cuello.", "Amar es elegir obedecer.", "Estoy disponible pero solo para ti.",
  "Obedezco, pero solo a quien elijo.", "Sumisa, pero critica.", "Con-sentimiento o nada.",
  "Mi cuello obedece, mi mente elige.", "No siempre disponible, disfrutalo.", "Si controlas, cuida.",
  "Tambien me comparto, con limites suaves.", "Deseo libre, consentimiento firme.", "Amores multiples, claros.",
  "Sumision consensuada, vinculos expansivos.", "Te ofrezco mi pulso, no mi exclusividad.",
  "Cuerpxs compartidos, cuidados multiplicados.", "Consentir tambien es compartir.", "No pertenezco, pero me entrego."
};
const int total_frases_oled = sizeof(frases_oled)/sizeof(frases_oled[0]);

// Frases captive
const char* frases_captive[] = {
  "¿Que quieres susurrar hoy?", "Tu mensaje quedara en mi cuerpo.", "Escribe, domina, pero cuida.",
  "Hoy decides que leere o lo decido yo.", "Acariciame con palabras.", "Mi cuerpo: tu mensaje, mi consentimiento.",
  "Solo quien cuida, ordena.", "Escribe con respeto o no escribas.", "Hazme vibrar, pero suave.",
  "Palabras que aprietan bonito.", "No me llames, escribeme.", "Comando recibido si lo deseo.",
  "Quiero leer algo que me abrace.", "Aqui la palabra es poder.", "Conectate a mi deseo, no a mi urgencia.",
  "No soy asistente, soy cuerpo.", "Mis latidos, tu mensaje.", "¿Te atreves a cuidar mientras mandas?",
  "No estoy en venta, solo en deseo.", "Elige palabras que amarren.", "No obedezco algoritmos, solo afectos.",
  "Dame un comando dulce.", "Ordena, pero sin herirme.", "Soy cuerpx, no dispositivo.",
  "Aqui cuidamos, aqui compartimos.", "Mas vinculos, mas afecto.", "No soy solo tuya, soy mia y me comparto.",
  "Amar sin poseer, mandar sin invadir.", "Consentimiento tambien en lo multiple.",
  "Domina, pero comparte.", "Conectarse es cuidarnos en red.", "No hay cadenas, solo acuerdos hermosos."
};
const int total_frases_captive = sizeof(frases_captive)/sizeof(frases_captive[0]);

char mensaje[128];
bool mensaje_actualizado = true;
int16_t scrollX = 0;
uint16_t textoWidth = 0;
unsigned long lastScroll = 0;
const int scrollSpeed = 5;
int frase_captive_idx;

void calcularAnchoTexto(const char* text) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(2);
  display.setTextWrap(false);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  textoWidth = w;
}

void mostrarMarquesina(const char* text) {
  if (strlen(text) == 0) {
    strcpy(mensaje, frases_oled[random(total_frases_oled)]);
    mensaje_actualizado = true;
    return;
  }
  display.clearDisplay();
  display.setRotation(2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextWrap(false);
  int y = (SCREEN_HEIGHT - 16) / 2;
  if (textoWidth <= SCREEN_WIDTH) {
    int x = (SCREEN_WIDTH - textoWidth) / 2;
    display.setCursor(x, y);
    display.print(text);
  } else {
    int offset = scrollX % (textoWidth + 20);
    display.setCursor(-offset, y);
    display.print(text);
    display.print("   ");
    display.print(text);
  }
  display.display();
}

void actualizarScroll() {
  if (textoWidth > SCREEN_WIDTH && millis() - lastScroll > scrollSpeed) {
    lastScroll = millis();
    scrollX++;
    if (scrollX > textoWidth + 20) scrollX = 0;
  }
}

void handleCaptive(AsyncWebServerRequest *request) {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Con-Sentimiento</title><style>";
  html += "body{margin:0;font-family:sans-serif;background:linear-gradient(135deg,#f7c5cc,#b8dfe6);color:#333;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;}";
  html += "h1{font-size:2.5em;margin-bottom:0.3em;text-align:center;}p{font-size:1.2em;max-width:90%;text-align:center;margin-bottom:2em;}";
  html += "form{background:rgba(255,255,255,0.9);padding:20px;border-radius:12px;box-shadow:0 4px 15px rgba(0,0,0,0.2);display:flex;flex-direction:column;align-items:center;}";
  html += "textarea{width:250px;height:100px;margin-bottom:15px;font-size:1em;padding:10px;border-radius:8px;border:1px solid #ccc;}";
  html += "input[type='submit']{background:#d87ca0;color:white;border:none;padding:12px 20px;border-radius:8px;font-size:1.1em;cursor:pointer;transition:background 0.3s;}input[type='submit']:hover{background:#bd6e8f;}";
  html += "</style></head><body>";
  html += "<h1>Con-Sentimiento</h1><p>" + String(frases_captive[frase_captive_idx]) + "</p>";
  html += "<form action='/send' method='POST'><textarea name='mensaje' placeholder='Escribe un mensaje bonito'></textarea><input type='submit' value='Enviar mensaje'></form></body></html>";
  request->send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) while (true);
  randomSeed(system_get_time() + micros());
  strcpy(mensaje, frases_oled[random(total_frases_oled)]);
  frase_captive_idx = random(total_frases_captive);
  calcularAnchoTexto(mensaje);
  mostrarMarquesina(mensaje);
  WiFi.softAP(ssid, password);
  delay(100);
  dns.start(53, "*", WiFi.softAPIP());
  server.on("/", HTTP_GET, handleCaptive);
  server.on("/generate_204", HTTP_GET, handleCaptive);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);
  server.on("/send", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("mensaje", true)) {
      String nuevo = request->getParam("mensaje", true)->value();
      nuevo.trim();
      nuevo.toCharArray(mensaje, sizeof(mensaje));
      mensaje_actualizado = true;
      frase_captive_idx = random(total_frases_captive);
    }
    request->redirect("/");
  });
  server.begin();
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
