#ifndef button_h
#define button_h
#include "Arduino.h"
/*!
    @brief  Class for debouncing diffeent objects.
    @details This class provides methods for debouncing a button, toggling a button and holding a button.
*/
class Button {
  private:
    uint8_t _pin;
    uint32_t state;
    uint32_t triggerState;

    int rotaryPosition;
    int oldPot;
    bool previousState;
    unsigned long pressTime = 0; // Zeitpunkt, zu dem der Button zuletzt gedrückt wurde
    const unsigned long holdDuration = 1000; // Mindestdauer in Millisekunden, die der Button gedrückt sein muss


  public:
    bool toggled; // Zustandsvariable für den Toggle-Switch
    bool isPressed; // Zustandsvariable, um zu prüfen, ob der Button aktuell gedrückt wird
    bool isHold;
    bool holdToggle;


    // Initialisierungsmethode
    void begin(uint8_t button) {
      _pin = button;
      state = 0;
      triggerState = 0;
      toggled = false; // Initialer Zustand des Toggle-Switches
      isPressed = false;
      int rotaryPosition = 0;
      int oldPot = 0;
      pinMode(_pin, INPUT_PULLUP);
    }

/*!
    @brief   Simply debounce a button.
    @return  True for a short impulse when the button is pressed.
*/
     bool debounce() {
      state = (state<<1) | digitalRead(_pin) | 0xfe000000;
      return (state == 0xffff0000);
    }
     bool trigger() {
      triggerState = (triggerState<<1) | digitalRead(_pin) | 0xfe000000;
      return (triggerState == 0xffff0000);
    }

/*!
    @brief   Simply debounce a button and toggle it.
    @return  True after the button is pressed once. False after pressing it again.
*/
    bool toggle() {
        if (trigger() && !isHold && !isPressed) { // Wenn der Button gedrückt wurde und der Zustand 0 ist
          toggled = !toggled; // Zustand umschalten
          return toggled; // Aktuellen Zustand zurückgeben
        }
     }


/*!
    @brief   Simply debounce a button and toggle it.
    @return  True while the button is actively pressed and hold for a anount of time.
*/
 bool hold() {
      unsigned long currentTime = millis(); // Aktuelle Zeit in Millisekunden seit dem Programmstart
      bool currentState = digitalRead(_pin) == LOW; // Aktuellen Zustand des Buttons lesen

      if (trigger()) { // Wenn der Button gedrückt wurde und der Zustand 0 ist
        pressTime = currentTime; // Zeitpunkt speichern, zu dem der Button gedrückt wurde
      //Serial.println("Button pressed");
        holdToggle = false;
        isPressed = true; // Vorherigen Zustand aktualisieren
        isHold = false; // is pressed
        toggled = !toggled; // Zustand umschalten
      } 
      if (currentState && pressTime > 0 && currentTime - pressTime > holdDuration && !isHold) { // Wenn die Mindestdauer erreicht wurde
    //Serial.println("Button hold");  
      isHold = true; // is hold
      holdToggle = !holdToggle;
        return true; // true zurückgeben
      } else if (!currentState && isPressed) { // Wenn der Button losgelassen wurde
        if (isPressed && isHold) {
          toggled = false; // Zustand umschalten

        }
        
        pressTime = 0; // Zeitpunkt zurücksetzen
      //Serial.println("Button released");  
        isPressed = false; // Vorherigen Zustand aktualisieren
        isHold = false; // is released

        return false; // false zurückgeben
      }
 }



/*!
    @brief   Get the position of the alpha 6-step switch.
    @details This method reads the analog value of the alpha 6-step switch and returns the corresponding position. 
             There need to be 1k resistors between the pins of the switch and the analog input.
    @return  Returns 0-5 for the 6 different positions of the switch.
*/
    int getPosition() {
      int pot = analogRead(_pin); //952-761-570-380-192-0
// Serial.println(pot);
      if (pot < 500) {  //12
        rotaryPosition = 0; 
      } else if (pot > 501 && pot < 600) { // 550
        rotaryPosition = 1;
      } else if (pot > 601 && pot < 750) { // 712
        rotaryPosition = 2;
      } else if (pot > 751 && pot < 809) { // 793
        rotaryPosition = 3;    
      } else if (pot > 810 && pot < 859) { // 840
        rotaryPosition = 4;    
      } else if (pot > 860) { // 870
        rotaryPosition = 5;    
      }
      return rotaryPosition;
    }

/*!
    @brief   Check if the position of an potentiometer has changed between calls.
    @details This method reads the analog value of a potentiometer and checks if the value has changed.
             It provides a small dead zone to prevent jittering.
    @return  Returns true if the value has changed between calls of the method.
*/
    bool hasChanged() {

      int rawPot = analogRead(_pin);

     if (abs(oldPot - rawPot) > 10) {
      state = true;
      oldPot = rawPot;
      } else {
        state = false;
      }
      return state;
    }
};
#endif
