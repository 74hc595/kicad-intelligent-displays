#include "pin_xmega.h"
#include "delay_ns.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/* active high */
#define LED_PORT        D
#define LED_PIN         7
/* ~WR - write (active low) */
#define nWR_PORT        E
#define nWR_PIN         1
/* ~CE - chip enable (active low) */
#define nCE_PORT        E
#define nCE_PIN         2
/* ~CLR (2416/3416/3422) / ~RST (HDSP-2xxx/PD2816) - clear/reset display (active low) */
#define nCLR_PORT       E
#define nCLR_PIN        3
/* ~RD (HDSP-2xxx/PD2816) - read (active low) */
#define nRD_PORT        B
#define nRD_PIN         0
/* CUE (2416/3416/3422) - cursor enable (active high) */
#define CUE_PORT        B
#define CUE_PIN         2
/* ~BL (2416/3416/3422) - blank display (active low) */
#define nBL_PORT        B
#define nBL_PIN         3
/* Button 1 (left) */
#define nSW1_PORT       C
#define nSW1_PIN        0
/* Button 2 (right) */
#define nSW2_PORT       C
#define nSW2_PIN        1
/* HDSP-2xxx clock detect */
#define HDSPCLK_PORT    C
#define HDSPCLK_PIN     2
/* PD2816 clock detect */
#define PD2816CLK_PORT  C
#define PD2816CLK_PIN   3
/* Data lines D0-D7 */
#define DATA_PORT       A
/* Address lines A0-A4 and ~FL. ~CU is A3. */
#define ADDRESS_PORT    F

/* Address bits */
#define ADDR_FL  5
#define ADDR_A4  4
#define ADDR_A3  3
#define ADDR_nCU ADDR_A3
#define ADDR_A2  2
#define ADDR_A1  1
#define ADDR_A0  0

/* Control register bits */
#define CR_CLEAR                  0b10000000
#define CR_PD2816_BRIGHTNESS_0    0b00000000
#define CR_PD2816_BRIGHTNESS_25   0b00000001
#define CR_PD2816_BRIGHTNESS_50   0b00000010
#define CR_PD2816_BRIGHTNESS_100  0b00000011
#define CR_PD2816_CHAR_SOLID      0b00000000
#define CR_PD2816_CHAR_BLINK      0b00000100
#define CR_PD2816_UNDERLINE_SOLID 0b00000000
#define CR_PD2816_UNDERLINE_BLINK 0b00001000
#define CR_PD2816_ATTRS_ON        0b00010000
#define CR_PD2816_BLINK_DISPLAY   0b00100000
#define CR_PD2816_LAMP_TEST       0b01000000
#define CR_HDSP_BRIGHTNESS_100    0b00000000
#define CR_HDSP_BRIGHTNESS_80     0b00000001
#define CR_HDSP_BRIGHTNESS_53     0b00000010
#define CR_HDSP_BRIGHTNESS_40     0b00000011
#define CR_HDSP_BRIGHTNESS_27     0b00000100
#define CR_HDSP_BRIGHTNESS_20     0b00000101
#define CR_HDSP_BRIGHTNESS_13     0b00000110
#define CR_HDSP_BRIGHTNESS_0      0b00000111
#define CR_HDSP_ATTRS_ON          0b00001000
#define CR_HDSP_BLINK_DISPLAY     0b00010000
#define CR_HDSP_SELF_TEST_RESULT  0b00100000
#define CR_HDSP_SELF_TEST_START   0b01000000

#define INTER_CHAR_DELAY_MS 250


/* Part-specific quirks */
static bool    LEFT_TO_RIGHT_DIGIT_NUMBERING = false;
static bool    IS_PD2816 = false;
static bool    IS_HDSP2xxx = false;
static uint8_t NUM_DIGITS = 4;
static uint8_t ASCIIVAL_MIN = ' ';
static uint8_t ASCIIVAL_MAX = '_';


static void writeByte(uint8_t addr, uint8_t data) {
  /* set up address and data */
  port_out(ADDRESS, addr);
  port_out(DATA, data);
  /* being conservative with timing here */
  pin_low(nCE);
  delay_ns_max(100);
  pin_low(nWR);
  delay_ns_max(200);
  pin_high(nWR);
  pin_high(nCE);
}


/* HDSP-2xxx and PD2816 only */
static uint8_t readByte(uint8_t addr) {
  /* set up address lines and tristate data lines */
  port_out(ADDRESS, addr);
  port_inputs(DATA);
  pin_low(nCE);
  pin_low(nRD);
  delay_ns_max(150);
  uint8_t data = port_value(DATA);
  pin_high(nRD);
  pin_high(nCE);
  port_outputs(DATA);
  return data;
}


