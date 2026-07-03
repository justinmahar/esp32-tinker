#include <DNSServer.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <Preferences.h>
#include <SPI.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

#include "boot_network_display.h"
#include "programs/program.h"
#include "setup_html.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5

#define AP_SSID_PREFIX "ESP32-Tinker-Setup-"
#define WOKWI_GUEST_SSID "Wokwi-GUEST"
#define DNS_PORT 53
#define WIFI_TIMEOUT_MS 15000
#define WOKWI_SETUP_TIMEOUT_MS 8000

const bool ENABLE_WOKWI_SETUP = true;

const unsigned int DEFAULT_SCROLL_SPEED_MS = 75;
const unsigned int DEFAULT_FIREWORKS_MIN_LAUNCH_DELAY_MS = 60;
const unsigned int DEFAULT_FIREWORKS_MAX_LAUNCH_DELAY_MS = 2000;
const unsigned int DEFAULT_FIREWORKS_ANIM_SPEED_MS = 60;
const unsigned int MIN_MAZE_DIMENSION = 4;
const unsigned int MAX_MAZE_WIDTH = 80;
const unsigned int MAX_MAZE_HEIGHT = 48;
const unsigned int DEFAULT_MAZE_MIN_WIDTH = 8;
const unsigned int DEFAULT_MAZE_MAX_WIDTH = 40;
const unsigned int DEFAULT_MAZE_MIN_HEIGHT = 4;
const unsigned int DEFAULT_MAZE_MAX_HEIGHT = 24;
const unsigned int DEFAULT_MAZE_HERO_MIN_SPEED_MS = 200;
const unsigned int DEFAULT_MAZE_HERO_MAX_SPEED_MS = 200;
const uint8_t DEFAULT_DISPLAY_BRIGHTNESS = 0;
const uint8_t DEFAULT_FIREWORKS_MAX_BRIGHTNESS = 15;

MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

String saved_ssid, saved_pass;
ProgramConfig programConfig;
bool configMode = false;
bool captivePortalActive = false;
char setupMacSuffix[5] = "";
char setupScrollBuffer[48] = "";

void loadPrefs();
void savePrefs(const String &ssid, const String &pass,
               const ProgramConfig &cfg);
ProgramConfig buildProgramConfig();
String html_escape(String value);
String buildPage();
void registerRoutes();
void applyDisplayBrightness();
void getSetupMacSuffix(char *suffix, size_t size);
String getSetupApSsid();
void initSetupDisplay();
void updateSetupDisplay();
bool startWokwiConfigPortal();
void startConfigPortal();

