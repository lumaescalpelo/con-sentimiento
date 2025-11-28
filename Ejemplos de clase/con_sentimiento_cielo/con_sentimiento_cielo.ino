/*
Con-Sentimiento.
Fecha: 2025-11-23
By: Luma Escalpelo
Para: e-Cuerpo

Versión ESP32 DevKitV1 + SSD1306 128x32 + Anillos NeoPixel
Core probado: ESP32 3.3.4
Librerías async: AsyncTCP / ESPAsyncWebServer (ESP32Async)
LEDs: FastLED + efectos tipo DemoReel100
*/

// --- INCLUDES BÁSICOS ---
#include <WiFi.h>               // WiFi para ESP32
#include <DNSServer.h>
#include <AsyncTCP.h>           // by ESP32Async
#include <ESPAsyncWebServer.h>  // by ESP32Async

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <FastLED.h>            // FastLED 3.9.20 By Daniel García y FastLED NeoPixel by David Madison
#include <esp_system.h>         // esp_random()

// -------------------------
// CONFIGURACIÓN OLED
// -------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------------------------
// CONFIGURACIÓN WiFi / DNS
// -------------------------
const char* ssid     = "con-sentimiento01";
const char* password = "amor01";

DNSServer dns;
AsyncWebServer server(80);

// -------------------------
// CONFIGURACIÓN LEDs (FastLED)
// -------------------------
#define LED_PIN      13
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB
#define ROTATIONS    0 // 0=normal, 1=90°, 2=180°, 3=270°

const uint8_t  NUM_ACCESORIOS_LED  = 1;   // p.ej. 1 anillo
const uint16_t LEDS_POR_ACCESORIO  = 16;  // p.ej. 16 LEDs por anillo
const uint16_t NUM_LEDS_TOTAL      = NUM_ACCESORIOS_LED * LEDS_POR_ACCESORIO;
#define BRILLO 64

CRGB leds[NUM_LEDS_TOTAL];

// -------------------------
// TEXTOS Y ESTADO OLED
//--------------------------
const char* frases_oled[] = {
  "Mas amor, menos prisa.", "Este cuerpo elige cuando.", "Hoy solo recibo dulzura."
};
const int total_frases_oled = sizeof(frases_oled)/sizeof(frases_oled[0]);

// Frases captive
const char* frases_captive[] = {
  "Tu mensaje quedara en mi cuerpo.", "Escribe, domina, pero cuida.",
  "Hoy decides que leere o lo decido yo."
};
const int total_frases_captive = sizeof(frases_captive)/sizeof(frases_captive[0]);

char mensaje[128];
bool mensaje_actualizado = true;
int16_t  scrollX     = 0;
uint16_t textoWidth  = 0;
unsigned long lastScroll = 0;
const int scrollSpeed = 5;
int frase_captive_idx;

// -------------------------
// CONFIGURACIÓN COLORES / EFECTOS (FastLED DemoReel-like)
// -------------------------

enum EffectType {
  FX_STATIC = 0,          // color fijo
  FX_BLINK,               // parpadeo simple
  FX_RAINBOW,             // arcoiris
  FX_RAINBOW_GLITTER,     // color + glitter (ya no forzamos arcoiris)
  FX_CONFETTI,
  FX_SINELON,
  FX_JUGGLE,
  FX_BPM,
  FX_COUNT
};

// Estado actual del anillo
CRGB      currentColor       = CRGB::White;
EffectType currentEffect     = FX_STATIC;
uint8_t   currentBrightness  = BRILLO;  // 0–255

bool       blinkOn           = true;
unsigned long lastLedUpdate  = 0;
const uint16_t blinkInterval = 300; // ms

// Tono global, anclado al color elegido pero animado con el tiempo
uint8_t gHue = 0;

// Palabras clave de colores
struct ColorKeyword {
  const char* word;
  CRGB color;
};

ColorKeyword colorKeywords[] = {
  {"lila",      CRGB(180, 100, 255)},
  {"rosa",      CRGB(255,  80, 160)},
  {"rojo",      CRGB(255,   0,   0)},
  {"naranja",   CRGB(255, 120,   0)},
  {"amarillo",  CRGB(255, 200,   0)},
  {"verde",     CRGB(  0, 200,  60)},
  {"turquesa",  CRGB( 30, 200, 180)},
  {"cian",      CRGB( 50, 220, 255)},
  {"azul",      CRGB(  0,  80, 255)},
  {"morado",    CRGB(160,  60, 230)},
  {"blanco",    CRGB(255, 255, 255)},
  {"negro",     CRGB(  0,   0,   0)}
};
const int NUM_COLOR_KEYWORDS = sizeof(colorKeywords)/sizeof(colorKeywords[0]);

// Palabras clave de efectos
struct EffectKeyword {
  const char* word;
  EffectType effect;
};

