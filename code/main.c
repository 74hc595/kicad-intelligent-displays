/**
 * Intelligent alphanumeric display tester
 * Matt Sarnoff (msarnoff.org)
 * February 5, 2020
 *
 * Tests the functionality and features of several types of 4-character and
 * 8-character "smart"/"intelligent" alphanumeric LED displays manufactured
 * by HP/Litronix/Siemens/Osram/Avago/Broadcom.
 *
 * The following devices are supported:
 * - DL1414, HPDL-1414 (4-char segmented), DLx-1414, HDLx-1414 (4-char dot matrix)
 * - DL1416, DL1416T, DL1416B, SP1-16 (4-char segmented)
 * - DL1814 (8-char segmented)
 * - DL2416, HPDL-2416 (4-char segmented), DLx-2416, HDLx-2416 (4-char dot matrix)
 * - DL3416 (4-char segmented), DLx-3416, HDLx-3416 (4-char dot matrix)
 * - DL3422 (4-char segmented)
 * - PD2816 (8-char segmented)
 * - HDSP-2xxx, PD188x (8-char dot matrix)
 *
 * Not currently supported:
 * - PD243x, PD353x, PD443x (4-char dot matrix)
 * - Extended features of HDLx-2416 and HDLx-3416
 *
 * Controls (during menu):
 * - SW1: advance to next menu item
 * - SW2: confirm menu item
 *
 * Controls (during test):
 * - SW1: return to main menu
 * - SW2: hold to freeze test, release to resume
 *
 * PD2816 and HDSP-2xxx/PD188x devices are auto-detected by checking for the
 * presence of their clock output signals. On powerup, if one of these devices
 * is detected, the display will show "HDSP2xxx" or "PD2816  ". Press SW2 to
 * begin the test.
 *
 * Otherwise, a menu is shown. Press SW1 to cycle through the menu items to
 * select the display type. Press SW2 to confirm. Some types have an additional
 * submenu, requiring the user to select "SEGM" (for segmented displays) or
 * "MTRX" (for 5x7 dot-matrix displays).
 * The full menu structure is:
 *     "1414"
 *       "SEGM" (DL1414, HPDL-1414)
 *       "MTRX" (DLx-1414, HDLx-1414)
 *     "1416"
 *       "'16T" (DL1416, DL1416T, SP1-16)
 *       "'16B" (DL1416B)
 *     "1814" (DL1814)
 *     "2416"
 *       "SEGM" (DL2416, HPDL-2416)
 *       "MTRX" (DLx-2416, HDLx-2416)
 *     "3416"
 *       "SEGM" (DL3416)
 *       "MTRX" (DLx-3416, HDLx-3416)
 *     "3422" (DL3422)
 *
 * Menu selections are saved in nonvolatile memory and recalled at powerup to
 * facilitate testing multiple displays in a row.
 *
 * Revision 1 of the tester board had pins A0 and A1 swapped on the DL3416/3422
 * footprint. This will cause the middle two characters to be swapped when
 * testing DL3416/3422 devices. (Others will not be affected.) To compensate for
 * this, hold down SW2 when powering on the board. This will make the display
 * appear correct for DL3416/3422, but the middle characters will be swapped
 * on all other display types. The setting is saved in nonvolatile memory and
 * recalled at powerup. "Normal" behavior can be restored by performing the
 * same procedure a second time.
 *
 * Test suite
 * ----------
 * 1. Display "ABCD" or "ABCDEFGH".
 * 2. Gradually replace each character with uppercase "U" from left to right.
 *    (tests even-numbered data lines: U is 0b1010101)
 * 3. Gradually replace each character with "*" from left to right.
 *    (tests odd-numbered data lines: * is 0b0101010)
 * 4. Gradually replace each character with uppercase "O" from left to right.
 *    (tests segments not illuminated with U and *)
 * 5. Gradually replace each character with "." from left to right.
 *    (tests extra dot segment on some display types)
 * 6. (PD2816/HDSP-2xxx only) Test read-back from display RAM and control
 *    register. Will show "READ OK" if the test passes.
 * 7. (DL1416/2416/3416/3422 only) Test cursor.
 *    7a. Display "ABCD".
 *    7b. Show the cursor character in each digit indivitually from right to left.
 *    7c. Show the cursor character in the left 2 digits.
 *    7d. Show the cursor character in the right 2 digits.
 *    7e. Show the cursor character in all digits.
 *    7f. Turn off cursor.
 * 8. (HDSP-2xxx only) Test flash RAM.
 *     8a. Display "ABCDEFGH".
 *     8b. Flash each digit individually from left to right.
 *     8c. Flash the left 4 digits.
 *     8d. Flash the right 4 digits.
 *     8e. Flash all digits.
 *     8f. Turn off flashing.
 * 9. (DL1814/2416/3416/3422) Test blanking pin.
 *     9a. Display "ABCD" or "ABCDEFGH".
 *     9b. Flash the display three times.
 * 10. (HDSP-2xxx only) Test user-defined-character RAM.
 *     10a. Blank the display.
 *     10b. Animate a pattern scrolling upward on each digit from left to right.
 *          (Tests user defined characters 0-7.)
 *     10c. Animate another pattern scrolling upward on each character from left
 *          to right. (Tests user defined characters 8-F.)
 * 11. (PD2816/HDSP-2xxx only) Test control register features.
 *     11a. Show all brightness levels.
 *     11b. (PD2816 only) Test highlight attribute styles: underline, blinking
 *          character with solid underline, blinking underline, and blinking
 *          character with blinking underline.
 *     11c. Test full display blink.
 *     11d. (PD2816 only) Lamp test. Alternate between the text "LAMPTEST" and
 *          all-segments-illuminated twice.
 *     11e. (HDSP-2xxx only) Perform built-in self-test. This takes approx.
 *          7 seconds. The display will show several patterns and will be blank
 *          for several seconds. "S.T.PASS" indicates the display passed its
 *          self-test. "S.T.FAIL" indicates failure.
 * 12. Show each displayable character and its code point. The  2-digit ASCII
 *     code shown in hexadecimal in leftmost two digits. Third digit is blank.
 *     Remaining digits show the character.
 * 13. Display "DONE".
 * 14. Scroll the full displayable character set. Loops continuously until SW1
 *     is pressed or power is disconnected.
 *
 * Note: It's not recommended to plug in or unplug displays while the board is
 * powered up. Even when using a ZIF socket, "hot-swapping" is not recommended.
 * These displays are old, rare, and expensive!
 */

