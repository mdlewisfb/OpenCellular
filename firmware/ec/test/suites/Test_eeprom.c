/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_eeprom.h"

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
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
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
void test_eeprom_init(void)
{
    EEPROM_regs[0x00] = 0x0505;
    Eeprom_GpioConfig[0x02] = OCGPIO_CFG_OUT_HIGH;

    TEST_ASSERT_EQUAL(1, eeprom_init(&eeprom_gbc_sid));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                                    Eeprom_GpioConfig[0x02]);

    /*Checking for NULL cfg */
    TEST_ASSERT_EQUAL(0, eeprom_init(invalid_cfg));

    /*Checking with invalid slave address */
    TEST_ASSERT_EQUAL(0, eeprom_init(&s_invalid_dev));

    /*Checking with invalid bus address */
    TEST_ASSERT_EQUAL(0, eeprom_init(&s_invalid_bus));
}

void test_eeprom_read(void)
{
    uint16_t buffer;
    EEPROM_regs[0xC601] = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read(&eeprom_gbc_sid, 0x01C6,
                                    &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX(0x0505, buffer);

    /*Checking with out of range value*/
    buffer = 0x0000;
    EEPROM_regs[0xC601] = 0x10003;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read(&eeprom_gbc_sid, 0x01C6,
                                    &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX16(0x0003, buffer);

    /*Checking for NULL cfg */
    buffer = 0x0000;
    EEPROM_regs[0xC601] = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read(invalid_cfg, 0x01C6,
                                    &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX16(0x0000, buffer);

    /*Checking with invalid slave address */
    buffer = 0x0000;
    EEPROM_regs[0xC601] = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read(&s_invalid_dev, 0x01C6,
                                    &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX16(0x0000, buffer);

    /*Checking with invalid bus address */
    buffer = 0x0000;
    EEPROM_regs[0xC601] = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read(&s_invalid_bus, 0x01C6,
                                    &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX16(0x0000, buffer);
}

void test_eeprom_write(void)
{
    uint16_t buffer = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write(&eeprom_gbc_inv, 0x01C6,
                                                            &buffer,0x0A));
    TEST_ASSERT_EQUAL_HEX16(0x0505, EEPROM_regs[0xC601]);

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write(&eeprom_gbc_inv, 0x01C6,
                                                            &buffer, 0xCA));
    TEST_ASSERT_EQUAL_HEX8(0x05, EEPROM_regs[0xC601]);

    /*Checking with out of range value*/
    buffer = 0x10003;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write(&eeprom_gbc_inv, 0x01C6,
                                                            &buffer,0x0A));
    TEST_ASSERT_EQUAL_HEX16(0x0003, EEPROM_regs[0xC601]);

    /*Checking for NULL cfg */
    EEPROM_regs[0xC601] = 0x0000;
    buffer = 0x0506;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_write(invalid_cfg, 0x01C6,
                                                        &buffer, 0x0A));
    TEST_ASSERT_EQUAL_HEX8(0x0000, EEPROM_regs[0xC601]);

    /*Checking with invalid slave address */
    EEPROM_regs[0xC601] = 0x0000;
    buffer = 0x0506;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_write(&s_invalid_dev, 0x01C6,
                                                            &buffer,0x0A));
    TEST_ASSERT_EQUAL_HEX8(0x0000, EEPROM_regs[0xC601]);

    /*Checking with invalid bus address */
    EEPROM_regs[0xC601] = 0x0000;
    buffer = 0x0506;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_write(&s_invalid_bus, 0x01C6,
                                                            &buffer,0x0A));
    TEST_ASSERT_EQUAL_HEX8(0x0000, EEPROM_regs[0xC601]);

}

void test_eeprom_disable_write(void)
{
    SX1509_regs[0x10] = 0x01;
    SX1509_regs[0x11] = 0x01;

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_disable_write(&enable_dev));
    TEST_ASSERT_EQUAL(0xFF, SX1509_regs[0x11]);

    /*Checking for NULL cfg */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_disable_write(invalid_cfg));

    /*Checking with invalid slave address */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_disable_write(&s_invalid_dev));

    /*Checking with invalid bus address */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_disable_write(&s_invalid_bus));
}