void loadPrefs() {
  prefs.begin("esp32tinker", true);
  saved_ssid = prefs.getString("ssid", "");
  saved_pass = prefs.getString("pass", "");
  programConfig.program =
      parseProgramId(prefs.getString("program", "scroller"));
  programConfig.scrollMessage = prefs.getString("scrollMsg", "");
  programConfig.scrollSpeedMs = prefs.getUInt("scrollSpeed", 0);
  if (programConfig.scrollSpeedMs == 0) {
    programConfig.scrollSpeedMs = DEFAULT_SCROLL_SPEED_MS;
  }
  programConfig.fireworksMinLaunchDelayMs = prefs.getUInt("fwMinDelayMs", 0);
  if (programConfig.fireworksMinLaunchDelayMs == 0) {
    programConfig.fireworksMinLaunchDelayMs =
        DEFAULT_FIREWORKS_MIN_LAUNCH_DELAY_MS;
  }
  programConfig.fireworksMaxLaunchDelayMs = prefs.getUInt("fwMaxDelayMs", 0);
  if (programConfig.fireworksMaxLaunchDelayMs == 0) {
    programConfig.fireworksMaxLaunchDelayMs =
        DEFAULT_FIREWORKS_MAX_LAUNCH_DELAY_MS;
  }
  if (programConfig.fireworksMaxLaunchDelayMs <
      programConfig.fireworksMinLaunchDelayMs) {
    programConfig.fireworksMaxLaunchDelayMs =
        programConfig.fireworksMinLaunchDelayMs;
  }
  programConfig.fireworksAnimSpeedMs = prefs.getUInt("fwAnimMs", 0);
  if (programConfig.fireworksAnimSpeedMs == 0) {
    programConfig.fireworksAnimSpeedMs = DEFAULT_FIREWORKS_ANIM_SPEED_MS;
  }
  programConfig.mazeMinWidth = prefs.getUInt("mzMinW", 0);
  if (programConfig.mazeMinWidth < MIN_MAZE_DIMENSION) {
    programConfig.mazeMinWidth = DEFAULT_MAZE_MIN_WIDTH;
  }
  programConfig.mazeMaxWidth = prefs.getUInt("mzMaxW", 0);
  if (programConfig.mazeMaxWidth < programConfig.mazeMinWidth) {
    programConfig.mazeMaxWidth = DEFAULT_MAZE_MAX_WIDTH;
  }
  if (programConfig.mazeMaxWidth < programConfig.mazeMinWidth) {
    programConfig.mazeMaxWidth = programConfig.mazeMinWidth;
  }
  programConfig.mazeMinHeight = prefs.getUInt("mzMinH", 0);
  if (programConfig.mazeMinHeight < MIN_MAZE_DIMENSION) {
    programConfig.mazeMinHeight = DEFAULT_MAZE_MIN_HEIGHT;
  }
  programConfig.mazeMaxHeight = prefs.getUInt("mzMaxH", 0);
  if (programConfig.mazeMaxHeight < programConfig.mazeMinHeight) {
    programConfig.mazeMaxHeight = DEFAULT_MAZE_MAX_HEIGHT;
  }
  if (programConfig.mazeMaxHeight < programConfig.mazeMinHeight) {
    programConfig.mazeMaxHeight = programConfig.mazeMinHeight;
  }
  programConfig.mazeHeroMinSpeedMs = prefs.getUInt("mzHeroMinMs", 0);
  if (programConfig.mazeHeroMinSpeedMs == 0) {
    programConfig.mazeHeroMinSpeedMs = DEFAULT_MAZE_HERO_MIN_SPEED_MS;
  }
  programConfig.mazeHeroMaxSpeedMs = prefs.getUInt("mzHeroMaxMs", 0);
  if (programConfig.mazeHeroMaxSpeedMs < programConfig.mazeHeroMinSpeedMs) {
    programConfig.mazeHeroMaxSpeedMs = DEFAULT_MAZE_HERO_MAX_SPEED_MS;
  }
  if (programConfig.mazeHeroMaxSpeedMs < programConfig.mazeHeroMinSpeedMs) {
    programConfig.mazeHeroMaxSpeedMs = programConfig.mazeHeroMinSpeedMs;
  }
  programConfig.brightness =
      prefs.getUChar("brightness", DEFAULT_DISPLAY_BRIGHTNESS);
  programConfig.fireworksMaxBrightness =
      prefs.getUChar("fwMaxBright", DEFAULT_FIREWORKS_MAX_BRIGHTNESS);
  prefs.end();

  if (programConfig.brightness > 15) {
    programConfig.brightness = 15;
  }
  if (programConfig.fireworksMaxBrightness > 15) {
    programConfig.fireworksMaxBrightness = 15;
  }
  if (programConfig.fireworksMaxBrightness < programConfig.brightness) {
    programConfig.fireworksMaxBrightness = programConfig.brightness;
  }

  Serial.print("Config loaded: ssid=");
  Serial.print(saved_ssid.length() ? saved_ssid : "(empty)");
  Serial.print(", program=");
  Serial.print(programIdToString(programConfig.program));
  Serial.print(", scrollMsg=");
  Serial.print(programConfig.scrollMessage.length()
                   ? programConfig.scrollMessage
                   : "(default)");
  Serial.print(", scrollSpeed=");
  Serial.print(programConfig.scrollSpeedMs);
  Serial.print(", fwMinDelayMs=");
  Serial.print(programConfig.fireworksMinLaunchDelayMs);
  Serial.print(", fwMaxDelayMs=");
  Serial.print(programConfig.fireworksMaxLaunchDelayMs);
  Serial.print(", fwAnimMs=");
  Serial.print(programConfig.fireworksAnimSpeedMs);
  Serial.print(", mazeWidth=");
  Serial.print(programConfig.mazeMinWidth);
  Serial.print("-");
  Serial.print(programConfig.mazeMaxWidth);
  Serial.print(", mazeHeight=");
  Serial.print(programConfig.mazeMinHeight);
  Serial.print("-");
  Serial.print(programConfig.mazeMaxHeight);
  Serial.print(", mazeHeroMs=");
  Serial.print(programConfig.mazeHeroMinSpeedMs);
  Serial.print("-");
  Serial.print(programConfig.mazeHeroMaxSpeedMs);
  Serial.print(", brightness=");
  Serial.print(programConfig.brightness);
  Serial.print(", fwMaxBright=");
  Serial.println(programConfig.fireworksMaxBrightness);
}