EffectKeyword effectKeywords[] = {
  {"fijo",        FX_STATIC},
  {"estatico",    FX_STATIC},
  {"static",      FX_STATIC},

  {"parpadear",   FX_BLINK},
  {"parpadeo",    FX_BLINK},
  {"blink",       FX_BLINK},
  {"titilar",     FX_BLINK},

  {"arcoiris",    FX_RAINBOW},
  {"rainbow",     FX_RAINBOW},

  {"glitter",     FX_RAINBOW_GLITTER},
  {"chispa",      FX_RAINBOW_GLITTER},
  {"chispas",     FX_RAINBOW_GLITTER},

  {"confeti",     FX_CONFETTI},
  {"confetti",    FX_CONFETTI},

  {"sinelon",     FX_SINELON},
  {"onda",        FX_SINELON},

  {"juggle",      FX_JUGGLE},
  {"bpm",         FX_BPM}
};
const int NUM_EFFECT_KEYWORDS = sizeof(effectKeywords)/sizeof(effectKeywords[0]);

// -------------------------
// Estructura que representa la "decisión" de la frase
// -------------------------
struct ParsedCommand {
  CRGB color;
  EffectType effect;
  uint8_t brightness;
};

// Prototipos
ParsedCommand interpretarMensaje(const String& original);
void procesarMensajeLeds(const String& msg);

// -------------------------
// FUNCIONES OLED
// -------------------------
void calcularAnchoTexto(const char* text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setRotation(ROTATIONS);   
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
  display.setRotation(ROTATIONS);
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

// -------------------------
// FUNCIONES AUXILIARES PARA FASTLED
// -------------------------

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS_TOTAL)] += CRGB::White;
  }
}

// Arcoiris "clásico", pero gHue viene del color elegido
void effectRainbow() {
  fill_rainbow(leds, NUM_LEDS_TOTAL, gHue, 7);
}

// Glitter basado en currentColor (no forzamos arcoiris)
void effectRainbowWithGlitter() {
  fill_solid(leds, NUM_LEDS_TOTAL, currentColor);
  addGlitter(80);
}

