#ifndef MODULES_SDK_INPUT_H_
#define MODULES_SDK_INPUT_H_

#include <stdint.h>

enum class InputKind : uint8_t
{
    SoftKey = 0,
    Encoder = 1,
    Navigate = 2,   // Up/Down
    HNavigate = 3,  // Left/Right
    Clear = 4,
    Back = 5,
};

enum class SoftKeyButtonState : uint8_t
{
    Release = 0,
    Press = 1,
    Hold = 2,
    Double = 3,
};

struct InputEvent
{
    InputKind kind = InputKind::SoftKey;
    uint8_t index = 0; // soft key 0..7 for InputKind::SoftKey
    SoftKeyButtonState state = SoftKeyButtonState::Press;
    int8_t delta = 0;
    bool shift = false;
};

#endif /* MODULES_SDK_INPUT_H_ */
