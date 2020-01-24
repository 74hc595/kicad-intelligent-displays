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
#define nSW1_PIN        1
/* Button 2 (right) */
#define nSW2_PORT       C
#define nSW2_PIN        0
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
struct quirks {
  uint8_t left_to_right_digit_numbering:1;
  uint8_t has_cursor:1;
  uint8_t controlreg_pd2816:1;
  uint8_t controlreg_hdsp2xxx:1;
  uint8_t controlreg_dlx:1;
  uint8_t :3;
};

/* Display properties */
struct display_spec {
  struct quirks quirks;
  uint8_t num_digits;
  uint8_t asciival_min;
  uint8_t asciival_max;
};

enum display_type {
  DL1414,   /* segmented */
  DLX1414,  /* dot matrix */
  DL1416,   /* segmented */
  DL1814,   /* segmented */
  DL2416,   /* segmented */
  DLX2416,  /* dot matrix */
  DL3416,   /* segmented */
  DLX3416,  /* dot matrix */
  DL3422,   /* segmented */
  PD2816,   /* segmented */
  HDSP2xxx, /* dot matrix */
  NUM_DISPLAY_TYPES
};

static const char msg_pd2816[] PROGMEM    = "PD2816  ";
static const char msg_hdsp2xxx[] PROGMEM  = "HDSP2xxx";
static const char msg_dl1414[] PROGMEM    = "1414";
static const char msg_dl1416[] PROGMEM    = "1416";
static const char msg_dl1814[] PROGMEM    = "1814";
static const char msg_dl2416[] PROGMEM    = "2416";
static const char msg_dl3416[] PROGMEM    = "3416";
static const char msg_dl3422[] PROGMEM    = "3422";
static const char msg_segmented[] PROGMEM = "SEGM";
static const char msg_matrix[] PROGMEM    = "MTRX";
static const char msg_abcdefgh[] PROGMEM  = "ABCDEFGH";

static const struct display_spec DISPLAYS[NUM_DISPLAY_TYPES] PROGMEM =
{
  [DL1414] = {
    .quirks={0},
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX1414] = {
    .quirks={0},
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL1416] = {
    .quirks={ .has_cursor=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DL1814] = {
    .quirks={ .has_cursor=1 },
    .num_digits=8, .asciival_min=' ', .asciival_max='_',
  },
  [DL2416] = {
    .quirks={ .has_cursor=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX2416] = {
    .quirks={ .has_cursor=1, .controlreg_dlx=1 },
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL3416] = {
    .quirks={ .has_cursor=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX3416] = {
    .quirks={ .has_cursor=1, .controlreg_dlx=1 },
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL3422] = {
    .quirks={ .has_cursor=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='\x7e',
  },
  [PD2816] = {
    .quirks={ .controlreg_pd2816=1 },
    .num_digits=8, .asciival_min=' ', .asciival_max='_',
  },
  [HDSP2xxx] = {
    .quirks={ .left_to_right_digit_numbering=1, .controlreg_hdsp2xxx=1 },
    .num_digits=8, .asciival_min='\0', .asciival_max='\x7f',
  },

};


static struct display_spec disp = {0};

static void setDisplayType(enum display_type type) {
  memcpy_P(&disp, DISPLAYS+type, sizeof(disp));
}


static void waitForButton2Press(void) {
  do { _delay_ms(50); } while (pin_is_high(nSW2));
  do { _delay_ms(50); } while (pin_is_low(nSW2));
}


static void waitMillis(uint16_t ms) {
  do {
    /* Pause if button 2 is held down */
    while (pin_is_low(nSW2)) {}
    _delay_ms(1);
  } while (ms--);
}


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
static uint8_t readControlRegister(void) {
  /* A3 must be low to access control register for PD2816 */
  /* ~FL and A4 must also be high to access character RAM on HDSP-2xxx */
  return readByte(_BV(ADDR_FL)|_BV(ADDR_A4));
}


static inline void hardResetDisplay(void) {
  /* datasheet specifies 15ms minimum */
  pin_low(nCLR); _delay_ms(16); pin_high(nCLR);
  _delay_us(120); /* datasheet specifies to wait 110us min. after rising edge */
}


static void softResetDisplay(void) {
  if (disp.quirks.controlreg_pd2816) {
    writeControlRegister(CR_CLEAR);
    writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_SOLID);
  } else if (disp.quirks.controlreg_hdsp2xxx) {
    writeControlRegister(CR_CLEAR);
    writeControlRegister(CR_HDSP_BRIGHTNESS_100);
  }
  pin_high(nBL); /* unblank */
}

static void displayChar(uint8_t pos, uint8_t c) {
  pos &= 0b111;
  if (!disp.quirks.left_to_right_digit_numbering) {
    pos = disp.num_digits-1-pos;
  }
  /* A3 must be high to access character RAM for PD2816 */
  /* ~FL, A4, and A3 must be high to access character RAM on HDSP-2xxx */
  /* Others don't care */
  writeByte(pos|_BV(ADDR_FL)|_BV(ADDR_A4)|_BV(ADDR_A3), c);
}


static void displayString_P(PGM_P str) {
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    displayChar(pos, pgm_read_byte(str+pos));
  }
}


static void fillDisplay(uint8_t c) {
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    displayChar(pos, c);
  }
}


static void fillDisplayGradual(uint8_t c, uint16_t delay) {
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    displayChar(pos, c);
    waitMillis(delay);
  }
}


static void incrementChar(uint8_t *p) {
  uint8_t c = *p;
  c++;
  if (c > disp.asciival_max) { c = disp.asciival_min; }
  *p = c;
}


static void scrollCharSet(uint16_t delay) {
  static uint8_t buf[8];
  /* initialize */
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    buf[pos] = disp.asciival_min+pos;
    displayChar(pos, buf[pos]);
  }
  /* loop */
  while (1) {
    waitMillis(delay);
    for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
      incrementChar(&buf[pos]);
      displayChar(pos, buf[pos]);
    }
  }
}



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
      setDisplayType(HDSP2xxx);
      softResetDisplay();
      displayString_P(msg_hdsp2xxx);
      waitForButton2Press();
      goto run;
    } else if (pin_is_low(PD2816CLK)) {
      setDisplayType(PD2816);
      softResetDisplay();
      displayString_P(msg_pd2816);
      waitForButton2Press();
      goto run;
    }
    _delay_us(1);
  }
  softResetDisplay();
  setDisplayType(DL2416);
  displayString_P(msg_dl2416);
  waitForButton2Press();


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

run:
  displayString_P(msg_abcdefgh);
  waitMillis(INTER_CHAR_DELAY_MS);
  /* test even bits */
  fillDisplayGradual('U', INTER_CHAR_DELAY_MS);
  /* test odd bits */
  fillDisplayGradual('*', INTER_CHAR_DELAY_MS);
  /* test other segments */
  fillDisplayGradual('O', INTER_CHAR_DELAY_MS);
  fillDisplayGradual('.', INTER_CHAR_DELAY_MS);
  /* scroll character set */
  scrollCharSet(INTER_CHAR_DELAY_MS);

  while (1) {
    pin_toggle(LED);
    _delay_ms(100);
  }
}
