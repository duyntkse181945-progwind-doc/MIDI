#ifndef WEBHANDLE_INO  
#define WEBHANDLE_INO
#include <LittleFS.h>

/* ================== WEB & TRẠNG THÁI ================== */
WebServer server(80);
volatile bool isPlaying = false;         // chỉ để UI
volatile bool stopRequested = false;     // cờ dừng thực sự
volatile uint32_t g_noteon_count = 0;    // đếm NOTE ON để quyết định fallback

/* ================== WEB UI ================== */
static inline const char* noteName(uint8_t n){
  switch(n){
    case 72: return "C"; case 74: return "D"; case 76: return "E";
    case 79: return "G"; case 81: return "A";
    default: return "?";
  }
}

// Endpoints UI JSON
void handleFluteMap(){
  String s = "{\"holes\":[";
  for (int j=0; j<FLUTE_N; ++j){
    if (j) s += ",";
    s += String(SOL[FLUTE_BASE + j].pin);
  }
  s += "],\"air\":";
  s += String(SOL[FLUTE_AIR].pin);
  s += "}";
  server.send(200, "application/json", s);
}

void handleFluteFingers(){
  String s = "[";
  for (int i=0;i<FINGER_N;i++){
    if (i) s += ",";
    uint8_t mask = FINGER[i].mask;
    s += "{";
    s += "\"note\":" + String(FINGER[i].note);
    s += ",\"name\":\""; s += noteName(FINGER[i].note); s += "\"";
    s += ",\"mask\":" + String(mask);
    s += ",\"pins\":[";
    bool first=true;
    for (int j=0;j<FLUTE_N;j++){
      if ((mask >> j) & 1){
        if (!first) s += ",";
        first=false;
        s += String(SOL[FLUTE_BASE + j].pin);
      }
    }
    s += "]}";
  }
  s += "]";
  server.send(200, "application/json", s);
}

// Nút test sáo từ UI
void handleFluteNote() {
  if (!server.hasArg("n")) { server.send(400,"text/plain","missing n"); return; }
  uint8_t note = server.arg("n").toInt();
  Serial.printf("[FLUTE-TEST] note=%u\n", note);
  bool ok = fluteApplyNote(note);
  if (ok){ fluteAirSet(true); }  // tự đóng sau FLUTE_HOLD_MS
  server.send(200,"text/plain", ok ? "OK" : "SKIP");
}



/* ================== HANDLERS ================== */
void handleUpload(){
  HTTPUpload& up = server.upload();
  static File f;
  if (up.status == UPLOAD_FILE_START){
    if (LittleFS.exists(UI_FIXED_FILE)) LittleFS.remove(UI_FIXED_FILE);
    f = LittleFS.open(UI_FIXED_FILE,"w");
    if (!f){ server.send(500,"text/plain","Open LittleFS failed"); return; }
    Serial.println("[Upload] start " UI_FIXED_FILE);
  } else if (up.status == UPLOAD_FILE_WRITE){
    if (f) f.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END){
    if (f) f.close();
    Serial.printf("[Upload] done %u bytes\n", up.totalSize);
    server.send(200,"text/plain","Upload OK (go back)");
  } else if (up.status == UPLOAD_FILE_ABORTED){
    if (f) f.close(); LittleFS.remove(UI_FIXED_FILE);
    server.send(500,"text/plain","Upload aborted");
  }
}

void handleKick(){     // /kick?v=idx
  if (!server.hasArg("v")){ server.send(400,"text/plain","missing v"); return; }
  int v = server.arg("v").toInt();
  if (v<0 || v>=NUM_SOL){ server.send(400,"text/plain","bad v"); return; }
  Serial.printf("KICK sol%d\n", v);
  solBangBlocking((uint8_t)v, 100);
  server.send(200,"text/plain","kicked");
}
void handleKickAll(){
  Serial.println("KICK ALL");
  for (uint8_t v=0; v<NUM_SOL; ++v){ solBangBlocking(v, 100); delay(80); }
  server.send(200,"text/plain","kicked all");
}

