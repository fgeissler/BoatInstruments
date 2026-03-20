#include <Arduino.h>
#include <Wire.h>

// Hardware-Konfiguration Rev 4
#define CH32_I2C_ADDR 0x24
#define I2C_SDA 15
#define I2C_SCL 7


//  CH32V003F4U6 TCA9555 Emulator
//
//  Name      PIN   Port  Reg   Usage
//  EXIO0      5    PD0         ETA6098 - Battery charger - Charge Status indication. LOW when charging, HIGH when charging completed
//  SWDIO     15    PD1   Serial Wire Data I/O (SWDIO)
//  EXIO2     16    PD2         TP_INT (Touch Pad)
//  EXIO3     17    PD3   0x02  LCD_RST (Display)
//  EXIO4     18    PD4         SDCS (SD Card "Chip Select"?)
//  EXIO5     19    PD5         SYS_EN (OTS) Enable/Lock power supply
//  EXIO6     20    PD6         BEE_EN Beeper enable
//  EXIO_PWM  10    PC3   0x01  AP3032KTR-G1 - Back Light - PWM
//  EXIO_ADC  11    PC4         ADC - Battery voltage
//  EXIO1     12    PC5   0x03  TP_RST (Touch Pad)
//  EXIO7     13    PC6         RTC_INT PCF85063ATL Realtime clock interrupt

// 0x04	REG_EXIO_DIR	Richtung der EXIOs: Setzt die Pins PC0–PC2 (X, SDA, SCL des Chips) als Eingang (0) oder Ausgang (1).

/* Register address */
#define STATUS_REG_ADDR           (0x01)
#define OUTPUT_REG_ADDR           (0x02)
#define DIRECTION_REG_ADDR        (0x03)
#define INPUT_REG_ADDR            (0x04)
#define PWM_REG_ADDR              (0x05)
#define ADC_REG_ADDR              (0x06)
#define RTC_REG_ADDR              (0x07)


/* Default register value on power-up */
// 1011 1110
#define OUTPUT_REG_DEFAULT_VAL    (0xBE)

// 0011 1010
#define DIRECTION_REG_DEFAULT_VAL (0x3A)

// 0111 1010
#define DIRECTION_REG_FINAL_VAL   (0x7A)

#define CHG_STAT_BIT 0
#define TP_RST_BIT   1
#define TP_INT_BIT   2
#define LCD_RST_BIT  3
#define SDCS_BIT     4
#define SYS_EN_BIT   5
#define BEE_EN_BIT   6
#define RTC_INT_BIT  7

void setRegisterBit(uint8_t reg, uint8_t bitIndex, bool state);
bool readRegisterBit(uint8_t reg, uint8_t bitIndex, uint8_t* value);
void beep(int duration_ms);

void initIOExpander() {
  // 1. Silence buzzer (FIRST THING — before Serial!)
  //    Simple writes only — loops and constructors crash the ESP32-S3
  Wire.begin(I2C_SDA, I2C_SCL);

  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(OUTPUT_REG_ADDR);
  Wire.write(OUTPUT_REG_DEFAULT_VAL);
  Wire.endTransmission();

  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(DIRECTION_REG_ADDR);
  Wire.write(DIRECTION_REG_DEFAULT_VAL);
  Wire.endTransmission();

//  Wire.end();
}

void initIOExpanderFinalPath() {
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // 4. Enable buzzer control + full direction mask
  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(OUTPUT_REG_ADDR);
  Wire.write(OUTPUT_REG_DEFAULT_VAL); // Ensure buzzer still OFF
  Wire.endTransmission();

  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(DIRECTION_REG_ADDR);
  Wire.write(DIRECTION_REG_FINAL_VAL); // Full mask with BEE_EN as output
  Wire.endTransmission();

  // 5. Switch to 400kHz for display init
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
}

