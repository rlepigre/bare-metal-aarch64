#pragma once
#include <bits.h>
#include <bcm2837/register.h>

// Registers for the auxiliaries (UART1, SPI1, and SPI2).

// Registers controling the three peripherals.
#define AUX_IRQ     bus_to_reg32(0x7e215000ULL)
#define AUX_ENABLES bus_to_reg32(0x7e215004ULL)

// Bit fields of the AUX_ENABLES register.
#define AUX_ENABLES_BIT_UART1 BIT_U32(0)
#define AUX_ENABLES_BIT_SPI1  BIT_U32(1)
#define AUX_ENABLES_BIT_SPI2  BIT_U32(2)

// Registers for UART1 (mini UART).
#define AUX_MU_IO_REG   bus_to_reg32(0x7e215040ULL)
#define AUX_MU_IER_REG  bus_to_reg32(0x7e215044ULL)
#define AUX_MU_IIR_REG  bus_to_reg32(0x7e215048ULL)
#define AUX_MU_LCR_REG  bus_to_reg32(0x7e21504cULL)
#define AUX_MU_MCR_REG  bus_to_reg32(0x7e215050ULL)
#define AUX_MU_LSR_REG  bus_to_reg32(0x7e215054ULL)
#define AUX_MU_MSR_REG  bus_to_reg32(0x7e215058ULL)
#define AUX_MU_SCRATCH  bus_to_reg32(0x7e21505cULL)
#define AUX_MU_CNTL_REG bus_to_reg32(0x7e215060ULL)
#define AUX_MU_STAT_REG bus_to_reg32(0x7e215064ULL)
#define AUX_MU_BAUD_REG bus_to_reg32(0x7e215068ULL)

// Bit fields for the AUX_MU_CNTL_REG register.
#define AUX_MU_CNTL_RX_ENABLE BIT_U32(0)
#define AUX_MU_CNTL_TX_ENABLE BIT_U32(1)

// Bit fields for the AUX_MU_LSR_REG register.
#define AUX_MU_LSR_DATA_READY BIT_U32(0)
#define AUX_MU_LSR_RX_OVERRUN BIT_U32(1)
#define AUX_MU_LSR_TX_EMPTY   BIT_U32(5)
#define AUX_MU_LSR_TX_IDLE    BIT_U32(6)

// Registers for SPI1.
#define AUX_SPI1_CNTL0_REG bus_to_reg32(0x7e215080ULL)
#define AUX_SPI1_CNTL1_REG bus_to_reg32(0x7e215084ULL)
#define AUX_SPI1_STAT_REG  bus_to_reg32(0x7e215088ULL)
#define AUX_SPI1_IO_REG    bus_to_reg32(0x7e215090ULL)
#define AUX_SPI1_PEEK_REG  bus_to_reg32(0x7e215094ULL)

// Registers for SPI2.
#define AUX_SPI2_CNTL0_REG bus_to_reg32(0x7e2150c0ULL)
#define AUX_SPI2_CNTL1_REG bus_to_reg32(0x7e2150c4ULL)
#define AUX_SPI2_STAT_REG  bus_to_reg32(0x7e2150c8ULL)
#define AUX_SPI2_IO_REG    bus_to_reg32(0x7e2150d0ULL)
#define AUX_SPI2_PEEK_REG  bus_to_reg32(0x7e2150d4ULL)
