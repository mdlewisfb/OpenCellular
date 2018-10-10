/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_EEPROM_1_H
#define _TEST_EEPROM_1_H

#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/devices/eeprom.h"
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include <ti/sysbios/knl/Task.h>
/* ======================== Constants & variables =========================== */
static uint16_t EEPROM_regs[] = {
    [0x0000] = 0x0000,
    [0x0A01] = 0x0000,
    [0x0A02] = 0x0000,
    [0x0A01] = 0x4153,
    [0x0A02] = 0x3731,
    [0x0A03] = 0x3831,
    [0x0A04] = 0x3043,
    [0x0A05] = 0x3534,
    [0x0A06] = 0x4130,
    [0x0A07] = 0x3031,
    [0x0A08] = 0x3430,
    [0x0A09] = 0x3131,
    [0xAC01] = 0x5341,
    [0xAC02] = 0x3137,
    [0xAC03] = 0x3138,
    [0xAC04] = 0x4330,
    [0xAC05] = 0x3534,
    [0xAC06] = 0x4130,
    [0xAC07] = 0x3031,
    [0xAC08] = 0x3430,
    [0xAC09] = 0x3131,
    [0xAC0A] = 0x3131,
    [0xC601] = 0x5341,
    [0xC602] = 0x3137,
    [0xC603] = 0x3138,
    [0xC604] = 0x4330,
    [0xC605] = 0x3534,
    [0xC606] = 0x4130,
    [0xC607] = 0x3031,
    [0xC608] = 0x3430,
    [0xC609] = 0x3131,
    [0xC60A] = 0x3131,
    [0xC641] = 0x0000,
};

static uint8_t SX1509_regs[] = {
    [0x00] = 0x00, /* Input buffer disable register B */
    [0x01] = 0x00, /* Input buffer disable register A */
    [0x02] = 0x00, /* Output buffer long slew register B */
    [0x03] = 0x00, /* Output buffer long slew register A */
    [0x04] = 0x00, /* Output buffer low drive register B */
    [0x05] = 0x00, /* Output buffer low drive register A */
    [0x06] = 0x00, /* Pull Up register B */
    [0x07] = 0x00, /* Pull Up register A */
    [0x08] = 0x00, /* Pull Down register B */
    [0x09] = 0x00, /* Pull Down register A */
    [0x0A] = 0x00, /* Open drain register B */
    [0x0B] = 0x00, /* Open drain register A */
    [0x0C] = 0x00, /* Polarity register B */
    [0x0D] = 0x00, /* Polarity register A */
    [0x0E] = 0x00, /* Direction register B */
    [0x0F] = 0x00, /* Direction register A */
    [0x10] = 0x00, /* Data register B */
    [0x11] = 0x00, /* Data register A */
    [0x12] = 0x00, /* Interrupt mask register B */
    [0x13] = 0x00, /* Interrupt mask register A */
    [0x14] = 0x00, /* Sense High register B */
    [0x15] = 0x00, /* Sense Low register B */
    [0x16] = 0x00, /* Sense High register A */
    [0x17] = 0x00, /* Sense Low register A */
    [0x18] = 0x00, /* Interrupt source register B */
    [0x19] = 0x00, /* Interrupt source register A */
    [0x1A] = 0x00, /* Event status register B */
    [0x1B] = 0x00, /* Event status register A */
    [0x1C] = 0x00, /* Level shifter register 1 */
    [0x1D] = 0x00, /* Level shifter register 2 */
    [0x1E] = 0x00, /* Clock management register */
    [0x1F] = 0x00, /* Miscellaneous device settings register */
    [0x20] = 0x00, /* LED driver enable register B */
    [0x21] = 0x00, /* LED driver enable register A */
    [0x22] = 0x00, /* Debounce configuration register */
    [0x23] = 0x00, /* Debounce enable register B */
    [0x24] = 0x00, /* Debounce enable register A */
    [0x25] = 0x00, /* Key scan configuration register 1 */
    [0x26] = 0x00, /* Key scan configuration register 2 */
    [0x27] = 0x00, /* Key value (column) 1 */
    [0x28] = 0x00, /* Key value (row) 2 */
    [0x29] = 0x00, /* ON time register I/O[0] */
    [0x2A] = 0x00, /* ON intensity register I/O[0] */
    [0x2B] = 0x00, /* OFF time/intensity register I/O[0] */
    [0x2C] = 0x00, /* ON time register I/O[1] */
    [0x2D] = 0x00, /* ON intensity register I/O[1] */
    [0x2E] = 0x00, /* OFF time/intensity register I/O[1] */
    [0x2F] = 0x00, /* ON time register I/O[2] */
    [0x30] = 0x00, /* ON intensity register I/O[2] */
    [0x31] = 0x00, /* OFF time/intensity register I/O[2] */
    [0x32] = 0x00, /* ON time register I/O[3] */
    [0x33] = 0x00, /* ON intensity register I/O[3] */
    [0x34] = 0x00, /* OFF time/intensity register I/O[3] */
    [0x35] = 0x00, /* ON time register I/O[4] */
    [0x36] = 0x00, /* ON intensity register I/O[4] */
    [0x37] = 0x00, /* OFF time/intensity register I/O[4] */
    [0x38] = 0x00, /* Fade in register I/O[4] */
    [0x39] = 0x00, /* Fade out register I/O[4] */
    [0x3A] = 0x00, /* ON time register I/O[5] */
    [0x3B] = 0x00, /* ON intensity register I/O[5] */
    [0x3C] = 0x00, /* OFF time/intensity register I/O[5] */
    [0x3D] = 0x00, /* Fade in register I/O[5] */
    [0x3E] = 0x00, /* Fade out register I/O[5] */
    [0x3F] = 0x00, /* ON time register I/O[6] */
    [0x40] = 0x00, /* ON intensity register I/O[6] */
    [0x41] = 0x00, /* OFF time/intensity register I/O[6] */
    [0x42] = 0x00, /* Fade in register I/O[6] */
    [0x43] = 0x00, /* Fade out register I/O[6] */
    [0x44] = 0x00, /* ON time register I/O[6] */
    [0x45] = 0x00, /* ON intensity register I/O[7] */
    [0x46] = 0x00, /* OFF time/intensity register I/O[7] */
    [0x47] = 0x00, /* Fade in register I/O[7] */
    [0x48] = 0x00, /* Fade out register I/O[7] */
    [0x49] = 0x00, /* ON time register I/O[8] */
    [0x4A] = 0x00, /* ON intensity register I/O[8] */
    [0x4B] = 0x00, /* OFF time/intensity register I/O[8]  */
    [0x4C] = 0x00, /* ON time register I/O[9] */
    [0x4D] = 0x00, /* ON intensity register I/O[9] */
    [0x4E] = 0x00, /* OFF time/intensity register I/O[9] */
    [0x4F] = 0x00, /* ON time register I/O[10] */
    [0x50] = 0x00, /* ON intensity register I/O[10] */
    [0x51] = 0x00, /* OFF time/intensity register I/O[10] */
    [0x52] = 0x00, /* ON time register I/O[11] */
    [0x53] = 0x00, /* ON intensity register I/O[11] */
    [0x54] = 0x00, /* OFF time/intensity register I/O[11] */
    [0x55] = 0x00, /* ON time register I/O[12] */
    [0x56] = 0x00, /* ON intensity register I/O[12] */
    [0x57] = 0x00, /* OFF time/intensity register I/O[12] */
    [0x58] = 0x00, /* Fade in register I/O[12] */
    [0x59] = 0x00, /* Fade out register I/O[12] */
    [0x5A] = 0x00, /* ON time register I/O[13] */
    [0x5B] = 0x00, /* ON intensity register I/O[13] */
    [0x5C] = 0x00, /* OFF time/intensity register I/O[13] */
    [0x5D] = 0x00, /* Fade in register I/O[13] */
    [0x5E] = 0x00, /* Fade out register I/O[13] */
    [0x5F] = 0x00, /* ON time register I/O[14] */
    [0x60] = 0x00, /* ON intensity register I/O[14] */
    [0x61] = 0x00, /* OFF time/intensity register I/O[14] */
    [0x62] = 0x00, /* Fade in register I/O[14] */
    [0x63] = 0x00, /* Fade out register I/O[14] */
    [0x64] = 0x00, /* ON time register I/O[15] */
    [0x65] = 0x00, /* ON intensity register I/O[15] */
    [0x66] = 0x00, /* OFF time/intensity register I/O[15] */
    [0x67] = 0x00, /* Fade in register I/O[115] */
    [0x68] = 0x00, /* Fade out register I/O[15] */
    [0x69] = 0x00, /*  */
    [0x6A] = 0x00, /*  */
    [0x7D] = 0x00, /*  */
    [0x7E] = 0x00, /*  */
    [0x7F] = 0x00, /*  */
};

