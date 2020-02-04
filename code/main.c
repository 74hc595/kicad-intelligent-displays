#include "pin_xmega.h"
#include "delay_ns.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
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
#define CR_HDSP_FLASH_ON          0b00001000
#define CR_HDSP_BLINK_DISPLAY     0b00010000
#define CR_HDSP_SELF_TEST_RESULT  0b00100000
#define CR_HDSP_SELF_TEST_START   0b01000000

#define INTER_CHAR_DELAY_MS         250
#define LONG_DELAY_MS               1000
#define HDSP_SELF_TEST_DURATION_MS  7000

/* Part-specific quirks */
struct quirks {
  uint8_t left_to_right_digit_numbering:1;
  uint8_t has_cursor:1;
  uint8_t cursor_parallel_load:1;
  uint8_t has_blanking:1;
  uint8_t has_read:1;
  uint8_t controlreg_pd2816:1;
  uint8_t controlreg_hdsp2xxx:1;
  uint8_t :1;
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
  DL1416T,  /* (and DL1416, SP1-16) segmented */
  DL1416B,  /* segmented */
  DL1814,   /* segmented */
  DL2416,   /* segmented */
  DLX2416,  /* dot matrix */
  DL3416,   /* segmented */
  DLX3416,  /* dot matrix */
  DL3422,   /* segmented */
  PD2816,   /* segmented */
  HDSP2xxx, /* (and PD188x) dot matrix */
  /* Not supported: PD243x/353x/443x and extended features of HDLx-2416/3416 */
  NUM_DISPLAY_TYPES
};

static const char msg_pd2816[] PROGMEM    = " PD2816 ";
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
static const char msg_readtest[] PROGMEM  = "READTEST";
static const char msg_readfail[] PROGMEM  = "RD  FAIL";
static const char msg_readok[] PROGMEM    = "READ OK ";
static const char msg_done[] PROGMEM      = "DONE    ";

/* Control register tests */
static const char msg_brightness_13[] PROGMEM             = " 13% BRI";
static const char msg_brightness_20[] PROGMEM             = " 20% BRI";
static const char msg_brightness_25[] PROGMEM             = " 25% BRI";
static const char msg_brightness_27[] PROGMEM             = " 27% BRI";
static const char msg_brightness_40[] PROGMEM             = " 40% BRI";
static const char msg_brightness_50[] PROGMEM             = " 50% BRI";
static const char msg_brightness_53[] PROGMEM             = " 53% BRI";
static const char msg_brightness_80[] PROGMEM             = " 80% BRI";
static const char msg_brightness_100[] PROGMEM            = "100% BRI";
static const char msg_underline[] PROGMEM                 = { 0x80|'U', 0x80|'N', 0x80|'D', 0x80|'E', 0x80|'R', 0x80|'L', 0x80|'I', 0x80|'N', 0 };
static const char msg_charblink_underline[] PROGMEM       = { 0x80|'C', 0x80|'B', 0x80|'L', 0x80|'I', 0x80|'N', 0x80|'K', 0x80|'+', 0x80|'U', 0 };
static const char msg_underline_blink[] PROGMEM           = { 0x80|'U', 0x80|'L', 0x80|'.', 0x80|'B', 0x80|'L', 0x80|'I', 0x80|'N', 0x80|'K', 0 };
static const char msg_char_and_underline_blink[] PROGMEM  = { 0x80|'B', 0x80|'L', 0x80|'I', 0x80|'N', 0x80|'K', 0x80|'+', 0x80|'U', 0x80|'L', 0 };
static const char msg_attributes_off[] PROGMEM            = { 0x80|'(', 0x80|'N', 0x80|'O', 0x80|'R', 0x80|'M', 0x80|'A', 0x80|'L', 0x80|')', 0 };
static const char msg_blink_all[] PROGMEM                 = "BLINKALL";
static const char msg_lamp_test[] PROGMEM                 = "LAMPTEST";
static const char msg_selftest[] PROGMEM                  = "SELFTEST";
static const char msg_selftest_pass[] PROGMEM             = "--PASS--";
static const char msg_selftest_fail[] PROGMEM             = "**FAIL**";
static const char msg_udc_test[] PROGMEM                  = "UDC TEST";
static const char msg_udc1[] PROGMEM                      = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0};
static const char msg_udc2[] PROGMEM                      = {0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0};