/* HDSP-2xxx and PD2816 only */
static void writeControlRegister(uint8_t data) {
  /* A3 must be low to access control register for PD2816 */
  /* ~FL and A4 must also be high to access character RAM on HDSP-2xxx */
  writeByte(_BV(ADDR_FL)|_BV(ADDR_A4), data);
}


/* HDSP-2xxx and PD2816 only */
static uint8_t readControlRegister() {
  /* A3 must be low to access control register for PD2816 */
  /* ~FL and A4 must also be high to access character RAM on HDSP-2xxx */
  return readByte(_BV(ADDR_FL)|_BV(ADDR_A4));
}


static void displayChar(uint8_t pos, uint8_t c) {
  pos &= 0b111;
  if (!LEFT_TO_RIGHT_DIGIT_NUMBERING) {
    pos = NUM_DIGITS-1-pos;
  }
  /* A3 must be high to access character RAM for PD2816 */
  /* ~FL, A4, and A3 must be high to access character RAM on HDSP-2xxx */
  /* Others don't care */
  writeByte(pos|_BV(ADDR_FL)|_BV(ADDR_A4)|_BV(ADDR_A3), c);
}


static void displayString_P(PGM_P str) {
  for (uint8_t pos = 0; pos < NUM_DIGITS; pos++) {
    displayChar(pos, pgm_read_byte(str+pos));
  }
}


static inline void hardResetDisplay() {
  /* datasheet specifies 15ms minimum */
  pin_low(nCLR); _delay_ms(16); pin_high(nCLR);
  _delay_us(120); /* datasheet specifies to wait 110us min. after rising edge */
}


static void softResetDisplay() {
  if (IS_PD2816) {
    writeControlRegister(CR_CLEAR);
    writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_SOLID);
  } else if (IS_HDSP2xxx) {
    writeControlRegister(CR_CLEAR);
    writeControlRegister(CR_HDSP_BRIGHTNESS_100);
  }
  pin_high(nBL); /* unblank */
}


static const char msg_pd2816[] PROGMEM   = "PD2816  ";
static const char msg_hdsp2xxx[] PROGMEM = "HDSP2xxx";
static const char msg_4digit[] PROGMEM   = "4DIG";

int main(void)
{
  /* disable clock prescaler, run at 20MHz */
  CCP = 0xD8; /* unlock CLKCTRL registers */
  CLKCTRL_MCLKCTRLB = 0;

  /* initialize pins */
  pin_output_low(nCLR); /* hold in reset */
  pin_output_high(nWR);
  pin_output_high(nCE);
  pin_output_high(nRD);
  pin_output_low(CUE);
  pin_output_low(nBL);
  pin_output_high(LED);
  port_outputs(DATA);
  port_outputs(ADDRESS);
  pin_input_pullup(nSW1);
  pin_input_pullup(nSW2);
  pin_input_pullup(HDSPCLK);
  pin_input_pullup(PD2816CLK);
  hardResetDisplay();

  /* if an HDSP/PDSP/PD2816 is present, we'll see a clock signal */
  uint8_t count = 0; /* poll 256 times, once per microsecond */
  while (--count) {
    /* It should not be physically possible to have a PD2816 and HDSP2xxx */
    /* inserted at the same time */
    if (pin_is_low(HDSPCLK)) {
      IS_HDSP2xxx = true;
      LEFT_TO_RIGHT_DIGIT_NUMBERING = true;
      ASCIIVAL_MIN = '\0'; ASCIIVAL_MAX = '\x7f';
      NUM_DIGITS = 8;
      break;
    } else if (pin_is_low(PD2816CLK)) {
      IS_PD2816 = true;
      LEFT_TO_RIGHT_DIGIT_NUMBERING = false;
      ASCIIVAL_MIN = ' '; ASCIIVAL_MAX = '_';
      NUM_DIGITS = 8;
      break;
    }
    _delay_us(1);
  }

  softResetDisplay();

  if (IS_HDSP2xxx) {
    displayString_P(msg_hdsp2xxx);
  } else if (IS_PD2816) {
    displayString_P(msg_pd2816);
  } else {
    displayString_P(msg_4digit);
  }

  /* is is an HDSP-2xxx? */
  /* otherwise, is it a PD2816? */
  /* otherwise, show the menu */
    /* user must choose from: 1414, 1416, 2416, 3416, 3422, 1814 */
    /* if 1414, 2416, or 3416, user must choose segmented vs. dot-matrix */

  // displayInit();
  // PD2816Init();
  // _delay_ms(1000);
  // fillDisplayGradual('.');
  // fillDisplayGradual('*');
  // fillDisplayGradual('O');
  // fillDisplayGradual('#');
  // scrollCharSet();
  while (1) {
    pin_toggle(LED);
    _delay_ms(100);
  }
}
