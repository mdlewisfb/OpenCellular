/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_LTC4015_H
#define _TEST_LTC4015_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/devices/ltc4015.h"
#include "platform/oc-sdr/schema.h"
#include <string.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static const I2C_Dev I2C_DEV = {
    .bus = 0,
    .slave_addr = 0x68,
};

static LTC4015_Dev s_dev = {
    .cfg = {
        .i2c_dev = {
            .bus = 0,
            .slave_addr = 0x68,
        },
    },
};

static uint16_t LTC4015_regs[] = {
    [0x01] = 0x00, /* Battery voltage low alert limit */
    [0x02] = 0x00, /* Battery voltage high alert limit */
    [0x03] = 0x00, /* Input voltage low alert limit */
    [0x04] = 0x00, /* Input voltage high alert limit */
    [0x05] = 0x00, /* Output voltage low alert limit */
    [0x06] = 0x00, /* Output voltage high alert limit */
    [0x07] = 0x00, /* Input current high alert limit */
    [0x08] = 0x00, /* Charge current low alert limit */
    [0x09] = 0x00, /* Die temperature high alert limit */
    [0x0A] = 0x00, /* Battery series resistance high alert limit */
    [0x0B] = 0x00, /* Thermistor ratio high(cold battery) alert limit */
    [0x0C] = 0x00, /* Thermistor ratio low(hot battery) alert limit */
    [0x0D] = 0x00, /* Enable limit monitoring and alert notification */
    [0x0E] = 0x00, /* Enable charger state alert notification */
    [0x0F] = 0x00, /* Enable charger status alert notification */
    [0x10] = 0x00, /* Columb counter QCOUNT low alert limit */
    [0x11] = 0x00, /* Columb counter QCOUNT high alert limit */
    [0x12] = 0x00, /* Columb counter prescale factor */
    [0x13] = 0x00, /* Columb counter value */
    [0x14] = 0x00, /* Configuration Settings */
    [0x15] = 0x00, /* Input current limit setting */
    [0x16] = 0x00, /* UVCLFB input buffer limit */
    [0x17] = 0x00, /* Reserved */
    [0x18] = 0x00, /* Reserved */
    [0x19] = 0x00, /* Arm ship mode */
    [0x1A] = 0x00, /* Charge current target */
    [0x1B] = 0x00, /* Charge voltage target */
    [0x1C] = 0x00, /* Low IBAT Threshold for C/x termination */
    [0x1D] = 0x00, /* Time in seconds with battery charger in the CV state before timer termination */
    [0x1E] = 0x00, /* Time in seconds before a max_charge_time fault is declared */
    [0x1F] = 0x00, /* JEITA_T1 */
    [0x20] = 0x00, /* JEITA_T2 */
    [0x21] = 0x00, /* JEITA_T3 */
    [0x22] = 0x00, /* JEITA_T4 */
    [0x23] = 0x00, /* JEITA_T5 */
    [0x24] = 0x00, /* JEITA_T6 */
    [0x25] = 0x00, /* VCHARGE values JEITA_6_5 */
    [0x26] = 0x00, /* VCHARGE values JEITA_4_3_2 */
    [0x27] = 0x00, /* ICHARGE_TARGET values JEITA_6_5 */
    [0x28] = 0x00, /* ICHARGE_TARGET values JEITA_4_3_2 */
    [0x29] = 0x00, /* Battery charger cfguration settings */
    [0x2A] = 0x00, /* LiFePO4/lead-acid absorb voltage adder */
    [0x2B] = 0x00, /* Maximum time for LiFePO4/lead-acid absorb charge */
    [0x2C] = 0x00, /* Lead-acid equalize charge voltage adder */
    [0x2D] = 0x00, /* Lead-acid equalization time */
    [0x2E] = 0x00, /* LiFeP04 recharge threshold */
    [0x2F] = 0x00, /* Reserved */
    [0x30] = 0x00, /* Max Charge Timer for lithium chemistries */
    [0x31] = 0x00, /* Constant-voltage regulation for lithium chemistries */
    [0x32] = 0x00, /* Absorb Timer for LiFePO4 and lead-acid batteries */
    [0x33] = 0x00, /* Eqaulize Timer for lead-acid batteries */
    [0x34] = 0x00, /* Real time battery charger state indicator */
    [0x35] = 0x00, /* Charge status indicator */
    [0x36] = 0x00, /* Limit alert register */
    [0x37] = 0x00, /* Charger state alert register */
    [0x38] = 0x00, /* Charger status alert indicator */
    [0x39] = 0x00, /* Real time system status indicator */
    [0x3A] = 0x00, /* VBAT value(BATSENS) */
    [0x3B] = 0x00, /* VIN */
    [0x3C] = 0x00, /* VSYS */
    [0x3D] = 0x00, /* Battery current(IBAT) */
    [0x3E] = 0x00, /* Input Current(IIN) */
    [0x3F] = 0x00, /* Die temperature */
    [0x40] = 0x00, /* NTC thermistor ratio */
    [0x41] = 0x00, /* Battery series resistance */
    [0x42] = 0x00, /* JEITA temperature region of the NTC thermistor (Li Only) */
    [0x43] = 0x00, /* CHEM and CELLS pin settings */
    [0x44] = 0x00, /* Charge current control DAC control bits */
    [0x45] = 0x00, /* Charge voltage control DAC control bits */
    [0x46] = 0x00, /* Input current limit control DAC control word */
    [0x47] = 0x00, /* Digitally filtered battery voltage */
    [0x48] = 0x00, /* Value of IBAT (0x3D) used in calculating BSR */
    [0x49] = 0x00, /* Reserved */
    [0x4A] = 0x00, /* Measurement valid bit */
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

static bool LTC4015_GpioPins[] = {
    [0x05] = 0x1,
};

static uint32_t LTC4015_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

extern const OcGpio_FnTable GpioSX1509_fnTable;

static OcGpio_Port s_fake_io_exp = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { 0, 0x45 },
        .pin_irq = NULL,
    },
    .object_data = &(SX1509_Obj){},
};

