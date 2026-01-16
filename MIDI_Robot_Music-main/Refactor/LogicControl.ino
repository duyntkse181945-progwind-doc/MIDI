#ifndef LOGICCONTROL_HPP
#define LOGICCONTROL_HPP
#include <Arduino.h>
#include <logiccontrol.hpp>
/* ================== DRUM MAP (CH10) ================== */
// Map trống phổ biến (Channel 10 = ch=9):
MapEntry MAP[] = {
  { 9, 36, 0 },   // Trống  -> sol0 (GPIO14)
  { 9, 56, 1 },   // Mỏ     -> sol1 (GPIO27)
  { 9, 42, 2 },   // Chũm chọe -> sol2 (GPIO26)
  { 9, 49, 3 },   // Chiêng -> sol3 (GPIO25)
  { 9, 37, 4 },   // Trống 2 -> sol4 (GPIO12)
};
const uint8_t MAP_N = sizeof(MAP)/sizeof(MAP[0]);

static inline int8_t mapVoice(uint8_t ch, uint8_t note){
  for (uint8_t i=0; i<MAP_N; ++i){
    bool okCh   = (MAP[i].ch   == 255) || (MAP[i].ch   == ch);
    bool okNote = (MAP[i].note == 255) || (MAP[i].note == note);
    if (okCh && okNote) return MAP[i].voice;
  }
  return -1;
}
// ===== FINGERING ====================================
Fingering FINGER[] = {
  {72, 0b00000000}, // C5 (Do):   không kích lỗ nào
  {74, 0b00100000}, // D5 (Re):   kích lỗ 6 (bit 5)
  {76, 0b00110000}, // E5 (Mi):   kích lỗ 5-6 (bit 5-4)
  {77, 0b00111000}, // F5 (Fa):   kích lỗ 4-5-6 (bit 5-3)
  {79, 0b00111100}, // G5 (Sol):  kích lỗ 3-4-5-6 (bit 5-2)
  {81, 0b00111110}, // A5 (La):   kích lỗ 2-3-4-5-6 (bit 5-1)
  {83, 0b00111111}, // B5 (Si):   kích lỗ 1-2-3-4-5-6 (tất cả)
};
const uint8_t FINGER_N = sizeof(FINGER)/sizeof(FINGER[0]);
/* ================== FLUTE STATE  ================== ================== */
// Trạng thái hơi
volatile bool     g_flute_air_open     = false;
volatile uint32_t g_flute_air_close_at = 0;

static inline void fluteAirSet(bool open){
  bool driveOn = FLUTE_AIR_BLOCK_ON ? !open : open;  // nếu van "đóng khi ON" thì đảo
  solDrive(FLUTE_AIR, driveOn);
  g_flute_air_open = open;
  if (open) g_flute_air_close_at = millis() + FLUTE_HOLD_MS;
}

static inline void fluteAirDefaultClosed(){ fluteAirSet(false); }

// Trạng thái giữ solenoid (kích ON và giữ đến hết time)

static FluteHold g_flute_holds[FLUTE_N];

// Khởi tạo hold system sáo
static inline void fluteHoldInit(){
  for (uint8_t i = 0; i < FLUTE_N; ++i){
    g_flute_holds[i].active = false;
    g_flute_holds[i].scheduled = false;
    g_flute_holds[i].holeIdx = i;
    g_flute_holds[i].onAt = 0;
    g_flute_holds[i].releaseAt = 0;
  }
}

// Service giữ solenoid: bật khi đến onAt, tắt khi đến releaseAt
static inline void fluteHoldService(){
  uint32_t now = millis();
  for (uint8_t i = 0; i < FLUTE_N; ++i){
    FluteHold &h = g_flute_holds[i];
    if (h.scheduled && (int32_t)(now - h.onAt) >= 0){
      // time to turn ON
      solDrive(FLUTE_BASE + i, true);
      h.active = true;
      h.scheduled = false;
      h.releaseAt = now + FLUTE_HOLD_MS;
#if TRACE_NOTES
      Serial.printf("FLUTE L%u ON (scheduled)\n", i+1);
#endif
    }
    if (h.active && (int32_t)(now - h.releaseAt) >= 0){
      solDrive(FLUTE_BASE + i, false);  // tắt solenoid
      h.active = false;
#if TRACE_NOTES
      Serial.printf("FLUTE L%u release\n", i+1);
#endif
    }
  }
}

// Áp dụng mask: kích ON những lỗ trong mask và giữ FLUTE_HOLD_MS
static inline void fluteApplyMask(uint8_t mask){
  uint32_t now = millis();
  // schedule staggered ON for each set bit to reduce simultaneous current
  uint8_t k = 0; // index of scheduled activation
  for (uint8_t i = 0; i < FLUTE_N; ++i){
    if ((mask >> i) & 1){  // bit = 1 → schedule this hole
      g_flute_holds[i].onAt = now + (uint32_t)k * FLUTE_STAGGER_MS;
      g_flute_holds[i].scheduled = true;
      g_flute_holds[i].active = false; // will be set when turned on
#if TRACE_NOTES
      Serial.printf("FLUTE L%u scheduled ON at +%ums\n", i+1, (unsigned)((uint32_t)k * FLUTE_STAGGER_MS));
#endif
      k++;
    }
  }
}

// Tìm mask theo note rồi áp dụng
static inline bool fluteApplyNote(uint8_t note){
  for (uint8_t i=0;i<FINGER_N;i++){
    if (FINGER[i].note == note){
      fluteApplyMask(FINGER[i].mask);
      return true;
    }
  }
  return false; // nốt ngoài ngũ cung → bỏ qua
}

// Service đóng hơi đúng hạn (không block) + hold solenoid
static inline void fluteService(){
  fluteHoldService();  // service hold solenoid trước
  
  if (g_flute_air_open && (int32_t)(millis() - g_flute_air_close_at) >= 0){
    fluteAirSet(false);
#if TRACE_NOTES
    Serial.println("FLUTE air auto-close");
#endif
  }
}
/* ================== ROUTE NOTE ================== */
static inline void hitRoute(uint8_t ch, uint8_t note, uint8_t vel){
  if (vel == 0) return;

  // --- Flute trên CH1 (ch == 0) ---
  if (ch == 0){
    bool ok = fluteApplyNote(note);
    if (ok){
      fluteAirSet(true);                 // mở hơi và đặt lịch đóng
      g_noteon_count++;
#if TRACE_NOTES
      Serial.printf("FLUTE ch=%u note=%u -> mask applied, HOLD=%ums\n", ch+1, note, (unsigned)FLUTE_HOLD_MS);
#endif
    } else {
#if TRACE_NOTES
      Serial.printf("FLUTE ch=%u note=%u -> SKIP (outside pentatonic)\n", ch+1, note);
#endif
    }
    return;
  }

  // --- Drum (map CH10) ---
  int8_t v = mapVoice(ch, note);
  if (v >= 0) {
#if TRACE_NOTES
    Serial.printf("DRUM ch=%u note=%u vel=%u -> sol=%d\n", ch+1, note, vel, (int)v);
#endif
    solHit((uint8_t)v, vel);
    g_noteon_count++;
  }
}

static inline uint16_t velToMs(uint8_t v){
  v = constrain(v, 1, 127);
  return map(v, 1, 127, 30, 120); // ms
}
#endif;
