#include "Plugin.h"
#include "Controller.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "core/SynthController.h"
#include "core/Synth.h"
#include "gui/ImGuiPlugin.h"

#include <chrono>

namespace Steinberg {
namespace Vst {

K64Plugin::K64Plugin()
{
    setControllerClass(FUID(0x64ABCDEF, 0x12340002, 0xAAAABBBB, 0xCCCCDDD2));
}

K64Plugin::~K64Plugin()
{
}

tresult PLUGIN_API K64Plugin::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
        return result;

    // No audio inputs — this is a synthesizer
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);

    // Event input for MIDI
    addEventInput(STR16("Event In"), 16);

    // Initialize the synth controller singleton
    SynthController::instance();

    // Create the canvas once here so its view state persists across
    // window open/close cycles (attached/removed on the PluginView).
    K64GUI::createCanvas();

    synthInitialized = true;

    return kResultOk;
}

tresult PLUGIN_API K64Plugin::terminate()
{
    K64GUI::destroyCanvas();
    return AudioEffect::terminate();
}

tresult PLUGIN_API K64Plugin::setActive(TBool state)
{
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API K64Plugin::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                  SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns == 0 && numOuts == 1 && outputs[0] == SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    return kResultFalse;
}

tresult PLUGIN_API K64Plugin::process(ProcessData& data)
{
    if (!synthInitialized)
        return kResultOk;

    SynthController* sc = SynthController::instance();

    // Try to acquire mutex with 1ms timeout — output silence on failure
    if (!SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(1)))
    {
        // Output silence
        if (data.numOutputs > 0 && data.outputs[0].numChannels >= 2)
        {
            int32 numSamples = data.numSamples;
            float* left = data.outputs[0].channelBuffers32[0];
            float* right = data.outputs[0].channelBuffers32[1];
            for (int32 i = 0; i < numSamples; i++)
            {
                left[i] = 0.f;
                right[i] = 0.f;
            }
        }
        return kResultOk;
    }

    // Process MIDI events
    if (data.inputEvents)
    {
        int32 numEvents = data.inputEvents->getEventCount();
        for (int32 i = 0; i < numEvents; i++)
        {
            Event e;
            if (data.inputEvents->getEvent(i, e) == kResultOk)
            {
                switch (e.type)
                {
                case Event::kNoteOnEvent:
                    sc->noteOn(e.noteOn.channel, e.noteOn.pitch,
                               (uint32_t)(e.noteOn.velocity * 127.f));
                    break;
                case Event::kNoteOffEvent:
                    sc->noteOff(e.noteOff.channel, e.noteOff.pitch,
                                (uint32_t)(e.noteOff.velocity * 127.f));
                    break;
                case Event::kPolyPressureEvent:
                    sc->noteAftertouch(e.polyPressure.channel, e.polyPressure.pitch,
                                       (uint32_t)(e.polyPressure.pressure * 127.f));
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Process parameter changes (MIDI CC routed via IMidiMapping)
    if (data.inputParameterChanges)
    {
        int32 numQueues = data.inputParameterChanges->getParameterCount();
        for (int32 q = 0; q < numQueues; q++)
        {
            IParamValueQueue* queue = data.inputParameterChanges->getParameterData(q);
            if (!queue)
                continue;
            ParamID paramID = queue->getParameterId();
            if (paramID < kMidiCCParamBase || paramID >= kMidiCCParamBase + kMidiCCParamCount)
                continue;
            int32 numPoints = queue->getPointCount();
            if (numPoints <= 0)
                continue;
            // Use last point in queue (most recent value for this block)
            int32 sampleOffset;
            ParamValue normVal;
            if (queue->getPoint(numPoints - 1, sampleOffset, normVal) != kResultOk)
                continue;
            int ch = (int)(paramID / 128);
            int cc = (int)(paramID % 128);
            int value = (int)(normVal * 127.f);
            if (value < 0) value = 0;
            if (value > 127) value = 127;
            sc->midiSignal(ch, value, cc);
        }
    }

    // Read BPM from process context
    if (data.processContext && (data.processContext->state & ProcessContext::kTempoValid))
    {
        _64klang_SetBPM((float)data.processContext->tempo);
    }

    // Render audio
    if (data.numOutputs > 0 && data.outputs[0].numChannels >= 2)
    {
        float* left = data.outputs[0].channelBuffers32[0];
        float* right = data.outputs[0].channelBuffers32[1];
        sc->tick(left, right, data.numSamples);
    }

    SynthController::DataAccessMutex.unlock();

    return kResultOk;
}

tresult PLUGIN_API K64Plugin::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    std::string xml = SynthController::instance()->savePatchToString();
    if (xml.empty())
        return kResultFalse;

    uint32_t size = (uint32_t)xml.size();
    state->write(&size, sizeof(size), nullptr);
    state->write((void*)xml.data(), size, nullptr);

    // Append viewport (pan + zoom) so each DAW track instance remembers its own view.
    float ox = 0.f, oy = 0.f, z = 1.f;
    K64GUI::getViewport(ox, oy, z);
    state->write(&ox, sizeof(ox), nullptr);
    state->write(&oy, sizeof(oy), nullptr);
    state->write(&z,  sizeof(z),  nullptr);

    return kResultOk;
}

tresult PLUGIN_API K64Plugin::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    uint32_t size = 0;
    int32 numBytesRead = 0;
    state->read(&size, sizeof(size), &numBytesRead);
    if (numBytesRead != sizeof(size) || size == 0 || size > 100 * 1024 * 1024)
        return kResultFalse;

    std::string xml(size, '\0');
    state->read(&xml[0], size, &numBytesRead);
    if ((uint32_t)numBytesRead != size)
        return kResultFalse;

    if (!SynthController::instance()->loadPatchFromString(xml))
        return kResultFalse;

    // Read viewport if present (absent in files saved before this feature was added).
    float ox = 0.f, oy = 0.f, z = 1.f;
    int32 vr = 0;
    state->read(&ox, sizeof(ox), &vr);
    if (vr == sizeof(ox))
    {
        state->read(&oy, sizeof(oy), &vr);
        state->read(&z,  sizeof(z),  &vr);
        K64GUI::setViewport(ox, oy, z);
    }

    return kResultOk;
}

} // namespace Vst
} // namespace Steinberg