#include "pin_xmega.h"
#include "delay_ns.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>
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
  uint8_t has_blanking_pin:1;
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

static const char msg_pd2816[] PROGMEM    = "PD2816  ";
static const char msg_hdsp2xxx[] PROGMEM  = "HDSP2xxx";
static const char msg_dl1414[] PROGMEM    = "1414";
static const char msg_dl1416[] PROGMEM    = "1416";
static const char msg_dl1416t[] PROGMEM   = "'16T";
static const char msg_dl1416b[] PROGMEM   = "'16B";
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
static const char msg_selftest_pass[] PROGMEM             = "S.T.PASS";
static const char msg_selftest_fail[] PROGMEM             = "S.T.FAIL";
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


/* Boxed hex digits 0-9, A-F */
static const char udc_hexdigits[] PROGMEM = {
  0b11111, 0b10001, 0b10101, 0b10101, 0b10101, 0b10001, 0b11111,
  0b11111, 0b11011, 0b10011, 0b11011, 0b11011, 0b10001, 0b11111,
  0b11111, 0b10001, 0b11101, 0b10001, 0b10111, 0b10001, 0b11111,
  0b11111, 0b10001, 0b11101, 0b11001, 0b11101, 0b10001, 0b11111,
  0b11111, 0b10101, 0b10101, 0b10001, 0b11101, 0b11101, 0b11111,
  0b11111, 0b10001, 0b10111, 0b10001, 0b11101, 0b10001, 0b11111,
  0b11111, 0b10001, 0b10111, 0b10001, 0b10101, 0b10001, 0b11111,
  0b11111, 0b10001, 0b10101, 0b10101, 0b11101, 0b11101, 0b11111,
  0b11111, 0b10001, 0b10101, 0b10001, 0b10101, 0b10001, 0b11111,
  0b11111, 0b10001, 0b10101, 0b10001, 0b11101, 0b10001, 0b11111,
  0b11111, 0b10001, 0b10101, 0b10001, 0b10101, 0b10101, 0b11111,
  0b11111, 0b10001, 0b10101, 0b10011, 0b10101, 0b10001, 0b11111,
  0b11111, 0b10001, 0b10111, 0b10111, 0b10111, 0b10001, 0b11111,
  0b11111, 0b10011, 0b10101, 0b10101, 0b10101, 0b10011, 0b11111,
  0b11111, 0b10001, 0b10111, 0b10011, 0b10111, 0b10001, 0b11111,
  0b11111, 0b10001, 0b10111, 0b10011, 0b10111, 0b10111, 0b11111,
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
    .quirks={ .has_blanking_pin=1 },
    .num_digits=8, .asciival_min=' ', .asciival_max='_',
  },
  [DL2416] = {
    .quirks={ .has_cursor=1, .has_blanking_pin=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX2416] = {
    .quirks={ .has_cursor=1, .has_blanking_pin=1 },
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL3416] = {
    .quirks={ .has_cursor=1, .has_blanking_pin=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='_',
  },
  [DLX3416] = {
    .quirks={ .has_cursor=1, .has_blanking_pin=1 },
    .num_digits=4, .asciival_min='\0', .asciival_max='\x7f',
  },
  [DL3422] = {
    .quirks={ .has_cursor=1, .has_blanking_pin=1 },
    .num_digits=4, .asciival_min=' ', .asciival_max='\x7e',
  },
  [PD2816] = {
    .quirks={ .controlreg_pd2816=1, .has_read=1 },
    .num_digits=8, .asciival_min=' ', .asciival_max='_',
  },
  [HDSP2xxx] = {
    .quirks={ .left_to_right_digit_numbering=1, .has_read=1, .controlreg_hdsp2xxx=1 },
    .num_digits=8, .asciival_min='\0', .asciival_max='\x8f',
  },
};


#define COUNT_OF(arr) (sizeof(arr)/sizeof((arr)[0]))

struct menu;

struct menu_item {
  PGM_P text;
  union {
    const struct menu * PROGMEM submenu;
    struct { uint8_t disptype; uint8_t ff; };
  };
};

struct menu {
  uint8_t idx_eeaddr;
  uint8_t nitems;
  const struct menu_item * PROGMEM items;
};

static const struct menu_item menu_items_dl1414[] PROGMEM = {
  { .text=msg_segmented, .disptype=DL1414, .ff=0xFF },
  { .text=msg_matrix, .disptype=DLX1414, .ff=0xFF },
};

static const struct menu menu_dl1414 PROGMEM = {
  .idx_eeaddr = 2,
  .nitems = COUNT_OF(menu_items_dl1414),
  .items = menu_items_dl1414
};

static const struct menu_item menu_items_dl1416[] PROGMEM = {
  { .text=msg_dl1416t, .disptype=DL1416T, .ff=0xFF },
  { .text=msg_dl1416b, .disptype=DL1416B, .ff=0xFF },
};

static const struct menu menu_dl1416 PROGMEM = {
  .idx_eeaddr = 3,
  .nitems = COUNT_OF(menu_items_dl1416),
  .items = menu_items_dl1416
};

static const struct menu_item menu_items_dl2416[] PROGMEM = {
  { .text=msg_segmented, .disptype=DL2416, .ff=0xFF },
  { .text=msg_matrix, .disptype=DLX2416, .ff=0xFF },
};

static const struct menu menu_dl2416 PROGMEM = {
  .idx_eeaddr = 4,
  .nitems = COUNT_OF(menu_items_dl2416),
  .items = menu_items_dl2416
};

static const struct menu_item menu_items_dl3416[] PROGMEM = {
  { .text=msg_segmented, .disptype=DL3416, .ff=0xFF },
  { .text=msg_matrix, .disptype=DLX3416, .ff=0xFF },
};

static const struct menu menu_dl3416 PROGMEM = {
  .idx_eeaddr = 5,
  .nitems = COUNT_OF(menu_items_dl3416),
  .items = menu_items_dl3416
};

static const struct menu_item main_menu_items[] PROGMEM = {
  { .text=msg_dl1414, .submenu=&menu_dl1414 },
  { .text=msg_dl1416, .submenu=&menu_dl1416 },
  { .text=msg_dl1814, .disptype=DL1814, .ff=0xFF },
  { .text=msg_dl2416, .submenu=&menu_dl2416 },
  { .text=msg_dl3416, .submenu=&menu_dl3416 },
  { .text=msg_dl3422, .disptype=DL3422, .ff=0xFF },
};

static const struct menu main_menu PROGMEM = {
  .idx_eeaddr = 1,
  .nitems = COUNT_OF(main_menu_items),
  .items = main_menu_items
};



static struct display_spec disp = {0};

static void waitMillis(uint16_t ms) {
  pin_toggle(LED);
  do {
    /* Pause if button 2 is held down */
    while (pin_is_low(nSW2)) {}
    _delay_ms(1);
  } while (ms--);
}


/* rev 1 board has A0 and A1 swapped on the DL3416/3422 footprint */
static bool a0_a1_not_swapped;
static uint8_t fixAddress(uint8_t addr) {
  if (a0_a1_not_swapped) { return addr; }
  bool a0 = addr & 1;
  bool a1 = (addr & 2) >> 1;
  addr &= 0b11111100;
  return addr | a1 | (a0<<1);
}


static void writeByte(uint8_t addr, uint8_t data) {
  /* set up address and data */
  addr = fixAddress(addr);
  port_out(ADDRESS, addr);
  port_out(DATA, data);
  /* being conservative with timing here */
  delay_ns_max(300);
  pin_low(nCE);
  delay_ns_max(300);
  pin_low(nWR);
  delay_ns_max(300);
  pin_high(nWR);
  delay_ns_max(300);
  pin_high(nCE);
  delay_ns_max(300);
}


/* HDSP-2xxx and PD2816 only */
static uint8_t readByte(uint8_t addr) {
  /* set up address lines and tristate data lines */
  addr = fixAddress(addr);
  port_out(ADDRESS, addr);
  port_inputs(DATA);
  delay_ns_max(300);
  pin_low(nCE);
  delay_ns_max(300);
  pin_low(nRD);
  delay_ns_max(300);
  uint8_t data = port_value(DATA);
  pin_high(nRD);
  delay_ns_max(300);
  pin_high(nCE);
  delay_ns_max(300);
  port_outputs(DATA);
  delay_ns_max(300);
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


static void fillDisplay(uint8_t c, uint8_t ndigits) {
  for (uint8_t pos = 0; pos < ndigits; pos++) {
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
  if (!disp.quirks.has_blanking_pin) { return; }
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
  //!!! TODO: hard-reset to synchronize flashing?
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
static void testUserDefinedChars(uint16_t delay)
{
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

  const char *pattern = udc_hexdigits;
  for (uint8_t i = 0; i < 16; i++) {
    setUserDefinedChar_P(i, pattern);
    pattern += 7;
  }
}


static void testReadback(uint16_t delay)
{
  if (!disp.quirks.has_read) { return; }
  displayString_P(msg_readtest);
  waitMillis(delay);

  /* test each bit of data bus */
  uint8_t writeValue = 1;
  for (uint8_t pos = 0; pos < disp.num_digits; pos++) {
    writeByte(pos|_BV(ADDR_FL)|_BV(ADDR_A4)|_BV(ADDR_A3), writeValue);
    /* this delay makes 2 of my PD2816s fail, bit 5 seems to get clobbered */
    /* on the next cycle of the multiplex timer */
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
      waitMillis(5000); /* long pause, then bail out of test */
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
    waitMillis(5000); /* long pause, then bail out of test */
    return;
  }

  /* passed */
  displayString_P(msg_readok);
  waitMillis(delay);
}


static void testControlRegisterPD2816(uint16_t delay)
{
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
  /* hard-resets merely reset the multiplex/blink phase */
  hardResetDisplay();
  displayString_P(msg_underline);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_SOLID);
  waitMillis(delay<<2);
  hardResetDisplay();
  displayString_P(msg_charblink_underline);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_BLINK|CR_PD2816_UNDERLINE_SOLID);
  waitMillis(delay<<2);
  hardResetDisplay();
  displayString_P(msg_underline_blink);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_SOLID|CR_PD2816_UNDERLINE_BLINK);
  waitMillis(delay<<2);
  hardResetDisplay();
  displayString_P(msg_char_and_underline_blink);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100|CR_PD2816_ATTRS_ON|CR_PD2816_CHAR_BLINK|CR_PD2816_UNDERLINE_BLINK);
  waitMillis(delay<<2);
  displayString_P(msg_attributes_off);
  writeControlRegister(CR_PD2816_BRIGHTNESS_100);
  waitMillis(delay<<2);
  /* test full display blink */
  hardResetDisplay();
  displayString_P(msg_blink_all);
  writeControlRegister(CR_PD2816_BLINK_DISPLAY|CR_PD2816_BRIGHTNESS_100);
  waitMillis(delay<<3);
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


static void testControlRegisterHDSP2xxx(uint16_t delay)
{
  //!!! TODO: hard-reset to synchronize flashing?
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
  /* wait for test to finish--blink LED so we know things haven't crashed */
  for (uint8_t i = 0; i < HDSP_SELF_TEST_DURATION_MS/INTER_CHAR_DELAY_MS; i++) {
    waitMillis(INTER_CHAR_DELAY_MS);
  }
  /* check result */
  uint8_t result = readControlRegister();
  if (result & CR_HDSP_SELF_TEST_RESULT) {
    displayString_P(msg_selftest_pass);
    waitMillis(delay<<2);
  } else {
    displayString_P(msg_selftest_fail);
    waitMillis(5000); /* long pause if selftest fails */
  }

  /* clear display and restore full brightness */
  softResetDisplay();
}


static void testControlRegister(uint16_t delay)
{
  if (disp.quirks.controlreg_pd2816) {
    testControlRegisterPD2816(delay);
  } else if (disp.quirks.controlreg_hdsp2xxx) {
    testControlRegisterHDSP2xxx(delay);
  }
}


static void setDisplayType(enum display_type type) {
  memcpy_P(&disp, DISPLAYS+type, sizeof(disp));
  softResetDisplay();
}


/* Waits until a button is pressed and released, returning true if SW2 was */
/* pressed, and false if SW1 was pressed. */
/* (Does not detect both buttons pressed simultaneously.) */
static bool waitForButtonPress(void) {
  uint8_t buttons = 0;
  uint8_t ret;
  do {
    buttons = pin_is_low(nSW1) | (pin_is_low(nSW2)<<1);
    _delay_ms(50); /* long delay to for debouncing */
  } while (buttons == 0);
  /* return value is true if SW2 was pressed, false otherwise */
  ret = (buttons & 2) != 0;
  /* wait for release */
  do {
    buttons = pin_is_low(nSW1) | (pin_is_low(nSW2)<<1);
    _delay_ms(50);
  } while (buttons != 0);
  return ret;
}


static inline void waitForButton2Press(void) {
  while (waitForButtonPress() == 0) {}
}


static void menu(void)
{
  fillDisplay(' ', 8);

  struct menu current_menu;
  memcpy_P(&current_menu, &main_menu, sizeof(struct menu));
  uint8_t idx = eeprom_read_byte((void*)((int)current_menu.idx_eeaddr));
  if (idx >= current_menu.nitems) { idx = 0; }

  struct menu_item item;
  while (1) {
    memcpy_P(&item, current_menu.items+idx, sizeof(struct menu_item));
    displayString_P(item.text);
    /* button 1: advance to next menu item */
    if (waitForButtonPress() == 0) {
      idx++;
      if (idx >= current_menu.nitems) { idx = 0; }
    }
    /* button 2: activate current menu item */
    else {
      /* save current index to eeprom */
      eeprom_update_byte((void*)((int)current_menu.idx_eeaddr), idx);
      if (item.ff == 0xFF) {
        /* set display type and return */
        setDisplayType(item.disptype);
        return;
      } else {
        /* enter submenu */
        memcpy_P(&current_menu, item.submenu, sizeof(struct menu));
        idx = eeprom_read_byte((void*)((int)current_menu.idx_eeaddr));
        if (idx >= current_menu.nitems) { idx = 0; }
      }
    }
  }
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

  /* if SW2 is held down on powerup, toggle the swap-A1/A0 bit */
  /* for rev1 boards that have A0/A1 swapped on the DL3416/3422 footprint */
  _delay_ms(50);
  a0_a1_not_swapped = !!(eeprom_read_byte((void*)7) & 1);
  if (pin_is_low(nSW2)) {
    a0_a1_not_swapped = !a0_a1_not_swapped;
    eeprom_update_byte((void*)7, a0_a1_not_swapped);
    eeprom_busy_wait();
    while (pin_is_low(nSW2)) {}
    _delay_ms(50);
  }

  /* if an HDSP/PDSP/PD2816 is present, we'll see a clock signal */
  uint8_t count = 0; /* poll 256 times, once per microsecond */
  while (--count) {
    /* It should not be physically possible to have a PD2816 and HDSP2xxx */
    /* inserted at the same time */
    if (pin_is_low(HDSPCLK)) {
      setDisplayType(HDSP2xxx);
      displayString_P(msg_hdsp2xxx);
      waitForButton2Press();
      goto run;
    } else if (pin_is_low(PD2816CLK)) {
      setDisplayType(PD2816);
      displayString_P(msg_pd2816);
      waitForButton2Press();
      goto run;
    }
    _delay_us(1);
  }
  /* if neither display type detected, show the menu */
  setDisplayType(DL1414);
  menu();

run:

  /* pressing SW1 reboots the tester */
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
