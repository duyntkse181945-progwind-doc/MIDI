#ifndef LOGICCONTROL_HPP
#define LOGICCONTROL_HPP

#include <Arduino.h>
#include "Struct.hpp"
// ===== MAP =====
extern MapEntry MAP[];
extern const uint8_t MAP_N;
int8_t mapVoice(uint8_t ch, uint8_t note);

// ===== FINGERING =====        
extern Fingering FINGER[];
extern const uint8_t FINGER_N;

// ===== FLUTE STATE =====
extern volatile bool     g_flute_air_open;
extern volatile uint32_t g_flute_air_close_at;
extern FluteHold g_flute_holds[FLUTE_N];

// ===== NOTE STATE =====
extern volatile uint32_t g_noteon_count;

uint16_t velToMs(uint8_t v);
void fluteAirSet(bool open);
void fluteHoldInit();
void fluteHoldService();
void fluteAirDefaultClosed();
void fluteApplyMask(uint8_t mask);
bool fluteApplyNote(uint8_t note);
void fluteService();
void hitRoute(uint8_t ch, uint8_t note, uint8_t vel);

#endif
