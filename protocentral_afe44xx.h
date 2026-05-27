//////////////////////////////////////////////////////////////////////////////////////////
//
//    Driver for the AFE4490 Pulse Oximeter
//    Ported from Arduino library to DA14695 (DA1469x SDK / SmartSnippets Studio)
//
//    Original Copyright (c) 2018 ProtoCentral
//    Port changes only: Arduino SPI/GPIO replaced with DA1469x SDK ad_spi adapter API
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//   PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   For information on how to use, visit https://github.com/Protocentral/AFE4490_Oximeter
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _AFE4490_H
#define _AFE4490_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

/* DA1469x SDK headers - replaces Arduino.h, SPI.h */
#include "ad_spi.h"
#include "hw_gpio.h"
#include "osal.h"

/* -----------------------------------------------------------------------
 * Data structure - identical to Arduino version, boolean -> bool (C99)
 * ----------------------------------------------------------------------- */
typedef struct afe44xx_Record {
    int32_t     heart_rate;
    int32_t     spo2;
    int32_t     IR_data;          /* signed long -> int32_t (same width on ARM) */
    int32_t     RED_data;
    bool        buffer_count_overflow;
} afe44xx_data;

/* -----------------------------------------------------------------------
 * Class replaced by plain C struct + init/get functions.
 * DA1469x SDK is C-based; the algorithms are unchanged.
 * ----------------------------------------------------------------------- */
typedef struct {
    ad_spi_controller_conf_t *spi_dev_conf; /* pointer to platform_devices entry  */
    HW_GPIO_PORT  pwdn_port;
    HW_GPIO_PIN   pwdn_pin;
} AFE44XX;

#ifdef __cplusplus
extern "C" {
#endif

void    AFE44XX_init     (AFE44XX *dev);
bool    AFE44XX_get_data (AFE44XX *dev, afe44xx_data *afe44xx_raw_data);

#ifdef __cplusplus
}
#endif

/* -----------------------------------------------------------------------
 * AFE44xx Register definitions - completely unchanged from original
 * ----------------------------------------------------------------------- */
#define CONTROL0      0x00
#define LED2STC       0x01
#define LED2ENDC      0x02
#define LED2LEDSTC    0x03
#define LED2LEDENDC   0x04
#define ALED2STC      0x05
#define ALED2ENDC     0x06
#define LED1STC       0x07
#define LED1ENDC      0x08
#define LED1LEDSTC    0x09
#define LED1LEDENDC   0x0a
#define ALED1STC      0x0b
#define ALED1ENDC     0x0c
#define LED2CONVST    0x0d
#define LED2CONVEND   0x0e
#define ALED2CONVST   0x0f
#define ALED2CONVEND  0x10
#define LED1CONVST    0x11
#define LED1CONVEND   0x12
#define ALED1CONVST   0x13
#define ALED1CONVEND  0x14
#define ADCRSTCNT0    0x15
#define ADCRSTENDCT0  0x16
#define ADCRSTCNT1    0x17
#define ADCRSTENDCT1  0x18
#define ADCRSTCNT2    0x19
#define ADCRSTENDCT2  0x1a
#define ADCRSTCNT3    0x1b
#define ADCRSTENDCT3  0x1c
#define PRPCOUNT      0x1d
#define CONTROL1      0x1e
#define SPARE1        0x1f
#define TIAGAIN       0x20
#define TIA_AMB_GAIN  0x21
#define LEDCNTRL      0x22
#define CONTROL2      0x23
#define SPARE2        0x24
#define SPARE3        0x25
#define SPARE4        0x26
#define RESERVED1     0x27
#define RESERVED2     0x28
#define ALARM         0x29
#define LED2VAL       0x2a
#define ALED2VAL      0x2b
#define LED1VAL       0x2c
#define ALED1VAL      0x2d
#define LED2ABSVAL    0x2e
#define LED1ABSVAL    0x2f
#define DIAG          0x30

#endif /* _AFE4490_H */
