#ifndef BI_IO_EXPANDER_H
#define BI_IO_EXPANDER_H

void initIOExpander();
void initIOExpanderFinalPath();
void beep(int duration_ms);
void resetMCU();

void setBackLightPercent(uint8_t percent);
void setBackLightOn();
void setBackLightOff();

bool getBatteryVoltage(uint16_t* batVoltage);
bool getChargerStatus(uint8_t* batVoltage);

#endif // BI_IO_EXPANDER_H