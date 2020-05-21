#!/bin/bash

cat <<'EOM'
#include <pgmspace.h>

#define BGCOLOR 0x1820
#define FGCOLOR 0xfdc1

const uint16_t panelWidth = 80;
const uint16_t panelHeight = 40;

const uint16_t resultWidth = 80;
const uint16_t resultHeight = 60;

EOM
for f in inc_b inc_r bed_b bed_r posi_b posi_r unknown_b unknown_r p_file p_result r_red r_green r_yellow; do
#for f in r_red r_green r_yellow ; do
  tail -n +15 $f.c  
done
