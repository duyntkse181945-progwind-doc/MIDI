#ifndef SELENOID_CONTROL_HPP
#define SELENOID_CONTROL_HPP

#include <Arduino.h>

volatile float g_power_boost = 1.0f;
const uint8_t NUM_SOL = sizeof(SOL)/sizeof(SOL[0]);
extern Solenoid SOL[]
void solDrive(uint8_t v, bool on);
void solHit(uint8_t v, uint8_t vel);
void solServiceAll();
void solInitAll();
void solBangBlocking(uint8_t v, uint8_t vel);
#endif // SELENOID_CONTROL_HPP