/* ===== POWER handlers ===== */
void handlePower(){
  if (!server.hasArg("x")) { server.send(400,"text/plain","use /power?x=0.5..3.0"); return; }
  float x = server.arg("x").toFloat();
  if (x < 0.5f) x = 0.5f;
  if (x > 3.0f) x = 3.0f;
  g_power_boost = x;
  char buf[64]; dtostrf(x, 1, 2, buf);
  String s = "power="; s += buf; s += "x";
  server.send(200,"text/plain", s);
}
void handleGetPower(){
  char buf[32]; dtostrf(g_power_boost, 1, 2, buf);
  server.send(200,"text/plain", buf);
}

/* ============ TASK PHÁT MIDI ============ */
TaskHandle_t playTaskHandle = NULL;
void playTask(void*){
  Serial.println("▶ Play start");
  g_noteon_count = 0;           // đếm NOTE ON
  bool ok = playMIDI_AllNotes(UI_FIXED_FILE);
  isPlaying = false;
  stopRequested = false;
  for (uint8_t v=0; v<NUM_SOL; ++v){ solDrive(v, false); SOL[v].pulseActive=false; }
  fluteAirDefaultClosed();      // đảm bảo đóng hơi
  Serial.printf("⏹ Play end (ok=%d, NOTEON=%lu)\n", ok ? 1 : 0, (unsigned long)g_noteon_count);
  playTaskHandle = NULL;
  vTaskDelete(NULL);
}

void handlePlay(){
  if (!LittleFS.exists(UI_FIXED_FILE)){ server.send(404,"text/plain","No /song.mid"); return; }
  if (!isPlaying){
    isPlaying = true;
    stopRequested = false;
    xTaskCreate(playTask, "MIDIPlay", 16384, NULL, 1, &playTaskHandle);
    server.send(200,"text/plain","Playing (mixer)...");
  } else server.send(200,"text/plain","Already playing");
}
void handleScan(){
  if (!LittleFS.exists(UI_FIXED_FILE)){ server.send(404,"text/plain","No /song.mid"); return; }
  bool ok = scanMIDI_JustNotes(UI_FIXED_FILE);
  server.send(200,"text/plain", ok ? "Scan: NOTE ON found (check Serial)" : "Scan: NO NOTE ON");
}
void handleStop(){
  if (isPlaying){
    stopRequested = true;
    Serial.println("🛑 Stop requested");
  }
  for (uint8_t v=0; v<NUM_SOL; ++v){ solDrive(v, false); SOL[v].pulseActive=false; }
  fluteAirDefaultClosed();
  server.send(200,"text/plain","Stopped");
}
void handleStatus(){ server.send(200,"text/plain", isPlaying ? "Playing..." : "Idle"); }

