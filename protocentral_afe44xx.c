//////////////////////////////////////////////////////////////////////////////////////////
//
//    Driver for the AFE44XX Pulse Oximeter
//    Ported from Arduino library to DA14695 (DA1469x SDK / SmartSnippets Studio)
//
//    Original Copyright (c) 2018 ProtoCentral
//    Port changes only:
//      - Arduino SPI.beginTransaction/transfer -> ad_spi_open/write/read/close
//      - Arduino digitalWrite/pinMode -> hw_gpio / hw_sys_pd_com API
//      - Arduino delay() -> OS_DELAY_MS()
//      - boolean -> bool, class -> C struct + functions
//    All AFE44xx register sequences, timing, data processing logic: UNCHANGED.
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
/////////////////////////////////////////////////////////////////////////////////////////

#include "protocentral_afe44xx.h"
#include "Protocentral_spo2_algorithm.h"
#include "protocentral_hr_algorithm.h"

/* DA1469x SDK */
#include "ad_spi.h"
#include "hw_gpio.h"
#include "hw_sys.h"
#include "osal.h"

/* -----------------------------------------------------------------------
 * Module-level state - identical variables to Arduino version
 * ----------------------------------------------------------------------- */
static volatile bool    afe44xx_data_ready  = false;
static volatile int8_t  n_buffer_count      = 0;    /* data length */

static int dec = 0;

static uint32_t IRtemp, REDtemp;

static int32_t  n_spo2;            /* SPO2 value */
static int32_t  n_heart_rate;      /* heart rate value */

static uint16_t aun_ir_buffer[100];   /* infrared LED sensor data */
static uint16_t aun_red_buffer[100];  /* red LED sensor data */

static int8_t   ch_spo2_valid;    /* indicator to show if the SPO2 calculation is valid */
static int8_t   ch_hr_valid;      /* indicator to show if the heart rate calculation is valid */

/* SPO2 lookup table - identical to original */
static const uint8_t uch_spo2_table[184] = {
    95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
    99, 99, 99, 99,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
   100,100,100,100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
    97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
    90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
    80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
    66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
    28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10,  9,  7,  6,  5,
     3,  2,  1
};

/* Algorithm instances - identical to original */
static spo2_algorithm Spo2;
static hr_algo        hral;

/* -----------------------------------------------------------------------
 * Internal SPI primitives
 *
 * Original Arduino used:
 *   SPI.beginTransaction(); digitalWrite(cs,LOW); SPI.transfer(x3); digitalWrite(cs,HIGH); SPI.endTransaction();
 *
 * DA1469x SDK equivalent:
 *   ad_spi_open() -> ad_spi_activate_cs() -> ad_spi_write()/read() -> ad_spi_deactivate_cs() -> ad_spi_close()
 *
 * The AFE44xx protocol is 1 byte address + 3 bytes data (write) or
 * 1 byte address then 3 bytes read. We use separate write+read transfers
 * with CS held low across both, matching the original behavior exactly.
 * ----------------------------------------------------------------------- */

static void afe44xxWrite(AFE44XX *dev, uint8_t address, uint32_t data)
{
    uint8_t tx_buf[4];
    tx_buf[0] = address;
    tx_buf[1] = (uint8_t)((data >> 16) & 0xFF);  /* top 8 bits    */
    tx_buf[2] = (uint8_t)((data >>  8) & 0xFF);  /* middle 8 bits */
    tx_buf[3] = (uint8_t)( data        & 0xFF);  /* bottom 8 bits */

    ad_spi_handle_t h = ad_spi_open(dev->spi_dev_conf);
    ad_spi_activate_cs(h);
    ad_spi_write(h, tx_buf, sizeof(tx_buf));
    ad_spi_deactivate_cs(h);
    ad_spi_close(h, false);
}