static const char udc[] PROGMEM = {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00010000,
  0b00011000,
  0b00011100,
  0b00011110,
  0b00011111,
  0b00001111,
  0b00000111,
  0b00000011,
  0b00000001,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000
};

static const char udc2[] PROGMEM = {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000001,
  0b00000011,
  0b00000111,
  0b00001111,
  0b00011111,
  0b00011110,
  0b00011100,
  0b00011000,
  0b00010000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000
};

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
  [DL1416T] = { /* or DL1416, SP1-16, uses a different cursor scheme */
    .quirks={ .has_cursor=1, .cursor_parallel_load=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DL1416B] = { /* uses the same cursor scheme as DL2416/3416/3422 */
    .quirks={ .has_cursor=1, },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DL1814] = {
    .quirks={ .has_blanking=1 },
    .num_digits=8, .asciival_min=' ', .asciival_max='_',
  },
  [DL2416] = {
    .quirks={ .has_cursor=1, .has_blanking=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX2416] = {
    .quirks={ .has_cursor=1, .has_blanking=1 },
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL3416] = {
    .quirks={ .has_cursor=1, .has_blanking=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX3416] = {
    .quirks={ .has_cursor=1, .has_blanking=1 },
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL3422] = {
    .quirks={ .has_cursor=1, .has_blanking=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='\x7e',
  },
  [PD2816] = {
    .quirks={ .controlreg_pd2816=1, .has_read=1 },
    .num_digits=8, .asciival_min=' ', .asciival_max='_',
  },
  [HDSP2xxx] = {
    .quirks={ .left_to_right_digit_numbering=1, .has_read=1, .controlreg_hdsp2xxx=1 },
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
  pin_toggle(LED);
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
  delay_ns_max(1000);
  pin_low(nCE);
  delay_ns_max(1000);
  pin_low(nWR);
  delay_ns_max(1000);
  pin_high(nWR);
  delay_ns_max(1000);
  pin_high(nCE);
  delay_ns_max(1000);
}


/* HDSP-2xxx and PD2816 only */
static uint8_t readByte(uint8_t addr) {
  /* set up address lines and tristate data lines */
  port_out(ADDRESS, addr);
  port_inputs(DATA);
  delay_ns_max(1000);
  pin_low(nCE);
  delay_ns_max(1000);
  pin_low(nRD);
  delay_ns_max(1000);
  uint8_t data = port_value(DATA);
  pin_high(nRD);
  delay_ns_max(1000);
  pin_high(nCE);
  delay_ns_max(1000);
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


static void setCursorMask(uint8_t bitmask) {
  if (disp.quirks.cursor_parallel_load) {
    /* DL1416 sets cursor for all digits with one write */
    writeByte(_BV(ADDR_FL)|_BV(ADDR_A4), bitmask);
  } else {
    /* Others require one write per digit */
    for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
      writeByte(pos|_BV(ADDR_FL)|_BV(ADDR_A4), (bitmask & 1));
      bitmask >>= 1;
    }
  }
}


/* HDSP-2xxx only */
static void setFlashMask(uint8_t bitmask) {
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    writeByte(pos|_BV(ADDR_A4)|_BV(ADDR_A3), (bitmask & 1));
    bitmask >>= 1;
  }
}


static inline void hardResetDisplay(void) {
  /* datasheet specifies 15ms minimum */
  pin_low(nCLR); _delay_ms(16); pin_high(nCLR);
  _delay_us(120); /* datasheet specifies to wait 110us min. after rising edge */
}


static void softResetDisplay(void) {
  if (disp.quirks.controlreg_pd2816) {
    writeControlRegister(CR_CLEAR);
    writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_SOLID);
  } else if (disp.quirks.controlreg_hdsp2xxx) {
    writeControlRegister(CR_CLEAR);
    writeControlRegister(CR_HDSP_BRIGHTNESS_100);
  }
  if (disp.quirks.has_cursor) {
    setCursorMask(0);
  }
  pin_high(nBL); /* unblank */
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


static inline char hexdigit(uint8_t i) {
  i &= 0xF; return '0' + i + ((i <= 9) ? 0 : 7);
}


static void showASCIIValues(uint16_t delay) {
  for (uint8_t c = disp.asciival_min; c <= disp.asciival_max; c++) {
    char hex1 = hexdigit(c >> 4);
    char hex2 = hexdigit(c & 0xF);
    displayChar(0, hex1);
    displayChar(1, hex2);
    displayChar(2, ' ');
    for (uint8_t pos = 3; pos < disp.num_digits; pos++) {
      displayChar(pos, c);
    }
    waitMillis(delay);
  }
}


static void testBlanking(uint16_t delay) {
  if (!disp.quirks.has_blanking) { return; }
  displayString_P(msg_abcdefgh);
  waitMillis(delay);
  /* flash three times */
  pin_low(nBL);  waitMillis(delay);
  pin_high(nBL); waitMillis(delay);
  pin_low(nBL);  waitMillis(delay);
  pin_high(nBL); waitMillis(delay);
  pin_low(nBL);  waitMillis(delay);
  /* unblank */
  pin_high(nBL);
}


static void testCursor(uint16_t delay) {
  if (!disp.quirks.has_cursor) { return; }
  /* clear cursor from all positions */
  setCursorMask(0);
  /* cursor on */
  pin_high(CUE);
  displayString_P(msg_abcdefgh);
  waitMillis(delay);
  /* show cursor individually in all digits */
  uint8_t mask = 1;
  for (uint8_t i = 0; i < disp.num_digits; i++) {
    setCursorMask(mask);
    waitMillis(delay);
    mask <<= 1;
  }
  /* show cursor in left and right halves */
  setCursorMask(0xCC);
  waitMillis(delay);
  setCursorMask(0x33);
  waitMillis(delay);  /* show cursor in all digits */
  setCursorMask(0xFF);
  waitMillis(delay);
  /* cursor off */
  setCursorMask(0);
  pin_low(CUE);
}


/* HDSP-2xxx only */
static void testFlash(uint16_t delay) {
  if (!disp.quirks.controlreg_hdsp2xxx) { return; }
  /* clear flash from all positions */
  setFlashMask(0);
  /* flash on */
  writeControlRegister(CR_HDSP_FLASH_ON|CR_HDSP_BRIGHTNESS_100);
  displayString_P(msg_abcdefgh);
  /* flash all digits individually */
  uint8_t mask = 1;
  for (uint8_t i = 0; i < disp.num_digits; i++) {
    setFlashMask(mask);
    waitMillis(delay);
    mask <<= 1;
  }
  /* flash left and right halves */
  setFlashMask(0x0F);
  waitMillis(delay);
  setFlashMask(0xF0);
  waitMillis(delay);
  /* flash all digits */
  setFlashMask(0xFF);
  waitMillis(delay);
  /* flash off */
  setFlashMask(0);
  writeControlRegister(CR_HDSP_BRIGHTNESS_100);
}


/* HDSP-2xxx only */
static void setUserDefinedChar_P(uint8_t idx, PGM_P pattern) {
  /* set UDC address */
  writeByte(_BV(ADDR_FL), idx);
  for (uint8_t row = 0; row < 7; row++) {
    writeByte(row|_BV(ADDR_FL)|_BV(ADDR_A3), pgm_read_byte(pattern));
    pattern++;
  }
}


/* HDSP-2xxx only */
static void testUserDefinedChars(uint16_t delay) {
  if (!disp.quirks.controlreg_hdsp2xxx) { return; }
  displayString_P(msg_udc_test);
  waitMillis(delay<<3);
  /* clear all user-defined characters */
  for (uint8_t i = 0; i < 16; i++) {
    setUserDefinedChar_P(i, udc);
  }
  displayString_P(msg_udc1);
  /* animate each UDC */
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    for (uint8_t i = 0; i < sizeof(udc)-6; i++) {
      setUserDefinedChar_P(pos, udc+i);
      waitMillis(delay);
    }
  }
  displayString_P(msg_udc2);
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    for (uint8_t i = 0; i < sizeof(udc2)-6; i++) {
      setUserDefinedChar_P(8+pos, udc2+i);
      waitMillis(delay);
    }
  }
}


static void testReadback(uint16_t delay) {
  if (!disp.quirks.has_read) { return; }
  displayString_P(msg_readtest);
  waitMillis(delay);

  /* test each bit of data bus */
  uint8_t writeValue = 1;
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    writeByte(pos|_BV(ADDR_FL)|_BV(ADDR_A4)|_BV(ADDR_A3), writeValue);
    _delay_ms(10);
    writeValue <<= 1;
  }
  /* read values back */
  uint8_t expectedReadValue = 1;
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    uint8_t readValue = readByte(pos|_BV(ADDR_FL)|_BV(ADDR_A4)|_BV(ADDR_A3));
    if (readValue != expectedReadValue) {
      displayString_P(msg_readfail);
      displayChar(2, '0'+pos);
      waitMillis(5000); /* long pause, then continue */
      return;
    }
    expectedReadValue <<= 1;
  }

  /* test control register */
  expectedReadValue = 0b00011011;
  writeControlRegister(expectedReadValue);
  uint8_t readValue = readControlRegister();
  /* restore control register to normal */
  softResetDisplay();

  if (readValue != expectedReadValue) {
    displayString_P(msg_readfail);
    displayChar(2, 'C');
    waitMillis(5000); /* long pause, then continue */
    return;
  }

  /* passed */
  displayString_P(msg_readok);
  waitMillis(delay);
}


