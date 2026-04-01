#include "Plugin.h"
#include "Controller.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"
#include "core/SynthController.h"
#include "core/Synth.h"
#include "gui/ImGuiPlugin.h"

#include <chrono>

namespace Steinberg {
namespace Vst {

// Definition of the static render-owner pointer.
std::atomic<K64Plugin*> K64Plugin::s_renderOwner{nullptr};
// Definition of the live-instance counter.
std::atomic<int> K64Plugin::s_instanceCount{0};

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

    // The first instance to initialize becomes the render owner — the only one
    // that will call sc->tick().  Subsequent instances (alias instruments) just
    // forward MIDI and output silence.
    K64Plugin* expected = nullptr;
    s_renderOwner.compare_exchange_strong(expected, this);

    ++s_instanceCount;
    synthInitialized = true;

    return kResultOk;
}

tresult PLUGIN_API K64Plugin::terminate()
{
    // Relinquish render ownership so another instance can take over if present.
    K64Plugin* me = this;
    s_renderOwner.compare_exchange_strong(me, nullptr);

    // Only destroy the canvas when the very last instance is gone.  On a
    // plugin reload the new instance's initialize() runs before the old one's
    // terminate(), so the canvas must survive until no instances are left.
    if (--s_instanceCount <= 0)
    {
        s_instanceCount = 0;
        K64GUI::destroyCanvas();
    }
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

    // Try to acquire mutex with (zero-timeout lock to avoid audio glitches)
    // MIDI events (especially note-offs) are processed unconditionally below so
    // that a timeout never causes stuck notes by swallowing a note-off event.
    bool mutexAcquired = SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(0));

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
                        sc->noteOn(e.noteOn.channel, e.noteOn.pitch, (uint32_t)(e.noteOn.velocity * 127.f));
                        // Track noteId → channel+pitch so NoteExpressionValueEvents
                        // can route back to the correct channel.
                        if (e.noteOn.noteId != -1)
                            noteIdMap[e.noteOn.noteId] = { e.noteOn.channel, e.noteOn.pitch };
                        break;
                    case Event::kNoteOffEvent:
                        sc->noteOff(e.noteOff.channel, e.noteOff.pitch, (uint32_t)(e.noteOff.velocity * 127.f));
                        if (e.noteOff.noteId != -1)
                            noteIdMap.erase(e.noteOff.noteId);
                        break;
                    case Event::kPolyPressureEvent:
                        sc->noteAftertouch(e.polyPressure.channel, e.polyPressure.pitch, (uint32_t)(e.polyPressure.pressure * 127.f));
                        break;
                    //case Event::kNoteExpressionValueEvent:
                    //{
                    //    // Resolve the channel from the note ID.  If the host didn't
                    //    // assign a note ID (-1) or we lost track of it, fall back to
                    //    // channel 0 (best effort — 64klang has no per-voice targeting).
                    //    int32 channel = 0;
                    //    {
                    //        auto it = noteIdMap.find(e.noteExpressionValue.noteId);
                    //        if (it != noteIdMap.end())
                    //            channel = it->second.channel;
                    //    }

                    //    // Normalized VST3 value [0.0, 1.0].  Each type has its own
                    //    // center convention; comments show the mapping.
                    //    const double v = e.noteExpressionValue.value;

                    //    switch (e.noteExpressionValue.typeId)
                    //    {
                    //        case NoteExpressionTypeIDs::kTuningTypeID:
                    //            // 0.5 = no detune, [0,1] = [-120,+120] semitones.
                    //            // Re-use the pitch-bend CC slot (CC 0) with the same
                    //            // 0–255 / center-128 scaling already used for MIDI pitch bend.
                    //            sc->midiSignal(channel, (int)(v * 256.f), 0);
                    //            break;

