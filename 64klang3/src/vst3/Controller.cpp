#include "Controller.h"
#include "PluginView.h"
#include "pluginterfaces/base/fstrdefs.h"
#include "base/source/fstring.h"

namespace Steinberg {
namespace Vst {

tresult PLUGIN_API K64Controller::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultOk)
        return result;

    // Add MIDI CC parameters (16 channels × kCountCtrlNumber CCs) for IMidiMapping.
    // ParamID = channel*kCountCtrlNumber + cc. Host maps incoming CC to these params.
    parameters.init(kMidiCCParamCount, 256);
    char titleBuf[32];
    for (int32 ch = 0; ch < 16; ch++)
    {
        for (int32 cc = 0; cc < kCountCtrlNumber; cc++)
        {
            ParamID tag = (ParamID)ch * kCountCtrlNumber + (ParamID)cc;
            snprintf(titleBuf, sizeof(titleBuf), "Ch%d CC%d", ch + 1, cc);
            String str(titleBuf);
            parameters.addParameter(
                str.text16(),  // title
                STR16(""),    // units
                kCountCtrlNumber,          // stepCount (0..127)
                0.,           // defaultValueNormalized
                ParameterInfo::kCanAutomate,
                (ParamID)tag,
                kRootUnitId,
                str.text16()  // shortTitle
            );
        }
    }

    return kResultOk;
}

tresult PLUGIN_API K64Controller::getMidiControllerAssignment(int32 busIndex, int16 channel,
                                                              CtrlNumber midiControllerNumber,
                                                              ParamID& id)
{
    if (busIndex != 0 || channel < 0 || channel >= 16 || midiControllerNumber < 0 || midiControllerNumber >= kCountCtrlNumber)
        return kResultFalse;
    id = midiCCParamID(channel, midiControllerNumber);
    return kResultTrue;
}

IPlugView* PLUGIN_API K64Controller::createView(FIDString name)
{
    if (FIDStringsEqual(name, ViewType::kEditor))
    {
        return new K64PluginView();
    }
    return nullptr;
}

} // namespace Vst
} // namespace Steinberg
