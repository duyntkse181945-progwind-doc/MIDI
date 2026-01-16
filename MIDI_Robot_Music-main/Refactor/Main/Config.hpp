#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <Arduino.h>
/* ================== CẤU HÌNH ================== */
const char* AP_SSID = "ESP32-MIDI";
const char* AP_PASS = "12345678";

#define MAX_TRACKS        24
#define TRACE_NOTES       1
#define DEBUG_RAW_EVENTS  0
#define UI_FIXED_FILE     "/song.mid"  // Play/Scan đọc file này

/* ================== FLUTE (CH1) ================== */
#define FLUTE_BASE 5       // SOL index bắt đầu của lỗ 1
#define FLUTE_N    6       // 6 lỗ
#define FLUTE_AIR  (FLUTE_BASE + FLUTE_N)  // index van hơi
const bool FLUTE_AIR_BLOCK_ON = true;     // true = van đóng khi coil ON (tuỳ van)

// Thời gian
#define FLUTE_HOLD_MS  450   // giữ solenoid ON khi chơi nốt (tự đóng bằng millis)
// Nếu bật nhiều solenoid cùng lúc, stagger time (ms) giữa các kích để giảm inrush current
#define FLUTE_STAGGER_MS 12

// Bitmask sáo: bit = 1 → kích solenoid lỗ đó ON. bit0=Hole1 … bit5=Hole6
// Ví dụ D5: kích lỗ 6 (bit 5 = 1) → 0b00100000

#endif // CONFIG_HPP