static uint32_t afe44xxRead(AFE44XX *dev, uint8_t address)
{
    uint8_t  tx_buf[1];
    uint8_t  rx_buf[3];
    uint32_t data = 0;

    tx_buf[0] = address;

    ad_spi_handle_t h = ad_spi_open(dev->spi_dev_conf);
    ad_spi_activate_cs(h);
    /* Send address byte */
    ad_spi_write(h, tx_buf, 1);
    /* Read 3 data bytes with CS still held low, matching original behavior */
    ad_spi_read(h, rx_buf, 3);
    ad_spi_deactivate_cs(h);
    ad_spi_close(h, false);

    data |= ((uint32_t)rx_buf[0] << 16);   /* top 8 bits    */
    data |= ((uint32_t)rx_buf[1] <<  8);   /* middle 8 bits */
    data |=  (uint32_t)rx_buf[2];          /* bottom 8 bits */

    return data;    /* returns 24-bit value, identical to original */
}

/* -----------------------------------------------------------------------
 * AFE44XX_init()
 *
 * Replaces:  AFE44XX::AFE44XX(cs_pin, pwdn_pin) constructor
 *            + AFE44XX::afe44xx_init()
 *
 * PWDN GPIO toggling logic: identical to original delay(500) sequences.
 * All register write values: completely unchanged from original.
 * ----------------------------------------------------------------------- */
void AFE44XX_init(AFE44XX *dev)
{
    /* --- GPIO for PWDN pin (replaces pinMode + digitalWrite) --- */
    hw_sys_pd_com_enable();

    hw_gpio_configure_pin(dev->pwdn_port, dev->pwdn_pin,
                          HW_GPIO_MODE_OUTPUT_PUSH_PULL,
                          HW_GPIO_FUNC_GPIO,
                          false);   /* initial state LOW */
    hw_gpio_pad_latch_enable(dev->pwdn_port, dev->pwdn_pin);

    hw_sys_pd_com_disable();

    /* Initialise HR algorithm - same call as original constructor */
    hr_algo_initStatHRM(&hral);

    /* --- Power cycle the AFE44xx - identical timing to original --- */
    /* digitalWrite(_pwdn_pin, LOW); delay(500); */
    hw_gpio_set_inactive(dev->pwdn_port, dev->pwdn_pin);
    OS_DELAY_MS(500);

    /* digitalWrite(_pwdn_pin, HIGH); delay(500); */
    hw_gpio_set_active(dev->pwdn_port, dev->pwdn_pin);
    OS_DELAY_MS(500);

    /* --- All register writes below: byte-for-byte identical to original --- */
    afe44xxWrite(dev, CONTROL0,     0x000000);
    afe44xxWrite(dev, CONTROL0,     0x000008);
    afe44xxWrite(dev, TIAGAIN,      0x000000); /* CF = 5pF, RF = 500kR */
    afe44xxWrite(dev, TIA_AMB_GAIN, 0x000001);
    afe44xxWrite(dev, LEDCNTRL,     0x001414);
    afe44xxWrite(dev, CONTROL2,     0x000000); /* LED_RANGE=100mA, LED=50mA */
    afe44xxWrite(dev, CONTROL1,     0x010707); /* Timers ON, average 3 samples */
    afe44xxWrite(dev, PRPCOUNT,     0X001F3F);
    afe44xxWrite(dev, LED2STC,      0X001770);
    afe44xxWrite(dev, LED2ENDC,     0X001F3E);
    afe44xxWrite(dev, LED2LEDSTC,   0X001770);
    afe44xxWrite(dev, LED2LEDENDC,  0X001F3F);
    afe44xxWrite(dev, ALED2STC,     0X000000);
    afe44xxWrite(dev, ALED2ENDC,    0X0007CE);
    afe44xxWrite(dev, LED2CONVST,   0X000002);
    afe44xxWrite(dev, LED2CONVEND,  0X0007CF);
    afe44xxWrite(dev, ALED2CONVST,  0X0007D2);
    afe44xxWrite(dev, ALED2CONVEND, 0X000F9F);
    afe44xxWrite(dev, LED1STC,      0X0007D0);
    afe44xxWrite(dev, LED1ENDC,     0X000F9E);
    afe44xxWrite(dev, LED1LEDSTC,   0X0007D0);
    afe44xxWrite(dev, LED1LEDENDC,  0X000F9F);
    afe44xxWrite(dev, ALED1STC,     0X000FA0);
    afe44xxWrite(dev, ALED1ENDC,    0X00176E);
    afe44xxWrite(dev, LED1CONVST,   0X000FA2);
    afe44xxWrite(dev, LED1CONVEND,  0X00176F);
    afe44xxWrite(dev, ALED1CONVST,  0X001772);
    afe44xxWrite(dev, ALED1CONVEND, 0X001F3F);
    afe44xxWrite(dev, ADCRSTCNT0,   0X000000);
    afe44xxWrite(dev, ADCRSTENDCT0, 0X000000);
    afe44xxWrite(dev, ADCRSTCNT1,   0X0007D0);
    afe44xxWrite(dev, ADCRSTENDCT1, 0X0007D0);
    afe44xxWrite(dev, ADCRSTCNT2,   0X000FA0);
    afe44xxWrite(dev, ADCRSTENDCT2, 0X000FA0);
    afe44xxWrite(dev, ADCRSTCNT3,   0X001770);
    afe44xxWrite(dev, ADCRSTENDCT3, 0X001770);

    OS_DELAY_MS(1000);
}

