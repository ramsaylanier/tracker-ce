#ifndef MODULES_SDK_IDS_H_
#define MODULES_SDK_IDS_H_

#include <stdint.h>

// Named ModuleBase screens that feature modules may extend.
enum class ScreenId : uint8_t
{
    Pattern = 0,
    Song = 1,
    Instrument = 2,
    Master = 3,
    Performance = 4,
};

// Stable ids of the builtin pages a module can target:
//   replacePage(): Master::Master / Mixer / Sends (only Master resolves replacements)
//   addSoftKey():  Pattern::Main, Instrument::Effects
//   addPage():     any ScreenId (appended after that screen's builtin pages)
// Registration fails for any other screen / page pair.
namespace BuiltinPage
{
// Mini menu options at or above this value select module pages.
static constexpr uint8_t kModulePageIdBase = 64;

namespace Pattern
{
static constexpr uint8_t Main = 0; // soft keys only today
}

namespace Song
{
static constexpr uint8_t Playlist = 0; // reserved; Song does not resolve replacePage yet
}

namespace Instrument
{
static constexpr uint8_t Effects = 2;
}

namespace Master
{
static constexpr uint8_t Master = 0;
static constexpr uint8_t Mixer = 1;
static constexpr uint8_t Sends = 2;
}
} // namespace BuiltinPage

#endif /* MODULES_SDK_IDS_H_ */
