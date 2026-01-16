#ifndef SELENOIDCONTROL_INOs
#define SELENOIDCONTROL_INO
#include "SelenoidControl.hpp"
/* ================== MULTI-SOLENOIDS ================== */

/* ====== POWER (hệ số lực đập cho solenoid dạng "hit") ====== */
volatile float g_power_boost = 1.0f;     // 0.5x..3.0x, mặc định 1.0x

static inline void solDrive(uint8_t v, bool on){
  if (v >= NUM_SOL) return;
  digitalWrite(SOL[v].pin, SOL[v].activeLow ? !on : on);
}
/* 
 * SỬA GPIO/activeLow CHO ĐÚNG PHẦN CỨNG CỦA BẠN:
 * - 0..4: Drum
 * - 5..10: Flute Hole1..Hole6 (1 = lỗ TRÊN, 6 = lỗ DƯỚI)
 * - 11:   Flute Air Valve (van hơi)
 */
Solenoid SOL[] = {
  {14, false, false, 0}, // 0: Drum Kick
  {27, false, false, 0}, // 1: Drum Snare
  {26, false, false, 0}, // 2: Drum Hi-hat
  {25, false, false, 0}, // 3: Drum Crash
  {12, false, false, 0}, // 4: Drum extra

  {33, false, false, 0}, // 5: Flute Hole1 (trên cùng)
  {32, false, false, 0}, // 6: Flute Hole2
  {23, false, false, 0}, // 7: Flute Hole3 (GPIO23)
  {22, false, false, 0}, // 8: Flute Hole4 (GPIO22)
  {21, false, false, 0}, // 9: Flute Hole5 (GPIO21)
  {19, false, false, 0}, // 10: Flute Hole6 (dưới cùng)
  {18, true,  false, 0}, // 11: Flute Air Valve (van hơi) - activeLow=true to keep closed at power
};
const uint8_t NUM_SOL = sizeof(SOL)/sizeof(SOL[0]);

/* ====== Helpers điều khiển solenoid ====== */
static inline void solHit(uint8_t v, uint8_t vel){  // non-blocking pulse (dùng cho Drum)
  if (v >= NUM_SOL) return;
  uint16_t base = velToMs(vel);
  uint16_t ms = (uint16_t)constrain((uint32_t)(base * g_power_boost), 5u, 400u);
#if TRACE_NOTES
  Serial.printf("SOL %u HIT vel=%u base=%ums power=%.2f -> %ums (pin=%u)\n",
                v, vel, base, g_power_boost, ms, SOL[v].pin);
#endif
  solDrive(v, true);
  SOL[v].pulseActive = true;
  SOL[v].offAt = millis() + ms;
}

static inline void solServiceAll(){
  uint32_t now = millis();
  for (uint8_t v=0; v<NUM_SOL; ++v){
    if (SOL[v].pulseActive && (int32_t)(now - SOL[v].offAt) >= 0){
      solDrive(v, false);
      SOL[v].pulseActive = false;
    }
  }
}
static inline void solInitAll(){
  for (uint8_t v=0; v<NUM_SOL; ++v){
    pinMode(SOL[v].pin, OUTPUT);
    solDrive(v, false);
    SOL[v].pulseActive = false;
  }
}

/* Xung blocking: dùng cho /scan, /kick */
static inline void solBangBlocking(uint8_t v, uint8_t vel){
  if (v >= NUM_SOL) return;
  uint16_t base = velToMs(vel);
  uint16_t ms = (uint16_t)constrain((uint32_t)(base * g_power_boost), 5u, 400u);
  solDrive(v, true);
  delay(ms);
  solDrive(v, false);
  delay(40);
}
#endif;