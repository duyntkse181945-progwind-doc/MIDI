#ifndef WEBHANDLE_INO
#define WEBHANDLE_INO
#include <arduino.h>
#include <WiFi.h>
#include <WebServer.h>
volatile bool isPlaying;
volatile bool stopRequested;
volatile uint32_t g_noteon_count;
 uint8_t processOneEvent(TrackBuf& tb, uint32_t& tempo_us_per_qn);
bool scheduleNextEvent(TrackBuf& tb);
inline bool bufSkipN(TrackBuf& tb, uint32_t n);
inline bool bufReadByte(TrackBuf& tb, uint8_t& out);
bool bufReadVarLen(TrackBuf& tb, uint32_t& out);
bool playMIDI_AllNotes(const char* path);
bool playSimpleSequential(const char* path);
bool scanMIDI_JustNotes(const char* path);
void playTask(void*);
void handlePlay();
void handleScan();
void handleStop();
void handleStatus();
void handleUpload();
void handleKick();
void handleKickAll();
void handlePower();
void handleGetPower();
void handleFluteMap();
void handleFluteFingers();
void handleFluteNote();
inline const char* noteName(uint8_t n)


#endif;


