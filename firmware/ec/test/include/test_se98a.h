/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_SE98A_H
#define _TEST_SE98A_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "inc/devices/se98a.h"
#include "platform/oc-sdr/schema.h"
#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include <ti/sysbios/knl/Task.h>
/* ======================== Constants & variables =========================== */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static SE98A_Dev s_dev = {
    .cfg = {
        .dev = {
            .bus = 3,
            .slave_addr = 0x1A,
        },
    },
};
static SE98A_Dev s_invalid = {
    .cfg = {
        .dev = {
            .bus = 3,
            .slave_addr = 0xFF,
        },
    },
};

static SE98A_Dev s_invalid_bus = {
    .cfg = {
        .dev = {
            .bus = 0xFF,
            .slave_addr = 0x1A,
        },
    },
};

static uint16_t SE98A_regs[] = {
    [0x00] = 0x00, /* Capabilities */
    [0x01] = 0x00, /* Config */
    [0x02] = 0x00, /* High limit */
    [0x03] = 0x00, /* Low limit */
    [0x04] = 0x00, /* Critical limit */
    [0x05] = 0x00, /* Measured Temperature */
    [0x06] = 0x00, /* MFG ID */
    [0x07] = 0x00, /* Device ID */
};
static bool SE98A_GpioPins[] = {
    [0x05] = 0x1,
};

static uint32_t SE98A_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

static const SE98A_Config fact_ap_se98a_ts1_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};

static struct Test_AlertData {
    bool triggered;
    SE98A_Event evt;
    int8_t temp;
    void *ctx;
} s_alert_data;

typedef enum Se98aStatus {
    SE98A_STATUS_TEMPERATURE = 0,
} Se98aStatus;

typedef enum Se98aConfig {
    SE98A_CONFIG_LIM_LOW = 0,
    SE98A_CONFIG_LIM_HIGH,
    SE98A_CONFIG_LIM_CRIT,
} Se98aConfig;

#endif