void savePrefs(const String &ssid, const String &pass,
               const ProgramConfig &cfg) {
  prefs.begin("esp32tinker", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("program", programIdToString(cfg.program));
  prefs.putString("scrollMsg", cfg.scrollMessage);
  prefs.putUInt("scrollSpeed", cfg.scrollSpeedMs);
  prefs.putUInt("fwMinDelayMs", cfg.fireworksMinLaunchDelayMs);
  prefs.putUInt("fwMaxDelayMs", cfg.fireworksMaxLaunchDelayMs);
  prefs.putUInt("fwAnimMs", cfg.fireworksAnimSpeedMs);
  prefs.putUInt("mzMinW", cfg.mazeMinWidth);
  prefs.putUInt("mzMaxW", cfg.mazeMaxWidth);
  prefs.putUInt("mzMinH", cfg.mazeMinHeight);
  prefs.putUInt("mzMaxH", cfg.mazeMaxHeight);
  prefs.putUInt("mzHeroMinMs", cfg.mazeHeroMinSpeedMs);
  prefs.putUInt("mzHeroMaxMs", cfg.mazeHeroMaxSpeedMs);
  prefs.putUChar("brightness", cfg.brightness);
  prefs.putUChar("fwMaxBright", cfg.fireworksMaxBrightness);
  prefs.end();
}

ProgramConfig buildProgramConfig() { return programConfig; }

String html_escape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}

String buildPage() {
  String page = String(FPSTR(SETUP_PORTAL_HTML));
  String ip =
      configMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  page.replace("IP_PLACEHOLDER", ip);
  page.replace("SSID_PLACEHOLDER", html_escape(saved_ssid));
  page.replace("PROGRAM_PLACEHOLDER",
               html_escape(programIdToString(programConfig.program)));
  page.replace("SCROLL_MESSAGE_PLACEHOLDER",
               html_escape(programConfig.scrollMessage));
  page.replace("SCROLL_SPEED_PLACEHOLDER", String(programConfig.scrollSpeedMs));
  page.replace("FW_MIN_DELAY_PLACEHOLDER",
               String(programConfig.fireworksMinLaunchDelayMs));
  page.replace("FW_MAX_DELAY_PLACEHOLDER",
               String(programConfig.fireworksMaxLaunchDelayMs));
  page.replace("FW_ANIM_MS_PLACEHOLDER",
               String(programConfig.fireworksAnimSpeedMs));
  page.replace("MAZE_MIN_WIDTH_PLACEHOLDER",
               String(programConfig.mazeMinWidth));
  page.replace("MAZE_MAX_WIDTH_PLACEHOLDER",
               String(programConfig.mazeMaxWidth));
  page.replace("MAZE_MIN_HEIGHT_PLACEHOLDER",
               String(programConfig.mazeMinHeight));
  page.replace("MAZE_MAX_HEIGHT_PLACEHOLDER",
               String(programConfig.mazeMaxHeight));
  page.replace("MAZE_HERO_MIN_SPEED_PLACEHOLDER",
               String(programConfig.mazeHeroMinSpeedMs));
  page.replace("MAZE_HERO_MAX_SPEED_PLACEHOLDER",
               String(programConfig.mazeHeroMaxSpeedMs));
  page.replace("MIN_BRIGHTNESS_PLACEHOLDER", String(programConfig.brightness));
  page.replace("MAX_BRIGHTNESS_PLACEHOLDER",
               String(programConfig.fireworksMaxBrightness));
  return page;
}

void handleRoot() { server.send(200, "text/html", buildPage()); }

