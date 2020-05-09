#!/bin/bash

cat <<'EOM'
#if defined(__AVR__) || defined(ARDUINO_ARCH_SAMD)
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#endif

#define BGCOLOR 0x1820
#define FGCOLOR 0xfdc1

const uint16_t panelWidth = 80;
const uint16_t panelHeight = 40;

EOM

for f in bed_b bed_r posi_b posi_r unknown_b unknown_r ; do
  tail -n +15 $f.c  
done