                    //        case NoteExpressionTypeIDs::kVolumeTypeID:
                    //            // 0.5 = 0 dB (unity), [0,1] → CC 7 (channel volume).
                    //            sc->midiSignal(channel, (int)(v * 127.f), 7);
                    //            break;

                    //        case NoteExpressionTypeIDs::kPanTypeID:
                    //            // 0.5 = centre, [0,1] → CC 10 (pan).
                    //            sc->midiSignal(channel, (int)(v * 127.f), 10);
                    //            break;

                    //        case NoteExpressionTypeIDs::kVibratoTypeID:
                    //            // [0,1] = depth → CC 1 (modulation wheel).
                    //            sc->midiSignal(channel, (int)(v * 127.f), 1);
                    //            break;

                    //        case NoteExpressionTypeIDs::kExpressionTypeID:
                    //            // [0,1] → CC 11 (expression).
                    //            sc->midiSignal(channel, (int)(v * 127.f), 11);
                    //            break;

                    //        case NoteExpressionTypeIDs::kBrightnessTypeID:
                    //            // [0,1] → CC 74 (filter cutoff / brightness).
                    //            sc->midiSignal(channel, (int)(v * 127.f + 0.5), 74);
                    //            break;

                    //        default:
                    //            break;
                    //    }
                    //    break;
                    //}
                    //case Event::kLegacyMIDICCOutEvent:
                    //    if (e.midiCCOut.controlNumber == Vst::kPitchBend)
                    //    {
                    //        // Reconstruct 14-bit value: (value2 << 7) | value
                    //        int32 value = ((int32)(e.midiCCOut.value2 & 0x7f) << 7) | (e.midiCCOut.value & 0x7f);
                    //        // pitch bend precision is 0 - 16383, center 8192
                    //        // we dont use full precision for the sake of equally sized streams
                    //        sc->midiSignal(e.midiCCOut.channel, (value >> 6), 0); // 0 - 255, center 128
                    //    }
                    //    break;
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
            if (normVal < 0.f) normVal = 0.f;
            if (normVal > 1.f) normVal = 1.f;
            int ch = (int)(paramID / kCountCtrlNumber);
            int cc = (int)(paramID % kCountCtrlNumber);
            if (cc == kPitchBend) {
                cc = 0; // map VST3's pitchbend from "CC"129 to 64klangs storage at CC0
                normVal*=2; // historically we transformed the pitchband range from 0..1 to 0..2 to get the full -/+ range with the same 128 steps, so we need to multiply by 2 here to restore that mapping.
            }
            int value = (int)(normVal * 127.f);
            // VST3 maps "CC"128 to channel pressure (aftertouch) which we dont support (could map to note aftertouch CC 0-127)
            if (cc != kAfterTouch)
                sc->midiSignal(ch, value, cc);
        }
    }

    // Read BPM from process context
    if (data.processContext && (data.processContext->state & ProcessContext::kTempoValid))
    {
        _64klang_SetBPM((float)data.processContext->tempo);
    }

    // Render audio — only the render owner advances the synth singleton.
    // Alias instrument shells output silence to avoid double-advancing state.
    // If the previous owner was destroyed (plugin reload) and we are the only
    // surviving instance, lazily claim ownership here.
    if (this != s_renderOwner.load(std::memory_order_relaxed))
    {
        K64Plugin* expected = nullptr;
        s_renderOwner.compare_exchange_strong(expected, this);
    }
    if (data.numOutputs > 0 && data.outputs[0].numChannels >= 2)
    {
        float* left  = data.outputs[0].channelBuffers32[0];
        float* right = data.outputs[0].channelBuffers32[1];
        if (mutexAcquired && this == s_renderOwner.load(std::memory_order_relaxed))
        {
            sc->tick(left, right, data.numSamples);
        }
        else
        {
            for (int32 i = 0; i < data.numSamples; i++)
            {
                left[i]  = 0.f;
                right[i] = 0.f;
            }
        }
    }

    if (mutexAcquired)
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
