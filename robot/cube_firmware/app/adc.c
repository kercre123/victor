#include <stdint.h>

#include "datasheet.h"
#include "board.h"
#include "adc.h"

#include "protocol.h"

extern void (*ble_send)(uint8_t length, const void* data);

#define ADC_TIMEOUT             (100000)
#define ADC_CHANNEL_VBAT1V      (9)
#define ADC_CHANNEL_VBAT3V      (7)

void adc_usDelay(uint32_t nof_us)
{
    while( nof_us-- ){
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
    }
}

static int adc_get_sample(void)
{
    int cnt = ADC_TIMEOUT;

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_START, 1);
    while (cnt-- && (GetWord16(GP_ADC_CTRL_REG) & GP_ADC_START) != 0x0000);
    SetWord16(GP_ADC_CLEAR_INT_REG, 0x0000); // Clear interrupt
    return GetWord16(GP_ADC_RESULT_REG);
}

static uint16_t adc_get_vbat_sample()
{
    int32_t  new_adc_off_p, new_adc_off_n;
    uint32_t adc_sample;

    SetWord16(GP_ADC_CTRL_REG, GP_ADC_LDO_EN|GP_ADC_SE|GP_ADC_EN);
    adc_usDelay(20);

    SetWord16(GP_ADC_OFFP_REG,  0x200);
    SetWord16(GP_ADC_OFFN_REG,  0x200);

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_MUTE, 1);
    SetBits16(GP_ADC_CTRL_REG, GP_ADC_SIGN, 0);
    new_adc_off_p = 0x200 - 2 * (adc_get_sample() - 0x200);

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_SIGN, 1);
    new_adc_off_n = 0x200 - 2 * (adc_get_sample() - 0x200);

    SetWord16(GP_ADC_OFFP_REG,  new_adc_off_p);
    SetWord16(GP_ADC_OFFN_REG,  new_adc_off_n);

    SetWord16(GP_ADC_CTRL_REG,  GP_ADC_LDO_EN | GP_ADC_SE | 0);
    SetWord16(GP_ADC_CTRL_REG,  GP_ADC_LDO_EN | GP_ADC_SE | GP_ADC_EN | 0);
    SetWord16(GP_ADC_CTRL2_REG, GP_ADC_DELAY_EN | GP_ADC_I20U | GP_ADC_ATTN3X );      // Enable 3x attenuation

    adc_usDelay(20);

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_SEL, ADC_CHANNEL_VBAT1V);
    adc_sample = adc_get_sample();

    adc_usDelay(1);

    SetWord16(GP_ADC_CTRL_REG,  GP_ADC_LDO_EN | GP_ADC_SE | GP_ADC_ATTN3X);
    SetWord16(GP_ADC_CTRL_REG,  GP_ADC_LDO_EN | GP_ADC_SE | GP_ADC_EN | GP_ADC_ATTN3X);
    SetWord16(GP_ADC_CTRL2_REG, GP_ADC_DELAY_EN | GP_ADC_I20U | GP_ADC_ATTN3X );      // Enable 3x attenuation

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_SEL, ADC_CHANNEL_VBAT1V);
    adc_sample += adc_get_sample();

    SetWord16(GP_ADC_CTRL_REG, 0);

    return adc_sample >> 1;
}

void hal_adc_init(void) {

}

void hal_adc_stop(void) {

}

void hal_adc_tick() {
    // Read battery once a second
    static int countdown = 1;
    if (--countdown > 0) return ;
    countdown = 100;

    static VoltageCommand volts = { COMMAND_VOLTAGE_DATA };
    volts.rail_v1 = adc_get_vbat_sample();

    ble_send(sizeof(volts), &volts);
}