/************************************************************************************************
 * WebHandle.ino
 * Xử lý WebServer endpoints:
 *           /play
 *           /scan
 *           /stop
 *           /status
 *           /upload  (POST)
 *           /kick?v=
 *           /kickall
// Xử lý 1 event (NOTE ON vel>0 → route)
static uint8_t processOneEvent(TrackBuf& tb, uint32_t& tempo_us_per_qn) {
  if (tb.ended) return 1;
  if (!tb.pending) { if (!scheduleNextEvent(tb)) return tb.ended ? 1 : 2; }
  if (tb.idx >= tb.len) { tb.ended = true; return 1; }

  uint8_t b = tb.data[tb.idx++];

  if (b < 0x80) {
    if (tb.run == 0) { return 2; }
    uint8_t hi = tb.run & 0xF0;
    uint8_t ch = tb.run & 0x0F;
    uint8_t d1 = b;
    if (hi == 0xC0 || hi == 0xD0) {
      // 1 data byte
    } else {
      if (tb.idx >= tb.len) { tb.ended = true; return 1; }
      uint8_t d2 = tb.data[tb.idx++];
      if (hi == 0x90 && d2 > 0) {
#if TRACE_NOTES
        Serial.printf("MIX NOTEON(rs) ch=%u note=%u vel=%u\n", ch+1, d1, d2);
#endif
        hitRoute(ch, d1, d2);
      }
    }
  } else if (b == 0xFF) {
    if (tb.idx >= tb.len) { tb.ended=true; return 1; }
    uint8_t type = tb.data[tb.idx++];
    uint32_t len=0; if (!bufReadVarLen(tb,len)) { tb.ended=true; return 1; }
    if (type==0x2F) { tb.ended = true; return 1; }              // End of Track
    if (type==0x51 && len==3 && tb.idx+3 <= tb.len) {           // SetTempo
      tempo_us_per_qn = ((uint32_t)tb.data[tb.idx]<<16) | ((uint32_t)tb.data[tb.idx+1]<<8) | tb.data[tb.idx+2];
#if TRACE_NOTES
      Serial.printf("META SetTempo %lu us/qn\n", (unsigned long)tempo_us_per_qn);
#endif
    }
    bufSkipN(tb, len);
    tb.run = 0;
  } else if (b == 0xF0 || b == 0xF7) {
    uint32_t len=0; if (!bufReadVarLen(tb,len)) { tb.ended=true; return 1; }
    bufSkipN(tb, len);
    tb.run = 0;
  } else {
    tb.run = b;
    uint8_t hi = b & 0xF0;
    uint8_t ch = b & 0x0F;
    if (hi == 0xC0 || hi == 0xD0) {               // Program/Channel Pressure
      if (tb.idx >= tb.len) { tb.ended=true; return 1; }
      (void)tb.data[tb.idx++];                    // ✅ đã sửa: không còn tb.idx sai phạm vi
    } else {
      if (tb.idx+1 >= tb.len) { tb.ended=true; return 1; }
      uint8_t d1 = tb.data[tb.idx++];
      uint8_t d2 = tb.data[tb.idx++];
      if (hi == 0x90 && d2 > 0) {
#if TRACE_NOTES
        Serial.printf("MIX NOTEON(st) ch=%u note=%u vel=%u\n", ch+1, d1, d2);
#endif
        hitRoute(ch, d1, d2);
      }
    }
  }
  tb.pending = false;
  return 0;
}

/* ======= Fallback player (tuần tự) ======= */
static bool playSimpleSequential(const char* path){
  Serial.println("🔁 Fallback: sequential player");
  File f = LittleFS.open(path, "r");
  if (!f){ Serial.println("❌ Fallback open fail"); return false; }

  char sig[4];
  if (f.read((uint8_t*)sig,4)!=4 || memcmp(sig,"MThd",4)!=0){ f.close(); return false; }
  uint32_t hdLen = readBE32(f);
  (void)readBE16(f);                   // fmt
  uint16_t ntr = readBE16(f);
  uint16_t div = readBE16(f);
  if (hdLen > 6) skipN(f, hdLen - 6);
  if (div & 0x8000){ f.close(); return false; } // SMPTE không hỗ trợ
  uint16_t ppqn = div;
  uint32_t tempo_us_per_qn = 500000;

  for (uint16_t t=0; t<ntr && !stopRequested; ++t){
    if (f.read((uint8_t*)sig,4)!=4 || memcmp(sig,"MTrk",4)!=0) { f.close(); return false; }
    uint32_t len = readBE32(f);
    uint32_t end = f.position() + len;
    uint8_t run = 0;

    while ((uint32_t)f.position() < end && !stopRequested){
      uint32_t dt = readVarLen(f);
      delayByTicks(dt, tempo_us_per_qn, ppqn);
      int ib = f.read(); if (ib<0) break;
      uint8_t b = (uint8_t)ib;

      if (b < 0x80){
        if (!run) continue;
        uint8_t hi = run & 0xF0;
        uint8_t ch = run & 0x0F;
        uint8_t d1 = b;
        if (hi==0xC0 || hi==0xD0) { /*done*/ }
        else {
          int d2i=f.read(); if (d2i<0) break;
          uint8_t d2=(uint8_t)d2i;
          if (hi==0x90 && d2>0) hitRoute(ch, d1, d2);
        }
      } else if (b==0xFF){
        int type=f.read(); if (type<0) break;
        uint32_t l=readVarLen(f);
        if (type==0x51 && l==3){
          uint8_t buf[3]; if (f.read(buf,3)==3){
            tempo_us_per_qn = ((uint32_t)buf[0]<<16)|((uint32_t)buf[1]<<8)|buf[2];
#if TRACE_NOTES
            Serial.printf("META SetTempo %lu us/qn\n", (unsigned long)tempo_us_per_qn);
#endif
          } else break;
        } else { if (!skipN(f,l)) break; }
        run=0;
      } else if (b==0xF0 || b==0xF7){
        uint32_t l=readVarLen(f); if (!skipN(f,l)) break; run=0;
      } else {
        run = b;
        uint8_t hi = b & 0xF0;
        uint8_t ch = b & 0x0F;
        if (hi==0xC0 || hi==0xD0){ (void)f.read(); }
        else {
          int d1i=f.read(); int d2i=f.read(); if (d1i<0||d2i<0) break;
          uint8_t d1=(uint8_t)d1i, d2=(uint8_t)d2i;
          if (hi==0x90 && d2>0) hitRoute(ch, d1, d2);
        }
      }
    }
    if ((uint32_t)f.position() < end) skipN(f, end - f.position());
  }
  f.close();
  return true;
}