void test_eeprom_enable_write(void)
{
    SX1509_regs[0x10] = 0x00;
    SX1509_regs[0x11] = 0x00;

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_enable_write(&enable_dev));
    TEST_ASSERT_EQUAL(0xFB, SX1509_regs[0x11]);

    /*Checking for NULL cfg */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_enable_write(invalid_cfg));

    /*Checking with invalid slave address */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_enable_write(&s_invalid_dev));

    /*Checking with invalid bus address */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_enable_write(&s_invalid_bus));
}

void test_eeprom_read_board_info(void)
{
    char *buffer = (char *) malloc(36);
    EEPROM_regs[0xAC01] = 0x4153;
    EEPROM_regs[0xAC02] = 0x3731;
    EEPROM_regs[0xAC03] = 0x3831;
    EEPROM_regs[0xAC04] = 0x494c;
    EEPROM_regs[0xAC05] = 0x4546;
    EEPROM_regs[0xAC06] = 0x4733;
    EEPROM_regs[0xAC07] = 0x4342;
    EEPROM_regs[0xAC08] = 0x3030;
    EEPROM_regs[0xAC09] = 0x3134;
    memset(buffer,0,36);

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&eeprom_gbc_inv, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3GBC0041",buffer);

    EEPROM_regs[0xAC01] = 0x4153;
    EEPROM_regs[0xAC02] = 0x3731;
    EEPROM_regs[0xAC03] = 0x3831;
    EEPROM_regs[0xAC04] = 0x494c;
    EEPROM_regs[0xAC05] = 0x4546;
    EEPROM_regs[0xAC06] = 0x5333;
    EEPROM_regs[0xAC07] = 0x5244;
    EEPROM_regs[0xAC08] = 0x3030;
    EEPROM_regs[0xAC09] = 0x3233;
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&eeprom_sdr_inv, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3SDR0032",buffer);

    EEPROM_regs[0xAC01] = 0x4153;
    EEPROM_regs[0xAC02] = 0x3731;
    EEPROM_regs[0xAC03] = 0x3831;
    EEPROM_regs[0xAC04] = 0x494c;
    EEPROM_regs[0xAC05] = 0x4546;
    EEPROM_regs[0xAC06] = 0x4633;
    EEPROM_regs[0xAC07] = 0x3045;
    EEPROM_regs[0xAC08] = 0x3030;
    EEPROM_regs[0xAC09] = 0x0035;
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&eeprom_fe_inv, buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3FE0005", buffer);

    /* While setting more then 18 bytes*/
    EEPROM_regs[0xAC01] = 0x4153;
    EEPROM_regs[0xAC02] = 0x3731;
    EEPROM_regs[0xAC03] = 0x3831;
    EEPROM_regs[0xAC04] = 0x494c;
    EEPROM_regs[0xAC05] = 0x4546;
    EEPROM_regs[0xAC06] = 0x4733;
    EEPROM_regs[0xAC07] = 0x4342;
    EEPROM_regs[0xAC08] = 0x3030;
    EEPROM_regs[0xAC09] = 0x3134;
    EEPROM_regs[0xAC0A] = 0x3134;
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&eeprom_gbc_inv,
                                                            buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE3GBC0041",buffer);

    /*Checking for NULL cfg */
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read_board_info(invalid_cfg,
                                                                buffer));
    TEST_ASSERT_EQUAL_STRING("\0",buffer);

    /*Checking with invalid slave address */
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read_board_info(&s_invalid_dev,
                                                                buffer));
    TEST_ASSERT_EQUAL_STRING("\0",buffer);

    /*Checking with invalid bus address */
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read_board_info(&s_invalid_bus,
                                                                buffer));
    TEST_ASSERT_EQUAL_STRING("\0",buffer);
}

