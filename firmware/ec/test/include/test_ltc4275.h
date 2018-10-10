/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_LTC4275_H
#define _TEST_LTC4275_H

#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "inc/devices/ltc4275.h"
#include <string.h>
#include "unity.h"

#include <ti/sysbios/knl/Task.h>

/* ======================== Constants & variables =========================== */
static uint8_t LTC4275_GpioPins[] = {
    [0x40] = 0x00, /* OC_EC_PWR_PD_NT2P = 64*/
    [0x60] = 0x00, /* OC_EC_PD_PWRGD_ALERT = 96 */
};

static uint32_t LTC4275_GpioConfig[] = {
    [0x40] = OCGPIO_CFG_INPUT,
    [0x60] = OCGPIO_CFG_INPUT,
};

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static LTC4275_Dev l_dev = {
    .cfg = {
        .pin_evt = &(OcGpio_Pin){ &s_fake_io_port, 0x60 },
        .pin_detect = &(OcGpio_Pin){ &s_fake_io_port, 0x40 },

    },
};

static LTC4275_Dev *invalid_cfg = NULL;
#endif
