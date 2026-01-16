#ifndef MIDIENGINE_INO
#define MIDIENGINE_INO
#include "MidiEngine.hpp"

/* ================== HỖ TRỢ ĐỌC FILE ================== */
uint32_t readBE32(File &f){
  uint8_t b[4]; if (f.read(b,4)!=4) return 0;
  return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];
}
uint16_t readBE16(File &f){
  uint8_t b[2]; if (f.read(b,2)!=2) return 0;
  return ((uint16_t)b[0]<<8)|b[1];
}
uint32_t readVarLen(File &f){
  uint32_t value=0; uint8_t c;
  do{ if (f.read(&c,1)!=1) return value; value=(value<<7)|(c&0x7F);}while(c&0x80);
  return value;
}
bool skipN(File &f, uint32_t n){
  while(n>0){ uint8_t buf[64]; uint32_t k=min<uint32_t>(64,n); int r=f.read(buf,k); if(r<=0)return false; n-=r; }
  return true;
}
void delayByTicks(uint32_t dticks, uint32_t tempo_us_per_qn, uint16_t ppqn){
  if (!dticks) return;
  uint64_t us_total = (uint64_t)dticks * (uint64_t)tempo_us_per_qn / (uint64_t)ppqn;
  if (us_total >= 1000){
    uint32_t ms = us_total/1000;
    uint32_t start = millis();
    while((millis()-start) < ms){ solServiceAll(); fluteService(); delay(1); }
    us_total -= (uint64_t)ms*1000ULL;
  }
  if (us_total>0) delayMicroseconds((uint32_t)us_total);
}

/* ================== BỘ TRỘN (MIXER) ================== */
static bool bufReadVarLen(TrackBuf& tb, uint32_t& out) {
  out = 0;
  while (tb.idx < tb.len) {
    uint8_t c = tb.data[tb.idx++];
    out = (out << 7) | (c & 0x7F);
    if ((c & 0x80) == 0) return true;
  }
  return false;
}
static inline bool bufReadByte(TrackBuf& tb, uint8_t& out) {
  if (tb.idx >= tb.len) return false;
  out = tb.data[tb.idx++];
  return true;
}
static inline bool bufSkipN(TrackBuf& tb, uint32_t n) {
  if (tb.idx + n > tb.len) { tb.idx = tb.len; return false; }
  tb.idx += n; return true;
}
static bool scheduleNextEvent(TrackBuf& tb) {
  if (tb.ended || tb.pending) return tb.pending;
  if (tb.idx >= tb.len) { tb.ended = true; return false; }
  uint32_t dt=0;
  if (!bufReadVarLen(tb, dt)) { tb.ended = true; return false; }
  tb.next_dt = dt;
  tb.abs_tick += dt;
  tb.pending = true;
  return true;
}

#endif; 