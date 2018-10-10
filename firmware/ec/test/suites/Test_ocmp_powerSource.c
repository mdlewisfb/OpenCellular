#include "include/test_powerSource.h"

extern const OcGpio_FnTable GpioSX1509_fnTable;
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(PWR_GpioPins, PWR_GpioConfig);
    fake_I2C_init();
    fake_I2C_registerDevSimple(I2C_BUS, I2C_ADDR, SX1509_regs,
                               sizeof(SX1509_regs), sizeof(SX1509_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(SX1509_regs, 0, sizeof(SX1509_regs));
    OcGpio_init(&s_fake_io_exp);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit();
}
/* ================================ Tests =================================== */
void test_init()
{
    PWR_GpioPins[0x1E] = 0x1;
    AlertData alert_data = {
        .subsystem = 7,
        .componentId = 1,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(5 , PWRSRC_fxnTable.cb_init(&p_dev,
                        NULL, alert_data_cp));
}

void test_probe()
{
    POSTData postData;
    PWR_GpioConfig[0x1E] = OCGPIO_CFG_OUTPUT;
    PWR_GpioConfig[0x55] = OCGPIO_CFG_OUTPUT;

    TEST_ASSERT_EQUAL(0, PWRSRC_fxnTable.cb_probe(&p_dev,&postData));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT, PWR_GpioConfig[0x1E]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT, PWR_GpioConfig[0x55]);
}

void test_get_status_poeavailable()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x0;      //PoE Enable
    PWR_GpioPins[0x1E] = 0x1;      //Aux/solar Disable
    SX1509_regs[0x10] = 0x18;      //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 0, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_poeaccessible()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x0;      //PoE Enable
    PWR_GpioPins[0x1E] = 0x1;      //Aux/solar Disable
    SX1509_regs[0x10] = 0x18;      //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 1, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_solaravailable()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;      //PoE Disable
    PWR_GpioPins[0x1E] = 0x0;      //Aux/solar Enable
    SX1509_regs[0x10] = 0x18;      //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 2, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_solaraccessible()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;      //PoE Disable
    PWR_GpioPins[0x1E] = 0x0;      //Aux/solar Enable
    SX1509_regs[0x10] = 0x18;      //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 3, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_extavailable()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;       //PoE Disable
    PWR_GpioPins[0x1E] = 0x1;       //Aux/solar Disable
    SX1509_regs[0x10] = 0x08;       //Int Batt OFF, Ext batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 4, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_extaccessible()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;      //PoE Disable
    PWR_GpioPins[0x1E] = 0x1;      //Aux/solar Disable
    SX1509_regs[0x10] = 0x08;      //Int Batt OFF, Ext batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 5, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_intavailable()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;       //PoE Disable
    PWR_GpioPins[0x1E] = 0x1;       //Aux/solar Disable
    SX1509_regs[0x10] = 0x10;       //Ext Batt OFF, Int batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 6, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_intaccessible()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;      //PoE Disable
    PWR_GpioPins[0x1E] = 0x1;      //Aux/solar Disable
    SX1509_regs[0x10] = 0x10;      //Ext Batt OFF, Int batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(1, PWRSRC_fxnTable.cb_get_status(&p_dev, 7, &powerStatus));
    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_get_status_invalid_param()
{
    uint8_t powerStatus = 0;
    PWR_GpioPins[0x55] = 0x1;      //PoE Disable
    PWR_GpioPins[0x1E] = 0x1;      //Aux/solar Disable
    SX1509_regs[0x10] = 0x10;      //Ext Batt OFF, Int batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    TEST_ASSERT_EQUAL(0, PWRSRC_fxnTable.cb_get_status(&p_dev, 9, &powerStatus));
    TEST_ASSERT_EQUAL(0, powerStatus);
}
