#include <Arduino.h>
#include <Wire.h>


#define GT911_I2C_ADDR 0x5D

/*

### GT911 I2C Address
The GT911's I2C address is determined by the level of the INT pin at reset:

| INT level at reset | Address |
|--------------------|---------|
| LOW                |   0x5D  |
| HIGH               |   0x14  |

On Rev 4, the INT level depends on the CH32V003 direction mask at the moment the GT911 boots. With the factory mask 0x3A (TP_INT as input with pull-up), the GT911 typically appears at 0x5D. The firmware scans both addresses.

*/

void initTouch() {

  // 1. Read full 186-byte config (0x8047 to 0x8100)
  uint8_t full_cfg[186];
  memset(full_cfg, 0, sizeof(full_cfg));
  for (uint16_t offset = 0; offset < 184; offset += 28) {
      uint8_t chunk = min((uint16_t)28, (uint16_t)(184 - offset));
      uint16_t reg = 0x8047 + offset;
      Wire.beginTransmission(GT911_I2C_ADDR);
      Wire.write((uint8_t)(reg >> 8));
      Wire.write((uint8_t)(reg & 0xFF));
      Wire.endTransmission(false);
      Wire.requestFrom(GT911_I2C_ADDR, chunk);
      for (uint8_t i = 0; i < chunk && Wire.available(); i++) {
          full_cfg[offset + i] = Wire.read();
      }
  }

  // 2. Patch config fields
  full_cfg[0] += 1;                   // Increment Config_Version (GT911 only accepts newer)
  if (full_cfg[0] == 0) full_cfg[0] = 1;
  full_cfg[1] = 0xE0; full_cfg[2] = 0x01;  // X_Output_Max = 480 (little-endian)
  full_cfg[3] = 0xE0; full_cfg[4] = 0x01;  // Y_Output_Max = 480
  full_cfg[5] = 5;                          // Touch_Number (max simultaneous touches)
  if (full_cfg[6] == 0) full_cfg[6] = 0x0D; // Module_Switch1: X2Y | INT rising

  // 3. Calculate checksum (bytes 0-183)
  uint8_t checksum = 0;
  for (int i = 0; i < 184; i++) checksum += full_cfg[i];
  checksum = (~checksum) + 1;
  full_cfg[184] = checksum;   // Register 0x80FF
  full_cfg[185] = 0x01;       // Register 0x8100 (config_fresh flag)

  // 4. Write back in 28-byte chunks
  for (uint16_t offset = 0; offset < 186; offset += 28) {
      uint8_t chunk = min((uint16_t)28, (uint16_t)(186 - offset));
      uint16_t reg = 0x8047 + offset;
      Wire.beginTransmission(GT911_I2C_ADDR);
      Wire.write((uint8_t)(reg >> 8));
      Wire.write((uint8_t)(reg & 0xFF));
      Wire.write(&full_cfg[offset], chunk);
      Wire.endTransmission();
  }
  delay(100);  // GT911 processes the new config
}
