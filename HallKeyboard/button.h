/*!
 * @file button.h
 * @brief Enhanced Button Library with Advanced Debouncing and State Management
 * @author Hall-Effect Keyboard Project
 * @version 2.0
 * @date Januar 2026
 * 
 * This library provides comprehensive button handling functionality including:
 * - Advanced bit-shift debouncing for reliable input detection
 * - State management (pressed, released, held)
 * - Toggle and hold functionality 
 * - Analog input support for rotary switches and potentiometers
 * - Resistor ladder switch support
 */

#ifndef button_h
#define button_h
#include "Arduino.h"

/*!
 * @brief Advanced Button Class with Debouncing and State Management
 * @details Provides methods for debouncing buttons, detecting press/release events,
 *          toggle functionality, hold detection, and analog input processing.
 *          Uses bit-shift debouncing for reliable and fast input detection.
 */
class Button {
  private:
    // === Pin Configuration ===
    uint8_t _pin;                           ///< Arduino pin number for this button
    
    // === Bit-Shift Debouncing Registers ===
    uint32_t state;                         ///< General state register for various operations
    uint32_t triggerState;                  ///< Bit-shift register for press detection (HIGH->LOW)
    uint32_t releaseState;                  ///< Bit-shift register for release detection (LOW->HIGH)
    
    // === Debounce State Management ===
    bool stableState;                       ///< Current stable debounced state (HIGH=not pressed, LOW=pressed)
    bool lastStableState;                   ///< Previous stable state for change detection
    uint8_t debounceCounter;                ///< Counter for stable samples during debouncing
    
    // === Single-Trigger State Management ===
    bool triggerFired;                      ///< Prevents multiple trigger events
    bool releaseFired;                      ///< Prevents multiple release events
    
    // === Hold Detection ===
    unsigned long pressTime;                ///< Timestamp when button was last pressed
    static const unsigned long holdDuration = 1000; ///< Minimum hold duration in milliseconds
    
    // === Analog Input Support ===
    int rotaryPosition;                     ///< Current position for rotary switches (0-5)
    int oldPot;                            ///< Previous potentiometer value for change detection
    bool previousState;                     ///< Previous state for legacy operations


  public:
    // === Public State Variables ===
    bool toggled;                           ///< Toggle state (true/false) for toggle switch functionality
    bool isPressed;                         ///< Current pressed state (updated during debouncing)
    bool isHold;                           ///< True when button is being held for minimum duration
    bool holdToggle;                       ///< Toggle state specifically for hold operations


    /*!
     * @brief Initialize button with specified pin
     * @param button Arduino pin number to use for this button
     * @details Sets up pin as INPUT_PULLUP and initializes all state variables.
     *          Must be called in setup() before using any other button functions.
     */
    void begin(uint8_t button) {
      _pin = button;
      
      // Initialize bit-shift registers
      state = 0;
      triggerState = 0;
      releaseState = 0;
      
      // Initialize state variables
      toggled = false;
      isPressed = false;
      isHold = false;
      holdToggle = false;
      
      // Initialize debounce state
      stableState = HIGH;
      lastStableState = HIGH;
      debounceCounter = 0;
      
      // Initialize single-trigger state
      triggerFired = false;
      releaseFired = false;
      
      // Initialize timing and analog variables
      pressTime = 0;
      rotaryPosition = 0;
      oldPot = 0;
      previousState = false;
      
      // Configure pin with internal pull-up resistor
      pinMode(_pin, INPUT_PULLUP);
    }

