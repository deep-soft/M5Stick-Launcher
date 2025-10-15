#ifndef _PRE_COMPILER
#define _PRE_COMPILER

#if !defined(PART_04MB) && !defined(PART_08MB) && !defined(PART_16MB) && !defined(HEADLESS)
#define PART_04MB 1
#endif

#ifndef ROTATION
#define ROTATION 3
#endif

#ifndef HAS_BTN
#define HAS_BTN 0
#define SEL_BTN -1
#define UP_BTN -1
#define DW_BTN -1
#define BTN_ACT LOW
#define BTN_ALIAS '"Boot"'
#endif
#ifndef LED
#define LED -1
#endif
#ifndef LED_ON
#define LED_ON 1
#endif

#ifndef TFT_WIDTH
#define TFT_WIDTH 135
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif

#ifndef FP
#define FP 1
#endif
#ifndef FM
#define FM 2
#endif
#ifndef FG
#define FG 3
#endif

#ifndef SDCARD_MOSI
#define SDCARD_MOSI -1
#endif
#ifndef SDCARD_MISO
#define SDCARD_MISO -1
#endif
#ifndef SDCARD_CS
#define SDCARD_CS -1
#endif
#ifndef SDCARD_SCK
#define SDCARD_SCK -1
#endif
#ifndef LH
#define LH 8
#endif
#ifndef LW
#define LW 6
#endif
#ifndef TFT_MISO
#define TFT_MISO -1
#endif

// Feed CYD envs with some sort of information to easily program.
#if !defined(TFT_DC) && !defined(TFT_IPS) && !defined(TFT_COL_OFS1) && !defined(TFT_COL_OFS2) &&             \
    !defined(TFT_MOSI)
#define TFT_DC -1
#define TFT_CS 15
#define TFT_SCLK 14
#define TFT_MOSI 13
#define TFT_RST -1
#define TFT_IPS 0
#define TFT_COL_OFS1 0
#define TFT_ROW_OFS1 0
#define TFT_COL_OFS2 0
#define TFT_ROW_OFS2 0
#endif

#ifndef OTA_TAG
#define OTA_TAG "third party"
#endif
#endif // _PRE_COMPILER
