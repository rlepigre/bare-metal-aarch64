#pragma once
#include <bcm2837/register.h>

// GPIO registers
#define GPFSEL0   bus_to_reg32(0x7e200000ULL)
#define GPFSEL1   bus_to_reg32(0x7e200004ULL)
#define GPFSEL2   bus_to_reg32(0x7e200008ULL)
#define GPFSEL3   bus_to_reg32(0x7e20000cULL)
#define GPFSEL4   bus_to_reg32(0x7e200010ULL)
#define GPFSEL5   bus_to_reg32(0x7e200014ULL)
// (reserved)                  0x7e200018
#define GPSET0    bus_to_reg32(0x7e20001cULL)
#define GPSET1    bus_to_reg32(0x7e200020ULL)
// (reserved)                  0x7e200024
#define GPCLR0    bus_to_reg32(0x7e200028ULL)
#define GPCLR1    bus_to_reg32(0x7e20002cULL)
// (reserved)                  0x7e200030
#define GPLEV0    bus_to_reg32(0x7e200034ULL)
#define GPLEV1    bus_to_reg32(0x7e200038ULL)
// (reserved)                  0x7e20003c
#define GPEDS0    bus_to_reg32(0x7e200040ULL)
#define GPEDS1    bus_to_reg32(0x7e200044ULL)
// (reserved)                  0x7e200048
#define GPREN0    bus_to_reg32(0x7e20004cULL)
#define GPREN1    bus_to_reg32(0x7e200050ULL)
// (reserved)                  0x7e200054
#define GPFEN0    bus_to_reg32(0x7e200058ULL)
#define GPFEN1    bus_to_reg32(0x7e20005cULL)
// (reserved)                  0x7e200060
#define GPHEN0    bus_to_reg32(0x7e200064ULL)
#define GPHEN1    bus_to_reg32(0x7e200068ULL)
// (reserved)                  0x7e20006c
#define GPLEN0    bus_to_reg32(0x7e200070ULL)
#define GPLEN1    bus_to_reg32(0x7e200074ULL)
// (reserved)                  0x7e200078
#define GPAREN0   bus_to_reg32(0x7e20007cULL)
#define GPAREN1   bus_to_reg32(0x7e200080ULL)
// (reserved)                  0x7e200084
#define GPAFEN0   bus_to_reg32(0x7e200088ULL)
#define GPAFEN1   bus_to_reg32(0x7e20008cULL)
// (reserved)                  0x7e200090
#define GPPUD     bus_to_reg32(0x7e200094ULL)
#define GPPUDCLK0 bus_to_reg32(0x7e200098ULL)
#define GPPUDCLK1 bus_to_reg32(0x7e20009cULL)
// (reserved)                  0x7e2000a0
// (test)                      0x7e2000b0

// Function select configuration (on 3 bits).
#define GPFSEL_IN   0
#define GPFSEL_OUT  1
#define GPFSEL_ALT0 4
#define GPFSEL_ALT1 5
#define GPFSEL_ALT2 6
#define GPFSEL_ALT3 7
#define GPFSEL_ALT4 3
#define GPFSEL_ALT5 2

// Configuration for GPPUD (on 2 bits).
#define GPPUD_OFF       0
#define GPPUD_PULL_DOWN 1
#define GPPUD_PULL_UP   2
