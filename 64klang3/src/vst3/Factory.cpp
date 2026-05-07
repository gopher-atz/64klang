#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/base/fplatform.h"

#include "Plugin.h"
#include "Controller.h"
#include "version.h"    // generated at build time by cmake/StampVersion.cmake

#define K64_VENDOR      "Alcatraz"
#define K64_URL         "https://github.com/gopher-atz/64klang"
#define K64_EMAIL       ""

// Processor UID
static const Steinberg::FUID kProcessorUID(0x64ABCDEF, 0x12340001, 0xAAAABBBB, 0xCCCCDDD1);
// Controller UID (must match setControllerClass in Plugin.cpp)
static const Steinberg::FUID kControllerUID(0x64ABCDEF, 0x12340002, 0xAAAABBBB, 0xCCCCDDD2);

BEGIN_FACTORY_DEF(K64_VENDOR, K64_URL, K64_EMAIL)

    DEF_CLASS2(
        INLINE_UID_FROM_FUID(kProcessorUID),
        1,                          // single instance — engine is a process-wide singleton
        kVstAudioEffectClass,
        "64klang3",
        0,                          // not distributable — singleton state cannot be split
        Steinberg::Vst::PlugType::kInstrumentSynth,
        K64_VERSION,
        kVstVersionString,
        Steinberg::Vst::K64Plugin::createInstance
    )

    DEF_CLASS2(
        INLINE_UID_FROM_FUID(kControllerUID),
        1,                          // single instance — paired with the singleton processor
        kVstComponentControllerClass,
        "64klang3 Controller",
        0,
        "",
        K64_VERSION,
        kVstVersionString,
        Steinberg::Vst::K64Controller::createInstance
    )

END_FACTORY
