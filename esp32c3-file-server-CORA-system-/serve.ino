#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <HTTPClient.h>

// ===== PINOS =====
#define TFT_RST   9
#define TFT_CS    10
#define TFT_DC    8
#define TFT_MOSI  7
#define TFT_SCLK  4

#define SD_CS     1
#define SD_MISO   5

// ===== WIFI =====
const char* ssid = "CAI FORA";
const char* password = "c9246ymr";

// ===== DUCKDNS =====
const char* duckToken  = "d433a33e-87ec-4d88-ba7b-759db0bbb905";   // coloque seu token DuckDNS
const char* duckDomain = "myserve32";        // seu domínio DuckDNS (sem .duckdns.org)

// ===== LOGIN =====
const char* www_username = "admin";
const char* www_password = "1234";

// ===== SPI =====
SPIClass spi = SPIClass(FSPI);

// ===== TELA =====
Adafruit_ST7735 tft = Adafruit_ST7735(&spi, TFT_CS, TFT_DC, TFT_RST);

// ===== SERVER =====
WebServer server(80);
File uploadFile;

// ===== CONTROLE DE PASTA =====
String currentPath = "/";

// ===== FUNÇÕES SD =====
String formatMB(uint64_t bytes) {
  return String(bytes / (1024 * 1024)) + " MB";
}

uint64_t sdTotal() { return SD.totalBytes(); }
uint64_t sdUsed()  { return SD.usedBytes(); }
uint64_t sdFree()  { return sdTotal() - sdUsed(); }

// ===== REMOÇÃO RECURSIVA =====
bool removeRecursive(String path) {
  File file = SD.open(path);
  if (!file) return false;

  if (!file.isDirectory()) {
    file.close();
    return SD.remove(path);
  }

  File entry = file.openNextFile();
  while (entry) {
    String entryPath = path + "/" + entry.name();
    entry.close();
    removeRecursive(entryPath);
    entry = file.openNextFile();
  }

  file.close();
  return SD.rmdir(path);
}

// ===== LISTAR DIRETÓRIO =====
String listDirHTML(const String& path) {
  String html = "<h3>Pasta: " + path + "</h3><ul>";

  if (path != "/") {
    String parent = path.substring(0, path.lastIndexOf('/'));
    if (parent == "") parent = "/";
    html += "<li><a href='/?path=" + parent + "'>📁 ..</a></li>";
  }

  File dir = SD.open(path);
  File file = dir.openNextFile();

  while (file) {
    String name = file.name();

    if (file.isDirectory()) {
      html += "<li>📁 <a href='/?path=" + String(name) + "'>" + String(name) + "</a> ";
      html += "<a href='/delete?path=" + path + "&name=" + name + "'>❌</a></li>";
    } else {
      html += "<li>📄 " + String(name);
      html += " <a href='/download?path=" + path + "&file=" + name + "'>⬇</a>";
      html += " <a href='/delete?path=" + path + "&name=" + name + "'>❌</a></li>";
    }

    file = dir.openNextFile();
  }

  html += "</ul>";
  return html;
}

// ===== AUTENTICAÇÃO =====
bool isAuthenticated() {
  if (!server.authenticate(www_username, www_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// ===== PÁGINA PRINCIPAL =====
void handleRoot() {
  if (!isAuthenticated()) return;

  if (server.hasArg("path")) currentPath = server.arg("path");

  String html = "<!DOCTYPE html><html><body>";
  html += "<h2>Servidor ESP32-C3</h2>";
  html += "<p>Usuário autenticado ✅</p>";

  html += "<p><b>Total:</b> " + formatMB(sdTotal()) + "</p>";
  html += "<p><b>Usado:</b> " + formatMB(sdUsed()) + "</p>";
  html += "<p><b>Livre:</b> " + formatMB(sdFree()) + "</p>";

  html += listDirHTML(currentPath);
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ===== CRIAR PASTA =====
void handleMkdir() {
  if (!isAuthenticated()) return;

  String path = server.arg("path");
  String name = server.arg("name");

  SD.mkdir(path + "/" + name);
  server.sendHeader("Location", "/?path=" + path);
  server.send(302, "text/plain", "");
}

// ===== EXCLUIR =====
void handleDelete() {
  if (!isAuthenticated()) return;

  String path = server.arg("path");
  String name = server.arg("name");
  String fullPath = path + "/" + name;

  removeRecursive(fullPath);

  tft.println("Excluido:");
  tft.println(fullPath);

  server.sendHeader("Location", "/?path=" + path);
  server.send(302, "text/plain", "");
}

// ===== DOWNLOAD =====
void handleDownload() {
  if (!isAuthenticated()) return;

  String path = server.arg("path");
  String fileName = server.arg("file");
  String fullPath = path + "/" + fileName;

  if (!SD.exists(fullPath)) {
    server.send(404, "text/plain", "Arquivo nao encontrado");
    return;
  }

  File file = SD.open(fullPath, FILE_READ);
  server.streamFile(file, "application/octet-stream");
  file.close();
}

// ===== UPLOAD =====
void handleUpload() {
  if (!isAuthenticated()) return;

  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadFile = SD.open(currentPath + "/" + upload.filename, FILE_WRITE);
    tft.println("Recebendo:");
    tft.println(upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    tft.println("Concluido");
  }
}

// ===== DUCKDNS UPDATE =====
void updateDuckDNS() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://www.duckdns.org/update?domains=" + String(duckDomain) + "&token=" + String(duckToken) + "&ip=";
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("DuckDNS atualizado!");
      tft.println("DuckDNS OK");
    } else {
      Serial.println("Falha DuckDNS");
      tft.println("DuckDNS ERRO");
    }
    http.end();
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  spi.begin(TFT_SCLK, SD_MISO, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 0);
  tft.println("Inicializando");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }

  tft.println("\nWiFi OK");
  tft.println(WiFi.localIP());

  if (!SD.begin(SD_CS, spi)) {
    tft.println("Erro SD");
    while (true);
  }

  tft.println("SD OK");
  tft.print("Livre: ");
  tft.println(formatMB(sdFree()));

  server.on("/", HTTP_GET, handleRoot);
  server.on("/mkdir", HTTP_GET, handleMkdir);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/upload", HTTP_POST, []() { server.send(200, "text/plain", "OK"); }, handleUpload);

  server.begin();
  tft.println("Servidor ON");
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 300000) { // a cada 5 minutos
    updateDuckDNS();
    lastUpdate = millis();
  }
}
