#ifndef FUNCTIONAL_INTERRUPT_H
#define FUNCTIONAL_INTERRUPT_H

#include <Arduino.h>
#include <functional>
#include <Wire.h>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// 1. attachInterrupt の曖昧さ回避
static std::function<void(void)> _interrupt_callback = nullptr;
static void IRAM_ATTR _interrupt_wrapper()
{
    if (_interrupt_callback)
        _interrupt_callback();
}

inline void attachInterrupt(uint8_t pin, std::function<void(void)> callback, int mode)
{
    _interrupt_callback = callback;
    ::attachInterrupt(digitalPinToInterrupt(pin), _interrupt_wrapper, (uint32_t)mode);
}

// 2. Wire.begin(sda, scl) のエラーを回避するための「ダミー関数」
// CST816S.cpp の中だけで有効な、引数2つの begin を定義します
namespace
{
    // Wire.begin(x, y) が呼ばれたら、中身のないこれ（引数2つのbegin）が呼ばれるようにします
    // ただし、これだと解決しない場合があるため、最も安全なのは「マクロを narrow (限定)」することです
}

#endif