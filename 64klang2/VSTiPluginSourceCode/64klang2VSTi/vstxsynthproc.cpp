#include "vstxsynth.h"
#include "../64klang2Core/SynthController.h"

//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------
void VstXSynth::setSampleRate (float sampleRate)
{
	AudioEffectX::setSampleRate (sampleRate);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setBlockSize (VstInt32 blockSize)
{
	AudioEffectX::setBlockSize (blockSize);
	// you may need to have to do something here...
}

//-----------------------------------------------------------------------------------------
void VstXSynth::initProcess ()
{
	// init plugin
	
}

//-----------------------------------------------------------------------------------------
void VstXSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	if (!SynthController::instance()->isInitialized())
		return;

	static int bpmCheck = 0;
	if ( bpmCheck <= 0 )
	{
		VstTimeInfo* myTime = getTimeInfo ( kVstTempoValid );
		SynthController::instance()->setBPM((float)myTime->tempo);
		bpmCheck = 10; // only check every 10th call of this function (bpm shouldnt change that often)
    }
	bpmCheck--;

	float* outl = outputs[0];
	float* outr = outputs[1];

	// try to acquire the mutex, if gui requests update after 1ms dont process the synth but return 0 values
	DWORD dwWaitResult = WaitForSingleObject(SynthController::instance()->DataAccessMutex, 1);
	if (dwWaitResult == WAIT_TIMEOUT)
	{
		for (int i = 0; i < sampleFrames; i++)
		{
			*outl++ = 0.0f;
			*outr++ = 0.0f;
		}
		m_currentEvents.clear();
		return;
	}

	int start = 0;
	int todo = 0;
	// midi events will occur this frame, so render partially
	if (m_currentEvents.size() > 0)
	{	
		// for all events
        for (unsigned int i = 0; i < m_currentEvents.size(); i++)
		{
			// process samples until next event
			int todo = m_currentEvents[i].deltaFrames - start;
			SynthController::instance()->tick(outl+start, outr+start, todo);
			start = m_currentEvents[i].deltaFrames;
			// apply changes due to event
			ApplyEvent(m_currentEvents[i]);
		}
	}
	SynthController::instance()->tick(outl+start, outr+start, sampleFrames - start);
	// clear event list (old event pointer wont be valid next frame anyway)
	m_currentEvents.clear();

	// unset data access mutex
	ReleaseMutex(SynthController::instance()->DataAccessMutex);
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::processEvents (VstEvents* ev)
{
	for (long i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;
		m_currentEvents.push_back(*((VstMidiEvent*)ev->events[i]));
	} 
	return 1;	// want more
}

//-----------------------------------------------------------------------------------------
void VstXSynth::ApplyEvent(VstMidiEvent &event)
{
	char* midiData = event.midiData;
	unsigned char status = midiData[0] & 0xf0;		// status
	unsigned char channel = midiData[0] & 0x0f;		// channel

	// note on/off events
	if (status == 0x90 || status == 0x80)
	{
		unsigned char note = midiData[1] & 0x7f;
		unsigned char velocity = midiData[2] & 0x7f;
		// note off
		if (status == 0x80 || ((status == 0x90) && (velocity == 0)))
		{
			SynthController::instance()->noteOff(channel, note, velocity);
		}
		// note on
		else if (status == 0x90)
		{
			SynthController::instance()->noteOn(channel, note, velocity);
		}		
	}
	// polyphonic aftertouch
	else if (status == 0xA)
	{
		byte note = midiData[1] & 0x7f;
		byte pressure = midiData[2] & 0x7f;
		SynthController::instance()->noteAftertouch(channel, note, pressure);
	}
/*	// channel aftertouch
	else if (status == 0xD)
	{
		byte pressure = midiData[1] & 0x7f;
		Go4kVSTi_ChannelAftertouch(channel, pressure);
	}
*/	// Controller Change
	else if (status == 0xB0)
	{
		unsigned char number = midiData[1] & 0x7f;
		unsigned char value = midiData[2] & 0x7f;
		SynthController::instance()->midiSignal(channel, value, number);
	}
	// Pitch Bend
	else if (status == 0xE0)
	{
		unsigned char lsb = midiData[1] & 0x7f;
		unsigned char msb = midiData[2] & 0x7f;
		int value = (((int)(msb)) << 7) + lsb;
		// pitch bend precision is 0 - 16383, center 8192
		// we dont use full precision for the sake of equally sized streams
		SynthController::instance()->midiSignal(channel, (value >> 6), 0); // 0 - 255, center 128
	}
}
