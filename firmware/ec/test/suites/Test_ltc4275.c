/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ltc4275.h"

extern tPower_PDStatus_Info PDStatus_Info;
/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;

xdc_Void ti_sysbios_knl_Task_sleep__E( xdc_UInt32 nticks )
{
    s_task_sleep_ticks += nticks;
}

void test_alert(void)
{
}

static void alert_handler(LTC4275_Event evt, void *context)
{

}

void post_update_POSTData(POSTData *pData, uint8_t I2CBus, uint8_t devAddress,
                                                uint16_t manId, uint16_t devId)
{
    pData->i2cBus = I2CBus;
    pData->devAddr = devAddress;
    pData->manId = manId;
    pData->devId = devId;
}
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(LTC4275_GpioPins, LTC4275_GpioConfig);
}

void setUp(void)
{

}

void tearDown(void)
{
}

void suite_tearDown(void)
{
}
/* ================================ Tests =================================== */
void test_ltc4275_init()
{
    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 1;
    LTC4275_GpioConfig[0x60] = OCGPIO_CFG_INPUT;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_init(&l_dev));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES,
                                            LTC4275_GpioConfig[0x60]);
    /*Invalid cfg case*/
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4275_init(invalid_cfg));
}

void test_ltc4275_get_power_good()
{
    ePDPowerState val;

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_power_good(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD, val);

    LTC4275_GpioPins[0x60] = 1;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_power_good(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD_NOTOK, val);

    /*Invalid cfg case*/
    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4275_get_power_good(invalid_cfg, &val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD_NOTOK, val);
}

void test_ltc4275_probe()
{
    LTC4275_GpioPins[0x60] = 1;
    POSTData postData;
    TEST_ASSERT_EQUAL(1, ltc4275_probe(&l_dev, &postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(3, ltc4275_probe(&l_dev, &postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);

    /*Invalid cfg case*/
    LTC4275_GpioPins[0x60] = 1;
    TEST_ASSERT_EQUAL(1, ltc4275_probe(invalid_cfg, &postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);
}

void test_ltc4275_get_class()
{
    LTC4275_GpioPins[0x40] = 1;
    ePDClassType val;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_class(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_2, val);

    LTC4275_GpioPins[0x40] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_class(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_1, val);

    /*Invalid cfg case*/
    LTC4275_GpioPins[0x40] = 0;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4275_get_class(invalid_cfg, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_1, val);
}

void test_ltc4275_set_alert_handler()
{
    ltc4275_set_alert_handler(&l_dev, alert_handler, 1);
    TEST_ASSERT_EQUAL(1, (int *)l_dev.obj.cb_context);
    TEST_ASSERT_EQUAL(alert_handler, (int *)l_dev.obj.alert_cb);
}

void test_ltc4275_update_status()
{
    LTC4275_GpioPins[0x60] = 1;
    LTC4275_GpioPins[0x40] = 1;
    ltc4275_update_status(&l_dev);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);

    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 0;
    ltc4275_update_status(&l_dev);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);

    /*Invalid cfg case*/
    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 0;
    PDStatus_Info.pdStatus.classStatus = 0;
    PDStatus_Info.state = 0;
    PDStatus_Info.pdalert = 0;
    ltc4275_update_status(invalid_cfg);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdalert);
}
