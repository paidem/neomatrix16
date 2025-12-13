
#include "Encoder.h"

Encoder* Encoder::_instances[6] = {nullptr};
uint8_t Encoder::_numInstances = 0;

Encoder::Encoder(uint8_t pinA, uint8_t pinB, uint8_t pinBtn, volatile int* valuePtr, ButtonCallback btnCallback)
    : _pinA(pinA), _pinB(pinB), _pinBtn(pinBtn), _valuePtr(valuePtr), _btnCallback(btnCallback), _lastBtnTime(0) {
    _lastStateA = LOW;
    _lastStateB = LOW;
    _lastBtnState = HIGH;
    _index = _numInstances;
    if (_numInstances < 6) {
        _instances[_numInstances++] = this;
    }
}

void Encoder::begin() {
    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    pinMode(_pinBtn, INPUT_PULLUP);
    _lastStateA = digitalRead(_pinA);
    _lastStateB = digitalRead(_pinB);
    _lastBtnState = digitalRead(_pinBtn);
    _old_AB = 3; // initial state for lookup
    _encval = 0;
    currentlyPressed = false;

    // Attach interrupts using lambdas and static_cast to call instance methods.
    // This allows each Encoder object to handle its own pins without global/static ISRs.
    // 'this' is passed as the argument, and inside the lambda, it is cast back to Encoder*.
    attachInterruptArg(digitalPinToInterrupt(_pinA), [](void* arg){ static_cast<Encoder*>(arg)->handleEncoderISR(); }, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(_pinB), [](void* arg){ static_cast<Encoder*>(arg)->handleEncoderISR(); }, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(_pinBtn), [](void* arg){ static_cast<Encoder*>(arg)->handleBtn(); }, this, CHANGE);
}

void Encoder::handleEncoderISR() {
    // Lookup table for state transitions
    static const int8_t enc_states[] = {0, -1, 1, 0, 1, 0, 0, -1, -1,
                                        0, 0, 1, 0, 1, -1, 0};

    _old_AB <<= 2;
    if (digitalRead(_pinA))
        _old_AB |= 0x02;
    if (digitalRead(_pinB))
        _old_AB |= 0x01;

    _encval += enc_states[_old_AB & 0x0f];

    // Only update value on full detent (4 steps)
    if (_encval > 3) {
        _encval = 0;
        (*_valuePtr)++;
    } else if (_encval < -3) {
        _encval = 0;
        (*_valuePtr)--;
    }
}

void Encoder::handleBtn() {
    int reading = digitalRead(_pinBtn);
    unsigned long now = millis();
    if (reading != _lastBtnState && (now - _lastBtnTime) > _debounceDelay) {
        currentlyPressed = (reading == LOW);
        if (_lastBtnState == LOW && reading == HIGH) {
            if (_btnCallback) _btnCallback();
        }
        _lastBtnTime = now;
    }
    _lastBtnState = reading;
}
