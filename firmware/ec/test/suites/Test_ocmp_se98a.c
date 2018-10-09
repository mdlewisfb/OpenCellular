#include "include/test_se98a.h"

/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E( xdc_UInt32 nticks )
{
    s_task_sleep_ticks += nticks;
}

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(SE98A_GpioPins, SE98A_GpioConfig);
    fake_I2C_init();
    fake_I2C_registerDevSimple(s_dev.cfg.dev.bus, s_dev.cfg.dev.slave_addr,
                               SE98A_regs, sizeof(SE98A_regs),
                               sizeof(SE98A_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(SE98A_regs, 0, sizeof(SE98A_regs));
    SE98A_regs[0x00] = 0x0037;
    SE98A_regs[0x06] = 0x1131;
    SE98A_regs[0x07] = 0xA102;

    s_task_sleep_ticks = 0;
    OcGpio_init(&s_fake_io_port);
    se98a_init(&s_dev);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

void OCMP_GenerateAlert(const AlertData *alert_data,
                        unsigned int alert_id,
                        const void *data)
{

}
/* ================================ Tests =================================== */
void test_probe()
{
    /* Test with the actual values
     * (dev id is hi-byte)
     * (1131h = NXP Semiconductors PCI-SIG)*/
    POSTData postData;
    SE98A_regs[0x07] = 0xA102;
    SE98A_regs[0x06] = 0x1131;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, SE98_fxnTable.cb_probe(&s_dev,
                                                                   &postData));

    /* Test with an incorrect device ID */
    SE98A_regs[0x07] = 0xFACE;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, SE98_fxnTable.cb_probe(&s_dev,
                                                                    &postData));

    /* Test with an incorrect mfg ID */
    SE98A_regs[0x07] = 0xA102;
    SE98A_regs[0x06] = 0xABCD;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, SE98_fxnTable.cb_probe(&s_dev,
                                                                    &postData));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, SE98_fxnTable.cb_probe(&s_invalid,
                                                                    &postData));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, SE98_fxnTable.cb_probe(&s_invalid_bus,
                                                                    &postData));
}

void test_init()
{
    SE98A_regs[0x01] = 0xFFFF;
    SE98A_regs[0x02] = 0xFFFF;
    SE98A_regs[0x03] = 0xFFFF;
    SE98A_regs[0x04] = 0xFFFF;
    AlertData alert_data = {
        .subsystem = 6,
        .componentId = 2,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;

    TEST_ASSERT_EQUAL(4 , SE98_fxnTable.cb_init(&s_dev,
                        &fact_ap_se98a_ts1_cfg, alert_data_cp));

    /* Test temprature values */
    TEST_ASSERT_EQUAL_HEX16(0x1EC0, SE98A_regs[0x03]);
    TEST_ASSERT_EQUAL_HEX16(0x04B0, SE98A_regs[0x02]);
    TEST_ASSERT_EQUAL_HEX16(0x0500, SE98A_regs[0x04]);


    /* Now try to init with a pin associated */
    SE98A_Dev alerted_dev = {
        .cfg = {
            .dev = s_dev.cfg.dev,
            .pin_evt = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        },
    };
    s_task_sleep_ticks = 0;
    TEST_ASSERT_EQUAL(4 , SE98_fxnTable.cb_init(&alerted_dev,
                        &fact_ap_se98a_ts1_cfg, alert_data_cp));

    TEST_ASSERT_MESSAGE(s_task_sleep_ticks >= 125,
        "Didn't wait long enough after setting alert window");

    /* Test that the enable alert flag is set */
    TEST_ASSERT_BITS_HIGH(0x08, SE98A_regs[0x01]);

    /* Test temprature values */
    TEST_ASSERT_EQUAL_HEX16(0x1EC0, SE98A_regs[0x03]);
    TEST_ASSERT_EQUAL_HEX16(0x04B0, SE98A_regs[0x02]);
    TEST_ASSERT_EQUAL_HEX16(0x0500, SE98A_regs[0x04]);
}

void test_get_status()
{
    /*
     * [15..13] Trip Status
     * [12..5] 8-bit integer part
     * [4..1] fractional part
     * [0] RFU
     */
    int8_t temp;
    Se98aStatus paramId = SE98A_STATUS_TEMPERATURE;
    SE98A_regs[0x05] = 0x019C; /* 25.75 */
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(26, temp);

    SE98A_regs[0x05] = 0x1E64; /* -25.75 */
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(-26, temp);

    SE98A_regs[0x05] = 0x07C0;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(124, temp);

    SE98A_regs[0x05] = 0x1FF0;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(-1, temp);

    SE98A_regs[0x05] = 0x1C90;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(-55, temp);

    /* The device shouldn't return temperatures larger than 125, so we only
     * support int8s - everything else is rounded for now */
    SE98A_regs[0x05] = 0x17E0; /* -130 */
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(-128, temp);

    SE98A_regs[0x05] = 0x0B40; /* 180 */
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(127, temp);

    /* Make sure we mask the status/RFU bits out */
    SE98A_regs[0x05] = 0xFC91;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_status(&s_dev, paramId, &temp));
    TEST_ASSERT_EQUAL(-55, temp);

    /* Test with a missing device */
    temp = 0;
    SE98A_regs[0x05] = 0xFC91;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_get_status(&s_invalid,
                                                    paramId, &temp));
    TEST_ASSERT_EQUAL(0, temp);

    /* Test with a missing bus*/
    temp = 0;
    SE98A_regs[0x05] = 0xFC91;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_get_status(&s_invalid_bus,
                                                    paramId, &temp));
    TEST_ASSERT_EQUAL(0, temp);

    /* Test with a invalid paramid*/
    temp = 0;
    SE98A_regs[0x05] = 0xFC91;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_get_status(&s_dev, 1, &temp));
    TEST_ASSERT_EQUAL(0, temp);
}