static void testControlRegisterPD2816(uint16_t delay) {
  /* test brightness levels */
  displayString_P(msg_brightness_25);
  writeControlRegister(CR_PD2816_BRIGHTNESS_25);
  waitMillis(delay);
  displayString_P(msg_brightness_50);
  writeControlRegister(CR_PD2816_BRIGHTNESS_50);
  waitMillis(delay);
  displayString_P(msg_brightness_100);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100);
  waitMillis(delay);
  /* test highlight styles */
  displayString_P(msg_underline);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_SOLID);
  waitMillis(delay<<2);
  displayString_P(msg_charblink_underline);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_BLINK|CR_PD2816_UNDERLINE_SOLID);
  waitMillis(delay<<2);
  displayString_P(msg_underline_blink);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_BLINK);
  waitMillis(delay<<2);
  displayString_P(msg_char_and_underline_blink);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_BLINK|CR_PD2816_UNDERLINE_BLINK);
  waitMillis(delay<<2);
  displayString_P(msg_attributes_off);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100);
  waitMillis(delay<<2);
  /* test full display blink */
  displayString_P(msg_blink_all);
  writeControlRegister(CR_PD2816_BLINK_DISPLAY|CR_PD2816_BRIGHTNESS_100);
  waitMillis(delay<<2);
  /* all-segments lamp test */
  displayString_P(msg_lamp_test);
  writeControlRegister(CR_PD2816_BRIGHTNESS_50);
  waitMillis(delay);
  /* all-segments lamp test */
  writeControlRegister(CR_PD2816_LAMP_TEST|CR_PD2816_BRIGHTNESS_50);
  waitMillis(delay);
  writeControlRegister(CR_PD2816_BRIGHTNESS_50);
  waitMillis(delay);
  writeControlRegister(CR_PD2816_LAMP_TEST|CR_PD2816_BRIGHTNESS_50);
  waitMillis(delay);
  writeControlRegister(CR_PD2816_BRIGHTNESS_50);
  waitMillis(delay);
  /* clear display and restore full brightness */
  softResetDisplay();
}