    /*!
     * @brief Advanced debounce with stable state tracking
     * @return True when button is pressed (HIGH->LOW transition detected)
     * @details Uses sample counting to ensure stable state before registering change.
     *          Requires 16 consistent samples to confirm state change, preventing
     *          false triggers from electrical noise or mechanical bounce.
     *          Updates internal state variables (stableState, isPressed) automatically.
     */
    bool debounce() {
      bool currentInput = digitalRead(_pin);
      
      // Check if input differs from current stable state
      if (currentInput != stableState) {
        debounceCounter++;
        
        // Require 16 consistent samples before accepting state change (prevents ghost triggers)
        if (debounceCounter >= 16) {
          lastStableState = stableState;     // Store previous state for change detection
          stableState = currentInput;        // Update to new stable state
          debounceCounter = 0;               // Reset counter
          
          // Update public state variable
          isPressed = (stableState == LOW);
          
          // Return true only on press event (HIGH->LOW)
          return (stableState == LOW && lastStableState == HIGH);
        }
      } else {
        // Input matches stable state - reset counter
        debounceCounter = 0;
      }
      
      return false;
    }

    /*!
     * @brief Robust trigger detection based on stable state changes
     * @return True for single pulse when button is pressed (HIGH->LOW)
     * @details Uses stable debounced state for reliable edge detection.
     *          Much more reliable than bit-shift register approach.
     */
    bool trigger() {
      // Update the stable state through debouncing
      debounce();
      
      // Detect HIGH->LOW transition (button press)
      if (lastStableState == HIGH && stableState == LOW && !triggerFired) {
        triggerFired = true;
        return true;
      }
      
      // Reset trigger flag when button is released
      if (stableState == HIGH) {
        triggerFired = false;
      }
      
      return false;
    }

    /*!
     * @brief Robust release detection based on stable state changes
     * @return True for single pulse when button is released (LOW->HIGH)
     * @details Uses stable debounced state for reliable edge detection.
     *          Much more reliable than inverted bit-shift approach.
     */
    bool released() {
      // Update the stable state through debouncing
      debounce();
      
      // Detect LOW->HIGH transition (button release)
      if (lastStableState == LOW && stableState == HIGH && !releaseFired) {
        releaseFired = true;
        return true;
      }
      
      // Reset release flag when button is pressed
      if (stableState == LOW) {
        releaseFired = false;
      }
      
      return false;
    }

    /*!
     * @brief Get current stable button state
     * @return True if button is currently pressed (LOW), False if released (HIGH)
     * @details Returns the current debounced state without triggering events.
     *          Updates internal state through debounce() call.
     *          Use for state queries and button combinations.
     */
    bool isDown() {
      debounce();  // Ensure state is current
      return (stableState == LOW);
    }

    /*!
     * @brief Check if button state has changed
     * @return True if state changed since last stable state
     * @details Compares current stable state with previous stable state.
     *          Useful for detecting any state transitions (press OR release).
     */
    bool stateChanged() {
      debounce();  // Update current state
      return (stableState != lastStableState);
    }

    /*!
     * @brief Toggle switch functionality
     * @return Current toggle state (true/false)
     * @details Toggles internal state each time button is pressed.
     *          Ignores presses when button is in hold mode.
     *          Perfect for on/off switches and mode toggles.
     */
    bool toggle() {
      if (trigger() && !isHold && !isPressed) {
        toggled = !toggled;
        return toggled;
      }
      return toggled;
    }

    /*!
     * @brief Advanced hold detection with timing
     * @return True while button is held beyond minimum duration
     * @details Detects when button is pressed and held for longer than holdDuration.
     *          Manages state transitions: press -> hold -> release.
     *          Updates holdToggle state and manages timing automatically.
     */
    bool hold() {
      unsigned long currentTime = millis();
      bool currentState = (digitalRead(_pin) == LOW);

      // Button just pressed
      if (trigger()) {
        pressTime = currentTime;
        holdToggle = false;
        isPressed = true;
        isHold = false;
        toggled = !toggled;
      }

      // Check for hold condition
      if (currentState && pressTime > 0 && 
          (currentTime - pressTime > holdDuration) && !isHold) {
        isHold = true;
        holdToggle = !holdToggle;
        return true;
      }

      // Button released
      if (!currentState && isPressed) {
        if (isHold) {
          toggled = false;
        }
        pressTime = 0;
        isPressed = false;
        isHold = false;
        return false;
      }

      return false;
    }