/* Helper to let us run through the various limits we can get */
static void test_get_x_limit(eTempSensor_ConfigParamsId limitToConfig,
                             uint8_t reg_addr)
{
    /* Register map:
     * [15..13] RFU
     * [12] sign
     * [11..4] 9-bit integer part
     * [3..2] fractional part (0.5, 0.25)
     * [1..0] RFU
     */
    int8_t limit;

    SE98A_regs[reg_addr] = 0x0000;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(0, limit);

    SE98A_regs[reg_addr] = 1 << 4;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(1, limit);

    SE98A_regs[reg_addr] = 75 << 4;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(75, limit);

    SE98A_regs[reg_addr] = 0x019C; /* 25.75 */
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(26, limit);

    SE98A_regs[reg_addr] = 0x1B50;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(-75, limit);

    SE98A_regs[reg_addr] = 0x07F0;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(127, limit);

    SE98A_regs[reg_addr] = 0x1800;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(-128, limit);

    /* Make sure we mask the RFU bits out */
    SE98A_regs[reg_addr] = 0x07FC;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_get_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(127, limit);

    /* Test with a missing device */
    limit = 0;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_get_config(&s_invalid, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL(0, limit);

    /* Test with a missing bus */
    limit = 0;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_get_config(&s_invalid_bus,
                                                    limitToConfig, &limit));
    TEST_ASSERT_EQUAL(0, limit);

    /* Test with a invalid paramId */
    limit = 0;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_get_config(&s_dev, 4, &limit));
    TEST_ASSERT_EQUAL(0, limit);
}

void test_temp_sens_get_limit(void)
{
    /* Register:
    *  SE98A_REG_HIGH_LIM = 0x02
    *  SE98A_REG_LOW_LIM  = 0x03
    *  SE98A_REG_CRIT_LIM = 0x04
    */
    test_get_x_limit(SE98A_CONFIG_LIM_LOW, 0x03);
    test_get_x_limit(SE98A_CONFIG_LIM_HIGH, 0x02);
    test_get_x_limit(SE98A_CONFIG_LIM_CRIT, 0x04);
}

/* Helper to let us run through the various limits we can set */
static void test_set_x_limit(eTempSensor_ConfigParamsId limitToConfig,
                             uint8_t reg_addr)
{
    /* Register map:
     * [15..13] RFU
     * [12]     SIGN (2's complement)
     * [11..4]  Integer part (8 bits)
     * [3..2]   Fractional part (0.5, 0.25)
     * [1..0]   RFU
     */
    int8_t limit = 0;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_set_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(0, SE98A_regs[reg_addr]);

    limit = 1;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_set_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(1 << 4, SE98A_regs[reg_addr]);

    limit = 75;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_set_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(75 << 4, SE98A_regs[reg_addr]);

    limit = -75;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_set_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(0x1B50, SE98A_regs[reg_addr]);

    limit = 127;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_set_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(0x07F0, SE98A_regs[reg_addr]);

    limit = -128;
    TEST_ASSERT_EQUAL(1, SE98_fxnTable.cb_set_config(&s_dev, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(0x1800, SE98A_regs[reg_addr]);

    /* Test with a missing device */
    SE98A_regs[reg_addr] = 0x000;
    limit = 20;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_set_config(&s_invalid, limitToConfig,
                                                                    &limit));
    TEST_ASSERT_EQUAL_HEX16(0x0000, SE98A_regs[reg_addr]);

    /* Test with a missing device */
    SE98A_regs[reg_addr] = 0x000;
    limit = 20;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_set_config(&s_invalid_bus,
                                                    limitToConfig, &limit));
    TEST_ASSERT_EQUAL_HEX16(0x0000, SE98A_regs[reg_addr]);

    /* Test with a missing device */
    SE98A_regs[reg_addr] = 0x000;
    limit = 20;
    TEST_ASSERT_EQUAL(0, SE98_fxnTable.cb_set_config(&s_dev, 4, &limit));
    TEST_ASSERT_EQUAL_HEX16(0x0000, SE98A_regs[reg_addr]);
}

void test_temp_sens_set_limit(void)
{
    /* Register:
    *  SE98A_REG_HIGH_LIM = 0x02
    *  SE98A_REG_LOW_LIM  = 0x03
    *  SE98A_REG_CRIT_LIM = 0x04
    */
    test_set_x_limit(SE98A_CONFIG_LIM_LOW, 0x03);
    test_set_x_limit(SE98A_CONFIG_LIM_HIGH, 0x02);
    test_set_x_limit(SE98A_CONFIG_LIM_CRIT, 0x04);
}