void handleSave() {
  ProgramConfig newConfig = programConfig;
  newConfig.program = parseProgramId(server.arg("program"));

  int displayBrightness = server.arg("brightness").toInt();
  if (displayBrightness < 0) {
    displayBrightness = 0;
  }
  if (displayBrightness > 15) {
    displayBrightness = 15;
  }
  newConfig.brightness = (uint8_t)displayBrightness;
  if (newConfig.fireworksMaxBrightness < newConfig.brightness) {
    newConfig.fireworksMaxBrightness = newConfig.brightness;
  }

  if (newConfig.program == ProgramId::Scroller) {
    String scrollMessage = server.arg("scrollMessage");
    scrollMessage.trim();
    if (scrollMessage.length() > MAX_SCROLL_MESSAGE_LENGTH) {
      server.send(400, "text/plain", "Scroll message is too long.");
      return;
    }

    unsigned int scrollSpeedMs = server.arg("scrollSpeed").toInt();
    if (scrollSpeedMs <= 0) {
      server.send(400, "text/plain", "Scroll speed must be greater than 0.");
      return;
    }

    newConfig.scrollMessage = scrollMessage;
    newConfig.scrollSpeedMs = scrollSpeedMs;
  } else if (newConfig.program == ProgramId::Fireworks) {
    unsigned int fireworksMinLaunchDelayMs =
        server.arg("fireworksMinDelay").toInt();
    unsigned int fireworksMaxLaunchDelayMs =
        server.arg("fireworksMaxDelay").toInt();
    if (fireworksMinLaunchDelayMs <= 0) {
      server.send(400, "text/plain",
                  "Min launch delay must be greater than 0.");
      return;
    }
    if (fireworksMaxLaunchDelayMs < fireworksMinLaunchDelayMs) {
      server.send(400, "text/plain",
                  "Max launch delay must be greater than or equal to min.");
      return;
    }

    unsigned int fireworksAnimSpeedMs = server.arg("fireworksAnimMs").toInt();
    if (fireworksAnimSpeedMs <= 0) {
      server.send(400, "text/plain", "Animation speed must be greater than 0.");
      return;
    }

    int fireworksMaxBrightness = server.arg("fireworksMaxBrightness").toInt();
    if (fireworksMaxBrightness < displayBrightness) {
      server.send(400, "text/plain",
                  "Max brightness must be greater than or equal to min.");
      return;
    }
    if (fireworksMaxBrightness > 15) {
      server.send(400, "text/plain", "Max brightness must be 0-15.");
      return;
    }

    newConfig.fireworksMinLaunchDelayMs = fireworksMinLaunchDelayMs;
    newConfig.fireworksMaxLaunchDelayMs = fireworksMaxLaunchDelayMs;
    newConfig.fireworksAnimSpeedMs = fireworksAnimSpeedMs;
    newConfig.fireworksMaxBrightness = (uint8_t)fireworksMaxBrightness;
  } else if (newConfig.program == ProgramId::MazeHero) {
    unsigned int mazeMinWidth = server.arg("mazeMinWidth").toInt();
    unsigned int mazeMaxWidth = server.arg("mazeMaxWidth").toInt();
    unsigned int mazeMinHeight = server.arg("mazeMinHeight").toInt();
    unsigned int mazeMaxHeight = server.arg("mazeMaxHeight").toInt();
    unsigned int mazeHeroMinSpeedMs = server.arg("mazeHeroMinSpeed").toInt();
    unsigned int mazeHeroMaxSpeedMs = server.arg("mazeHeroMaxSpeed").toInt();

    if (mazeMinWidth < MIN_MAZE_DIMENSION ||
        mazeMinHeight < MIN_MAZE_DIMENSION) {
      server.send(400, "text/plain", "Maze minimum size must be at least 4x4.");
      return;
    }
    if (mazeMaxWidth < mazeMinWidth || mazeMaxHeight < mazeMinHeight) {
      server.send(
          400, "text/plain",
          "Maze max width/height must be greater than or equal to min.");
      return;
    }
    if (mazeMaxWidth > MAX_MAZE_WIDTH || mazeMaxHeight > MAX_MAZE_HEIGHT) {
      server.send(400, "text/plain", "Maze max size is 80x48.");
      return;
    }
    if (mazeHeroMinSpeedMs <= 0) {
      server.send(400, "text/plain", "Hero min speed must be greater than 0.");
      return;
    }
    if (mazeHeroMaxSpeedMs < mazeHeroMinSpeedMs) {
      server.send(400, "text/plain",
                  "Hero max speed must be greater than or equal to min.");
      return;
    }

    newConfig.mazeMinWidth = mazeMinWidth;
    newConfig.mazeMaxWidth = mazeMaxWidth;
    newConfig.mazeMinHeight = mazeMinHeight;
    newConfig.mazeMaxHeight = mazeMaxHeight;
    newConfig.mazeHeroMinSpeedMs = mazeHeroMinSpeedMs;
    newConfig.mazeHeroMaxSpeedMs = mazeHeroMaxSpeedMs;
  }

  String new_ssid = server.arg("ssid");
  new_ssid.trim();
  if (new_ssid.length() == 0) {
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == WOKWI_GUEST_SSID) {
      new_ssid = WOKWI_GUEST_SSID;
    } else {
      new_ssid = saved_ssid;
    }
  }
  String new_pass = server.arg("pass");
  if (new_pass.length() == 0) {
    new_pass = saved_pass;
  }

  savePrefs(new_ssid, new_pass, newConfig);
  server.send(200, "text/plain", "Saved! Rebooting now...");
  delay(1500);
  ESP.restart();
}

