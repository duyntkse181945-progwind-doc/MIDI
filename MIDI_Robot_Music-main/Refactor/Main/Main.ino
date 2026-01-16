#ifndef MAININO
#define MAININO
#include <Arduino.h>
#include <LittleFS.h>
#include "Struct.hpp"
#include "Config.hpp"

#include "WebHandle.hpp"
/* ================== SETUP / LOOP ================== */
void setup(){
  Serial.begin(115200);
  Serial.printf("BUILD: %s %s\n", __DATE__, __TIME__);

  solInitAll();
  fluteHoldInit();  // khởi tạo hold system

  if (!LittleFS.begin(true)) Serial.println("❌ LittleFS mount failed");
  else                      Serial.println("✅ LittleFS ready");

  WiFi.softAP(AP_SSID, AP_PASS);
  delay(300);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", [](){ server.send(200,"text/html", INDEX_HTML); });
  server.on("/upload", HTTP_POST, [](){}, handleUpload);
  server.on("/play", handlePlay);
  server.on("/scan", handleScan);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.on("/kick", handleKick);
  server.on("/kick_all", handleKickAll);

  // POWER endpoints
  server.on("/power", handlePower);
  server.on("/getpower", handleGetPower);

  // Flute endpoints
  server.on("/flute_note", handleFluteNote);
  server.on("/flute_map", handleFluteMap);
  server.on("/flute_fingers", handleFluteFingers);

  server.begin();
  Serial.println("🌐 Web server ready");

  // --- Sáo mặc định: tất cả lỗ đã OFF khi boot ---
  fluteAirDefaultClosed();
  Serial.println("✅ Flute ready (all holes OFF)");
}

void loop(){
  server.handleClient();
  solServiceAll();   // tắt xung Drum đúng hạn
  fluteService();    // auto-close hơi khi đủ 450ms
}
#endif;
