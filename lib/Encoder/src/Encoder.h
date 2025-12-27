#ifndef ENCODER_H
#define ENCODER_H


#include <Arduino.h>

// Encoder class using hardware interrupts for rotary encoder and button
// Button callback must be a static function or a function pointer (no std::function).
// Value pointer must point to a volatile int.
class Encoder {
public:
    typedef void (*ButtonCallback)();
    bool currentlyPressed;
    unsigned long totalClicks = 0;
    Encoder(uint8_t pinA, uint8_t pinB, uint8_t pinBtn, volatile int* valuePtr, ButtonCallback btnCallback = nullptr, ButtonCallback btnLongCallback = nullptr, unsigned long longPressDuration = 1000);
    void begin();

private:
    void handleEncoderISR();
    void handleBtn();

    uint8_t _pinA, _pinB, _pinBtn;
    volatile int* _valuePtr;
    unsigned long _longPressStartClicks = 0;
    ButtonCallback _btnCallback;
    ButtonCallback _btnLongCallback;
    volatile int _lastStateA;
    volatile int _lastStateB;
    volatile int _lastBtnState;
    volatile unsigned long _lastBtnTime;
    volatile uint8_t _old_AB;
    volatile int8_t _encval;
    static Encoder* _instances[6];
    static uint8_t _numInstances;
    uint8_t _index;
    static const unsigned long _debounceDelay = 50;
    unsigned long _longPressDuration; 
    TimerHandle_t _longPressTimer = nullptr;
    static void longPressTimerCallback(TimerHandle_t xTimer);
    bool _longPressFired = false;
};

#endif // ENCODER_H