void effectConfetti() {
  fadeToBlackBy(leds, NUM_LEDS_TOTAL, 10);
  int pos = random16(NUM_LEDS_TOTAL);
  // gHue anclado al color base, random alrededor
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void effectSinelon() {
  fadeToBlackBy(leds, NUM_LEDS_TOTAL, 20);
  int pos = beatsin16(13, 0, NUM_LEDS_TOTAL - 1);
  leds[pos] += CHSV(gHue, 255, 255);
}

void effectJuggle() {
  fadeToBlackBy(leds, NUM_LEDS_TOTAL, 20);
  byte dothue = gHue;  // arranca desde el tono del color actual
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS_TOTAL - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// BPM que respeta el color elegido
void effectBpm() {
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);

  CHSV base = rgb2hsv_approximate(currentColor); // tono del color actual

  for (int i = 0; i < NUM_LEDS_TOTAL; i++) {
    uint8_t offset    = (i * 255) / NUM_LEDS_TOTAL;
    uint8_t localBeat = beat - offset;
    leds[i] = CHSV(base.h, base.s, localBeat);
  }
}

// -------------------------
// Hash FNV-1a para que los textos sí cambien
// -------------------------
uint32_t hashFrase(const String& s) {
  uint32_t h = 2166136261u; // FNV offset basis
  for (size_t i = 0; i < s.length(); i++) {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h;
}

// Cuenta ocurrencias de un substring en una String
int contarOcurrencias(const String& texto, const String& palabra) {
  if (palabra.length() == 0) return 0;
  int count = 0;
  int index = texto.indexOf(palabra);
  while (index != -1) {
    count++;
    index = texto.indexOf(palabra, index + palabra.length());
  }
  return count;
}

// Interpreta cualquier frase y decide color, efecto y brillo
ParsedCommand interpretarMensaje(const String& original) {
  ParsedCommand cmd;
  String msg = original;
  msg.toLowerCase();

  uint32_t h = hashFrase(msg);

  // --- Colores ---
  int colorCounts[NUM_COLOR_KEYWORDS] = {0};
  int totalColorHits = 0;
  for (int i = 0; i < NUM_COLOR_KEYWORDS; i++) {
    int c = contarOcurrencias(msg, String(colorKeywords[i].word));
    colorCounts[i] = c;
    totalColorHits += c;
  }

  int bestColorIdx = -1;
  int bestColorCount = 0;
  for (int i = 0; i < NUM_COLOR_KEYWORDS; i++) {
    if (colorCounts[i] > bestColorCount) {
      bestColorCount = colorCounts[i];
      bestColorIdx = i;
    }
  }

  if (bestColorIdx == -1) {
    bestColorIdx = h % NUM_COLOR_KEYWORDS;
  }
  cmd.color = colorKeywords[bestColorIdx].color;

  // --- Efectos ---
  int effectCounts[NUM_EFFECT_KEYWORDS] = {0};
  int totalEffectHits = 0;
  for (int i = 0; i < NUM_EFFECT_KEYWORDS; i++) {
    int c = contarOcurrencias(msg, String(effectKeywords[i].word));
    effectCounts[i] = c;
    totalEffectHits += c;
  }

  EffectType chosenEffect = FX_STATIC;
  int bestEffectIdx = -1;
  int bestEffectCount = 0;
  for (int i = 0; i < NUM_EFFECT_KEYWORDS; i++) {
    if (effectCounts[i] > bestEffectCount) {
      bestEffectCount = effectCounts[i];
      bestEffectIdx = i;
    }
  }

  if (bestEffectIdx != -1) {
    chosenEffect = effectKeywords[bestEffectIdx].effect;
  } else {
    chosenEffect = (EffectType)(h % FX_COUNT);
  }

  cmd.effect = chosenEffect;

  // --- Brillo ---
  int totalHits = totalColorHits + totalEffectHits;
  if (totalHits > 0) {
    int b = 60 + totalHits * 40;
    if (b > 255) b = 255;
    cmd.brightness = (uint8_t)b;
  } else {
    cmd.brightness = 120 + (h % 80); // 120–199 aprox
  }

  return cmd;
}

// Aplica el comando parsed al estado global de LEDs
void procesarMensajeLeds(const String& msg) {
  ParsedCommand cmd = interpretarMensaje(msg);
  currentColor      = cmd.color;
  currentEffect     = cmd.effect;
  currentBrightness = cmd.brightness;

  // Anclar gHue al color elegido
  CHSV hsv = rgb2hsv_approximate(currentColor);
  gHue = hsv.h;
}

// Actualiza la animación de los LEDs (no bloqueante)
void actualizarAnimacionLeds() {
  // Animación de tono global
  EVERY_N_MILLISECONDS(20) { gHue++; }

  FastLED.setBrightness(currentBrightness);

  switch (currentEffect) {
    case FX_STATIC:
      fill_solid(leds, NUM_LEDS_TOTAL, currentColor);
      break;

    case FX_BLINK: {
      unsigned long now = millis();
      if (now - lastLedUpdate > blinkInterval) {
        lastLedUpdate = now;
        blinkOn = !blinkOn;
      }
      if (blinkOn) fill_solid(leds, NUM_LEDS_TOTAL, currentColor);
      else         fill_solid(leds, NUM_LEDS_TOTAL, CRGB::Black);
      break;
    }

    case FX_RAINBOW:
      effectRainbow();
      break;

    case FX_RAINBOW_GLITTER:
      effectRainbowWithGlitter();
      break;

    case FX_CONFETTI:
      effectConfetti();
      break;

    case FX_SINELON:
      effectSinelon();
      break;

    case FX_JUGGLE:
      effectJuggle();
      break;

    case FX_BPM:
      effectBpm();
      break;

    default:
      fill_solid(leds, NUM_LEDS_TOTAL, currentColor);
      break;
  }

  FastLED.show();
}

// -------------------------
// FUNCIONES CAPTIVE PORTAL
// -------------------------
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

// -------------------------
// SETUP Y LOOP
// -------------------------
void setup() {
  Serial.begin(115200);

  // OLED
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true); // Si falla la pantalla, nos quedamos aquí
  }

  // FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS_TOTAL).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(currentBrightness);
  fill_solid(leds, NUM_LEDS_TOTAL, CRGB::Black);
  FastLED.show();

  // Aleatoriedad robusta
  randomSeed(esp_random());

  // Mensaje inicial OLED
  strcpy(mensaje, frases_oled[random(total_frases_oled)]);
  frase_captive_idx = random(total_frases_captive);
  calcularAnchoTexto(mensaje);
  mostrarMarquesina(mensaje);

  // Mensaje inicial para LEDs (usa la misma frase)
  procesarMensajeLeds(String(mensaje));

  // WiFi AP + DNS + servidor
  WiFi.softAP(ssid, password);
  delay(100);
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleCaptive);
  server.on("/generate_204", HTTP_GET, handleCaptive);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);

  // Endpoint de envío
  server.on("/send", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("mensaje", true)) {
      String nuevo = request->getParam("mensaje", true)->value();
      nuevo.trim();
      nuevo.toCharArray(mensaje, sizeof(mensaje));
      mensaje_actualizado = true;
      frase_captive_idx = random(total_frases_captive);

      // Interpretar la frase para colores/efectos
      procesarMensajeLeds(nuevo);
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

  // Actualizar animación LED (FastLED)
  actualizarAnimacionLeds();

  yield();
}
