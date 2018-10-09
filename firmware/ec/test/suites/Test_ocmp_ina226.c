#include "include/test_ina226.h"

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(INA226_GpioPins, INA226_GpioConfig);
    fake_I2C_init();

    fake_I2C_registerDevSimple(ina226_dev.cfg.dev.bus, ina226_dev.cfg.dev.slave_addr,
                               INA226_regs, sizeof(INA226_regs) + 1,
                               sizeof(INA226_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(INA226_regs, 0, sizeof(INA226_regs));

    OcGpio_init(&s_fake_io_port);

    ina226_init(&ina226_dev);
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
    POSTData postData;
    /* Test with the actual values */
    INA226_regs[0xFF] = 0x2260;
    INA226_regs[0xFE] = 0x5449;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, INA226_fxnTable.cb_probe(&ina226_dev,
                                                                &postData));

    /* Test with an incorrect device ID */
    INA226_regs[0xFF] = 0xC802;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, INA226_fxnTable.cb_probe(
                                                    &ina226_dev, &postData));

    /* Test with an incorrect mfg ID */
    INA226_regs[0xFF] = 0x2260;
    INA226_regs[0xFE] = 0x5DC7;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, INA226_fxnTable.cb_probe(
                                                    &ina226_dev, &postData));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, INA226_fxnTable.cb_probe(
                                            &ina226_invalid_dev, &postData));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, INA226_fxnTable.cb_probe(
                                            &ina226_invalid_bus, &postData));
}

void test_init()
{
    INA226_regs[0x00] = 0x1234;
    INA226_regs[0x05] = 0x0000; /* Calibration reg */
    INA226_regs[0x07] = 0x0320; //800
    AlertData alert_data = {
        .subsystem = 7,
        .componentId = 1,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(4, INA226_fxnTable.cb_init(&ina226_dev,
                        &fact_sdr_3v_ps_cfg, alert_data_cp));

    /* Make sure we've reset the device */
    TEST_ASSERT_BITS_HIGH(0x0001, INA226_regs[0x00]);

    /* Make sure calibration register gets initialized */
    TEST_ASSERT_NOT_EQUAL(0, INA226_regs[0x05]);
    TEST_ASSERT_EQUAL_HEX16(0x0960, INA226_regs[0x07]); //2400
    TEST_ASSERT_EQUAL_HEX16(0x8000, INA226_regs[0x06]);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(6, INA226_fxnTable.cb_init(&ina226_invalid_dev,
                        &fact_sdr_3v_ps_cfg, alert_data_cp));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(6, INA226_fxnTable.cb_init(&ina226_invalid_bus,
                        &fact_sdr_3v_ps_cfg, alert_data_cp));
}

void test_get_status()
{
    uint16_t busVoltage_val = 0xffff;
    uint16_t shuntVoltage_val = 0xffff;
    uint16_t current_val = 0xffff;
    uint16_t power_val = 0xffff;

    INA226_regs[0x02] = 0x2580; //9600
    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_get_status(&ina226_dev,
                            INA226_STATUS_BUS_VOLTAGE,&busVoltage_val));
    TEST_ASSERT_EQUAL_HEX16(12000, busVoltage_val); //12000mV

    INA226_regs[0x01] = 0x0168; //360
    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_get_status(&ina226_dev,
                            INA226_STATUS_SHUNT_VOLTAGE,&shuntVoltage_val));
    TEST_ASSERT_EQUAL_HEX16(900, shuntVoltage_val); //900uV

    INA226_regs[0x04] = 0x1388; //5000
    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_get_status(&ina226_dev,
                            INA226_STATUS_CURRENT,&current_val));
    TEST_ASSERT_EQUAL_HEX16(500, current_val);  //500mA

    INA226_regs[0x03] = 0x02A8; //680
    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_get_status(&ina226_dev,
                            INA226_STATUS_POWER,&power_val));
    TEST_ASSERT_EQUAL_HEX16(1700, power_val); //1700mW

    /* Test with a invalid paramID */
    INA226_regs[0x03] = 0x02A8; //680
    power_val = 0;
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_get_status(&ina226_dev,
                                                4, &power_val));
    TEST_ASSERT_EQUAL_HEX16(0, power_val); //1700mW

    /* Test with a missing device */
    INA226_regs[0x03] = 0x02A8; //680
    power_val = 0;
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_get_status(&ina226_invalid_dev,
                                INA226_STATUS_POWER, &power_val));
    TEST_ASSERT_EQUAL_HEX16(0, power_val); //1700mW

    /* Test with a missing bus */
    INA226_regs[0x03] = 0x02A8; //680
    power_val = 0;
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_get_status(&ina226_invalid_bus,
                            INA226_STATUS_POWER,&power_val));
    TEST_ASSERT_EQUAL_HEX16(0, power_val); //1700mW
}

void test_get_config()
{
    uint16_t current_val = 0xffff;
    INA226_regs[0x07] = 0x0320; //800

    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_get_config(&ina226_dev,
                                INA226_CONFIG_CURRENT_LIM, &current_val));
    TEST_ASSERT_EQUAL(1000, current_val); //1000mA

    /* Test with a invalid param id */
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_get_config(&ina226_dev,
                                                    1, &current_val));
    /* Test with a missing device */
    current_val = 0;
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_get_config(&ina226_invalid_dev,
                                INA226_CONFIG_CURRENT_LIM, &current_val));
    TEST_ASSERT_EQUAL(0, current_val);

    /* Test with a missing device */
    current_val = 0;
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_get_config(&ina226_invalid_bus,
                                INA226_CONFIG_CURRENT_LIM, &current_val));
    TEST_ASSERT_EQUAL(0, current_val);
}

void test_set_config()
{
    uint16_t current_val = 0x0BB8;
    INA226_regs[0x07] = 0x0320; //800
    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_set_config(&ina226_dev,
                                INA226_CONFIG_CURRENT_LIM, &current_val));
    TEST_ASSERT_EQUAL_HEX16(0x0960, INA226_regs[0x07]); //2400

    /* Test with a invalid param id */
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_set_config(&ina226_dev,
                                                    2, &current_val));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_set_config(&ina226_invalid_dev,
                                INA226_CONFIG_CURRENT_LIM, &current_val));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_set_config(&ina226_invalid_bus,
                                INA226_CONFIG_CURRENT_LIM, &current_val));
}

void test_invalid()
{
    uint16_t current_val = 66540;
    int16_t value = -345;
    INA226_regs[0x07] = 0x0320; //800
    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_set_config(&ina226_dev,
                                INA226_CONFIG_CURRENT_LIM, &current_val));
    TEST_ASSERT_EQUAL(803, INA226_regs[0x07]);

    TEST_ASSERT_EQUAL(1, INA226_fxnTable.cb_set_config(&ina226_dev,
                                INA226_CONFIG_CURRENT_LIM, &value));
    TEST_ASSERT_EQUAL(52152, INA226_regs[0x07]);

    TEST_ASSERT_EQUAL(0, INA226_fxnTable.cb_set_config(&ina226_dev, 1,
                                                            &current_val));
}
