#ifndef STRUCT_HPP
#define STRUCT_HPP
#include <Arduino.h>

 
/* ====== TrackBuf (đặt trước để tránh lỗi prototype của Arduino) ====== */
#include <stdint.h>
struct TrackBuf {
  uint8_t* data = nullptr;
  uint32_t len  = 0;
  uint32_t idx  = 0;
  uint8_t  run  = 0;       // running status
  bool     ended   = false;
  bool     pending = false;
  uint32_t next_dt = 0;    // delta ticks của event đang chờ
  uint32_t abs_tick= 0;    // tổng tick tính từ đầu track
};
/* ================== MULTI-SOLENOIDS ================== */
struct Solenoid {
  uint8_t  pin;
  bool     activeLow;
  volatile bool     pulseActive;
  volatile uint32_t offAt;
};
/* ch: 0..15 (kênh 1..16), 255 = mọi kênh; note: 0..127, 255 = mọi nốt; voice: index SOL[] */
struct MapEntry { uint8_t ch; uint8_t note; int8_t voice; };
struct Fingering { uint8_t note; uint8_t mask; };
struct FluteHold {
  uint8_t  holeIdx;     // 0..5 (hole index)
  uint32_t onAt;        // millis khi sẽ bật ON (stagger scheduling)
  uint32_t releaseAt;   // millis khi hết thời gian giữ
  bool     scheduled;   // đã được đặt lịch bật chưa
  bool     active;      // đang ON hay không
};
#endif