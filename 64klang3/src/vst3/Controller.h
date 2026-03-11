#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

namespace Steinberg {
namespace Vst {

// ParamID for MIDI CC: channel*128 + cc (0..2047). Matches SynthGlobalState.MidiSignals index.
static const ParamID kMidiCCParamBase = 0;
static const ParamID kMidiCCParamCount = 16 * 128;

inline ParamID midiCCParamID(int16 channel, CtrlNumber cc)
{
    return kMidiCCParamBase + (ParamID)channel * 128 + (ParamID)cc;
}

class K64Controller : public EditController
                 , public IMidiMapping
{
public:
    K64Controller() = default;

    static FUnknown* createInstance(void* /*context*/)
    {
        return static_cast<IEditController*>(new K64Controller());
    }

    // EditController overrides
    tresult PLUGIN_API initialize(FUnknown* context) override;
    IPlugView* PLUGIN_API createView(FIDString name) override;

    // IMidiMapping — map all 16 channels × 128 CCs to our CC parameters
    tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel,
                                                   CtrlNumber midiControllerNumber,
                                                   ParamID& id) override;

    OBJ_METHODS(K64Controller, EditController)
    DEFINE_INTERFACES
        DEF_INTERFACE(IMidiMapping)
    END_DEFINE_INTERFACES(EditController)
    REFCOUNT_METHODS(EditController)
};

} // namespace Vst
} // namespace Steinberg