/* ======== Mixer chính: phát tất cả track/kênh (route theo MAP/FLUTE) ======== */
static bool playMIDI_AllNotes(const char* path){
  File f = LittleFS.open(path, "r");
  if (!f){ Serial.println("❌ Cannot open MIDI"); return false; }
  size_t fsz = f.size();
  Serial.printf("ℹ️ File size = %u bytes\n", (unsigned)fsz);

  char sig[4];
  if (f.read((uint8_t*)sig,4)!=4 || memcmp(sig,"MThd",4)!=0){ Serial.println("❌ Not MThd"); f.close(); return false; }
  uint32_t hdLen = readBE32(f);
  uint16_t fmt = readBE16(f);
  uint16_t ntr = readBE16(f);
  uint16_t div = readBE16(f);
  Serial.printf("ℹ️ MThd len=%lu fmt=%u tracks=%u div=0x%04X\n",
                (unsigned long)hdLen, fmt, ntr, div);
  if (hdLen>6) skipN(f, hdLen-6);
  if (div & 0x8000){ Serial.println("❌ SMPTE timebase not supported"); f.close(); return false; }
  uint16_t ppqn = div;
  if (ntr == 0 || ntr > MAX_TRACKS){ Serial.println("❌ Unsupported track count"); f.close(); return false; }

  TrackBuf tracks[MAX_TRACKS];
  for (uint16_t t=0; t<ntr; ++t){
    if (f.read((uint8_t*)sig,4)!=4 || memcmp(sig,"MTrk",4)!=0){
      Serial.printf("❌ Track %u header missing\n", t); f.close(); return false;
    }
    uint32_t len = readBE32(f);
    Serial.printf("ℹ️ Track %u len=%lu\n", t, (unsigned long)len);
    tracks[t].data = (uint8_t*)malloc(len);
    if (!tracks[t].data){ Serial.println("❌ OOM while malloc track"); f.close(); return false; }
    tracks[t].len = len;
    int readed = f.read(tracks[t].data, len);
    if (readed != (int)len){ Serial.println("❌ Read track failed"); f.close(); return false; }
    tracks[t].idx = 0; tracks[t].run = 0; tracks[t].ended = false;
    tracks[t].pending = false; tracks[t].next_dt = 0; tracks[t].abs_tick = 0;
  }
  f.close();

  uint32_t tempo_us_per_qn = 500000;
  for (uint16_t t=0; t<ntr; ++t) scheduleNextEvent(tracks[t]);
  uint32_t last_tick = 0;

  // Mixer loop
  while (!stopRequested) {
    bool anyPending = false;
    uint32_t minTick = 0xFFFFFFFF;
    for (uint16_t t=0; t<ntr; ++t){
      if (!tracks[t].ended && tracks[t].pending){
        anyPending = true;
        if (tracks[t].abs_tick < minTick) minTick = tracks[t].abs_tick;
      }
    }
    if (!anyPending) {
      for (uint16_t t=0; t<ntr; ++t)
        if (!tracks[t].ended && !tracks[t].pending) scheduleNextEvent(tracks[t]);
      anyPending = false; minTick = 0xFFFFFFFF;
      for (uint16_t t=0; t<ntr; ++t){
        if (!tracks[t].ended && tracks[t].pending){
          anyPending = true;
          if (tracks[t].abs_tick < minTick) minTick = tracks[t].abs_tick;
        }
      }
      if (!anyPending) break;  // hết bài
    }

    if (minTick > last_tick) {
      uint32_t dt = minTick - last_tick;
      delayByTicks(dt, tempo_us_per_qn, ppqn);  // có gọi solServiceAll() + fluteService() bên trong
      if (stopRequested) break;
      last_tick = minTick;
    }

    for (uint16_t t=0; t<ntr; ++t){
      if (!tracks[t].ended && tracks[t].pending && tracks[t].abs_tick == minTick){
        uint8_t b = tracks[t].data[tracks[t].idx++];

        if (b == 0xFF) {  // Meta
          if (tracks[t].idx >= tracks[t].len) { tracks[t].ended=true; continue; }
          uint8_t type = tracks[t].data[tracks[t].idx++];
          uint32_t len=0; bufReadVarLen(tracks[t], len);
          if (type==0x2F) { tracks[t].ended = true; }
          else if (type==0x51 && len==3 && tracks[t].idx+3<=tracks[t].len) {
            tempo_us_per_qn = ((uint32_t)tracks[t].data[tracks[t].idx]<<16) | ((uint32_t)tracks[t].data[tracks[t].idx+1]<<8) | tracks[t].data[tracks[t].idx+2];
          }
          tracks[t].idx += len;
          tracks[t].pending = false;

        } else if (b == 0xF0 || b == 0xF7) { // SysEx skip
          uint32_t len=0; bufReadVarLen(tracks[t], len);
          tracks[t].idx += len;
          tracks[t].pending = false;

        } else { // MIDI channel event
          if (b < 0x80) {  // running status
            if (tracks[t].run == 0) { tracks[t].pending=false; continue; }
            uint8_t hi = tracks[t].run & 0xF0;
            uint8_t ch = tracks[t].run & 0x0F;
            uint8_t d1 = b;
            if (hi != 0xC0 && hi != 0xD0) {
              if (tracks[t].idx >= tracks[t].len) { tracks[t].ended=true; continue; }
              uint8_t d2 = tracks[t].data[tracks[t].idx++];
              if (hi == 0x90 && d2 > 0) hitRoute(ch, d1, d2);
            }
          } else {         // new status
            tracks[t].run = b;
            uint8_t hi = b & 0xF0;
            uint8_t ch = b & 0x0F;
            if (hi == 0xC0 || hi == 0xD0) {
              if (tracks[t].idx >= tracks[t].len) { tracks[t].ended=true; continue; }
              (void)tracks[t].data[tracks[t].idx++]; // ✅ fixed
            } else {
              if (tracks[t].idx+1 >= tracks[t].len) { tracks[t].ended=true; continue; }
              uint8_t d1 = tracks[t].data[tracks[t].idx++];
              uint8_t d2 = tracks[t].data[tracks[t].idx++];
              if (hi == 0x90 && d2 > 0) hitRoute(ch, d1, d2);
            }
          }
          tracks[t].pending = false;
        }
        scheduleNextEvent(tracks[t]);
      }
    }
  }

  for (uint16_t t=0; t<ntr; ++t){ if (tracks[t].data) free(tracks[t].data); }

  // CHỈ fallback nếu KHÔNG có NOTE ON nào
  if (!stopRequested && g_noteon_count == 0) {
    Serial.println("⚠️ No NOTE ON detected in mixer — trying fallback...");
    return playSimpleSequential(path);
  }
  return true;
}