    /*!
     * @brief Read 6-position rotary switch
     * @return Position value (0-5) based on analog voltage divider
     * @details Designed for Alpha 6-step rotary switches with voltage divider.
     *          Requires 1kΩ resistors between switch positions and analog input.
     *          Voltage thresholds are optimized for 5V operation.
     * 
     * Typical ADC values:
     * - Position 0: <500   (GND)
     * - Position 1: 501-600 (≈550)
     * - Position 2: 601-750 (≈712) 
     * - Position 3: 751-809 (≈793)
     * - Position 4: 810-859 (≈840)
     * - Position 5: >860    (≈870)
     */
    int getPosition() {
      int adcValue = analogRead(_pin);
      
      if (adcValue < 500) {
        rotaryPosition = 0;
      } else if (adcValue >= 501 && adcValue < 600) {
        rotaryPosition = 1;
      } else if (adcValue >= 601 && adcValue < 750) {
        rotaryPosition = 2;
      } else if (adcValue >= 751 && adcValue < 809) {
        rotaryPosition = 3;
      } else if (adcValue >= 810 && adcValue < 859) {
        rotaryPosition = 4;
      } else if (adcValue >= 860) {
        rotaryPosition = 5;
      }
      
      return rotaryPosition;
    }

    /*!
     * @brief Detect potentiometer value changes
     * @return True if potentiometer value changed significantly since last call
     * @details Implements dead zone of ±10 ADC units to prevent noise-induced
     *          false triggers. Ideal for volume controls, parameter adjustment.
     *          Automatically updates internal reference value.
     */
    bool hasChanged() {
      int currentValue = analogRead(_pin);
      
      // Check if change exceeds dead zone threshold
      if (abs(oldPot - currentValue) > 10) {
        oldPot = currentValue;  // Update reference
        state = true;
      } else {
        state = false;
      }
      
      return state;
    }
};

/*!
 * @brief Resistor Ladder Switch Reader Class
 * @details Handles 4 switches connected through a resistor ladder network to a single analog pin.
 *          Provides debounced switch detection with configurable thresholds.
 *          Ideal for reducing pin usage when multiple switches are needed.
 * 
 * Typical resistor ladder configuration:
 * - Switch 1: Direct to GND        (ADC ≈ 0)
 * - Switch 2: Through R1 to GND    (ADC ≈ 128)
 * - Switch 3: Through R1+R2 to GND (ADC ≈ 384)
 * - Switch 4: Through R1+R2+R3     (ADC ≈ 640)
 * - No switch: Pull-up resistor    (ADC ≈ 1023)
 */
class LadderSwitch {
  private:
    // === Pin Configuration ===
    uint8_t _pin;                    ///< Analog pin for reading ladder voltage
    
    // === State Management ===
    int currentSwitch;               ///< Currently detected switch (0-3, -1=none)
    int lastSwitch;                  ///< Previously detected switch for change detection
    
    // === Timing and Debouncing ===
    unsigned long lastReadTime;      ///< Timestamp of last reading for debounce timing
    unsigned long debounceDelay;     ///< Debounce delay in milliseconds
    
    // === ADC Thresholds ===
    int threshold1;                  ///< ADC threshold for switch 1 (lowest voltage)
    int threshold2;                  ///< ADC threshold for switch 2
    int threshold3;                  ///< ADC threshold for switch 3
    int threshold4;                  ///< ADC threshold for switch 4 (highest voltage)

  public:
    /*!
     * @brief Initialize ladder switch with pin and thresholds
     * @param pin Analog pin connected to resistor ladder
     * @param t1 Threshold for switch 1 (default: 128)
     * @param t2 Threshold for switch 2 (default: 384) 
     * @param t3 Threshold for switch 3 (default: 640)
     * @param t4 Threshold for switch 4 (default: 896)
     * @details Default thresholds work with equal-value resistor ladder.
     *          Adjust thresholds based on actual resistor values used.
     */
    void begin(uint8_t pin, int t1 = 128, int t2 = 384, int t3 = 640, int t4 = 896) {
      _pin = pin;
      pinMode(_pin, INPUT);
      
      // Initialize state
      currentSwitch = -1;
      lastSwitch = -1;
      lastReadTime = 0;
      debounceDelay = 50;  // 50ms default debounce
      
      // Set ADC thresholds
      threshold1 = t1;
      threshold2 = t2;
      threshold3 = t3;
      threshold4 = t4;
    }