void test_eeprom_read_oc_info(void)
{
    char *buffer = (char *) malloc(36);
    EEPROM_regs[0xC601] = 0x4153;
    EEPROM_regs[0xC602] = 0x3731;
    EEPROM_regs[0xC603] = 0x3831;
    EEPROM_regs[0xC604] = 0x3043;
    EEPROM_regs[0xC605] = 0x3534;
    EEPROM_regs[0xC606] = 0x4130;
    EEPROM_regs[0xC607] = 0x3031;
    EEPROM_regs[0xC608] = 0x3430;
    EEPROM_regs[0xC609] = 0x3131;
    memset(buffer,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_oc_info(buffer));
    TEST_ASSERT_EQUAL_STRING("SA1718C0450A100411", buffer);
}

void test_eeprom_read_device_info_record(void)
{
    uint8_t recordno= 1;
    EEPROM_regs[0x0A01] = 0x4153;
    EEPROM_regs[0x0A02] = 0x3731;
    EEPROM_regs[0x0A03] = 0x3831;
    EEPROM_regs[0x0A04] = 0x3043;
    EEPROM_regs[0x0A05] = 0x3534;
    char *deviceinfo = (char *) malloc(36);

    memset(deviceinfo,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(&eeprom_gbc_inv,
                                                    recordno, deviceinfo));
    TEST_ASSERT_EQUAL_STRING("SA1718C045", deviceinfo);

    uint8_t recordno1= 1;
    EEPROM_regs[0x0A01] = 0x4153;
    EEPROM_regs[0x0A02] = 0x3731;
    EEPROM_regs[0x0A03] = 0x3831;
    EEPROM_regs[0x0A04] = 0x494c;
    EEPROM_regs[0x0A05] = 0x4546;
    char *deviceinfo1 = (char *) malloc(36);
    memset(deviceinfo1,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(&eeprom_sdr_inv,
                                                    recordno1, deviceinfo1));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE", deviceinfo1);

    uint8_t recordno2= 1;
    EEPROM_regs[0x0A01] = 0x4153;
    EEPROM_regs[0x0A02] = 0x3731;
    EEPROM_regs[0x0A03] = 0x3831;
    EEPROM_regs[0x0A04] = 0x494c;
    EEPROM_regs[0x0A05] = 0x4546;
    char *deviceinfo2 = (char *) malloc(36);
    memset(deviceinfo2,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(&eeprom_fe_inv,
                                                    recordno2, deviceinfo2));
    TEST_ASSERT_EQUAL_STRING("SA1718LIFE", deviceinfo2);

    /*Trying to read from more then 0X0A bytes*/
    EEPROM_regs[0x0A01] = 0x4153;
    EEPROM_regs[0x0A02] = 0x3731;
    EEPROM_regs[0x0A03] = 0x3831;
    EEPROM_regs[0x0A04] = 0x3043;
    EEPROM_regs[0x0A05] = 0x3534;
    EEPROM_regs[0x0A06] = 0x4130;
    EEPROM_regs[0x0A07] = 0x3031;
    EEPROM_regs[0x0A08] = 0x3430;
    EEPROM_regs[0x0A09] = 0x3131;
    char *deviceinfo4 = (char *) malloc(36);
    memset(deviceinfo4,0,36);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(&eeprom_gbc_inv,
                                                    recordno, deviceinfo4));
    TEST_ASSERT_EQUAL_STRING("SA1718C045", deviceinfo4);

    /*Checking for NULL cfg */
    memset(deviceinfo4,0,36);
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read_device_info_record(invalid_cfg,
                                                    recordno, deviceinfo4));
    TEST_ASSERT_EQUAL_STRING("\0", deviceinfo4);

    /*Checking with invalid slave address */
    memset(deviceinfo4,0,36);
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read_device_info_record(
                                        &s_invalid_dev, recordno, deviceinfo4));
    TEST_ASSERT_EQUAL_STRING("\0", deviceinfo4);

    /*Checking with invalid bus address */
    memset(deviceinfo4,0,36);
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_read_device_info_record(
                                        &s_invalid_bus, recordno, deviceinfo4));
    TEST_ASSERT_EQUAL_STRING("\0", deviceinfo4);
}