static const I2C_Dev I2C_DEV = {
    .bus = 6,
    .slave_addr = 0x50,
};
static const I2C_Dev I2C_DEV_1 = {
    .bus = 6,
    .slave_addr = 0x51,
};

static const I2C_Dev I2C_DEV_SDR = {
    .bus = 3,
    .slave_addr = 0x50,
};

static const I2C_Dev I2C_DEV_FE = {
    .bus = 4,
    .slave_addr = 0x50,
};

static Eeprom_Cfg s_invalid_dev = {
    .i2c_dev = {
        .bus = 6,
        .slave_addr = 0xFF,
    },
};

static Eeprom_Cfg s_invalid_bus = {
    .i2c_dev = {
        .bus = 0xFF,
        .slave_addr = 0x50,
    },
};

static Eeprom_Cfg *invalid_cfg = NULL;

static bool Eeprom_GpioPins[] = {
    [0x01] = 0x1, /* Pin = 1 */
    [0x02] = 0x1, /* Pin = 2 */
};

static uint32_t Eeprom_GpioConfig[] = {
    [0x01] = OCGPIO_CFG_INPUT,
    [0x02] = OCGPIO_CFG_INPUT,
};

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

extern const OcGpio_FnTable GpioSX1509_fnTable;

static OcGpio_Port s_fake_io_exp = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { 6, 0x45 },
        .pin_irq = NULL,
    },
    .object_data = &(SX1509_Obj){},
};

static OcGpio_Pin pin_inven_eeprom_wp = { &s_fake_io_exp, 2, 32 };

static Eeprom_Cfg eeprom_gbc_inv = {
    .i2c_dev = { 6, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
    .type = { .page_size = 64, .mem_size = (256 / 8) },
    .ss = 0,
};

static Eeprom_Cfg eeprom_sdr_inv = {
    .i2c_dev = { 3, 0x50 },
    .pin_wp = NULL,
    .type = { .page_size = 64, .mem_size = (256 / 8) },
    .ss = 7,
};

static Eeprom_Cfg eeprom_fe_inv = {
    .i2c_dev = { 4, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
    .type = { .page_size = 64, .mem_size = (256 / 8) },
    .ss = 8,
};

static Eeprom_Cfg enable_dev = {
  .i2c_dev = { 6, 0x45 },
  .pin_wp = &pin_inven_eeprom_wp,
  .type = { .page_size = 64, .mem_size = (256 / 8) },
  .ss = 0,
};
#endif