static void testControlRegisterHDSP2xxx(uint16_t delay) {
  /* test brightness levels */
  displayString_P(msg_brightness_13);
  writeControlRegister(CR_HDSP_BRIGHTNESS_13);
  waitMillis(delay);
  displayString_P(msg_brightness_20);
  writeControlRegister(CR_HDSP_BRIGHTNESS_20);
  waitMillis(delay);
  displayString_P(msg_brightness_27);
  writeControlRegister(CR_HDSP_BRIGHTNESS_27);
  waitMillis(delay);
  displayString_P(msg_brightness_40);
  writeControlRegister(CR_HDSP_BRIGHTNESS_40);
  waitMillis(delay);
  displayString_P(msg_brightness_53);
  writeControlRegister(CR_HDSP_BRIGHTNESS_53);
  waitMillis(delay);
  displayString_P(msg_brightness_80);
  writeControlRegister(CR_HDSP_BRIGHTNESS_80);
  waitMillis(delay);
  displayString_P(msg_brightness_100);
  writeControlRegister(CR_HDSP_BRIGHTNESS_100);
  waitMillis(delay);
  /* test full display blink */
  displayString_P(msg_blink_all);
  writeControlRegister(CR_HDSP_BLINK_DISPLAY|CR_HDSP_BRIGHTNESS_100);
  waitMillis(delay<<2);
  /* invoke the self test */
  displayString_P(msg_selftest);
  writeControlRegister(CR_HDSP_BRIGHTNESS_100);
  waitMillis(delay<<1);
  writeControlRegister(CR_HDSP_SELF_TEST_START);
  /* wait for test to finish */
  waitMillis(HDSP_SELF_TEST_DURATION_MS);
  /* check result */
  uint8_t result = readControlRegister();
  if (result & CR_HDSP_SELF_TEST_RESULT) {
    displayString_P(msg_selftest_pass);
    waitMillis(delay<<2);
  } else {
    displayString_P(msg_selftest_fail);
    while (1) {} __builtin_unreachable(); /* hang here if test failed */
  }

  /* clear display and restore full brightness */
  softResetDisplay();
}


