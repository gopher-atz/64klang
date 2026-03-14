#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include <unordered_map>
#include <cstdint>

namespace Steinberg {
namespace Vst {

class K64Plugin : public AudioEffect
{
public:
    K64Plugin();
    ~K64Plugin() override;

    static FUnknown* createInstance(void* /*context*/)
    {
        return static_cast<IAudioProcessor*>(new K64Plugin());
    }

    // AudioEffect overrides
    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;
    tresult PLUGIN_API setActive(TBool state) override;
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                          SpeakerArrangement* outputs, int32 numOuts) override;
    tresult PLUGIN_API process(ProcessData& data) override;
    tresult PLUGIN_API getState(IBStream* state) override;
    tresult PLUGIN_API setState(IBStream* state) override;

private:
    bool synthInitialized = false;

    // Tracks active notes by VST3 note ID so NoteExpressionValueEvents can
    // find the right channel.  Note IDs are assigned by the host at note-on;
    // -1 means "no ID" (host didn't assign one).
    struct NoteInfo { int32 channel; int32 pitch; };
    std::unordered_map<int32, NoteInfo> noteIdMap;
};

} // namespace Vst
} // namespace Steinberg
