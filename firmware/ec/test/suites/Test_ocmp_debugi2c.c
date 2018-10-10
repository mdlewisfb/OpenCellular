#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "common/inc/ocmp_wrappers/ocmp_debugi2c.h"
#include "inc/devices/debug_oci2c.h"
#include <string.h>
#include "unity.h"

/* ============================= Fake Functions ============================= */
#include <ti/sysbios/knl/Task.h>
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E( xdc_UInt32 nticks )
{
    s_task_sleep_ticks += nticks;
}
/* ======================== Constants & variables =========================== */
static const S_I2C_Cfg I2C_DEV = {
    .bus = 7,
};

static const S_I2C_Cfg I2C_INVALID_DEV = {
    .bus = 10,
};

S_OCI2C s_oci2c = {
    .slaveAddress = 0x2F,
    .reg_address = 1,
    .reg_value = 12,
    .number_of_bytes = 1
};

S_OCI2C s_oci2c_invalid = {
    .slaveAddress = 0x48,
    .reg_address = 1,
    .reg_value = 12,
    .number_of_bytes = 1
};

static uint8_t DEBUG_I2C_regs[] = {
    [0x00] = 0x00, /* INTERRUPT_STATUS */
    [0x01] = 0x00, /* INTERRUPT_MASK  */
};
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();
    fake_I2C_registerDevSimple(I2C_DEV.bus, s_oci2c.slaveAddress, &DEBUG_I2C_regs,
                               sizeof(DEBUG_I2C_regs), sizeof(DEBUG_I2C_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(DEBUG_I2C_regs, 0, sizeof(DEBUG_I2C_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
/* ================================ Tests =================================== */
void test_i2c_read(void)
{
    uint8_t value = 1;

    DEBUG_I2C_regs[0x01] = 0x5A;

    TEST_ASSERT_EQUAL(true, i2c_read(&I2C_DEV, &s_oci2c));
    TEST_ASSERT_EQUAL_HEX8(0x5A, s_oci2c.reg_value);

    TEST_ASSERT_EQUAL(false, i2c_read(&I2C_INVALID_DEV, &s_oci2c));
    TEST_ASSERT_EQUAL(false, i2c_read(&I2C_DEV, &s_oci2c_invalid));
}

void test_i2c_write(void)
{
    uint8_t read = 0xff;
    DEBUG_I2C_regs[0x01] = 0x0A;

    TEST_ASSERT_EQUAL(true, i2c_write(&I2C_DEV, &s_oci2c));
    TEST_ASSERT_EQUAL_HEX8(0x5A, s_oci2c.reg_value);
    TEST_ASSERT_EQUAL(false, i2c_read(&I2C_INVALID_DEV, &s_oci2c));
    TEST_ASSERT_EQUAL(false, i2c_read(&I2C_DEV, &s_oci2c_invalid));
}