static void testControlRegister(uint16_t delay) {
  if (disp.quirks.controlreg_pd2816) {
    testControlRegisterPD2816(delay);
  } else if (disp.quirks.controlreg_hdsp2xxx) {
    testControlRegisterHDSP2xxx(delay);
  }
}


static void menu(void) {
  setDisplayType(DL1416T);
  softResetDisplay();
  displayString_P(msg_dl1416);
  waitForButton2Press();
}


ISR(port_isr(nSW1)) {
  /* wait for button release */
  do { _delay_ms(50); } while (pin_is_low(nSW1));
  _delay_ms(100);
  /* software reset */
  CCP = 0xD8; /* unlock RSTCTRL registers */
  RSTCTRL.SWRR = RSTCTRL_SWRE_bm;
  while (1) {} __builtin_unreachable();
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
  menu();

run:

  /* pressing Button 1 reboots the tester */
  pin_ctrl(nSW1) |= PORT_ISC0_bm|PORT_ISC1_bm;
  sei();

  displayString_P(msg_abcdefgh);
  waitMillis(INTER_CHAR_DELAY_MS);
  /* test even bits */
  fillDisplayGradual('U', INTER_CHAR_DELAY_MS);
  /* test odd bits */
  fillDisplayGradual('*', INTER_CHAR_DELAY_MS);
  /* test other segments */
  fillDisplayGradual('O', INTER_CHAR_DELAY_MS);
  fillDisplayGradual('.', INTER_CHAR_DELAY_MS);
  /* test features */
  testReadback(INTER_CHAR_DELAY_MS);
  testCursor(INTER_CHAR_DELAY_MS);
  testFlash(LONG_DELAY_MS);
  testBlanking(INTER_CHAR_DELAY_MS);
  testUserDefinedChars(50);
  testControlRegister(INTER_CHAR_DELAY_MS);
  /* show each character and its ASCII code */
  showASCIIValues(INTER_CHAR_DELAY_MS);
  /* scroll character set (loops until SW1 is pressed) */
  displayString_P(msg_done);
  waitMillis(LONG_DELAY_MS);
  scrollCharSet(INTER_CHAR_DELAY_MS);
  __builtin_unreachable();
}
