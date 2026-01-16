/******************  ESP32 MIDI → MULTI-SOLENOIDS (LittleFS + WebUI + POWER)  ******************
 * - Phát tất cả track/kênh; NOTE ON (vel>0) kể cả running status
 * - Drum: CH10 (map trong MAP[])
 * - Flute: CH1; nốt C mặc định khi boot (đè 6 lỗ); giữ hơi 450ms (tự đóng bằng millis)
 * - Bitmask sáo: 1 = ĐÈ (giữ yên, KHÔNG kích), 0 = NHẢ (kích 1 nhịp)
 * - Chỉ kích relay những lỗ cần đổi trạng thái giữa 2 nốt (tránh “tạch” dư)
 * - Web UI: / (UI) /upload /play /stop /scan /kick?v= /kick_all
 *           /power?x= /getpower
 *           /flute_note?n=72|74|76|79|81
 *           /flute_map /flute_fingers
 ************************************************************************************************/



/* ================== PHẦN CÒN LẠI ================== 
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <vector>
*/


















