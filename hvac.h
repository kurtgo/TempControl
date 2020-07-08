#pragma once

// 10th, January 2016 - Fun with ESP8266
// (c)  Vincent Cruvellier
//  The Sketch use an ESP 8266 to read DHT sensor Values, Send HVAC IR command and sleep.
//
// I have used ESP 8266 ESP-12 Module
//  the IR led Emitter should be drive from a 5V current. You can power the IR LED from the 3.3V but the
//  emission distance will be considerably reduced. The bes is to drive the IR led from 5V and drive through
//  a transitor the led emission (Signal). Caution a lot of IR-Led module do not have real Signal drive.
//  Indeed, lot of them have Signal directly connected to VDD.
//
// Sketch Exemple with ESP8266 + HVAC IR Emission Capability + DHT Reading
// Sketch has been very simplified to not use IR-Remote lib but just what we need for this example
//   Mean we need just to be able to send HVAC IR command.
// ESP Deep sleep Feauture is used on this sketch. That requires ESP/RST connected to ESP/GPIO16
//   in order to wakeup from the deep sleep.
//
// Hardware Connection
//  IR LED SIGNAL => ESP/GPIO_4
//  DHT SIGNAL => ESP/GPIO_5
// RESET => ESP/GPIO_16

class HVAC_CONTROL {

private:
int halfPeriodicTime;
int IRpin;
int khz;

public:
	HVAC_CONTROL(int hvac_pin);

typedef enum HvacMode {
  HVAC_HOT,
  HVAC_COLD,
  HVAC_DRY,
  HVAC_FAN, // used for Panasonic only
  HVAC_AUTO
} HvacMode_t; // HVAC  MODE

typedef enum HvacFanMode {
  FAN_SPEED_1,
  FAN_SPEED_2,
  FAN_SPEED_3,
  FAN_SPEED_4,
  FAN_SPEED_5,
  FAN_SPEED_AUTO,
  FAN_SPEED_SILENT
} HvacFanMode_;  // HVAC  FAN MODE

typedef enum HvacVanneMode {
  VANNE_AUTO,
  VANNE_H1,
  VANNE_H2,
  VANNE_H3,
  VANNE_H4,
  VANNE_H5,
  VANNE_AUTO_MOVE
} HvacVanneMode_;  // HVAC  VANNE MODE

typedef enum HvacWideVanneMode {
  WIDE_LEFT_END,
  WIDE_LEFT,
  WIDE_MIDDLE,
  WIDE_RIGHT,
  WIDE_RIGHT_END
} HvacWideVanneMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacAreaMode {
  AREA_SWING,
  AREA_LEFT,
  AREA_AUTO,
  AREA_RIGHT
} HvacAreaMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacProfileMode {
  NORMAL,
  QUIET,
  BOOST
} HvacProfileMode_t;  // HVAC PANASONIC OPTION MODE


// HVAC MITSUBISHI_
#define HVAC_MITSUBISHI_HDR_MARK    3400
#define HVAC_MITSUBISHI_HDR_SPACE   1750
#define HVAC_MITSUBISHI_BIT_MARK    450
#define HVAC_MITSUBISHI_ONE_SPACE   1300
#define HVAC_MISTUBISHI_ZERO_SPACE  420
#define HVAC_MITSUBISHI_RPT_MARK    440
#define HVAC_MITSUBISHI_RPT_SPACE   17100 // Above original iremote limit


/****************************************************************************
** Send IR command to Mitsubishi HVAC - sendHvacMitsubishi
****************************************************************************/
void sendHvacMitsubishi(
  HvacMode                HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
  int                     HVAC_Temp,           // Example 21  (°c)
  HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
  HvacVanneMode           HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
  int                     OFF                  // Example false
);
/****************************************************************************
** enableIROut : Set global Variable for Frequency IR Emission
****************************************************************************/
void enableIROut(int khz);
void mark(int time);
void space(int time);
void sendRaw (unsigned int buf[], int len, int hz);
};
