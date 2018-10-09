/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_INA226_H
#define _TEST_INA226_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "inc/devices/ina226.h"
#include "platform/oc-sdr/schema.h"
#include <string.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static INA226_Dev ina226_dev = {
    .cfg = {
        .dev = {
            .bus = 4,
            .slave_addr = 0x01,
        },
    },
};

static INA226_Dev ina226_invalid_dev = {
    .cfg = {
        .dev = {
            .bus = 4,
            .slave_addr = 0x02,
        },
    },
};

static INA226_Dev ina226_invalid_bus = {
    .cfg = {
        .dev = {
            .bus = 0xFF,
            .slave_addr = 0x01,
        },
    },
};

static uint16_t INA226_regs[] = {
    [0x00] = 0x0000, /* Configuration */
    [0x01] = 0x0000, /* Shunt Volatge */
    [0x02] = 0x0000, /* Bus Voltage */
    [0x03] = 0x0000, /* Power */
    [0x04] = 0x0000, /* Current */
    [0x05] = 0x0000, /* Calibration */
    [0x06] = 0x0000, /* Mask Enable */
    [0x07] = 0x0000, /* Alert Limit */
    [0xFE] = 0x0000, /* Manf Id */
    [0xFF] = 0x0000, /* Die Id */
};

static bool INA226_GpioPins[] = {
    [0x05] = 0x1,
};

static uint32_t INA226_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

static const INA226_Config fact_sdr_3v_ps_cfg = {
    .current_lim = 3000,
};

typedef enum INA226Status {
    INA226_STATUS_BUS_VOLTAGE = 0,
    INA226_STATUS_SHUNT_VOLTAGE,
    INA226_STATUS_CURRENT,
    INA226_STATUS_POWER,
} INA226Status;

typedef enum INA226Config {
    INA226_CONFIG_CURRENT_LIM = 0,
} INA226Config;
#endif
