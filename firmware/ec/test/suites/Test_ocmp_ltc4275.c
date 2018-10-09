#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4275.h"
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

void OCMP_GenerateAlert(const AlertData *alert_data,
                        unsigned int alert_id,
                        const void *data)
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
void test_init()
{
    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 1;
    LTC4275_GpioConfig[0x60] = OCGPIO_CFG_INPUT;
    AlertData alert_data = {
        .subsystem = 1,
        .componentId = 6,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;

    TEST_ASSERT_EQUAL(4, LTC4275_fxnTable.cb_init(&l_dev, NULL, alert_data_cp));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES,
                                                  LTC4275_GpioConfig[0x60]);
}

void test_probe()
{
    POSTData postData;

    LTC4275_GpioPins[0x60] = 1;
    TEST_ASSERT_EQUAL(1, LTC4275_fxnTable.cb_probe(&l_dev,&postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.pdStatus.powerGoodStatus);

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(3, LTC4275_fxnTable.cb_probe(&l_dev,&postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.powerGoodStatus);
}

void test_get_status()
{
    ePDPowerState val;

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(1, LTC4275_fxnTable.cb_get_status(&l_dev, 1, &val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD, val);

    LTC4275_GpioPins[0x40] = 1;
    TEST_ASSERT_EQUAL(1, LTC4275_fxnTable.cb_get_status(&l_dev, 0, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_2, val);

    /*Invalid ParamID*/
    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(0, LTC4275_fxnTable.cb_get_status(&l_dev, 2, &val));
}