void handleUpdate() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA start: %s\n", upload.filename.c_str());
    Display.print("OTA...");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA success: %u bytes\n", upload.totalSize);
      Display.print("Rebooting");
    } else {
      Update.printError(Serial);
      Display.print("OTA fail");
    }
  }
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) {
      json += ",";
    }
    json += "{\"ssid\":\"" + WiFi.SSID(i) +
            "\","
            "\"rssi\":" +
            String(WiFi.RSSI(i)) +
            ","
            "\"secure\":" +
            (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void handleUpdateFinish() {
  if (Update.hasError()) {
    server.send(500, "text/plain", Update.errorString());
  } else {
    server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  }
}

void registerRoutes() {
  server.on("/", handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/update", HTTP_POST, handleUpdateFinish, handleUpdate);
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
}

void applyDisplayBrightness() {
  Display.setIntensity(programConfig.brightness);
}

void getSetupMacSuffix(char *suffix, size_t size) {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(suffix, size, "%02X%02X", mac[4], mac[5]);
}

String getSetupApSsid() {
  char suffix[5];
  getSetupMacSuffix(suffix, sizeof(suffix));
  return String(AP_SSID_PREFIX) + suffix;
}

void initSetupDisplay() {
  getSetupMacSuffix(setupMacSuffix, sizeof(setupMacSuffix));
  snprintf(setupScrollBuffer, sizeof(setupScrollBuffer),
           "Connect to hotspot %s%s", AP_SSID_PREFIX, setupMacSuffix);
  Display.displayClear();
  Display.displayScroll(setupScrollBuffer, PA_LEFT, PA_SCROLL_LEFT, 80);
}

void updateSetupDisplay() {
  if (Display.displayAnimate()) {
    Display.displayReset();
  }
}

bool startWokwiConfigPortal() {
  Display.print(" Wokwi...");
  Serial.println("Trying Wokwi-GUEST for setup portal...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WOKWI_GUEST_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WOKWI_SETUP_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wokwi-GUEST not found.");
    return false;
  }

  configMode = true;
  registerRoutes();
  server.begin();

  Serial.println("Wokwi setup portal ready.");
  Serial.println("Open http://localhost:8180 in your browser.");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());

  initSetupDisplay();
  return true;
}

void startConfigPortal() {
  configMode = true;

  WiFi.mode(WIFI_AP);
  String setupApSsid = getSetupApSsid();
  WiFi.softAP(setupApSsid.c_str());
  delay(500);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  captivePortalActive = true;
  registerRoutes();
  server.begin();

  Serial.println("Config portal started.");
  Serial.print("AP SSID: ");
  Serial.println(setupApSsid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  initSetupDisplay();
}

bool connectToSavedWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(saved_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

  ProgramConfig cfg = buildProgramConfig();
  programStart(cfg);

  unsigned long start = millis();
  unsigned long lastDotMs = start;
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    programTick(cfg);
    if (millis() - lastDotMs >= 500) {
      Serial.print(".");
      lastDotMs = millis();
    }
    delay(10);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi failed.");
    return false;
  }

  Serial.println("");
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
  if (!ENABLE_WOKWI_SETUP) {
    showBootIpAddress(Display, WiFi.localIP());
    programStart(buildProgramConfig());
  }
  return true;
}

void setup() {
  Serial.begin(9600);

  Display.begin();
  Display.setIntensity(0);
  Display.setTextAlignment(PA_CENTER);

  loadPrefs();
  applyDisplayBrightness();

  if (saved_ssid.length() > 0) {
    if (connectToSavedWiFi()) {
      registerRoutes();
      server.begin();
    } else if (!ENABLE_WOKWI_SETUP || !startWokwiConfigPortal()) {
      startConfigPortal();
    }
  } else {
    Serial.println("No credentials. Starting config portal.");
    if (!ENABLE_WOKWI_SETUP || !startWokwiConfigPortal()) {
      startConfigPortal();
    }
  }
}

void loop() {
  if (configMode) {
    if (captivePortalActive) {
      dnsServer.processNextRequest();
    }
    server.handleClient();
    updateSetupDisplay();
    return;
  }

  server.handleClient();
  programTick(buildProgramConfig());
}