/* ================== SCAN (không timing): test parser + đập blocking) ================== */
static bool scanMIDI_JustNotes(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f){ Serial.println("❌ Cannot open MIDI"); return false; }

  char sig[4];
  if (f.read((uint8_t*)sig,4)!=4 || memcmp(sig,"MThd",4)!=0){ Serial.println("❌ Not MThd"); f.close(); return false; }
  uint32_t hdLen = readBE32(f);
  (void)readBE16(f); // fmt
  uint16_t ntr = readBE16(f);
  (void)readBE16(f); // div
  if (hdLen>6) skipN(f, hdLen-6);

  uint32_t totalOn = 0;

  for (uint16_t t=0; t<ntr; ++t){
    if (f.read((uint8_t*)sig,4)!=4 || memcmp(sig,"MTrk",4)!=0){ Serial.println("❌ MTrk missing"); break; }
    uint32_t len = readBE32(f);
    uint32_t end = f.position() + len;

    uint8_t run = 0;
    while ((uint32_t)f.position() < end){
      (void)readVarLen(f); // bỏ delta
      int ib = f.read(); if (ib<0) break;
      uint8_t b = (uint8_t)ib;

      if (b < 0x80){
        if (!run) continue;
        uint8_t hi = run & 0xF0;
        uint8_t ch = run & 0x0F;
        uint8_t d1 = b;
        if (hi!=0xC0 && hi!=0xD0){
          int d2i=f.read(); if (d2i<0) break;
          uint8_t d2=(uint8_t)d2i;
          if (hi==0x90 && d2>0) {
            totalOn++;
            int8_t v = mapVoice(ch, d1);  // chỉ scan Drum để tiết kiệm coil sáo
            if (v>=0){
              Serial.printf("SCAN NOTEON ch=%u note=%u vel=%u -> sol=%d\n", ch+1, d1, d2, (int)v);
              solBangBlocking((uint8_t)v, d2);
            }
          }
        }
      } else if (b == 0xFF){
        int type=f.read(); if (type<0) break;
        uint32_t l=readVarLen(f); if (!skipN(f,l)) break;
        run = 0;
      } else if (b == 0xF0 || b == 0xF7){
        uint32_t l=readVarLen(f); if (!skipN(f,l)) break;
        run = 0;
      } else {
        run = b;
        uint8_t hi = b & 0xF0;
        uint8_t ch = b & 0x0F;
        if (hi==0xC0 || hi==0xD0){ (void)f.read(); }
        else {
          int d1i=f.read(); int d2i=f.read(); if (d1i<0||d2i<0) break;
          uint8_t d1=(uint8_t)d1i, d2=(uint8_t)d2i;
          if (hi==0x90 && d2>0) {
            totalOn++;
            int8_t v = mapVoice(ch, d1);
            if (v>=0){
              Serial.printf("SCAN NOTEON ch=%u note=%u vel=%u -> sol=%d\n", ch+1, d1, d2, (int)v);
              solBangBlocking((uint8_t)v, d2);
            }
          }
        }
      }
    }
    if ((uint32_t)f.position() < end) skipN(f, end - f.position());
  }
  f.close();
  Serial.printf("SCAN DONE: total NOTEON=%lu\n", (unsigned long)totalOn);
  return totalOn>0;
}
#endif;