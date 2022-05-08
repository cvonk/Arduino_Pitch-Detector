#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "../../config.h"
#include "display.h"

# if DST == DST_STAFF

typedef enum staffSymbolName_t {
  STAFFSYMBOL_NAME_FLAT = 0,
  STAFFSYMBOL_NAME_TO_LOW,
  STAFFSYMBOL_NAME_TO_HIGH,
  STAFFSYMBOL_NAME_NOTE,
# if GKEY != GKEY_NONE
  STAFFSYMBOL_NAME_GKEY,
#endif  
  STAFFSYMBOL_NAME_COUNT
} staffSymbolName_t;

uint_least8_t const staffSymbolName_COUNT = static_cast<int>(STAFFSYMBOL_NAME_COUNT);

void staffsymbol_init(Adafruit_ST7735 * tft,
                      xCoordinate_t const displayWidth, yCoordinate_t const displayHeight,
                      yCoordinate_t const noteRadius, yCoordinate_t const bottom2loStaff,
                      yCoordinate_t const top2hiStaff, yCoordinate_t const yG4);

void staffsymbol_draw(xCoordinate_t const center_x, yCoordinate_t const center_y,
                      staffSymbolName_t const nr, color_t const color);

# endif