/* -----------------------------------------------------------------------
 * AFE44XX_get_data()
 *
 * Replaces: AFE44XX::get_AFE44XX_Data()
 *
 * All data processing logic (sign extension via shift, decimation counter,
 * buffer management, SpO2/HR algorithm calls): completely unchanged.
 * ----------------------------------------------------------------------- */
bool AFE44XX_get_data(AFE44XX *dev, afe44xx_data *afe44xx_raw_data)
{
    afe44xxWrite(dev, CONTROL0, 0x000001);
    IRtemp  = afe44xxRead(dev, LED1VAL);

    afe44xxWrite(dev, CONTROL0, 0x000001);
    REDtemp = afe44xxRead(dev, LED2VAL);

    afe44xx_data_ready = true;

    /* Sign-extension of 22-bit two's-complement value - identical to original */
    IRtemp  = (uint32_t)(IRtemp  << 10);
    afe44xx_raw_data->IR_data  = (int32_t)(IRtemp);
    afe44xx_raw_data->IR_data  = (int32_t)((afe44xx_raw_data->IR_data)  >> 10);

    REDtemp = (uint32_t)(REDtemp << 10);
    afe44xx_raw_data->RED_data = (int32_t)(REDtemp);
    afe44xx_raw_data->RED_data = (int32_t)((afe44xx_raw_data->RED_data) >> 10);

    /* Decimation counter: every 20th sample goes into buffer - identical to original */
    if (dec == 20)
    {
        aun_ir_buffer [n_buffer_count] = (uint16_t)((afe44xx_raw_data->IR_data)  >> 4);
        aun_red_buffer[n_buffer_count] = (uint16_t)((afe44xx_raw_data->RED_data) >> 4);
        n_buffer_count++;
        dec = 0;
    }

    dec++;

    /* Buffer full: run SpO2 algorithm - identical to original */
    if (n_buffer_count > 99)
    {
        spo2_algorithm_estimate_spo2(&Spo2,
                                     aun_ir_buffer, 100,
                                     aun_red_buffer,
                                     &n_spo2, &ch_spo2_valid,
                                     &n_heart_rate, &ch_hr_valid);
        afe44xx_raw_data->spo2 = n_spo2;
        n_buffer_count = 0;
        afe44xx_raw_data->buffer_count_overflow = true;
    }

    /* Heart rate runs on every sample - identical to original */
    hr_algo_statHRMAlgo(&hral, (uint32_t)afe44xx_raw_data->RED_data);
    afe44xx_raw_data->heart_rate = hral.HeartRate;

    afe44xx_data_ready = false;
    return true;
}