void test_eeprom_write_device_info_record(void)
{
    uint8_t recordno= 1;
    char *deviceinfo = (char *) malloc(10);
    memset(deviceinfo,0,10);

    strcpy(deviceinfo,"SA1718C045");
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(&eeprom_gbc_inv,
                                                recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4153, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x3731, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x3831, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x3043, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x3534, EEPROM_regs[0x0A05]);

    strcpy(deviceinfo, "SA1718LIFE");
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(&eeprom_sdr_inv,
                                                recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4153, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x3731, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x3831, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x494c, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x4546, EEPROM_regs[0x0A05]);

    strcpy(deviceinfo, "SA1718LIFE");
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(&eeprom_fe_inv,
                                                recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4153, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x3731, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x3831, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x494c, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x4546, EEPROM_regs[0x0A05]);

    /*Trying to write from more then 0X0A bytes*/
    EEPROM_regs[0x0A01] = 0x0000;
    EEPROM_regs[0x0A02] = 0x0000;
    EEPROM_regs[0x0A03] = 0x0000;
    EEPROM_regs[0x0A04] = 0x0000;
    EEPROM_regs[0x0A05] = 0x0000;
    EEPROM_regs[0x0A06] = 0x0000;
    EEPROM_regs[0x0A07] = 0x0000;
    EEPROM_regs[0x0A08] = 0x0000;
    EEPROM_regs[0x0A09] = 0x0000;
    memset(deviceinfo,0,10);
    strcpy(deviceinfo,"SA1718C0450A100411");
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(&eeprom_gbc_inv,
                                                    recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4153, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x3731, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x3831, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x3043, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x3534, EEPROM_regs[0x0A05]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A06]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A07]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A08]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A09]);

    /*Checking for NULL cfg */
    EEPROM_regs[0x0A01] = 0x0000;
    EEPROM_regs[0x0A02] = 0x0000;
    EEPROM_regs[0x0A03] = 0x0000;
    EEPROM_regs[0x0A04] = 0x0000;
    EEPROM_regs[0x0A05] = 0x0000;
    EEPROM_regs[0x0A06] = 0x0000;
    EEPROM_regs[0x0A07] = 0x0000;
    EEPROM_regs[0x0A08] = 0x0000;
    EEPROM_regs[0x0A09] = 0x0000;
    strcpy(deviceinfo,"SA1718C0450A100411");
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_write_device_info_record(invalid_cfg,
                                                        recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A05]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A06]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A07]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A08]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A09]);


    /*Checking with invalid slave address */
    EEPROM_regs[0x0A01] = 0x0000;
    EEPROM_regs[0x0A02] = 0x0000;
    EEPROM_regs[0x0A03] = 0x0000;
    EEPROM_regs[0x0A04] = 0x0000;
    EEPROM_regs[0x0A05] = 0x0000;
    EEPROM_regs[0x0A06] = 0x0000;
    EEPROM_regs[0x0A07] = 0x0000;
    EEPROM_regs[0x0A08] = 0x0000;
    EEPROM_regs[0x0A09] = 0x0000;
    strcpy(deviceinfo,"SA1718C0450A100411");
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_write_device_info_record(&s_invalid_dev,
                                                            recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A05]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A06]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A07]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A08]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A09]);

    /*Checking with invalid bus address */
    EEPROM_regs[0x0A01] = 0x0000;
    EEPROM_regs[0x0A02] = 0x0000;
    EEPROM_regs[0x0A03] = 0x0000;
    EEPROM_regs[0x0A04] = 0x0000;
    EEPROM_regs[0x0A05] = 0x0000;
    EEPROM_regs[0x0A06] = 0x0000;
    EEPROM_regs[0x0A07] = 0x0000;
    EEPROM_regs[0x0A08] = 0x0000;
    EEPROM_regs[0x0A09] = 0x0000;
    strcpy(deviceinfo,"SA1718C0450A100411");
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_write_device_info_record(&s_invalid_bus,
                                                            recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A01]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A02]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A03]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A04]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A05]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A06]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A07]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A08]);
    TEST_ASSERT_EQUAL(0x0000, EEPROM_regs[0x0A09]);
}
