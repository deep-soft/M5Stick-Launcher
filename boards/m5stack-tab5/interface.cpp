#include "powerSave.h"
#include <interface.h>
#include "TouchDrvGT911.hpp"
TouchDrvGT911 touch;

struct TouchPointPro {
    int16_t x = 0;
    int16_t y = 0;
};

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() { }

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    if (!Wire.begin(31, 32)) Serial.println("Fail Starting I2C");
    pinMode(23, INPUT);
    touch.setPins(-1, 23);
    if (!touch.begin(Wire, GT911_SLAVE_ADDRESS_H,31,32)) {
        Serial.println("Failed to find GT911 - check your wiring!");
        touch.begin(Wire, GT911_SLAVE_ADDRESS_L,31,32);
    }
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    char keyValue = 0;
    static unsigned long tm = millis();
    TouchPointPro t;
    uint8_t touched = 0;
    uint8_t rot = 5;
    if (rot != rotation) {
        if (rotation == 1) {
            touch.setMaxCoordinates(1280, 720);
            touch.setSwapXY(true);
            touch.setMirrorXY(false, true);
        }
        if (rotation == 3) {
            touch.setMaxCoordinates(1280, 720);
            touch.setSwapXY(true);
            touch.setMirrorXY(true, false);
        }
        if (rotation == 0) {
            touch.setMaxCoordinates(720, 1280);
            touch.setSwapXY(false);
            touch.setMirrorXY(false, false);
        }
        if (rotation == 2) {
            touch.setMaxCoordinates(720, 1280);
            touch.setSwapXY(false);
            touch.setMirrorXY(true, true);
        }
        rot = rotation;
    }
    touched = touch.getPoint(&t.x, &t.y);

    if ((millis() - tm) > 190 || LongPress) { // one reading each 190ms
        // Serial.printf("\nPressed x=%d , y=%d, rot: %d",t.x, t.y, rotation);
        if (touched) {

            Serial.printf("\nPressed x=%d , y=%d, rot: %d, millis=%d, tmp=%d",t.x, t.y, rotation, millis(),tm);
            tm = millis();

            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
            touched = 0;
            return;
        }
    }
}
/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