void beep(int duration_ms) {
  setRegisterBit(OUTPUT_REG_ADDR, BEE_EN_BIT, true);
  delay(duration_ms);
  setRegisterBit(OUTPUT_REG_ADDR, BEE_EN_BIT, false);
}

// Dimming range (calibrated for visible response)
#define DUTY_BRIGHTEST   30     // Minimum duty = maximum brightness
#define DUTY_DIMMEST     240    // Maximum useful duty = minimum brightness

// Convert brightness percentage (0-100) to PWM duty
uint8_t brightness_to_duty(uint8_t percent) {
    if (percent >= 100) return DUTY_BRIGHTEST;
    if (percent == 0)   return DUTY_DIMMEST;
    // Linear map across the useful range
    uint16_t range = DUTY_DIMMEST - DUTY_BRIGHTEST;
    return DUTY_DIMMEST - (uint8_t)((uint16_t)percent * range / 100);
}

void setBackLightPercent(uint8_t percent) {
  uint8_t duty = brightness_to_duty(percent);
  
  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(PWM_REG_ADDR);
  Wire.write(duty);
  Wire.endTransmission();
}

void setBackLightOn() {
  setBackLightPercent(100);
}

void setBackLightOff() {
  setBackLightPercent(0);
}

bool getBatteryVoltage(uint16_t* batVoltage) {

  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(ADC_REG_ADDR);
  if (Wire.endTransmission(false) != 0) {
    return false;
  };

  Wire.requestFrom(CH32_I2C_ADDR, 2);

  uint32_t startWait = millis();
  
  // Warten, bis 2 Bytes im Puffer sind ODER Timeout erreicht ist
  while (Wire.available() < 2) {
    if (millis() - startWait > 500) {
      return false; // Abbruch wegen Zeitüberschreitung
    }
    yield(); // Erlaubt dem ESP32 Hintergrundaufgaben (WiFi etc.)
  }
  
  uint8_t lowByte = Wire.read();  // Erstes gelesenes Byte
  uint8_t highByte = Wire.read(); // Zweites gelesenes Byte

  // Zu einem 16-Bit Wert kombinieren (Little Endian Beispiel)
  *batVoltage = (highByte << 8) | lowByte;

  return true;
}

bool getChargerStatus(uint8_t* chargerStatus) {
  return readRegisterBit(INPUT_REG_ADDR, CHG_STAT_BIT, chargerStatus);
}

void setRegisterBit(uint8_t reg, uint8_t bitIndex, bool state) {

  // 1. Aktuellen Wert vom CH32 lesen
  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(CH32_I2C_ADDR, 1);
  
  if (Wire.available()) {
    uint8_t currentVal = Wire.read();
    uint8_t newVal;

    if (state) {
      // Bit auf 1 setzen: bitwise OR
      newVal = currentVal | (1 << bitIndex);
    } else {
      // Bit auf 0 setzen: bitwise AND mit invertierter Maske
      newVal = currentVal & ~(1 << bitIndex);
    }

    // 2. Den geänderten Wert zurückschreiben
    Wire.beginTransmission(CH32_I2C_ADDR);
    Wire.write(reg);
    Wire.write(newVal);
    Wire.endTransmission();
  }
}

bool readRegisterBit(uint8_t reg, uint8_t bitIndex, uint8_t* value) {
  uint8_t currentVal = 0xFF;

  // 1. Aktuellen Wert vom CH32 lesen
  Wire.beginTransmission(CH32_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(CH32_I2C_ADDR, 1);

  uint32_t startWait = millis();
  
  // Warten, bis 1 Bytes im Puffer sind ODER Timeout erreicht ist
  while (Wire.available() < 1) {
    if (millis() - startWait > 500) {
      return false; // Abbruch wegen Zeitüberschreitung
    }
    yield(); // Erlaubt dem ESP32 Hintergrundaufgaben (WiFi etc.)
  }
  
  uint8_t lowByte = Wire.read();  // Ein Byte lesen

  *value = lowByte & (1 << bitIndex);
  return true;
}