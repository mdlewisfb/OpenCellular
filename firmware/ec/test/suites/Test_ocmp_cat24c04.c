#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_eeprom_cat24c04.h"
#include "platform/oc-sdr/schema.h"
#include "include/test_eeprom.h"

/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E( xdc_UInt32 nticks )
{
    s_task_sleep_ticks += nticks;
}

bool SYS_post_enable(void **postActivate) {
    return 0;
}

bool SYS_post_get_results(void **getpostResult) {
    return 0;
}
/* ======================== Constants & variables =========================== */
Eeprom_Cfg eeprom_gbc_sid = {
    .i2c_dev = { 6, 0x51 },
    .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
    .type = { .page_size = 64, .mem_size = (256 / 8) },
    .ss = 0,
};
/* ============================= Boilerplate ================================ */
void EEPROM_init(Eeprom_Cfg *eeprom_gbc_sid)
{
    int8_t ret = 0;
}

void suite_setUp(void)
{
    fake_I2C_init();
    FakeGpio_registerDevSimple(Eeprom_GpioPins, Eeprom_GpioConfig);
    fake_I2C_registerDevSimple(I2C_DEV.bus, I2C_DEV.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs) * 2, sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(I2C_DEV_1.bus, I2C_DEV_1.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(I2C_DEV_SDR.bus, I2C_DEV_SDR.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(I2C_DEV_FE.bus, I2C_DEV_FE.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(6, 0x45, SX1509_regs,
                               sizeof(SX1509_regs), sizeof(SX1509_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(EEPROM_regs, 0, sizeof(EEPROM_regs));
    OcGpio_init(&s_fake_io_port);
    OcGpio_init(&s_fake_io_exp);
    EEPROM_init(&eeprom_gbc_sid);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_init_eeprom()
{
    SX1509_regs[0x10] = 0x00;
    SX1509_regs[0x11] = 0x00;
    EEPROM_regs[0x00] = 0x0505;
    EEPROM_regs[0xFFFF] = 0x0001;

    Eeprom_GpioConfig[0x02] = OCGPIO_CFG_OUT_HIGH;
    Eeprom_Cfg init_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &pin_inven_eeprom_wp,
        .type = NULL,
        .ss = 0,
    };
    AlertData alert_data = {
        .subsystem = 0,
        .componentId = 1,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;

    TEST_ASSERT_EQUAL(4, CAT24C04_gbc_sid_fxnTable.cb_init(&init_dev,
                                                NULL, alert_data_cp));
    TEST_ASSERT_EQUAL_HEX8(0x01, EEPROM_regs[0xFFFF]);

    TEST_ASSERT_EQUAL(0xFF, SX1509_regs[0x11]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                                            Eeprom_GpioConfig[0x02]);
    TEST_ASSERT_EQUAL_HEX8(0x01, EEPROM_regs[0xFFFF]);

    /*Invalid Test*/
    TEST_ASSERT_EQUAL(6, CAT24C04_gbc_sid_fxnTable.cb_init(&s_invalid_dev,
                                                        NULL, alert_data_cp));
}

void test_RFFE_InventoryGetStatus()
{
    char *buffer = (char *) malloc(36);

    EEPROM_regs[0xAC01] = 0x4153; /* Init */
    EEPROM_regs[0xAC02] = 0x3731; /* SERIAL INFO */
    EEPROM_regs[0xAC03] = 0x3831;/* BOARD INFO */
    EEPROM_regs[0xAC04] = 0x494c;/* DEVICE INFO */
    EEPROM_regs[0xAC05] = 0x4546; /* DEVICE INFO */
    EEPROM_regs[0xAC06] = 0x4633; /* DEVICE INFO */
    EEPROM_regs[0xAC07] = 0x3045; /* DEVICE INFO */
    EEPROM_regs[0xAC08] = 0x3030; /* DEVICE INFO */
    EEPROM_regs[0xAC09] = 0x0035; /* DEVICE INFO */
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(1, CAT24C04_fe_inv_fxnTable.cb_get_status(&eeprom_fe_inv,
                                                            0, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3FE0005", buffer);

    /*Invalid Param ID */
    memset(buffer,0,36);
    EEPROM_regs[0xAC01] = 0x4153; /* Init */
    EEPROM_regs[0xAC02] = 0x3731; /* SERIAL INFO */
    EEPROM_regs[0xAC03] = 0x3831;/* BOARD INFO */
    EEPROM_regs[0xAC04] = 0x494c;/* DEVICE INFO */
    EEPROM_regs[0xAC05] = 0x4546; /* DEVICE INFO */
    EEPROM_regs[0xAC06] = 0x4633; /* DEVICE INFO */
    EEPROM_regs[0xAC07] = 0x3045; /* DEVICE INFO */
    EEPROM_regs[0xAC08] = 0x3030; /* DEVICE INFO */
    EEPROM_regs[0xAC09] = 0x0035; /* DEVICE INFO */
    TEST_ASSERT_EQUAL(0, CAT24C04_fe_inv_fxnTable.cb_get_status(&eeprom_fe_inv,
                                                                1, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);
}

void test_Sdr_InventoryGetStatus()
{
    char *buffer = (char *) malloc(36);

    EEPROM_regs[0xAC01] = 0x4153; /* Init */
    EEPROM_regs[0xAC02] = 0x3731; /* SERIAL INFO */
    EEPROM_regs[0xAC03] = 0x3831;/* BOARD INFO */
    EEPROM_regs[0xAC04] = 0x494c;/* DEVICE INFO */
    EEPROM_regs[0xAC05] = 0x4546; /* DEVICE INFO */
    EEPROM_regs[0xAC06] = 0x5333; /* DEVICE INFO */
    EEPROM_regs[0xAC07] = 0x5244; /* DEVICE INFO */
    EEPROM_regs[0xAC08] = 0x3030; /* DEVICE INFO */
    EEPROM_regs[0xAC09] = 0x3233; /* DEVICE INFO */
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(1, CAT24C04_sdr_inv_fxnTable.cb_get_status(&eeprom_sdr_inv,
                                                            0, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3SDR0032",buffer);

    /*Invalid Param ID*/
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(0, CAT24C04_sdr_inv_fxnTable.cb_get_status(&eeprom_sdr_inv,
                                                                    1, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);
}

void test_sid_get_status_parameters_data()
{
    char *buffer = (char *) malloc(36);
    /*For OC_STAT_SYS_SERIAL_ID */
    EEPROM_regs[0xC601] = 0x4153; /* Init */
    EEPROM_regs[0xC602] = 0x3731; /* SERIAL INFO */
    EEPROM_regs[0xC603] = 0x3831;/* BOARD INFO */
    EEPROM_regs[0xC604] = 0x3043;/* DEVICE INFO */
    EEPROM_regs[0xC605] = 0x3534; /* DEVICE INFO */
    EEPROM_regs[0xC606] = 0x4130; /* DEVICE INFO */
    EEPROM_regs[0xC607] = 0x3031; /* DEVICE INFO */
    EEPROM_regs[0xC608] = 0x3430; /* DEVICE INFO */
    EEPROM_regs[0xC609] = 0x3131; /* DEVICE INFO */
    memset(buffer,0,36);

    TEST_ASSERT_EQUAL(1, CAT24C04_gbc_sid_fxnTable.cb_get_status(&eeprom_gbc_sid,
                                                                0, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718C0450A100411", buffer);

    /*For OC_STAT_SYS_GBC_BOARD_ID */
    memset(buffer,0,36);
    EEPROM_regs[0xAC01] = 0x4153; /* Init */
    EEPROM_regs[0xAC02] = 0x3731; /* SERIAL INFO */
    EEPROM_regs[0xAC03] = 0x3831;/* BOARD INFO */
    EEPROM_regs[0xAC04] = 0x494c;/* DEVICE INFO */
    EEPROM_regs[0xAC05] = 0x4546; /* DEVICE INFO */
    EEPROM_regs[0xAC06] = 0x4733; /* DEVICE INFO */
    EEPROM_regs[0xAC07] = 0x4342; /* DEVICE INFO */
    EEPROM_regs[0xAC08] = 0x3030; /* DEVICE INFO */
    EEPROM_regs[0xAC09] = 0x3134; /* DEVICE INFO */

    TEST_ASSERT_EQUAL(1, CAT24C04_gbc_sid_fxnTable.cb_get_status(&eeprom_gbc_sid,
                                                                1, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3GBC0041",buffer);

    /* Invalid ParamID */
    memset(buffer,0,36);
    EEPROM_regs[0xAC01] = 0x4153; /* Init */
    EEPROM_regs[0xAC02] = 0x3731; /* SERIAL INFO */
    EEPROM_regs[0xAC03] = 0x3831;/* BOARD INFO */
    EEPROM_regs[0xAC04] = 0x494c;/* DEVICE INFO */
    EEPROM_regs[0xAC05] = 0x4546; /* DEVICE INFO */
    EEPROM_regs[0xAC06] = 0x4733; /* DEVICE INFO */
    EEPROM_regs[0xAC07] = 0x4342; /* DEVICE INFO */
    EEPROM_regs[0xAC08] = 0x3030; /* DEVICE INFO */
    EEPROM_regs[0xAC09] = 0x3134; /* DEVICE INFO */

    TEST_ASSERT_EQUAL(0, CAT24C04_gbc_sid_fxnTable.cb_get_status(&eeprom_gbc_sid,
                                                                2, buffer));
    TEST_ASSERT_EQUAL_STRING("\0",buffer);
}