    /*!
     * @brief Read raw switch position from ADC value
     * @return Switch number (0-3) or -1 if no switch pressed
     * @details Compares ADC reading against configured thresholds.
     *          Lower ADC values indicate switches closer to ground.
     *          No debouncing - use getSwitch() for debounced reading.
     */
    int readSwitch() {
      int adcValue = analogRead(_pin);
      
      if (adcValue < threshold1) {
        return 0;  // Switch 1 (closest to GND)
      } else if (adcValue < threshold2) {
        return 1;  // Switch 2
      } else if (adcValue < threshold3) {
        return 2;  // Switch 3
      } else if (adcValue < threshold4) {
        return 3;  // Switch 4 (furthest from GND)
      } else {
        return -1; // No switch pressed (pull-up active)
      }
    }

    /*!
     * @brief Get debounced switch reading
     * @return Switch number (0-3) when switch changes, -1 if no change
     * @details Implements time-based debouncing to prevent noise.
     *          Only returns switch number when state actually changes.
     *          Use in main loop for reliable switch detection.
     */
    int getSwitch() {
      unsigned long now = millis();
      
      // Debounce timing check
      if (now - lastReadTime < debounceDelay) {
        return -1;  // Still in debounce period
      }
      lastReadTime = now;
      
      int switchIndex = readSwitch();
      
      // Only report changes
      if (switchIndex != lastSwitch) {
        lastSwitch = switchIndex;
        currentSwitch = switchIndex;
        return switchIndex;
      }
      
      return -1;  // No change detected
    }

    /*!
     * @brief Set debounce delay
     * @param delay Debounce delay in milliseconds
     * @details Adjust based on switch characteristics and noise environment.
     *          Shorter delays = more responsive, longer delays = more stable.
     */
    void setDebounceDelay(unsigned long delay) {
      debounceDelay = delay;
    }

    /*!
     * @brief Update ADC threshold values
     * @param t1 Threshold for switch 1 (lowest voltage)
     * @param t2 Threshold for switch 2
     * @param t3 Threshold for switch 3  
     * @param t4 Threshold for switch 4 (highest voltage)
     * @details Calibrate thresholds based on actual measured ADC values.
     *          Set thresholds at midpoint between adjacent switch readings.
     */
    void setThresholds(int t1, int t2, int t3, int t4) {
      threshold1 = t1;
      threshold2 = t2;
      threshold3 = t3;
      threshold4 = t4;
    }
};

#endif // button_h

/*
 * === Usage Examples ===
 * 
 * // Basic button usage:
 * Button myButton;
 * myButton.begin(2);               // Initialize on pin 2
 * if (myButton.trigger()) {        // Detect press
 *   // Button was just pressed
 * }
 * if (myButton.isDown()) {         // Check current state
 *   // Button is currently held down
 * }
 * if (myButton.released()) {       // Detect release
 *   // Button was just released
 * }
 * 
 * // Rotary switch usage:
 * Button rotary;
 * rotary.begin(A0);                // Analog input
 * int pos = rotary.getPosition();  // Get position 0-5
 * 
 * // Ladder switch usage:
 * LadderSwitch ladder;
 * ladder.begin(A1);                // Initialize
 * int sw = ladder.getSwitch();     // Get switch 0-3 or -1
 * 
 * === Hardware Notes ===
 * 
 * - All digital pins automatically configured with INPUT_PULLUP
 * - Analog pins should use external pull-up for ladder switches
 * - Debounce timing can be adjusted per application needs
 * - Bit-shift debouncing provides fastest response for gaming/music applications
 * - Sample-counting debouncing provides most reliable operation for noisy environments
 */