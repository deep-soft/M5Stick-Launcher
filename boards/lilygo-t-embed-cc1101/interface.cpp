#include "util/powerSave.h"
#include <globals.h>
#include <interface.h>

#include <RotaryEncoder.h>
// extern RotaryEncoder encoder;
extern RotaryEncoder *encoder;
IRAM_ATTR void checkPosition();

// Battery libs
#if defined(T_EMBED_1101)
// Power handler for battery detection
#include <Wire.h>
#include <XPowersLib.h>
#include <esp32-hal-dac.h>
XPowersPPM PPM;
#endif

#ifdef USE_BQ27220_VIA_I2C
#include <bq27220.h>
BQ27220 bq;
#endif
/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(SEL_BTN, INPUT);
#ifdef T_EMBED_1101
    // T-Embed CC1101 has a antenna circuit optimized to each frequency band, controlled by SW0 and SW1
    // Set antenna frequency settings
    pinMode(CC1101_SW1_PIN, OUTPUT);
    pinMode(CC1101_SW0_PIN, OUTPUT);

    // before powering on, all CS signals should be pulled high and in an unselected state;
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    // Chip Select CC1101 to HIGH State
    pinMode(CC1101_SS_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);

    // Set NRF24 CS pin to HIGH
    pinMode(44, OUTPUT);
    digitalWrite(44, HIGH);

    // Power chip pin
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH); // Power on CC1101 and LED
    bool pmu_ret = false;
    Wire.begin(GROVE_SDA, GROVE_SCL);
    pmu_ret = PPM.init(Wire, GROVE_SDA, GROVE_SCL, BQ25896_SLAVE_ADDRESS);
    if (pmu_ret) {
        PPM.setSysPowerDownVoltage(3300);
        PPM.setInputCurrentLimit(3250);
        Serial.printf("getInputCurrentLimit: %d mA\n", PPM.getInputCurrentLimit());
        PPM.disableCurrentLimitPin();
        PPM.setChargeTargetVoltage(4208);
        PPM.setPrechargeCurr(64);
        PPM.setChargerConstantCurr(832);
        PPM.getChargerConstantCurr();
        Serial.printf("getChargerConstantCurr: %d mA\n", PPM.getChargerConstantCurr());
        PPM.enableADCMeasure();
        PPM.enableCharge();
        PPM.enableOTG();
        PPM.disableOTG();
    }
#else
    pinMode(BAT_PIN, INPUT); // Battery value
#endif

#ifdef T_EMBED_1101
    pinMode(BK_BTN, INPUT);
#endif
    pinMode(ENCODER_KEY, INPUT);
    // use TWO03 mode when PIN_IN1, PIN_IN2 signals are both LOW or HIGH in latch position.
    encoder = new RotaryEncoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);

    // register interrupt routine
    attachInterrupt(digitalPinToInterrupt(ENCODER_INA), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INB), checkPosition, CHANGE);
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = 0;
#if defined(USE_BQ27220_VIA_I2C)
    // percent=bq.getChargePcnt(); // this function runs bq.getRemainCap()/bq.getFullChargeCap()....
    // bq.getFullChargeCap() is hardcoded int 3000.
    percent = bq.getRemainCap() / 12.5; // My battery is 1300mAh and bq.getRemainCap() doesn't go upper than
                                        // 1083, that is why i'm dividing by 12.5 (var/1250)*100

#elif defined(T_EMBED)
    uint32_t volt = analogReadMilliVolts(GPIO_NUM_4);

    float mv = volt;
    percent = (mv - 3300) * 100 / (float)(4150 - 3350);

#endif

    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}
/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

// RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);
RotaryEncoder *encoder = nullptr;
IRAM_ATTR void checkPosition() {
    encoder->tick(); // just call tick() to check the state.
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis();
    static int _last_dir = 0;
    bool sel = !BTN_ACT;
    bool esc = !BTN_ACT;

    if (millis() - tm < 200) return;
    sel = digitalRead(SEL_BTN);
#ifdef T_EMBED_1101
    esc = digitalRead(BK_BTN);
#endif

    _last_dir = (int)encoder->getDirection();
    // pinMode(SEL_BTN, INPUT);
    if (_last_dir != 0 || sel == BTN_ACT || esc == BTN_ACT) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (_last_dir > 0) PrevPress = true;
    if (_last_dir < 0) NextPress = true;
    if (sel == BTN_ACT) SelPress = true;
    if (esc == BTN_ACT) EscPress = true;

    if (_last_dir != 0) _last_dir = 0;
    if (sel == BTN_ACT || esc == BTN_ACT) tm = millis();
}

void powerOff() {
#ifdef T_EMBED_1101
    digitalWrite(PIN_POWER_ON, LOW);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_6, LOW);
    esp_deep_sleep_start();
#endif
}

void checkReboot() {
#ifdef T_EMBED_1101
    int countDown;
    /* Long press power off */
    if (digitalRead(BK_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(BK_BTN) == BTN_ACT) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                tft->setTextSize(1);
                tft->setTextColor(FGCOLOR, BGCOLOR);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft->drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft->fillScreen(BGCOLOR);
                    while (digitalRead(BK_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }

        // Clear text after releasing the button
        delay(30);
        tft->fillRect(60, 12, tftWidth - 60, 8, BGCOLOR);
    }
#endif
}