static LTC4015_Dev l_dev = {
    .cfg = {
        .i2c_dev = {
            .bus = 0,
            .slave_addr = 0x68,
        },
        .chem = 0,
       .r_snsb = 30,
       .r_snsi = 7,
       .cellcount = 3,
       .pin_lt4015_i2c_sel = { &s_fake_io_exp, 2, 32 },
    },
};

static LTC4015_Dev s_invalid_dev = {
    .cfg = {
        .i2c_dev = {
            .bus = 2,
            .slave_addr = 0x52,
        },
        .chem = 0,
        .r_snsb = 30,
        .r_snsi = 7,
        .cellcount = 3,
        .pin_lt4015_i2c_sel = { &s_fake_io_exp, 2, 32 },
    },
};
static LTC4015_Dev s_invalid_bus = {
    .cfg = {
        .i2c_dev = {
            .bus = 0xFF,
            .slave_addr = 0x52,
        },
        .chem = 0,
        .r_snsb = 30,
        .r_snsi = 7,
        .cellcount = 3,
        .pin_lt4015_i2c_sel = { &s_fake_io_exp, 2, 32 },
    },
};

//Lead acid battery charge controller.
static LTC4015_Dev ext_bat_dev = {
    .cfg = {
        .i2c_dev = {
            .bus = 0,
            .slave_addr = 0x68, /* LTC4015 I2C address in 7-bit format */
        },
        .chem = 2,
        .r_snsb = 3,
        .r_snsi = 2,
        .cellcount = 6,
        .pin_lt4015_i2c_sel = { &s_fake_io_exp, 4, 32 },
    },
};
typedef enum LTC4015Status {
    LTC4015_STATUS_BATTERY_VOLTAGE = 0,
    LTC4015_STATUS_BATTERY_CURRENT,
    LTC4015_STATUS_SYSTEM_VOLTAGE,
    LTC4015_STATUS_INPUT_VOLATGE,
    LTC4015_STATUS_INPUT_CURRENT,
    LTC4015_STATUS_DIE_TEMPERATURE,
    LTC4015_STATUS_ICHARGE_DAC
} LTC4015Status;

typedef enum LTC4015Config {
    LTC4015_CONFIG_BATTERY_VOLTAGE_LOW = 0,
    LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
    LTC4015_CONFIG_BATTERY_CURRENT_LOW,
    LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
    LTC4015_CONFIG_INPUT_CURRENT_HIGH,
    LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
    LTC4015_CONFIG_ICHARGE,
    LTC4015_CONFIG_VCHARGE,
    LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
} LTC4015Config;

static const LTC4015_Config fact_lithiumIon_cfg = {
    .batteryVoltageLow = 9500,
    .batteryVoltageHigh = 12600,
    .batteryCurrentLow = 100,
    .inputVoltageLow = 16200,
    .inputCurrentHigh = 5000,
    .inputCurrentLimit = 5570,
};

static const LTC4015_Config fact_leadAcid_cfg = {
    .batteryVoltageLow = 9500,
    .batteryVoltageHigh = 13800,
    .batteryCurrentLow = 100,
    .inputVoltageLow = 16200,
    .inputCurrentHigh = 17000,
    .inputCurrentLimit = 16500,
    .icharge = 10660,
    .vcharge = 12000,
};
#endif
