// get skip node definitions in exe mode
#ifndef COMPILE_VSTI
	#include "64k2Patch.h"
#endif

#include "SynthNode.h"
#include "SynthAllocator.h"
#pragma warning( disable : 4244)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macros and helpers
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// for crinkler shuffle options, use to rearrange for a certain configuration in kkrunchy packer
//#define CODE_SECTIONS
// next: ".sn50"

#define NODE_STATEFUL_PROLOG if(n->lastTick == SynthGlobalState.CurrentTick) return; n->lastTick = SynthGlobalState.CurrentTick;
#define NODE_STATELESS_PROLOG 
//if(n->lastTick == SynthGlobalState.CurrentTick) return; n->lastTick = SynthGlobalState.CurrentTick;

// not calling constants at all gains about 5-10% cpu time but increases size
#define DONT_CALL_CONSTANTS_
#ifdef DONT_CALL_CONSTANTS
// use 0 constant tick and check before each call
#define NODE_CALL_INPUT(x) if(((SynthNode*)(n->input[x]))->tick) ((SynthNode*)(n->input[x]))->tick((SynthNode*)(n->input[x]));
#define CONTSTANT_TICK 0
#else
// call all nodes without check
#define NODE_CALL_INPUT(x) ((SynthNode*)(n->input[x]))->tick((SynthNode*)(n->input[x]));
#define CONTSTANT_TICK CONSTANT_tick
#endif

#ifdef COMPILE_VSTI
#define NODE_UPDATE_RATE 16
#define NODE_UPDATE_BEGIN if (--n->currentUpdateStep <= 0) { n->currentUpdateStep = n->updateRate; 
#else
// for exe: since all events are frame aligned we dont need the updatestep variable directly but can just use the tick with a mask
#define NODE_UPDATE_BEGIN if ((n->lastTick & 0x0f) == 1) {
#endif
#define NODE_UPDATE_END }

#define INP(x) (*(n->input[x]))

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// global states for the synth
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthGlobalStateStruct SynthGlobalState;

#ifdef COMPILE_VSTI
int SynthNode::numActiveVoices = 0;
#endif

// the id specific tick functions
SynthFunction NodeTickFunctions[MAXIMUM_ID] =
{
#ifdef COMPILE_VSTI
	SYNTHROOT_tick, // SYNTHROOT_tick // always needed
#else
	MUL_tick, // SYNTHROOT_tick // always needed
#endif
	CHANNELROOT_tick, // always needed
	NOTECONTROLLER_tick, // always needed
	VOICEMANAGER_tick, // always needed
	VOICEROOT_tick, // always needed

#ifndef ADSR_SKIP
	ADSR_tick,
#else
	0,
#endif
#ifndef LFO_SKIP
	LFO_tick,
#else
	0,
#endif
#ifndef OSCILLATOR_SKIP
	OSCILLATOR_tick,
#else
	0,
#endif
#ifndef NOISEGEN_SKIP
	NOISEGEN_tick,
#else
	0,
#endif
#ifndef BQFILTER_SKIP
	BQFILTER_tick,
#else
	0,
#endif
#ifndef ONEPOLEFILTER_SKIP
	ONEPOLEFILTER_tick,
#else
	0,
#endif
#ifndef ONEZEROFILTER_SKIP
	ONEZEROFILTER_tick,
#else
	0,
#endif
#ifndef SHAPER_SKIP
	SHAPER_tick,
#else
	0,
#endif
#ifndef PANNING_SKIP
	PANNING_tick,
#else
	0,
#endif
#ifndef SCALE_SKIP
	MUL_tick,
#else
	0,
#endif

	MUL_tick, // always needed

#ifndef DIV_SKIP
	DIV_tick,
#else
	0,
#endif

	ADD_tick, // always needed

#ifndef SUB_SKIP
	SUB_tick,
#else
	0,
#endif
#ifndef CLIP_SKIP
	CLIP_tick,
#else
	0,
#endif
#ifndef MIX_SKIP
	MIX_tick,
#else
	0,
#endif
#ifndef MULTIADD_SKIP
	NOTECONTROLLER_tick,
#else
	0,
#endif
#ifndef MONO_SKIP
	MONO_tick,
#else
	0,
#endif
#ifndef ENVFOLLOWER_SKIP
	ENVFOLLOWER_tick,
#else
	0,
#endif
#ifndef LOGIC_SKIP
	LOGIC_tick,
#else
	0,
#endif
#ifndef COMPARE_SKIP
	COMPARE_tick,
#else
	0,
#endif
#ifndef SELECT_SKIP
	SELECT_tick,
#else
	0,
#endif
#ifndef EVENTSIGNAL_SKIP
	EVENTSIGNAL_tick,
#else
	0,
#endif
#ifndef PROCESS_SKIP
	PROCESS_tick,
#else
	0,
#endif
#ifndef MIDISIGNAL_SKIP
	MIDISIGNAL_tick,
#else
	0,
#endif
#ifndef DISTORTION_SKIP
	DISTORTION_tick,
#else
	0,
#endif
#ifndef CROSSMIX_SKIP
	CROSSMIX_tick,
#else
	0,
#endif
#ifndef DELAY_SKIP
	DELAY_tick,
#else
	0,
#endif
#ifndef FBDELAY_SKIP
	FBDELAY_tick,
#else
	0,
#endif
#ifndef DCFILTER_SKIP
	DCFILTER_tick,
#else
	0,
#endif
#ifndef OSRAND_SKIP
	OSRAND_tick,
#else
	0,
#endif
#ifndef REVERB_SKIP
	REVERB_tick,
#else
	0,
#endif
#ifndef EQ_SKIP
	EQ_tick,
#else
	0,
#endif
#ifndef COMPEXP_SKIP
	COMPEXP_tick,
#else
	0,
#endif
#ifndef TRIGGER_SKIP
	TRIGGER_tick,
#else
	0,
#endif
#ifndef TRIGGERSEQ_SKIP
	TRIGGERSEQ_tick,
#else
	0,
#endif
#ifndef SAMPLEREC_SKIP
	SAMPLEREC_tick,
#else
	0,
#endif
#ifndef SAMPLER_SKIP
	SAMPLER_tick,
#else
	0,
#endif
#ifndef SVFILTER_SKIP
	SVFILTER_tick,
#else
	0,
#endif
#ifndef ABS_SKIP
	ABS_tick,
#else
	0,
#endif
#ifndef NEG_SKIP
	NEG_tick,
#else
	0,
#endif
#ifndef SQRT_SKIP
	SQRT_tick,
#else
	0,
#endif
#ifndef MIN_SKIP
	MIN_tick,
#else
	0,
#endif
#ifndef MAX_SKIP
	MAX_tick,
#else
	0,
#endif
#ifndef GLITCH_SKIP
	GLITCH_tick,
#else
	0,
#endif
#ifndef SAPI_SKIP
	SAPI_tick,
#else
	0,
#endif
#ifndef COMBDELAY_SKIP
	COMBDELAY_tick,
#else
	0,
#endif
#ifndef GMDLS_SKIP
	GMDLS_tick,
#else
	0,
#endif
#ifndef BOWED_SKIP
	BOWED_tick,
#else
	0,
#endif
#ifndef FORMULA_SKIP
	FORMULA_tick,
#else
	0,
#endif
#ifndef SNH_SKIP
	SNH_tick,
#else
	0,
#endif
#ifndef WTFOSC_SKIP
	WTFOSC_tick,
#else
	0,
#endif
#ifndef FORMANT_SKIP
	FORMANT_tick,
#else
	0,
#endif
#ifndef EQ3_SKIP
	EQ3_tick,
#else
	0,
#endif
#ifndef OSCSYNC_SKIP
	OSCSYNC_tick,
#else
	0,
#endif
	// reserved slots
	0, 0, 0, 0,
	CONTSTANT_TICK,	

	VOICEPARAM_tick, //VOICE_FREQUENCY_ID,		
	VOICEPARAM_tick, //VOICE_NOTE_ID,		
	VOICEPARAM_tick, //VOICE_ATTACKVELOCITY_ID,	
	VOICEPARAM_tick, //VOICE_RELEASEVELOCITY_ID,	
	VOICEPARAM_tick, //VOICE_TRIGGER_ID,			
	VOICEPARAM_tick, //VOICE_GATE_ID,			
};

// the id specific init functions (if any)
SynthFunction NodeInitFunctions[MAXIMUM_ID] =
{
	0,	//SYNTHROOT_tick
	0,	//CHANNELROOT_tick
	0,	//NOTECONTROLLER_tick
#ifdef COMPILE_VSTI
	VOICEMANAGER_init,	//VOICEMANAGER_tick
#else
	REVERB_init,
#endif
	0,	//VOICEROOT_tick,
	0,	//ADSR_tick,
	0,	//LFO_tick,
	0,	//OSCILLATOR_tick,
	0,	//NOISEGEN_tick,
	0,	//BQFILTER_tick,
	0,	//ONEPOLEFILTER_tick,
	0,	//ONEZEROFILTER_tick,
	0,	//SHAPER_tick,
	0,	//PANNING_tick,
	0,	//SCALE_tick,
	0,	//MUL_tick,
	0,	//DIV_tick,
	0,	//ADD_tick,
	0,	//SUB_tick,
	0,	//CLIP_tick,
	0,	//MIX_tick,
	0,	//MULTIADD_tick
	0,	//MONO_tick,
	0,	//ENVFOLLOWER_tick,
	0,	//LOGIC_tick,
	0,	//COMPARE_tick,
	0,	//SELECT_tick,
	0,	//EVENTSIGNAL_tick,
	0,	//PROCESS_tick,
	0,	//MIDISIGNAL_tick,
	0,	//DISTORTION_tick,
	0,	//CROSSMIX_tick,
#ifndef DELAY_SKIP
	DELAY_init,	//DELAY_tick
#else
	0,
#endif
#ifndef FBDELAY_SKIP
	DELAY_init,	//FBDELAY_tick
#else
	0,
#endif
	0,	//DCFILTER_tick
	0,	//OSRAND_tick
#ifndef REVERB_SKIP
	REVERB_init,	//REVERB_tick
#else
	0,
#endif
#ifndef EQ_SKIP
	REVERB_init,	//EQ_tick
#else
	0,
#endif
	0,	//COMPEXP_tick,	
	0,	//TRIGGER_init,
	0,	//TRIGGERSEQ_tick,
	0,	//SAMPLEREC_tick,
	0,	//SAMPLER_tick,
	0,	//SVFILTER_tick,
	0,	//ABS_tick,
	0,	//NEG_tick,
	0,	//SQRT_tick,
	0,	//MIN_tick,
	0,	//MAX_tick,
#ifndef GLITCH_SKIP
	GLITCH_init,
#else
	0,
#endif
	0,	//SAPI_tick
#ifndef COMBDELAY_SKIP
	DELAY_init,	//COMBDELAY_tick
#else
	0,
#endif
	0,	//GMDLS_tick
#ifndef BOWED_SKIP
	REVERB_init,	//BOWED_tick
#else
	0,
#endif
#ifndef FORMULA_SKIP
	REVERB_init,	//FORMULA_tick
#else
	0,
#endif
	0,	//SNH_tick,
	0,	//WTFOSC_tick,
	0,	//FORMANT_tick
	0,	//EQ3_tick
	0,	//OSCSYNC_tick
	// reserved slots
	0, 0, 0, 0,

	0,	//CONSTANT_tick,

	0,	//VOICE_FREQUENCY_ID,		
	0,	//VOICE_NOTE_ID,		
	0,	//VOICE_ATTACKVELOCITY_ID,	
	0,	//VOICE_RELEASEVELOCITY_ID,	
	0,	//VOICE_TRIGGER_ID,			
	0,	//VOICE_GATE_ID,			
};

// factors for bpm sync
float DELAY_TIME_FRACTION[33] = 
{	
	4.0f * (1.0f/128.0f),
	4.0f * (1.0f/64.0f) * (2.0f/3.0f),
	4.0f * (1.0f/128.0f)* (3.0f/2.0f),
	4.0f * (1.0f/64.0f),
	4.0f * (1.0f/32.0f) * (2.0f/3.0f),	
	4.0f * (1.0f/64.0f) * (3.0f/2.0f),
	4.0f * (1.0f/32.0f),
	4.0f * (1.0f/16.0f) * (2.0f/3.0f),
	4.0f * (1.0f/32.0f) * (3.0f/2.0f),
	4.0f * (1.0f/16.0f),
	4.0f * (1.0f/ 8.0f) * (2.0f/3.0f),
	4.0f * (1.0f/16.0f) * (3.0f/2.0f),
	4.0f * (1.0f/ 8.0f),
	4.0f * (1.0f/ 4.0f) * (2.0f/3.0f),
	4.0f * (1.0f/ 8.0f) * (3.0f/2.0f),
	4.0f * (1.0f/ 4.0f),
	4.0f * (1.0f/ 2.0f) * (2.0f/3.0f),
	4.0f * (1.0f/ 4.0f) * (3.0f/2.0f),
	4.0f * (1.0f/ 2.0f),
	4.0f * (1.0f      ) * (2.0f/3.0f),
	4.0f * (1.0f/ 2.0f) * (3.0f/2.0f),
	4.0f * (1.0f      ),
	4.0f * (1.0f      ) * (3.0f/2.0f),
	4.0f * (3.0f /8.0f),
	4.0f * (5.0f /8.0f),
	4.0f * (7.0f /8.0f),
	4.0f * (9.0f /8.0f),
	4.0f * (11.0f/8.0f),
	4.0f * (13.0f/8.0f),
	4.0f * (15.0f/8.0f),
	4.0f * (3.0f /4.0f),
	4.0f * (5.0f /4.0f),
	4.0f * (7.0f /4.0f),
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// node implementaions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef COMPILE_VSTI
// same behaviour with envfollower for synthroot, so the signal activity display can be used in gui
void SYNTHCALL SYNTHROOT_tick(SynthNode* n)
{
	MUL_tick(n);

	// envelope follower
	sample_t v = s_abs(n->out);
	n->v[1] = (s_ifthen(v > n->v[1], SC[S_0_5], SC[CHANNELROOT_EFC]) * (n->v[1] - v)) + v;
}
#endif

#ifdef CODE_SECTIONS
#pragma code_seg(".sn01")
#endif
void SYNTHCALL CHANNELROOT_tick(SynthNode* n)
{
	// check if at least one channel still above signal threshold
	if (!_mm_testz_si128(n->v[0].pi, n->v[0].pi))
	{
		MUL_tick(n);

		// envelope follower
		sample_t v = s_abs(n->out);
#ifdef COMPILE_VSTI
		n->v[1] = (s_ifthen(v > n->v[1], SC[S_0_5], SC[CHANNELROOT_EFC]) * (n->v[1] - v)) + v;
#else
		n->v[1] = (SC[CHANNELROOT_EFC] * (n->v[1] - v)) + v;
#endif

		// set active bitmask according to threshold in both channels
		n->v[0] = n->v[1] > SC[CHANNELROOT_EFT];
	}

#ifdef COMPILE_VSTI
	if (n->processingFlags & NODE_PROCESSING_MUTE)
		n->out = sample_t::zero();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// same as multiadd, combined event not really needed
#ifdef CODE_SECTIONS
#pragma code_seg(".sn02")
#endif
void SYNTHCALL NOTECONTROLLER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;
	
	int i = n->numInputs;
	while (i--)
		NODE_CALL_INPUT(i);

	// process
	//n->e = 
	n->out = sample_t::zero();
	i = n->numInputs;
	while (i--)
	{
		n->out += n->input[i];
		//n->e |= ((SynthNode*)(n->input[i]))->e;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define W ((VMWork*)(n->customMem))

#ifdef COMPILE_VSTI
void SYNTHCALL VOICEMANAGER_init(SynthNode* n)
{
	n->customMem = (DWORD*)SynthMalloc(sizeof(VMWork));
	for (int i = 0; i < 16; i++)
	{
		W->ArpSequence[i*2+0] = 0x7fe0;
		W->ArpSequence[i*2+1] = 0x00e0;
	}
	W->ArpSequencePtr = W->ArpSequence;
	W->ArpSequenceLoopIndex = 0;
}
#endif

#ifdef CODE_SECTIONS
#pragma code_seg(".sn04")
#endif
inline sample_t GetDelayTimeFraction(const sample_t& value)
{
	sample_t fracindex = s_toInt(s_clamp(value, SC[S_1_0], sample_t::zero())*SC[S_32_0]);
	return sample_t(DELAY_TIME_FRACTION[fracindex.i[0]], DELAY_TIME_FRACTION[fracindex.i[1]]) * SC[S_60_0] * SC[S_SAMPLERATE] / SynthGlobalState.CurrentBPM;
}

#ifndef VOICEMANAGER_ARP_SKIP
// get the next arpeggio key
#ifdef CODE_SECTIONS
#pragma code_seg(".sn05")
#endif
void InsertArpKeys(SynthNode* n, int mode, int transpose, int velocity)
{
	velocity = velocity << 8;
	int runmode = (mode & VOICEMANAGER_ARP_RUNMASK) >> VOICEMANAGER_ARP_RUNSHIFT;
	int octaves = (mode & VOICEMANAGER_ARP_OCTMASK) >> VOICEMANAGER_ARP_OCTSHIFT;
	// not ARP_OFF?
	if (runmode != 0)
	{
		// init stepper	
		int firstKey = W->LowestKey;
		int lastKey = W->HighestKey;
		int deltaKey = 1;

		// ARP_UP/ARP_DOWN
		if (runmode != 3)
		{
			// mode down?
			if (runmode == 2)
			{
				firstKey = W->HighestKey;
				lastKey = W->LowestKey;
				deltaKey = -1;
			}

			// initial key?
			if (W->ArpCurrentKey == -1)
			{
				W->ArpCurrentOct = 0;
				W->ArpCurrentKey = firstKey;
			}
			else
			{
				// next octave? wrap around
				if (W->ArpCurrentKey == lastKey)
				{
					W->ArpCurrentKey = firstKey;
					W->ArpCurrentOct++;
					// wrap octaves
					if (W->ArpCurrentOct >= octaves)
						W->ArpCurrentOct = 0;
				}
				// next note
				else
				{
					while (W->ArpCurrentKey != lastKey)
					{
						W->ArpCurrentKey += deltaKey;
						if (W->Keys[W->ArpCurrentKey])
							break;
					}
				}
			}
			// insert to note list for trigger later
			*(((DWORD*)(n->v)) + 32 + n->v[20].i[1]) = (W->ArpCurrentKey + 12 * W->ArpCurrentOct + transpose) | velocity;
			n->v[20].i[1]++;
		}
		// ARP_POLY
		else
		{
			while (lastKey >= firstKey)
			{
				if (W->Keys[lastKey])
				{
					// insert to note list for trigger later
					*(((DWORD*)(n->v)) + 32 + n->v[20].i[1]) = (lastKey + transpose) | velocity;
					n->v[20].i[1]++;
				}
				lastKey--;
			}
		}
	}
}

void ClearArpVoices(SynthNode* n)
{
	int numActive = W->NumActiveVoices;
	while (numActive--)
	{
		VMVoice* voice = &(W->Voice[W->ActiveVoices[numActive]]);		
#ifndef VOICEMANAGER_GLIDE_SKIP
		// kill voice when no keys left or not a glider
		if ((W->NumKeys == 0) || (voice != W->Glider))
#endif
		{
			voice->Gate = sample_t::zero();
			//voice->ReleaseVelocity = sample_t((double)velo/SC[S_128_0].d[0]);
			voice->NoteValue = -1;
#ifndef VOICEMANAGER_GLIDE_SKIP
			// free glider on last key
			if (voice == W->Glider)
				W->Glider = NULL;
#endif
		}
	}	
}

float ArpStepGateFraction[4] = 
{
	3.0f,
	2.0f,
	1.5f,
	1.0f,
};

#ifdef CODE_SECTIONS
#pragma code_seg(".sn06")
#endif
void ArpeggiatorStep(SynthNode* n, int mode)
{
	// check for arpeggio
	if ((mode & VOICEMANAGER_ARP_RUNMASK))
	{		
#ifndef COMPILE_VSTI
		// first time init, set step data pointer to the corresponding offset in the values
		if (W->ArpSequencePtr == 0)
		{
			WORD* stepdata = &(SynthGlobalState.NodeValues[SynthGlobalState.MaxOffset + 1 + ((mode & VOICEMANAGER_ARP_INDEXMASK) >> VOICEMANAGER_ARP_INDEXSHIFT) * 33]);
			W->ArpSequenceLoopIndex = *stepdata++;
			W->ArpSequencePtr = stepdata;
		}
#endif
		// keys available?
		if (W->NumKeys)
		{
			// get current steptime
			sample_t steptime = GetDelayTimeFraction(INP(VOICEMANAGER_ARPSPEED));
			int velocity = 0;
			int transpose = 0;

			// new step?
			n->v[17] -= SC[S_1_0];
			sample_t arpstep = n->v[17] < sample_t::zero();			
			if (!_mm_testz_si128(arpstep.pi, arpstep.pi))
			{
				// get current step pattern value
				int value = W->ArpSequencePtr[n->v[19].i[0]];				
				// set velocity
				velocity = value >> 8;						
				// gate
				value = value & 0xff;
				int gate = value >> 6;
				// set transpose
				transpose = (value & 0x3f) - 32;

				// new note (velocity != 0) or hold (value != 0), so set/increaset the gate time
				if ((velocity != 0) || (value != 0))
					n->v[18] = steptime/sample_t((double)ArpStepGateFraction[gate]);

				// reset step counter
				n->v[17] = steptime;

				// wrap pattern index to loop pos
				n->v[19].i[0]++;
				if (n->v[19].i[0] >= 32)
					n->v[19].i[0] = W->ArpSequenceLoopIndex;
			}

			// proceed gate time			
			sample_t gateoff = n->v[18] < sample_t::zero();
			// send note off to all voices when gate is over or new notes are due
			if (velocity || !_mm_testz_si128(gateoff.pi, gateoff.pi))
			{
				// if gate is over set gate counter to some larger number
				n->v[18] = s_ifthen(gateoff, SC[ADSR_SCALE], n->v[18]);
				ClearArpVoices(n);
#ifdef COMPILE_VSTI
				// force update so the voices are processed. this is not needed in exe mode as we are aligned with the frames anyway, so update step will follow directly
				n->currentUpdateStep = 0;
#endif
			}
			// send note on for new key in arpeggio
			if (velocity)
			{
				InsertArpKeys(n, mode, transpose, velocity);
#ifdef COMPILE_VSTI
				// force update so the voices are processed. this is not needed in exe mode as we are aligned with the frames anyway, so update step will follow directly
				n->currentUpdateStep = 0;
#endif
			}
			// step the gate counter
			n->v[18] -= SC[S_1_0];
		}
		// no arp voices
		else
		{
#ifdef COMPILE_VSTI
			n->v[19] = sample_t::zero();
#endif
			ClearArpVoices(n);
		}
	}
}
#endif

#ifdef CODE_SECTIONS
#pragma code_seg(".sn07")
#endif
void SYNTHCALL VOICEMANAGER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(VOICEMANAGER_TRANSPOSE);
	NODE_CALL_INPUT(VOICEMANAGER_GLIDE);
	NODE_CALL_INPUT(VOICEMANAGER_ARPSPEED);
	//NODE_CALL_INPUT(VOICEMANAGER_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[VOICEMANAGER_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// keys changed? rebuild key array and get min/max note. used by arp and glide
	if (W->Rescan)
	{
#ifndef VOICEMANAGER_ARP_SKIP
		// a reset is needed when playing the first note
		if (W->NumKeys == 0)
			W->ArpCurrentKey = -1;
#endif
		// rescan keys
		W->NumKeys = 0;
		W->LowestKey = 128;
		int note = 128;
		while (note--)
		{
			if (W->Keys[note])
			{
				if (W->NumKeys++ == 0)
					W->HighestKey = note;						
				W->LowestKey = note;
			}
		}
#ifndef VOICEMANAGER_ARP_SKIP
		// reset stepper when having the first note, or when retrigger is active
		if (W->ArpCurrentKey == -1)
		{
			n->v[17] = sample_t::zero(); // step counter in samples
			n->v[18] = sample_t::zero(); // gate counter in samples
			n->v[19] = sample_t::zero(); // step index (int)
		}
		// ensure valid current key otherwise
		else
		{
			if (W->ArpCurrentKey > W->HighestKey)
				W->ArpCurrentKey = W->HighestKey;
			if (W->ArpCurrentKey < W->LowestKey)
				W->ArpCurrentKey = W->LowestKey;
		}
#endif
		W->Rescan = 0;
	}

#ifndef VOICEMANAGER_ARP_SKIP
	// step the arp if it is enabled
	ArpeggiatorStep(n, mode);
#endif

	NODE_UPDATE_BEGIN

#ifndef VOICEMANAGER_GLIDE_SKIP
		// no glide?
		sample_t gi = INP(VOICEMANAGER_GLIDE);
		if (_mm_testz_si128(gi.pi, gi.pi))
		{
			n->v[31] = sample_t::zero();
		}
		// get current glide time
		else
		{
			n->v[31] = GetDelayTimeFraction(INP(VOICEMANAGER_GLIDE));
			n->v[31] = s_ifthen(gi >= SC[S_1_0], SC[S_1_0], n->v[31]);
		}
#endif

		// 1. loop all note off events (slots 0..31)
		while (n->v[20].i[0])
		{
			n->v[20].i[0]--;
			DWORD data = *(((DWORD*)(n->v))+n->v[20].i[0]);
			int note = data & 0xff;
			//DWORD velo = data >> 8;
			// loop through active voices
			int numActive = W->NumActiveVoices;
			while (numActive--)
			{
				VMVoice* voice = &(W->Voice[W->ActiveVoices[numActive]]);		
				// found a voice with the note
				if (note == voice->NoteValue)
				{
#ifndef VOICEMANAGER_GLIDE_SKIP
#ifdef COMPILE_VSTI
					// is glide mode active?
					if (!_mm_testz_si128(n->v[31].pi, n->v[31].pi))
					{
#endif
						// found the glider?
						if (voice == W->Glider)
						{
							// check for the second oldest key if exists
							int nextkey = 0;
							DWORD nexttime = 0;
							int key = W->HighestKey;
							while (key >= W->LowestKey)
							{
								if (W->Keys[key] > nexttime)
								{
									nextkey = key;
									nexttime = W->Keys[key];
								}
								key--;
							}
							// found key to glide to?
							if (nexttime != 0)
							{
								// set new glide base and target for the glider and reset glidestep to 0
								voice->NoteValue = nextkey;
								voice->GlideTargetValue = sample_t((double)nextkey);
								voice->GlideBaseValue = voice->GlideCurrentValue;								
								voice->GlideStep = sample_t::zero();
								// done, no voice needs to be deleted								
								break;
							}
							else
							{
								W->Glider = NULL;
							}
						}	
#ifdef COMPILE_VSTI
					}
#endif
#endif		
					// mark as off
					voice->NoteValue = -1;
					// remove gate
					voice->Gate = sample_t::zero();
					//voice->ReleaseVelocity = sample_t((double)velo/SC[S_128_0].d[0]);					
					break;
				}
			}			
		}

		// 2. loop all note on events (slots 32..63)
		int slot = 64 >> ((mode & VOICEMANAGER_MAXPOLY_RUNMASK) >> VOICEMANAGER_MAXPOLY_RUNSHIFT); // maximum polyphony is 64, reduced can be 32, 16, 8
		while (n->v[20].i[1])
		{
			n->v[20].i[1]--;
			DWORD data = *(((DWORD*)(n->v))+32+n->v[20].i[1]);
			int note = data & 0xff;
			DWORD velo = data >> 8;

			// depending on a define constants dont have tick function anymore, so need to check that now
#ifdef COMPILE_VSTI
			if (((SynthNode*)(n->input[VOICEMANAGER_VOICEROOT]))->tick == 0)
				continue;
#endif

			VMVoice* voice;
#ifndef VOICEMANAGER_GLIDE_SKIP
#ifdef COMPILE_VSTI
			// is glide mode active?
			if (!_mm_testz_si128(n->v[31].pi, n->v[31].pi))
			{
#endif
				// we have a glider already?
				if (W->Glider)
				{
					voice = W->Glider;
					// set new glide base and target for the glider and reset glidestep to 0
					voice->NoteValue = note;
					voice->GlideTargetValue = sample_t((double)note);
					voice->GlideBaseValue = voice->GlideCurrentValue;					
					voice->GlideStep = sample_t::zero();
					// done, no new voice needs to be spawned					
					continue;
				}
#ifdef COMPILE_VSTI
			}
#endif
#endif
			// loop through all voice slots			
			int oldestvoicetime = -1, oldestslot = -1, oldestofftime = -1, offslot = -1, checkslot = slot;
			while (checkslot--)
			{
				voice = &(W->Voice[checkslot]);
				// found a free voice?
				if (voice->VoiceRoot == NULL)
				{
					W->ActiveVoices[W->NumActiveVoices++] = checkslot;
#ifdef COMPILE_VSTI
					SynthNode::numActiveVoices++;
#endif
					goto VoiceManager_CreateVoice;
				}
				// found a used but keyed off voice
				if (voice->NoteValue < 0)
				{
					// mark the oldest one 
					if (voice->VoiceTime < (DWORD)oldestofftime)
					{
						oldestofftime = voice->VoiceTime;
						offslot = checkslot;
					}
				}
				// if nothing else, find the overall oldest voice
				else if (voice->VoiceTime < (DWORD)oldestvoicetime)
				{
					oldestvoicetime = voice->VoiceTime;
					oldestslot = checkslot;
				}
			}	

			// use the keyed off slot
			if (offslot >= 0)
				checkslot = offslot;
			// use the oldest slot
			else
				checkslot = oldestslot;
				
			// destroy old voice
			voice = &(W->Voice[checkslot]);
			DestroyVoiceNodes(voice->VoiceRoot);

VoiceManager_CreateVoice:
			// create new voice
			voice->VoiceTime = SynthGlobalState.CreateNodeTick++;
			voice->NoteValue = note;
			voice->BaseNote = sample_t((double)note)/SC[S_128_0];
			voice->AttackVelocity = sample_t((double)velo)/SC[S_128_0];	
			voice->BaseFrequency = (SC[S_440_0]*s_exp2(sample_t((double)(note-69))*SC[S_1_O_12])) / SC[S_SAMPLERATE];								
			voice->Trigger = voice->Gate = SC[S_1_0];

#ifdef COMPILE_VSTI
			// in plugin there's the template instrument root already connected, so use that offset
			voice->VoiceRoot = CreateNodes(((SynthNode*)(n->input[VOICEMANAGER_VOICEROOT]))->valueOffset, false);
#else
			// in intro mode, get the offset directly frm our nodes values
			voice->VoiceRoot = CreateNodes(SynthGlobalState.NodeValues[n->valueOffset+1], false);
#endif				

#ifndef VOICEMANAGER_GLIDE_SKIP
			// is glide mode active?
			if (!_mm_testz_si128(n->v[31].pi, n->v[31].pi))
			{
				// init glide for the new voice
				W->Glider = voice;
				voice->GlideTargetValue = sample_t((double)note);
				voice->GlideBaseValue = voice->GlideTargetValue;
				voice->GlideStep = sample_t::zero();
			}
#endif				
		}

		n->v[16] = s_exp2(INP(VOICEMANAGER_TRANSPOSE)*SC[S_128_0] * SC[S_1_O_12]);

	NODE_UPDATE_END

	// process	

	// call all active voices and accumulate output
	n->out = sample_t::zero();
	int numActive = W->NumActiveVoices;
	while (numActive--)
	{
		VMVoice* voice = &(W->Voice[W->ActiveVoices[numActive]]);
		SynthGlobalState.CurrentVoice = voice;		
#ifndef VOICEMANAGER_GLIDE_SKIP
#ifdef COMPILE_VSTI
		// is glide mode active?
		if (!_mm_testz_si128(n->v[31].pi, n->v[31].pi))
		{
#endif
			if (voice == W->Glider)
			{
				voice->GlideCurrentValue = s_lerp(voice->GlideBaseValue, voice->GlideTargetValue, s_min(voice->GlideStep/n->v[31], SC[S_1_0]));
				voice->BaseNote = voice->GlideCurrentValue/SC[S_128_0];
				voice->BaseFrequency = (SC[S_440_0]*s_exp2((voice->GlideCurrentValue-sample_t((double)69))*SC[S_1_O_12])) / SC[S_SAMPLERATE];
				voice->GlideStep += SC[S_1_0];
			}
#ifdef COMPILE_VSTI
		}
#endif
#endif
		voice->Note = voice->BaseNote+INP(VOICEMANAGER_TRANSPOSE);
		voice->Frequency = voice->BaseFrequency*n->v[16];
#ifndef SONG_AFTERTOUCH_SKIP
		// set current aftertouch value
		if (voice->NoteValue >= 0)
		{
			voice->Aftertouch = SynthGlobalState.NoteAftertouch[(n->v[23].i[0] << 7) + voice->NoteValue]; // v[23] got filled by note insertion from synth.cpp with channel index
		}
#endif
		voice->VoiceRoot->tick(voice->VoiceRoot);
		n->out += voice->VoiceRoot->out;
		// remove voice trigger
		voice->Trigger = sample_t::zero();
		// voice done? destroy
		if (_mm_testz_si128(voice->VoiceRoot->e.pi, voice->VoiceRoot->e.pi))
		{		
			DestroyVoiceNodes(voice->VoiceRoot);			
			voice->VoiceRoot = NULL;			
			--W->NumActiveVoices;
#ifdef COMPILE_VSTI
			--SynthNode::numActiveVoices;
#endif
			// swap with last slot if needed
			if (numActive != W->NumActiveVoices)
				W->ActiveVoices[numActive] = W->ActiveVoices[W->NumActiveVoices];
#ifndef VOICEMANAGER_GLIDE_SKIP
			if (voice == W->Glider)
				W->Glider = NULL;
#endif
		}
	}
	// set event to active when keys are active
	if (W->NumKeys > 0)
		n->e = SC[S_1_0];
	else
		n->e = sample_t::zero();

#ifdef COMPILE_VSTI
	if (n->processingFlags & NODE_PROCESSING_MUTE)
		n->out = sample_t::zero();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sn08")
#endif
void SYNTHCALL VOICEROOT_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(VOICEROOT_IN);
	NODE_CALL_INPUT(VOICEROOT_GAIN);
	NODE_CALL_INPUT(VOICEROOT_GATE_OUT);

	// process
	n->out = INP(VOICEROOT_IN) * INP(VOICEROOT_GAIN);
	// update event status so parent knows when we are done with the voice	
	n->e = ((SynthNode*)(n->input[VOICEROOT_GATE_OUT]))->out != sample_t::zero();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline sample_t SYNTHCALL ADSR_Map(const sample_t& in)
{
	sample_t temp = s_max(in, sample_t::zero());
	return SC[S_1_0] / (temp*temp*temp*SC[ADSR_SCALE] + SC[S_1_0]);
}

#ifndef ADSR_SKIP

// TODO: need to specify this manually if sure that stereo is not needed
#ifndef ADSR_SKIP_STEREO
#define ADSR_IFTHEN(X,Y,Z) s_ifthen(X,Y,Z)
#else
#define ADSR_IFTHEN(X,Y,Z) Y
#endif

#ifdef CODE_SECTIONS
#pragma code_seg(".sn09")
#endif
void SYNTHCALL ADSR_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

#ifndef ADSR_SKIP_TRIGGER_GATE
	NODE_CALL_INPUT(ADSR_TRIGGER);
	NODE_CALL_INPUT(ADSR_GATE);
#endif
	NODE_CALL_INPUT(ADSR_ATTACK);
	NODE_CALL_INPUT(ADSR_DECAY);
	NODE_CALL_INPUT(ADSR_SUSTAIN);
	NODE_CALL_INPUT(ADSR_RELEASE);
	NODE_CALL_INPUT(ADSR_GAIN);
	//NODE_CALL_INPUT(ADSR_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[ADSR_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN

#ifndef ADSR_SKIP_DBGAIN
		if (mode & ADSR_DBGAIN)
			n->v[8] = s_db2lin((INP(ADSR_GAIN) - SC[S_0_75]) * SC[S_128_0]);
		else
#endif
			n->v[8] = INP(ADSR_GAIN);

	NODE_UPDATE_END

	// process
	
	// if trigger is active set state to attack (trigger signal >= 0.5)
#ifndef ADSR_SKIP_TRIGGER_GATE
	sample_t tmask = INP(ADSR_TRIGGER);
	if (mode & ADSR_VOICETRIGGER)
		tmask += SynthGlobalState.CurrentVoice->Trigger;
	tmask = tmask >= SC[S_0_5];
#else
	sample_t tmask = SynthGlobalState.CurrentVoice->Trigger >= SC[S_0_5];
#endif
	// at least one channel is triggered? switch to attack
	if (!_mm_testz_si128(tmask.pi, tmask.pi))
	{
		// set attack state (0.5)
		n->v[0] = ADSR_IFTHEN(tmask, SC[S_0_5], n->v[0]); 
		// set level to 0
		n->v[1] = ADSR_IFTHEN(tmask, sample_t::zero(), n->v[1]);
		// calculate current attack rate
		n->v[2] = ADSR_IFTHEN(tmask, ADSR_Map(INP(ADSR_ATTACK)), n->v[2]);				
	}
	
	// release state? = gate is no longer active (gate signal < 0.5)
#ifndef ADSR_SKIP_TRIGGER_GATE
	sample_t rmask = INP(ADSR_GATE);
	if (mode & ADSR_VOICEGATE)
		rmask += SynthGlobalState.CurrentVoice->Gate;
	rmask = rmask < SC[S_0_5];
#else
	sample_t rmask = SynthGlobalState.CurrentVoice->Gate < SC[S_0_5];
#endif
	sample_t rbmask = (n->v[0] <= SC[S_2_0]) & rmask;
	// at least one channel not yet in release? switch to release
	if (!_mm_testz_si128(rbmask.pi, rbmask.pi))
	{
		// set release state (10.0)
		n->v[0] = ADSR_IFTHEN(rbmask, SC[S_10_0], n->v[0]);
		// calculate current release rate
		sample_t rr = ADSR_Map(INP(ADSR_RELEASE));
		// linear delta
		n->v[2] = ADSR_IFTHEN(rbmask, rr*n->v[1], n->v[2]); 
#ifndef ADSR_SKIP_EXP
		// exp factor
		n->v[4] = ADSR_IFTHEN(rbmask, s_exp2(rr*SC[ADSR_LOG2]), n->v[4]);
#endif
	}	
#ifndef ADSR_SKIP_EXP
	if (mode & ADSR_EXP)
	{
		n->v[1] = ADSR_IFTHEN(rmask, n->v[1]*n->v[4], n->v[1]);
	}
	else
#endif
	{
		n->v[1] = ADSR_IFTHEN(rmask, s_max(n->v[1] - n->v[2], sample_t::zero()), n->v[1]);
	}
		
	// sustain state? = 2.0
	sample_t sustainlevel = s_max(INP(ADSR_SUSTAIN), sample_t::zero());
	sample_t smask = n->v[0] != SC[S_2_0]; // state sustain = 2.0	
	// at least one channel not yet in sustain? process
	if (!_mm_testz_si128(smask.pi, smask.pi))
	{	
		// decay state? = 1.0
		sample_t dmask = n->v[0] == SC[S_1_0];
#ifndef ADSR_SKIP_EXP
		if (mode & ADSR_EXP)
		{
			n->v[5] = ADSR_IFTHEN(dmask, n->v[5]*n->v[4], n->v[5]);
			n->v[1] = ADSR_IFTHEN(dmask, s_lerp(sustainlevel, SC[S_1_0], n->v[5]), n->v[1]);
		}
		else
#endif
		{
			n->v[1] = ADSR_IFTHEN(dmask, s_max(n->v[1] - n->v[2], sustainlevel), n->v[1]);
		}

		dmask = (n->v[1] <= (sustainlevel+SC[ADSR_ZERO])) & dmask;	
		// set sustain state when needed (2.0)
		n->v[0] = ADSR_IFTHEN(dmask, SC[S_2_0], n->v[0]);
	
		// attack state? = 0.5
		sample_t amask = n->v[0] == SC[S_0_5];
		n->v[1] = ADSR_IFTHEN(amask, s_min(n->v[1] + n->v[2], SC[S_1_0]), n->v[1]);
		amask = (n->v[1] == SC[S_1_0]) & amask;
		// at least one channel in attack and above 1.0 boundary? switch to decay
		if (!_mm_testz_si128(amask.pi, amask.pi))
		{
			// set decay state (1.0)
			n->v[0] = ADSR_IFTHEN(amask, SC[S_1_0], n->v[0]); // set state decay
			// calculate current decay rate
			sample_t rr = ADSR_Map(INP(ADSR_DECAY));
			// linear delta
			n->v[2] = ADSR_IFTHEN(amask, rr*(SC[S_1_0] - sustainlevel), n->v[2]);
#ifndef ADSR_SKIP_EXP
			// exp factor
			n->v[4] = ADSR_IFTHEN(amask, s_exp2(rr*SC[ADSR_LOG2]), n->v[4]);
			// exp runner 
			n->v[5] = ADSR_IFTHEN(amask, SC[S_1_0], n->v[5]);
#endif
		}
	}
	// both channels in sustain
	else
		n->v[1] = sustainlevel;

	// adsr event signal (gate): true when in attack mode or level above zero
	n->e = (n->v[0] == SC[S_0_5]) | (n->v[1] > SC[ADSR_ZERO]);

	// multipy level with gain
	n->out = n->v[1] * n->v[8];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline sample_t TriSaw(sample_t& phase, sample_t& color)
{
	sample_t check = phase > color;
	return s_ifthen(check, SC[S_1_0] - phase, phase) / s_ifthen(check, SC[S_1_0] - color, color) * SC[S_2_0] - SC[S_1_0];
}

inline sample_t Pulse(sample_t& phase, sample_t& color, sample_t& slope)
{
	sample_t islope = SC[S_1_0]/slope;
	return s_ifthen(phase > color,
				s_max((color - phase)	* SC[S_2_0] * islope + SC[S_1_0], SC[S_M1_0]),
				s_min(phase	* SC[S_2_0] * islope - SC[S_1_0], SC[S_1_0]))
		+ (SC[S_1_0] - color * SC[S_2_0]); // dc offset compensation;
}

#if !((defined LFO_SKIP) && (defined OSCILLATOR_SKIP)) 
// common for lfo and oscillator
#ifdef CODE_SECTIONS
#pragma code_seg(".sn0a")
#endif
sample_t SYNTHCALL Osc_Tick(SynthNode* n, int mode, int oscindex)
{
	sample_t ret;
	sample_t color = INP(LFO_COLOR);
	sample_t phase = INP(LFO_PHASE);
	sample_t* oscwrk = &(n->v[2]);
	if (oscindex > 0)
	{
		oscwrk += oscindex * 4;
		// detune max is about 100cent when all additional 6 unison voices are active
		n->v[0] *= SC[S_1_0]+(SC[S_0_01] * INP(OSCILLATOR_UDETUNE));
	}

	// add phase offset to phase and wrap
	sample_t wrkphase = oscwrk[0] + phase;
	wrkphase -= s_floor(wrkphase);
	// old workphase > current workphase? signal a phase wrap
	n->e = oscwrk[1] > wrkphase;
	oscwrk[1] = wrkphase;
	// phase integration with frequency modulation (wrapping is done in the update part of lfo and osc)
	sample_t wrkfreq = n->v[0] + n->v[1];
	oscwrk[0] += wrkfreq;

	// bandlimited color in n[4], corner [0,1] has to have an offset of at least the current phase integration size.
	// test with double factor: wrkfreq *= SC[S_2_0];
	sample_t blcolor = s_clamp(color, SC[S_1_0] - wrkfreq, wrkfreq);

	switch (mode & LFO_WAVEMASK)
	{
		// sine/peak, color defines sine percentage. color==0 minimal peak, color==1 pure sine
#ifndef WAVE_SKIP_SINE
		case LFO_SINE:
		{

			sample_t tp = s_clamp(color, SC[S_1_0], wrkfreq); 
			ret = s_ifthen(wrkphase < tp, s_sin(wrkphase / tp*SC[S_2PI]), sample_t::zero());
			break;
		}
#endif
		// pure saw, color defines direction up/down. color==0 saw down, color==1 saw up
#ifndef WAVE_SKIP_SAW
		case LFO_SAW:
		{
			sample_t tout = wrkphase+wrkphase - SC[S_1_0];
			ret = s_ifthen(blcolor >= SC[S_0_5], tout, s_neg(tout));
			break;
		}
#endif
		// square/pulse, color defines up cycle length. color==0 pulse with minimal up, color==0.5 square, color==1 pule with minimal down. !! introduces DC offset !!
#ifndef WAVE_SKIP_PULSE
		case LFO_PULSE:
		{
			ret = s_ifthen(wrkphase < blcolor, SC[S_1_0], SC[S_M1_0]);
			break;
		}
#endif
		// bandlimited triangle/saw, color defines breakpoint up/down. color==0 saw down, color==0.5 triangle, color==1 saw up
#ifndef WAVE_SKIP_AATRISAW
		case LFO_AATRISAW:
		{
			ret = TriSaw(wrkphase, blcolor);
			break;
		}
#endif
		// bandlimited square/pulse, color defines up cycle length. color==0 pulse with minimal up, color==0.5 square, color==1 pule with minimal down.
#ifndef WAVE_SKIP_AAPULSE
		case LFO_AAPULSE:
		{
			ret = Pulse(wrkphase, blcolor, wrkfreq);
			break;
		}
#endif
		// bandlimited bi trisaw (like in vanguard, only color is modulatabel here), color defines breakpoint up/down. color==0 saw down, color==0.5 triangle, color==1 saw up
#ifndef WAVE_SKIP_AABITRISAW
		case LFO_AABITRISAW:
		{
			// first trisaw with phase offset
			ret = TriSaw(wrkphase, blcolor);
			// second trisaw without phase offset
			sample_t tp = (wrkphase - phase);
			tp -= s_floor(tp);
			ret += TriSaw(tp, blcolor);
			break;
		}
#endif
		// bandlimited bi pulse (like in vanguard, only color is modulatabel here), color defines up cycle length. color==0 pulse with minimal up, color==0.5 square, color==1 pule with minimal down.
#ifndef WAVE_SKIP_AABIPULSE
		case LFO_AABIPULSE:
		{
			// first pulse with phase offset
			ret = Pulse(wrkphase, blcolor, wrkfreq);
			// second pulse without phase offset
			sample_t tp = (wrkphase - phase);
			tp -= s_floor(tp);
			ret += Pulse(tp, blcolor, wrkfreq);
			break;
		}
#endif
		// random pitched sine playing one cycle of the base frequency with a random pitch (a multiple of the base frequency), color defines how high the pitch range can go
#ifndef WAVE_SKIP_RPSINE
		case LFO_RPSINE:
		{
			ret = s_sin(wrkphase*(oscwrk[2]+SC[S_1_0])*SC[S_2PI]);
			if (!_mm_testz_si128(n->e.pi, n->e.pi))
			{				
				oscwrk[2] = s_ifthen(n->e, s_floor(color*color*SC[S_440_0]*(s_rand()+SC[S_1_0])), oscwrk[2]);
			}
			break;
		}
#endif
		// pitched noise using cosine interpolation of random values per cycle, color defines the fraction of the cycle to use for the interpolation
#ifndef WAVE_SKIP_PNOISE
		case LFO_PNOISE:
		{
			// at least one channel is wrapped?
			if (!_mm_testz_si128(n->e.pi, n->e.pi))
			{
				oscwrk[2] = s_ifthen(n->e, oscwrk[3], oscwrk[2]);
				oscwrk[3] = s_ifthen(n->e, s_rand(), oscwrk[3]);
			}
			// cosine interpolation between the two random points
			sample_t tout = s_lerp(oscwrk[2], oscwrk[3], (SC[S_1_0] - s_cos(wrkphase*SC[S_PI]))*SC[S_0_5]);
			ret = s_ifthen(wrkphase < color, tout, oscwrk[3]);
			break;
		}
#endif
		// bandlimited trisaw sequence (like saw i/o in vanguard, only the color is modulateable here.)
#ifndef WAVE_SKIP_AATRISAWSEQ
		case LFO_AATRISAWSEQ:
		{
			// saw i/o
			sample_t factor = s_ifthen(wrkphase > SC[S_0_5], s_ifthen(wrkphase > SC[S_0_75], SC[S_8_0], SC[S_4_0]), SC[S_2_0]);
			// TODO: check if color adjusting is needed based on the frequency factor
			/*wrkfreq *= factor;
			blcolor = s_clamp(blcolor, SC[S_1_0] - wrkfreq, wrkfreq);*/
			wrkphase *= factor;
			wrkphase -= s_floor(wrkphase);			
			ret = TriSaw(wrkphase, blcolor); 			
			break;
		}
#endif
#ifndef WAVE_SKIP_SINESEQ
		case LFO_SINESEQ:
		{
			sample_t tp = s_clamp(phase, SC[S_1_0], wrkfreq);
			ret = s_ifthen(wrkphase < tp, s_sin(wrkphase*(oscwrk[2] + SC[S_1_0]) / tp*SC[S_2PI]), sample_t::zero());
			if (!_mm_testz_si128(n->e.pi, n->e.pi))
			{

				oscwrk[2] += s_ifthen(n->e, SC[S_1_0], sample_t::zero());
				oscwrk[2] = s_ifthen(oscwrk[2] < color*SC[S_32_0], oscwrk[2], sample_t::zero());
			}
			break;
		}
#endif
#ifdef COMPILE_VSTI
		case LFO_TEST:
		{
			sample_t tp = s_clamp(phase, SC[S_1_0], wrkfreq);
			ret = s_ifthen(wrkphase < tp, s_sin(wrkphase*(oscwrk[2] + SC[S_1_0]) / tp*SC[S_2PI]), sample_t::zero());
			if (!_mm_testz_si128(n->e.pi, n->e.pi))
			{

				oscwrk[2] += s_ifthen(n->e, SC[S_1_0], sample_t::zero());
				oscwrk[2] = s_ifthen(oscwrk[2] < color*SC[S_32_0], oscwrk[2], sample_t::zero());
			}
			break;
		}
#endif
	}
	return ret;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LFO_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn0c")
#endif
void SYNTHCALL LFO_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(LFO_FREQ);
	NODE_CALL_INPUT(LFO_PHASE);
	NODE_CALL_INPUT(LFO_COLOR);	
	NODE_CALL_INPUT(LFO_GAIN);
	//NODE_CALL_INPUT(LFO_MODE); // mode is always a constant

#ifdef COMPILE_VSTI
	int mode = n->input[LFO_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif		
	
	// update values
	NODE_UPDATE_BEGIN
		
		if (mode & LFO_BPMSYNC)
		{
			n->v[0] = SynthGlobalState.CurrentBPM * s_exp2((INP(LFO_FREQ)-SC[S_0_5])*SC[S_128_0]*SC[S_1_O_12]) / (SC[S_60_0] * SC[S_SAMPLERATE]);
		}
		else
		{
			// frequency = YToTheX(freq*(128.0f/60.0f), 5.0f)*0.0000226757369614f;
			sample_t temp = s_max(INP(LFO_FREQ), sample_t::zero());
			n->v[0] = temp*temp*temp*temp*temp*SC[S_0_001];
		}		
			
#ifndef WAVE_SKIP_PNOISE
		// init pnoise random
		if (_mm_testz_si128(n->v[31].pi, n->v[31].pi))
		{
			n->v[31] = SC[S_1_0];
			n->v[5] = s_rand();
		}
#endif
		
	NODE_UPDATE_END

	// process
	n->out = Osc_Tick(n, mode, 0);
	
#ifndef LFO_SKIP_POLARITY
	// polarity
	if ((mode & LFO_BIPOLAR) == 0)
	{
		if (mode & LFO_UNIPOLAR_N)
			n->out = (n->out + SC[S_M1_0]) * SC[S_0_5];
		else
			n->out = (n->out + SC[S_1_0]) * SC[S_0_5];		
	}
#endif

	// multipy gain
	n->out *= INP(LFO_GAIN);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef OSCILLATOR_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn0d")
#endif
void SYNTHCALL OSCILLATOR_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

#ifndef OSCILLATOR_SKIP_FREQUENCY
	NODE_CALL_INPUT(OSCILLATOR_FREQ);
#endif
	NODE_CALL_INPUT(OSCILLATOR_PHASE);
	NODE_CALL_INPUT(OSCILLATOR_COLOR);	
	NODE_CALL_INPUT(OSCILLATOR_TRANSPOSE);
	NODE_CALL_INPUT(OSCILLATOR_DETUNE);	
	NODE_CALL_INPUT(OSCILLATOR_GAIN);
	NODE_CALL_INPUT(OSCILLATOR_UDETUNE);	
	//NODE_CALL_INPUT(OSCILLATOR_MODE); // mode is always a constant
	
#ifdef COMPILE_VSTI
	int mode = n->input[OSCILLATOR_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif				

#ifndef OSCILLATOR_SKIP_FREQUENCY
	// FM (max = half samplerate)
	n->v[1] = INP(OSCILLATOR_FREQ)*SC[S_0_5]; 
#endif

	// update values
	NODE_UPDATE_BEGIN
				
		//  voice frequency mapping , just multiply incoming frequency by 2 to get in range to 22050hz
		if (mode & OSCILLATOR_VOICEFREQ)
		{
			n->v[0] = SynthGlobalState.CurrentVoice->Frequency * s_exp2((INP(OSCILLATOR_TRANSPOSE)+INP(OSCILLATOR_DETUNE)*SC[S_0_01])*SC[S_128_0]*SC[S_1_O_12]);
		}
#ifndef OSCILLATOR_SKIP_FREQUENCY
		//  directly using freq as input
		else
		{
			n->v[0] = sample_t::zero();
		}
#endif
		// first time init
		if (_mm_testz_si128(n->v[31].pi, n->v[31].pi))
		{
			n->v[31] = SC[S_1_0];			
			// random start phase
			if (mode & LFO_RANDPHASE)
			{
				sample_t* oscwrk = &(n->v[2]);
				int numvoice = 7;
				while (numvoice--)
				{
					oscwrk[3] = s_rand();			// random target value for pnoise
					oscwrk[0] = s_abs(oscwrk[3]);	// random phase								
					oscwrk += 4;
				}
			}
		}

	NODE_UPDATE_END

	// process

	// loop as many additional voices as requested
	n->out = sample_t::zero();
	sample_t backup = n->v[0];
	int numvoices = (mode & OSCILLATOR_LOOPMASK) >> OSCILLATOR_LOOPSHIFT;
	int voice = 0;
	while (voice <= numvoices)
	{ 
		n->out += Osc_Tick(n, mode, voice++);
	};
	n->v[0] = backup;

	// multipy gain
	n->out *= INP(OSCILLATOR_GAIN);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NOISEGEN_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn0e")
#endif
void SYNTHCALL NOISEGEN_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(NOISEGEN_MIX);
	NODE_CALL_INPUT(NOISEGEN_GAIN);

	// white noise
	sample_t white = s_rand();

	// no mix in both channels? just white and done
	sample_t mix = INP(NOISEGEN_MIX);
	if (_mm_testz_si128(mix.pi, mix.pi))
	{
		n->out = white;
	}
	else
	{
		// pink noise filter
		n->v[0] = n->v[0] * SC[NOISEGEN_B0] + white * SC[NOISEGEN_W0];
		n->v[1] = n->v[1] * SC[NOISEGEN_B1] + white * SC[NOISEGEN_W1];
		n->v[2] = n->v[2] * SC[NOISEGEN_B2] + white * SC[NOISEGEN_W2];			
		sample_t pink = s_clamp((n->v[0] + n->v[1] + n->v[2] + white * SC[NOISEGEN_W3]) * SC[S_0_2], SC[S_1_0], SC[S_M1_0]);		

#ifndef NOISEGEN_SKIP_BROWN
		// brown noise loop
		sample_t lastRand(white);
		sample_t mask(SC[S_ALLBITS].pi);
		sample_t brown;		
		while (true)
		{
			lastRand *= SC[S_0_125] * SC[S_0_5];
			n->v[3] += lastRand & mask;
			// get mask
			mask = (s_abs(n->v[3]) > SC[S_1_0]);
			// leave loop as soon as both signals have magnitude below 1
			if (_mm_testz_si128(mask.pi, mask.pi))
			{
				brown = n->v[3];
				break;
			}
			n->v[3] -= lastRand & mask;
			lastRand = s_rand();
		}

		// mix according to factor		
		mix += mix;
		n->out = s_ifthen(mix > SC[S_1_0], s_lerp(pink, brown, mix-SC[S_1_0]), s_lerp(white, pink, mix));
#else
		n->out = s_lerp(white, pink, mix);
#endif
	}

	n->out *= INP(NOISEGEN_GAIN);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// common for biquad, onepole and onezero
#define FS(x) ((BQFilterState*)(x->v))

// common for nodes that need to map input range 0 .. 1 to frequency up to 22050hz
#ifdef CODE_SECTIONS
#pragma code_seg(".sn0f")
#endif
inline sample_t SYNTHCALL Frequency_Map(const sample_t& in, int mode)
{
	sample_t ret;
	//  voice frequency mapping , just multiply incoming frequency by 2 to get in range to 22050hz
	if (mode & BQFILTER_NOTEMAP)
	{
		ret = SynthGlobalState.CurrentVoice->Frequency*SC[S_2_0];
	}
	// custom frequency
	else
	{
		// quadratic mapping
		if (mode & BQFILTER_QUADRATIC)
			ret = s_clamp(in*in, SC[NOISEGEN_B0], sample_t::zero());
#ifndef FREQUENCY_MAP_SKIP
		//  log mapping up to ~22050hz 
		else
			ret = s_min(SC[S_1_0] / s_exp2((SC[S_1_0] - in) * SC[S_10_0]), SC[NOISEGEN_B0]);
#endif
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BQFILTER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn10")
#endif
void SYNTHCALL BQFILTER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(BQFILTER_IN);	
	NODE_CALL_INPUT(BQFILTER_FREQ);
	NODE_CALL_INPUT(BQFILTER_Q);
	NODE_CALL_INPUT(BQFILTER_DBGAIN);
	//NODE_CALL_INPUT(BQFILTER_MODE); // mode is always a constant
		
#ifdef COMPILE_VSTI
	int mode = n->input[BQFILTER_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN
		
		sample_t temp = Frequency_Map(INP(BQFILTER_FREQ), mode);		
		
		sample_t w = SC[S_PI]*temp;
		sample_t sw = s_sin(w);
		sample_t cw = s_cos(w);
		sample_t q = s_exp2(INP(BQFILTER_Q)*SC[S_10_0]);
		sample_t alpha = sw/(q+q);
		sample_t beta, A;

		// components are same for all filter types
		if ((mode & BQFILTER_MODEMASK) < BQFILTER_PEAK)
		{			
			FS(n)->a0 = SC[S_1_0] + alpha;
			FS(n)->a2 = SC[S_1_0] - alpha;
			FS(n)->a1 = s_neg(cw+cw);
		}
#if !((defined BQFILTER_SKIP_PEAK) && (defined BQFILTER_SKIP_LOWSHELF) && (defined BQFILTER_SKIPHIGHSHELF))
		// peak + shelving
		else if ((mode & BQFILTER_MODEMASK) < BQFILTER_RESONANCE)
		{
			A = s_db2lin(SC[S_0_5] * (INP(BQFILTER_DBGAIN) - SC[S_0_75]) * SC[S_128_0]);
			beta = s_sqrt(A)*sw/q;
		}
#endif
#if !((defined BQFILTER_SKIP_RESONANCE) && (defined BQFILTER_SKIP_RESONANCENORM))
		// resonators
		else
		{
			q = s_min(INP(BQFILTER_Q), SC[CHANNELROOT_EFC]);
			FS(n)->a0 = SC[S_1_0];
			FS(n)->a1 = q*s_neg(cw+cw);
			FS(n)->a2 = q*q;				
			FS(n)->b1 = sample_t::zero();	
		}
#endif

		// caclculate b components according to filter mode
		switch (mode & BQFILTER_MODEMASK)
		{
#ifndef BQFILTER_SKIP_LOWPASS
			case BQFILTER_LOWPASS:	
			{
				FS(n)->b0 = FS(n)->b2 = (SC[S_1_0] - cw)*SC[S_0_5];
				FS(n)->b1 =  SC[S_1_0] - cw;
				break;
			}
#endif
#ifndef BQFILTER_SKIP_HIGHPASS
			case BQFILTER_HIGHPASS:	
			{
				FS(n)->b0 = FS(n)->b2 = (SC[S_1_0] + cw)*SC[S_0_5];
				FS(n)->b1 =	s_neg(SC[S_1_0] + cw);	            
				break;
			}
#endif
#ifndef BQFILTER_SKIP_BANDPASS
			case BQFILTER_BANDPASSQ:	
			{
				FS(n)->b0 = sw*SC[S_0_5];		        
				FS(n)->b2 = s_neg(FS(n)->b0);
				FS(n)->b1 = sample_t::zero();
				break;
			}
#endif
#ifndef BQFILTER_SKIP_BANDPASSBW
			case BQFILTER_BANDPASSBW:	
			{
				FS(n)->b0 = alpha;				
				FS(n)->b2 = s_neg(FS(n)->b0);
				FS(n)->b1 = sample_t::zero();
				break;
			}
#endif
#ifndef BQFILTER_SKIP_NOTCH
			case BQFILTER_NOTCH:	
			{
				FS(n)->b0 = FS(n)->b2 = SC[S_1_0];				
				FS(n)->b1 = FS(n)->a1;
				break;
			}
#endif
#ifndef BQFILTER_SKIP_ALLPASS
			case BQFILTER_ALLPASS:	
			{
				FS(n)->b0 = FS(n)->a2;				
				FS(n)->b2 = FS(n)->a0;
				FS(n)->b1 = FS(n)->a1;
				break;
			}
#endif
#ifndef BQFILTER_SKIP_PEAK
			case BQFILTER_PEAK:	
			{
				FS(n)->b0 = SC[S_1_0] + alpha*A;			
				FS(n)->b2 = SC[S_1_0] - alpha*A;
				FS(n)->a0 = SC[S_1_0] + alpha/A;
				FS(n)->a2 = SC[S_1_0] - alpha/A;
				FS(n)->b1 = FS(n)->a1 = s_neg(cw+cw);
				break;
			}
#endif
#ifndef BQFILTER_SKIP_LOWSHELF
			case BQFILTER_LOWSHELF:
			{
				cw = s_neg(cw);
				sample_t ap1 = A+SC[S_1_0];
				sample_t am1 = A-SC[S_1_0];
				FS(n)->b0 =			 A*(ap1 + am1*cw + beta );				
				FS(n)->b2 =			 A*(ap1 + am1*cw - beta );
				FS(n)->a0 =				ap1 - am1*cw + beta;
				FS(n)->a2 =				ap1 - am1*cw - beta;	
				FS(n)->b1 =			A*(am1 + ap1*cw        )*SC[S_2_0];
				FS(n)->a1 =		s_neg((am1 - ap1*cw        )*SC[S_2_0]);
				break;
			}
#endif
#ifndef BQFILTER_SKIP_HIGHSHELF
			case BQFILTER_HIGHSHELF:	
			{
				sample_t ap1 = A+SC[S_1_0];
				sample_t am1 = A-SC[S_1_0];
				FS(n)->b0 =			 A*(ap1 + am1*cw + beta );				
				FS(n)->b2 =			 A*(ap1 + am1*cw - beta );
				FS(n)->a0 =				ap1 - am1*cw + beta;
				FS(n)->a2 =				ap1 - am1*cw - beta;	
				FS(n)->b1 =  s_neg(A*(am1 + ap1*cw        )*SC[S_2_0]);
				FS(n)->a1 =			 (am1 - ap1*cw        )*SC[S_2_0];				
				break;
			}
#endif
#ifndef BQFILTER_SKIP_RESONANCE
			case BQFILTER_RESONANCE:	
			{
				FS(n)->b0 = SC[S_1_0];
				FS(n)->b2 = sample_t::zero();				
				break;
			}
#endif
#ifndef BQFILTER_SKIP_RESONANCENORM
			case BQFILTER_RESONANCENORM:	
			{
				FS(n)->b0 = SC[S_0_5] - SC[S_0_5]*FS(n)->a2;
				FS(n)->b2 = s_neg(FS(n)->b0);				
				break;
			}
#endif
		}

		// normalize coefficients
		sample_t ia0 = SC[S_1_0] / FS(n)->a0;
		FS(n)->b0 *= ia0;
		FS(n)->b1 *= ia0;
		FS(n)->b2 *= ia0;
		FS(n)->a1 *= ia0;
		FS(n)->a2 *= ia0;
	
	NODE_UPDATE_END

	// process
	sample_t* inout = FS(n)->inout;
	sample_t tin = INP(0);
	sample_t tout;

	int loop = 0;
	int stages = ((mode & BQFILTER_STAGEMASK) >> (BQFILTER_STAGESHIFT-1)); // 0, 2, 4, 8
	do 
	{	
#if 1
		// process filter (direct form 2 transposed), numerically more stable and needs only 2 memory slots instead of 4
		tout = FS(n)->b0 * tin + inout[0];
		inout[0] = FS(n)->b1 * tin - FS(n)->a1 * tout + inout[1];
		inout[1] = FS(n)->b2 * tin - FS(n)->a2 * tout;
		// next stage work vars
		inout += 2;
		tin = tout;		
#else
		// process filter (direct form 2)
		tout =	FS(n)->b0*tin + FS(n)->b1 * inout[0] + FS(n)->b2 * inout[1]	- 
								FS(n)->a1 * inout[2] - FS(n)->a2 * inout[3];
		inout[1] = inout[0];
		inout[0] = tin;
		inout[3] = inout[2];
		inout[2] = tout;		
		// next stage work vars
		tin = tout;
		inout += 4;
#endif
	} while (++loop < stages);

	n->out = tout;
}
#endif
#undef FS

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ONEPOLEFILTER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn11")
#endif
void SYNTHCALL ONEPOLEFILTER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(ONEPOLEFILTER_IN);
	NODE_CALL_INPUT(ONEPOLEFILTER_POLE);

	// update values
	NODE_UPDATE_BEGIN

		// clamp to -1..1
		sample_t f = s_clamp(INP(ONEPOLEFILTER_POLE), SC[S_1_0], SC[S_M1_0]);		
		// b0 = 1-abs(pole)
		n->v[0] = SC[S_1_0] - s_abs(f);	
		// a1 = -pole (but can keep as it is when we flip the sign below
		n->v[1] = f;//s_neg(f);

	NODE_UPDATE_END

	// process
	n->out = n->v[0]*INP(0) + n->v[1]*n->v[2];
	n->v[2] = n->out;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ONEZEROFILTER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn12")
#endif
void SYNTHCALL ONEZEROFILTER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(ONEZEROFILTER_IN);
	NODE_CALL_INPUT(ONEZEROFILTER_ZERO);

	// update values
	NODE_UPDATE_BEGIN

		// clamp to -1..1
		sample_t f = s_clamp(INP(ONEZEROFILTER_ZERO), SC[S_1_0], SC[S_M1_0]);
		// b0 = 1 / (1+abs(zero))
		n->v[0] = SC[S_1_0] / (SC[S_1_0] + s_abs(f));
		// b1 = -zero*b0
		n->v[1] = s_neg(f) * n->v[0];		

	NODE_UPDATE_END

	// process
	n->out = n->v[0]*INP(0) + n->v[1]*n->v[2];
	n->v[2]  = INP(0);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SHAPER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn13")
#endif
void SYNTHCALL SHAPER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SHAPER_IN);
	NODE_CALL_INPUT(SHAPER_DRIVE);

	// update values
	NODE_UPDATE_BEGIN

		// clamp to -1..1 (a little less)
		sample_t f = s_clamp(INP(SHAPER_DRIVE), SC[NOISEGEN_B0], s_neg(SC[NOISEGEN_B0]));
		// k = 2*amount/(1-amount);
		n->v[0] = (f+f)/(SC[S_1_0]-f);
	
	NODE_UPDATE_END

	// process f(x) = (1+k)*x/(1+k*abs(x))
	n->out = (SC[S_1_0]+n->v[0])*INP(SHAPER_IN)/(SC[S_1_0]+n->v[0]*s_min(s_abs(INP(SHAPER_IN)), SC[S_1_0]));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PANNING_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn14")
#endif
void SYNTHCALL PANNING_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(PANNING_IN);
	NODE_CALL_INPUT(PANNING_PAN);

	// update values
	NODE_UPDATE_BEGIN

		sample_t r = s_clamp(INP(PANNING_PAN), SC[S_1_0], sample_t::zero());
		sample_t l = sample_t(SC[S_1_0] - r);
		n->v[0] = s_sqrt(s_shuffle(l, r));
	
	NODE_UPDATE_END

	// process
	n->out = INP(PANNING_IN) * n->v[0];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sn15")
#endif
void SYNTHCALL MUL_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(MUL_IN1);
	NODE_CALL_INPUT(MUL_IN2);
	
	// process
	n->out = INP(MUL_IN1) * INP(MUL_IN2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sn16")
#endif
void SYNTHCALL ADD_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(ADD_IN1);
	NODE_CALL_INPUT(ADD_IN2);
	
	// process
	n->out = INP(ADD_IN1) + INP(ADD_IN2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SUB_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn17")
#endif
void SYNTHCALL SUB_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SUB_IN1);
	NODE_CALL_INPUT(SUB_IN2);
	
	// process
	n->out = INP(SUB_IN1) - INP(SUB_IN2);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MIN_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn18")
#endif
void SYNTHCALL MIN_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(MIN_IN1);
	NODE_CALL_INPUT(MIN_IN2);
	
	// process
	n->out = s_min(INP(MIN_IN1), INP(MIN_IN2));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MAX_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn19")
#endif
void SYNTHCALL MAX_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(MAX_IN1);
	NODE_CALL_INPUT(MAX_IN2);
	
	// process
	n->out = s_max(INP(MAX_IN1), INP(MAX_IN2));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ABS_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn1a")
#endif
void SYNTHCALL ABS_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(ABS_IN);
	
	// process
	n->out = s_abs(INP(ABS_IN));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NEG_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn1b")
#endif
void SYNTHCALL NEG_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(NEG_IN);
	
	// process
	n->out = s_neg(INP(NEG_IN));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SQRT_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn1c")
#endif
void SYNTHCALL SQRT_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SQRT_IN);
	
	// process
	n->out = s_sqrt(s_max(INP(SQRT_IN), sample_t::zero()));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DIV_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn1d")
#endif
void SYNTHCALL DIV_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(DIV_IN1);
	NODE_CALL_INPUT(DIV_IN2);
	
	// process
	n->out = s_ifthen(INP(DIV_IN2) != sample_t::zero(), INP(DIV_IN1) / INP(DIV_IN2), sample_t::zero());
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLIP_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn1e")
#endif
void SYNTHCALL CLIP_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(CLIP_IN);
	NODE_CALL_INPUT(CLIP_LEVEL);

	// process
	n->out = s_clamp(INP(CLIP_IN), s_abs(INP(CLIP_LEVEL)), s_neg(s_abs(INP(CLIP_LEVEL))));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MIX_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn1f")
#endif
void SYNTHCALL MIX_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(MIX_IN1);
	NODE_CALL_INPUT(MIX_IN2);
	NODE_CALL_INPUT(MIX_MIX);

	// process
	n->out = s_lerp(INP(MIX_IN1), INP(MIX_IN2), INP(MIX_MIX));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// function is identical to notecontroller, but needs own id, as notecontroller is a special node at the beginning and must not be merged with multiadd nodes
//#ifndef MULTIADD_SKIP
//void SYNTHCALL MULTIADD_tick(SynthNode* n)
//{
//	NODE_STATEFUL_PROLOG;
//
//	int i = n->numInputs;
//	while (i--)
//		NODE_CALL_INPUT(i);
//
//	// process
//	n->out = sample_t::zero();
//	i = n->numInputs;
//	while (i--)
//		n->out += n->input[i];
//}
//#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MONO_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn20")
#endif
void SYNTHCALL MONO_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(MONO_IN);
	//NODE_CALL_INPUT(MONO_MODE); // mode is always a constant

#ifdef COMPILE_VSTI
	int mode = n->input[MONO_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// process
	if (mode)
		n->out = s_dupright(INP(MONO_IN));
	else
		n->out = s_dupleft(INP(MONO_IN));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ENVFOLLOWER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn21")
#endif
void SYNTHCALL ENVFOLLOWER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(ENVFOLLOWER_IN);
	NODE_CALL_INPUT(ENVFOLLOWER_ATTACK);
	NODE_CALL_INPUT(ENVFOLLOWER_RELEASE);

	// update values
	NODE_UPDATE_BEGIN
	
		// attack rate
		n->v[0] = s_exp( SC[S_LN0_01] / ( s_max(INP(ENVFOLLOWER_ATTACK ), SC[S_0_001]) * SC[ENVFOLLOWER_SCALE] ) );			// 0-128.0ms * 44100.0 * 0.001;
		// release rate
		n->v[1] = s_exp( SC[S_LN0_01] / ( s_max(INP(ENVFOLLOWER_RELEASE), SC[S_0_001]) * SC[ENVFOLLOWER_SCALE] * SC[S_10_0] ) ); // 0-1280.0ms * 44100.0 * 0.001

	NODE_UPDATE_END

	// process
	sample_t v = s_abs(INP(ENVFOLLOWER_IN));
	n->out = (s_ifthen(v > n->out, n->v[0], n->v[1]) * (n->out - v)) + v;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LOGIC_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn22")
#endif
void SYNTHCALL LOGIC_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(LOGIC_IN1);
	NODE_CALL_INPUT(LOGIC_IN2);
	//NODE_CALL_INPUT(LOGIC_MODE); // mode is always a constant
		
#ifdef COMPILE_VSTI
	int mode = n->input[LOGIC_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// process
	sample_t a = INP(LOGIC_IN1) >= SC[S_0_5];
	sample_t b = INP(LOGIC_IN2) >= SC[S_0_5];
	sample_t mask;
	switch (mode & 0xffff)
	{
		case LOGIC_AND:	
		{				
			mask = a & b;
			break;
		}
		case LOGIC_OR:	
		{				
			mask = a | b;
			break;
		}
		case LOGIC_XOR:	
		{				
			mask = a ^ b;
			break;
		}
		case LOGIC_NOT:	
		{				
			mask = !a;
			break;
		}
	}	
	n->out = n->e = s_ifthen(mask, SC[S_1_0], sample_t::zero());
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef COMPARE_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn23")
#endif
void SYNTHCALL COMPARE_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(COMPARE_IN1);
	NODE_CALL_INPUT(COMPARE_IN2);
	//NODE_CALL_INPUT(COMPARE_MODE); // mode is always a constant
		
#ifdef COMPILE_VSTI
	int mode = n->input[COMPARE_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// process
	sample_t a = INP(COMPARE_IN1);
	sample_t b = INP(COMPARE_IN2);
	sample_t mask;
	switch (mode & 0xffff)
	{
		case COMPARE_GT:	
		{				
			mask = a > b;
			break;
		}
		case COMPARE_GTE:	
		{				
			mask = a >= b;
			break;
		}
		case COMPARE_E:	
		{				
			mask = a == b;
			break;
		}
		case COMPARE_NE:	
		{				
			mask = a != b;
			break;
		}
	}
	n->out = n->e = s_ifthen(mask, SC[S_1_0], sample_t::zero());
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SELECT_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn24")
#endif
void SYNTHCALL SELECT_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SELECT_IN1);
	NODE_CALL_INPUT(SELECT_IN2);
	NODE_CALL_INPUT(SELECT_CONDITION);

	// process
	n->out = n->e = s_ifthen(INP(SELECT_CONDITION) >= SC[S_0_5], INP(SELECT_IN2), INP(SELECT_IN1));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef EVENTSIGNAL_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn25")
#endif
void SYNTHCALL EVENTSIGNAL_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(EVENTSIGNAL_IN);

	// process

	// get event signal from input node and set output signal 1 when != 0 
	n->out = n->e = (((SynthNode*)(n->input[EVENTSIGNAL_IN]))->e != sample_t::zero()) & SC[S_1_0];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PROCESS_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn26")
#endif
void SYNTHCALL PROCESS_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	//NODE_CALL_INPUT(PROCESS_IN);
	NODE_CALL_INPUT(PROCESS_CONDITION);

	// process
	sample_t pmask = INP(PROCESS_CONDITION) >= SC[S_0_5];
	// at least one channel is to be processed
	if (!_mm_testz_si128(pmask.pi, pmask.pi))
	{
		NODE_CALL_INPUT(PROCESS_IN);
		n->out = INP(PROCESS_IN);
	}
	else
	{
		n->out = sample_t::zero();
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MIDISIGNAL_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn27")
#endif
void SYNTHCALL MIDISIGNAL_tick(SynthNode* n)
{
	NODE_STATELESS_PROLOG;

	//NODE_CALL_INPUT(MIDISIGNAL_SCALE);
	//NODE_CALL_INPUT(MIDISIGNAL_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[MIDISIGNAL_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	n->out = SynthGlobalState.MidiSignals[mode&0xffff] * INP(MIDISIGNAL_SCALE);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DISTORTION_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn28")
#endif
void SYNTHCALL DISTORTION_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(DISTORTION_IN);
	NODE_CALL_INPUT(DISTORTION_DRIVE);
	NODE_CALL_INPUT(DISTORTION_THRESHOLD);
	//NODE_CALL_INPUT(DISTORTION_MODE);
		
#ifdef COMPILE_VSTI
	int mode = n->input[DISTORTION_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN
		
		sample_t thresh = s_db2lin((INP(DISTORTION_THRESHOLD) - SC[S_0_75]) * SC[S_128_0]);
		n->v[2] = SC[S_1_0] / thresh;
		n->v[3] = thresh;

		sample_t temp = s_max(INP(DISTORTION_DRIVE), sample_t::zero());		
		switch (mode & 0xffff)
		{
#ifndef DISTORTION_SKIP_OVERDRIVE
			case DISTORTION_OVERDRIVE:	
			{				
				// real atan
				/*temp *= temp;
				n->v[0] = s_exp10(temp + temp + temp) - SC[CHANNELROOT_EFC];
				n->v[1] = SC[S_1_0] / s_atan(n->v[0]);			*/
				
				// fake atan, same formula as shaper, saves us the atan implementation and constants
				temp = s_sqrt(s_min(temp, SC[NOISEGEN_B0]));
				n->v[0] = (temp+temp)/(SC[S_1_0]-temp);
				break;
			}
#endif
#ifndef DISTORTION_SKIP_DAMP
			case DISTORTION_DAMP:
			{
				temp = s_neg(s_min(temp, SC[NOISEGEN_B0]));
				n->v[0] = (temp+temp)/(SC[S_1_0]-temp);
				break;
			}
#endif
#ifndef DISTORTION_SKIP_QUANT
			case DISTORTION_QUANT:	
			{				
				n->v[0] = temp * temp + SC[CHANNELROOT_EFT];
				break;
			}
#endif
#ifndef DISTORTION_SKIP_LIMIT
			case DISTORTION_LIMIT:	
			{				
				n->v[0] = s_exp2(s_neg(temp*temp*SC[S_8_0]));
				break;
			}
#endif
#ifndef DISTORTION_SKIP_ASYM
			case DISTORTION_ASYM:	
			{	
				temp *= temp;
				n->v[0] = temp * SC[S_32_0] + SC[CHANNELROOT_EFT];
				n->v[1] = SC[S_1_0] / s_ifthen(n->v[0] < SC[S_1_0], s_sin(n->v[0]) + SC[S_0_1], SC[S_1_0] + SC[S_0_1]);
				break;
			}
#endif
#ifndef DISTORTION_SKIP_SIN
			case DISTORTION_SIN:	
			{				
				n->v[0] = temp * SC[S_32_0] * SC[S_PI];
				break;
			}
#endif
		}

	NODE_UPDATE_END

	// process

	// scale to threshold level
	sample_t dist = INP(DISTORTION_IN) * n->v[2];

	switch (mode & 0xffff)
	{
#ifndef DISTORTION_SKIP_OVERDRIVE
		case DISTORTION_OVERDRIVE:	
		{	
			// real atan
			//n->out = s_atan(dist * n->v[0]) * n->v[1];

			// fake atan, same formula as shaper, saves us the atan implementation and constants
			n->out = (SC[S_1_0] + n->v[0])*dist / (SC[S_1_0] + n->v[0] * s_abs(dist));
			break;
		}
#endif
#ifndef DISTORTION_SKIP_DAMP
		case DISTORTION_DAMP:
		{
			n->out = (SC[S_1_0] + n->v[0])*dist / (SC[S_1_0] + n->v[0] * s_min(s_abs(dist), SC[S_1_0]));
			break;
		}
#endif
#ifndef DISTORTION_SKIP_QUANT
		case DISTORTION_QUANT:	
		{				
			n->out = s_floor(dist / n->v[0] + SC[S_0_5]) * n->v[0];
			break;
		}
#endif
#ifndef DISTORTION_SKIP_LIMIT
		case DISTORTION_LIMIT:	
		{							
			n->out = s_ifthen(s_abs(dist) > n->v[0], s_ifthen(dist > sample_t::zero(), SC[S_1_0], SC[S_M1_0]), dist / n->v[0]);
			break;
		}
#endif
#ifndef DISTORTION_SKIP_ASYM
		case DISTORTION_ASYM:	
		{				
			n->out = s_sin(dist * (SC[S_0_1] + n->v[0] - n->v[0] * dist)) * n->v[1];
			break;
		}
#endif
#ifndef DISTORTION_SKIP_SIN
		case DISTORTION_SIN:	
		{				
			n->out = dist*s_cos(dist * n->v[0]);
			break;
		}
#endif
	}

	// rescale to original level
	n->out *= n->v[3];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CROSSMIX_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn29")
#endif
void SYNTHCALL CROSSMIX_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(CROSSMIX_IN);
	NODE_CALL_INPUT(CROSSMIX_MIX);

	// update values

	// process
	sample_t mix = s_clamp(INP(CROSSMIX_MIX), SC[S_1_0], sample_t::zero());
	n->out = INP(CROSSMIX_IN) + s_shuffle(mix, s_neg(mix))*(s_dupright(INP(CROSSMIX_IN)) - s_dupleft(INP(CROSSMIX_IN)));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sninit")
#endif
void SYNTHCALL DELAY_init(SynthNode* n)
{
	n->customMem = (DWORD*)SynthMalloc(sizeof(sample_t)*(SYNTH_SAMPLERATE)*4);
}

#ifdef CODE_SECTIONS
#pragma code_seg(".sn2b")
#endif
void UpdateDelayParams(SynthNode* n, int allpass, int mode)
{    
	sample_t temp = s_max(INP(DELAY_TIME), sample_t::zero());


	// calculate delay time in samples depending on mode and store in v[1]	
	switch (mode & DELAY_TIMEMASK)
	{
#ifndef DELAY_SKIP_BPMSYNC
		case DELAY_BPMSYNC:
		{
			temp = GetDelayTimeFraction(temp);
			break;
		}
#endif
#ifndef DELAY_SKIP_SHORT
		case DELAY_SHORT:
		{
			// 0..37ms
			temp *= SC[REVERB_MAXCOMBSIZE];
			break;
		}
#endif
#ifndef DELAY_SKIP_MIDDLE
		case DELAY_MIDDLE:
		{
			// 0..128ms
			temp *= SC[ENVFOLLOWER_SCALE];
			break;
		}
#endif
#ifndef DELAY_SKIP_LONG
		case DELAY_LONG:
		{
			// 0..2000ms
			temp *= SC[S_SAMPLERATE] * SC[S_2_0];
			break;
		}
#endif
#ifndef DELAY_SKIP_NOTEMAP
		case DELAY_NOTEMAP:
		{
			temp = (SC[S_1_0]/SynthGlobalState.CurrentVoice->Frequency)*s_exp2((temp-SC[S_0_5])*SC[S_128_0]*SC[S_1_O_12]);
			break;
		}
#endif
#ifndef DELAY_SKIP_NOTEMAP2
		case DELAY_NOTEMAP2:
		{
			temp = (SC[S_1_0]/SynthGlobalState.CurrentVoice->Frequency)*temp;
			break;
		}
#endif
	}

	// maximum delay time
	n->v[10] = SC[S_SAMPLERATE]*SC[S_4_0];
	sample_t readpos = n->v[10] + SC[S_1_0] + n->v[0] - s_clamp(temp, n->v[10]-SC[S_1_0], SC[S_4_0]); // read chases write
	if (allpass)
	{
#ifndef DELAY_SKIP_ALLPASS
		readpos += SC[S_1_0]; // one sample to compensate for allpass introduced delay

		n->v[1] = s_floor(readpos);
		sample_t alpha = SC[S_1_0] + n->v[1] - readpos;
		
		// keep alpha in range 0.5 - 1.5 for maximum flat phase response. need to adjust readoffset then as well.
		sample_t mask = alpha < SC[S_0_5];
		alpha = s_ifthen(mask, alpha + SC[S_1_0], alpha);
		n->v[1] = s_ifthen(mask, n->v[1] + SC[S_1_0], n->v[1]);
		//// v[1] readpointer
		n->v[1] = s_ifthen(n->v[1] >= n->v[10], n->v[1] - n->v[10], n->v[1]);
		// v[2] allpass coeficient
		n->v[2] = (SC[S_1_0]-alpha)/(SC[S_1_0]+alpha);
#endif
	}
	else
	{
#ifndef DELAY_SKIP_LINEAR
		readpos = s_ifthen(readpos >= n->v[10], readpos - n->v[10], readpos);
		// v[1] readpointer
		n->v[1] = s_floor(readpos);
		// v[2] alpha
		n->v[2] = readpos - n->v[1];
#endif
	}	
}

//#ifdef CODE_SECTIONS
//#pragma code_seg(".sn0b")
//#endif
//inline sample_t GetBufferSample(SynthNode* n)
//{
//	// read pointer
//	sample_t bpos = s_toInt(n->v[1]);
//	return sample_t((((sample_t*)n->customMem)[bpos.i[0]]).d[0], (((sample_t*)n->customMem)[bpos.i[1]]).d[1]);
//}

#ifdef CODE_SECTIONS
#pragma code_seg(".sn2c")
#endif
inline void SetBufferSample(SynthNode* n, sample_t& in)
{
	sample_t bpos = s_toInt(n->v[0]);
	(((sample_t*)n->customMem)[bpos.i[0]]).d[0] = in.d[0];
	(((sample_t*)n->customMem)[bpos.i[1]]).d[1] = in.d[1];
	// increase/wrap buffer indices
	n->v[0] += SC[S_1_0];
	n->v[0] = s_ifthen(n->v[0] >= n->v[10], n->v[0] - n->v[10], n->v[0]);
	n->v[1] += SC[S_1_0];	
	n->v[1] = s_ifthen(n->v[1] >= n->v[10], n->v[1] - n->v[10], n->v[1]);
}

#ifndef COMBDELAY_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn2d")
#endif
void SYNTHCALL COMBDELAY_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(COMBDELAY_IN);
	NODE_CALL_INPUT(COMBDELAY_TIME);
	//NODE_CALL_INPUT(COMBDELAY_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[COMBDELAY_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN

		UpdateDelayParams(n, 0, mode);		

	NODE_UPDATE_END

	// process

	sample_t bpos = s_toInt(n->v[1]);
	sample_t inl = sample_t((((sample_t*)n->customMem)[bpos.i[0]]).d[0], (((sample_t*)n->customMem)[bpos.i[1]]).d[1]);
	SetBufferSample(n, INP(DELAY_IN));
	bpos = s_toInt(n->v[1]);
	sample_t inr = sample_t((((sample_t*)n->customMem)[bpos.i[0]]).d[0], (((sample_t*)n->customMem)[bpos.i[1]]).d[1]);

	n->out = s_lerp(inl, inr, n->v[2]);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DELAY_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn2e")
#endif
void SYNTHCALL DELAY_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(DELAY_IN);
	NODE_CALL_INPUT(DELAY_TIME);
	//NODE_CALL_INPUT(DELAY_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[DELAY_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN

		UpdateDelayParams(n, 1, mode);		

	NODE_UPDATE_END

	// process
	
	sample_t bpos = s_toInt(n->v[1]);
	sample_t in = sample_t((((sample_t*)n->customMem)[bpos.i[0]]).d[0], (((sample_t*)n->customMem)[bpos.i[1]]).d[1]);
	SetBufferSample(n, INP(DELAY_IN));

	// allpass filter samples v[4] = apOut, v[3] = apIn, v[2] = apCoeff
	n->v[4] = n->v[3] + (in - n->v[4]) * n->v[2];
	n->v[3] = in;	

	n->out = n->v[4];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FBDELAY_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn2f")
#endif
void SYNTHCALL FBDELAY_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(FBDELAY_IN);
	NODE_CALL_INPUT(FBDELAY_TIME);
	NODE_CALL_INPUT(FBDELAY_FEEDBACK);
	NODE_CALL_INPUT(FBDELAY_DAMP);
	NODE_CALL_INPUT(FBDELAY_MIX);
	//NODE_CALL_INPUT(FBDELAY_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[FBDELAY_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN
	
		UpdateDelayParams(n, 1, mode);		

		n->v[5] = s_clamp(INP(FBDELAY_DAMP), SC[S_1_0], sample_t::zero());
		n->v[7] = s_clamp(INP(FBDELAY_FEEDBACK), SC[NOISEGEN_B0], sample_t::zero());
		n->v[8] = s_clamp(INP(FBDELAY_MIX), SC[S_1_0], sample_t::zero());

	NODE_UPDATE_END

	// process

	sample_t bpos = s_toInt(n->v[1]);
	sample_t in = sample_t((((sample_t*)n->customMem)[bpos.i[0]]).d[0], (((sample_t*)n->customMem)[bpos.i[1]]).d[1]);
	// allpass filter samples v[4] = apOut, v[3] = apIn, v[2] = apCoeff	
	n->v[4] = n->v[3] + (in - n->v[4]) * n->v[2];
	n->v[3] = in;	
	// get damped value for feedback
	in = INP(FBDELAY_IN) + n->v[4]*n->v[7];
	n->v[6] = in + n->v[5]*(n->v[6] - in);
	in = n->v[6];
	// crossmix if needed
	if (mode & DELAY_CROSS)
		in = s_shuffle(in, in);

	SetBufferSample(n, in);	

	n->out = s_lerp(INP(FBDELAY_IN), n->v[4], n->v[8]);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DCFILTER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn30")
#endif
void SYNTHCALL DCFILTER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(DCFILTER_IN);
	NODE_CALL_INPUT(DCFILTER_POLE);

	// process
	n->out = INP(DCFILTER_IN) - n->v[0] + s_min(INP(DCFILTER_POLE), SC[CHANNELROOT_EFC])*n->out;
	n->v[0] = INP(DCFILTER_IN);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef OSRAND_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn31")
#endif
void SYNTHCALL OSRAND_tick(SynthNode* n)
{
	NODE_STATELESS_PROLOG;
	
	// process (once)
	if (_mm_testz_si128(n->out.pi, n->out.pi))
	{
		NODE_CALL_INPUT(OSRAND_SCALE);
		n->out = s_rand() * INP(OSRAND_SCALE);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef REVERB_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sninit")
#endif
void SYNTHCALL REVERB_init(SynthNode* n)
{
	n->customMem = (DWORD*)SynthMalloc(sizeof(sample_t)*1640*12);
}

#ifdef CODE_SECTIONS
#pragma code_seg(".sn33")
#endif
void SYNTHCALL REVERB_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(REVERB_IN)
	NODE_CALL_INPUT(REVERB_GAIN)
	NODE_CALL_INPUT(REVERB_ROOMSIZE)
	NODE_CALL_INPUT(REVERB_DAMP)
	NODE_CALL_INPUT(REVERB_MIX)
	NODE_CALL_INPUT(REVERB_WIDTH)

	// implementation and coefficients taken from
	// https://ccrma.stanford.edu/~jos/pasp/Freeverb.html

	// TODO: check variant from csound
	// needs less delays and basically does some sort of chrorus on each delay line, so changing reverberation delays over time
	/* reverbParams[n][0] = delay time (in seconds)                     */
	/* reverbParams[n][1] = random variation in delay time (in seconds) */
	/* reverbParams[n][2] = random variation frequency (in 1/sec)       */
	/* reverbParams[n][3] = random seed (0 - 32767)                     */
	/*static const double reverbParams[8][4] = {
			{ (2473.0 / DEFAULT_SRATE), 0.0010, 3.100,  1966.0 },
			{ (2767.0 / DEFAULT_SRATE), 0.0011, 3.500, 29491.0 },
			{ (3217.0 / DEFAULT_SRATE), 0.0017, 1.110, 22937.0 },
			{ (3557.0 / DEFAULT_SRATE), 0.0006, 3.973,  9830.0 },
			{ (3907.0 / DEFAULT_SRATE), 0.0010, 2.341, 20643.0 },
			{ (4127.0 / DEFAULT_SRATE), 0.0011, 1.897, 22937.0 },
			{ (2143.0 / DEFAULT_SRATE), 0.0017, 0.891, 29491.0 },
			{ (1933.0 / DEFAULT_SRATE), 0.0006, 3.221, 14417.0 }
	};*/

	sample_t lroffset = sample_t(0.0, 23.0);
	sample_t feedback = s_max(INP(REVERB_ROOMSIZE),  sample_t::zero()) * SC[REVERB_RSF] + SC[REVERB_RSO];

	// a test to provide variable feedback gain for comb delays
	// doesnt make a noticable difference
//#define REVERB_ADJUST_FBGAIN    
#ifdef REVERB_ADJUST_FBGAIN
	// update values
	NODE_UPDATE_BEGIN

		if (!_mm_testz_si128((feedback != n->v[NODE_MAX_WORKVARS-2]).pi, (feedback != n->v[NODE_MAX_WORKVARS-2]).pi))    
		{
			// calculate reverbaration time based on shortest comb delay and the feedback gain (base is the t60 criteria, which is time to reach -60db))
			sample_t t60loops = s_log2(SC[S_0_001]) / s_log2(feedback);
			sample_t t60time = s_dupleft(SC[REVERB_COMB0] * t60loops);

			int i = 8;
			while (i--)
			{
				// number of loops the current combs get
				t60loops = t60time / (SC[REVERB_COMB0 + i] + lroffset);            
				// the derived feedback gain factors
				n->v[20 + i] = s_pow(SC[S_0_001], SC[S_1_0] / t60loops);
				// shuffle offset
				lroffset = s_shuffle(lroffset, lroffset);
			}

			n->v[NODE_MAX_WORKVARS-2] = feedback;
		}

	NODE_UPDATE_END
#endif    


	// process
	sample_t suminput = (s_dupleft(INP(REVERB_IN)) + s_dupright(INP(REVERB_IN))) * s_max(INP(REVERB_GAIN)*INP(REVERB_GAIN)*INP(REVERB_GAIN), sample_t::zero());
	sample_t damp = s_clamp(INP(REVERB_DAMP), SC[S_1_0], sample_t::zero());
	sample_t sumcomb = sample_t::zero();
	sample_t bufferoffset = sample_t::zero();

	int i = 12;    
	while (i--)
	{
		// get left and right samples from current comb pair    
		sample_t bpos = s_toInt(bufferoffset + n->v[i]);
		sample_t combout = sample_t((((sample_t*)n->customMem)[bpos.i[0]]).d[0], (((sample_t*)n->customMem)[bpos.i[1]]).d[1]);
		sample_t combin;
		// 8 parallel comb delays
		if (i > 3)
		{
			// add comb output to summed output
			sumcomb += combout;
			// get damped value and feed back to comb pair
#ifdef REVERB_ADJUST_FBGAIN            
			combin = suminput + combout * n->v[i+20-4];
#else         
			combin = suminput + combout * feedback;
#endif
			n->v[i+8] = combin + damp*(n->v[i+8] - combin);
			combin = n->v[i+8];
		}
		// 4 sequential allpass delays
		else
		{
			combin = sumcomb + combout*SC[DELAY_APC];			
			sumcomb = combout - combin*SC[DELAY_APC];
		}
		// write back to buffer pair
		(((sample_t*)n->customMem)[bpos.i[0]]).d[0] = combin.d[0];
		(((sample_t*)n->customMem)[bpos.i[1]]).d[1] = combin.d[1];
		// inc and wrap buffer positions (right combs always have 23 samples more than left)
		n->v[i] += SC[S_1_0];
		n->v[i] = s_ifthen(n->v[i] < (SC[REVERB_ALLP0+i] + lroffset), n->v[i], sample_t::zero());
		// increment buffer pointer to next buffer pair
		bufferoffset += SC[REVERB_MAXCOMBSIZE];
		// shuffle offset
		lroffset = s_shuffle(lroffset, lroffset);
	}
	// dc filter (using n->e and memory at unused input slots due to lack of local vars)
	n->e = sumcomb - n->v[NODE_MAX_WORKVARS-1] + SC[DCFILTER_C]*n->e;
	n->v[NODE_MAX_WORKVARS-1] = sumcomb;

	// output width crossmixed dry/wet mix
	sample_t width = s_clamp(INP(REVERB_WIDTH), SC[S_1_0], sample_t::zero());
	sample_t wet1 = (SC[S_1_0]+width)*SC[S_0_5];
	sample_t wet2 = (SC[S_1_0]-width)*SC[S_0_5];
	n->out = s_lerp(INP(REVERB_IN), n->e*wet1 + s_shuffle(n->e, n->e)*wet2, s_clamp(INP(REVERB_MIX), SC[S_1_0], sample_t::zero()));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef EQ_SKIP
float EQ_Coeff[10][3] = 
{
#if 0
	/* 16k Hz*/
	{ 0.24198119086616821f, 0.37900940456691590f, -0.80845085113149739f },
	/* 8k Hz*/
	{ 0.44858358977228030f, 0.27570820511385985f, 0.60517475334083082f },
	/* 4k Hz*/
	{ 0.66840529852229358f, 0.16579735073885321f, 1.40471898632410960f },
	/* 2k Hz*/
	{ 0.81751373987237486f, 9.1243130063812572e-02f, 1.74422291149741040f },
	/* 1k Hz*/
	{ 0.90416308662408451f, 4.7918456687957744e-02f, 1.88486910231743530f },
	/* 500 Hz*/
	{ 0.95087485436560537f, 2.4562572817197315e-02f, 1.94592675617410030f },
	/* 250 Hz*/
	{ 0.97512812038326835f, 1.2435939808365826e-02f, 1.97387531983433200f },
	/* 125 Hz*/
	{ 0.98748575676916839f, 6.2571216154158060e-03f, 1.98717057206433490f },
	/* 62 Hz*/
	{ 0.99377323680705854f, 3.1133815964707323e-03f, 1.99369544947451250f },
	/* 31 Hz*/
	{ 0.99688175788596889f, 1.5591210570155556e-03f, 1.99686228063800080f },
#else
	// slightly stripped floats (last 4 bits 0)
	{ 0.241981030f, 0.379009247f, -0.808450699f },
	{ 0.448583603f, 0.275708199f, 0.605175018f },
	{ 0.668405533f, 0.165797234f, 1.40471840f },
	{ 0.817513466f, 0.0912431479f, 1.74422264f },
	{ 0.904163361f, 0.0479184389f, 1.88486862f },
	{ 0.950874329f, 0.0245625675f, 1.94592667f },
	{ 0.975128174f, 0.0124359429f, 1.97387505f },
	{ 0.987484932f, 0.00625711679f, 1.98716927f },
	{ 0.993772507f, 0.00311338156f, 1.99369431f },
	{ 0.996881485f, 0.00155912153f, 1.99686241f }
#endif
};

#ifdef CODE_SECTIONS
#pragma code_seg(".sn35")
#endif
void SYNTHCALL EQ_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(EQ_IN);
	NODE_CALL_INPUT(EQ_B1);
	NODE_CALL_INPUT(EQ_B2);
	NODE_CALL_INPUT(EQ_B3);
	NODE_CALL_INPUT(EQ_B4);
	NODE_CALL_INPUT(EQ_B5);
	NODE_CALL_INPUT(EQ_B6);
	NODE_CALL_INPUT(EQ_B7);
	NODE_CALL_INPUT(EQ_B8);
	NODE_CALL_INPUT(EQ_B9);
	NODE_CALL_INPUT(EQ_B10);

	// update values
	NODE_UPDATE_BEGIN
   
		// band gain from db
		int i = 10;
		while (i--)
		{
			n->v[i] = s_db2lin((INP(i+1) - SC[S_0_75]) * SC[S_128_0]);
		}

	NODE_UPDATE_END

	// process

	n->out = sample_t::zero();
	sample_t tin = INP(EQ_IN) * SC[S_0_75];
	sample_t* inout = (sample_t*)(n->customMem);	
	float* val = ((float*)EQ_Coeff);		
	int band = 10;
	while (band--)
	{		
		// process filter
		sample_t tout = sample_t((double)val[1])*(tin - inout[1]) + sample_t((double)val[2])*inout[2] - sample_t((double)val[0])*inout[3];
		inout[1] = inout[0];
		inout[0] = tin;
		inout[3] = inout[2];
		inout[2] = tout;
		n->out += n->v[band]*tout;
		// next filter
		inout += 4;
		val += 3;
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef COMPEXP_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn36")
#endif
void SYNTHCALL COMPEXP_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(COMPEXP_IN);
	NODE_CALL_INPUT(COMPEXP_CONTROL);	
	NODE_CALL_INPUT(COMPEXP_THRESHOLD);	
	NODE_CALL_INPUT(COMPEXP_RATIO);	
	NODE_CALL_INPUT(COMPEXP_ATTACK);	
	NODE_CALL_INPUT(COMPEXP_RELEASE);	

	// update values
	NODE_UPDATE_BEGIN
	
		// attack rate
		n->v[0] = s_exp( SC[S_LN0_01] / ( s_max(INP(COMPEXP_ATTACK ), SC[CHANNELROOT_EFT]) * SC[ENVFOLLOWER_SCALE] ) );			// 0-128.0ms * 44100.0 * 0.001;
		// release rate
		n->v[1] = s_exp( SC[S_LN0_01] / ( s_max(INP(COMPEXP_RELEASE), SC[CHANNELROOT_EFT]) * SC[ENVFOLLOWER_SCALE] * SC[S_10_0] ) ); // 0-1280.0ms * 44100.0 * 0.001
		// ratio
		n->v[2] = s_exp2(INP(COMPEXP_RATIO) *SC[S_6_0]);
		// threshold in db
		n->v[3] = (INP(COMPEXP_THRESHOLD) - SC[S_0_75]) * SC[S_128_0];		

	NODE_UPDATE_END

	// process
		
	// recified control input (db)
	sample_t vdb = s_lin2db(INP(COMPEXP_CONTROL));
	// update envelope follower (db)
	n->v[4] = s_ifthen(vdb > n->v[4], n->v[0], n->v[1]) * (n->v[4] - vdb) + vdb;
	// delta to threshold (db)
	sample_t deltadb = n->v[4] - n->v[3];
	// processing when (compression AND positive delta) OR (expansion AND negative delta)
	sample_t compflag = n->v[2] <= SC[S_1_0];
	sample_t deltaflag = deltadb > sample_t::zero();
	sample_t cemask = (compflag & deltaflag) | !(compflag | deltaflag);
	sample_t gain = SC[S_1_0];
	// at least one channel needs processing?
	if (!_mm_testz_si128(cemask.pi, cemask.pi))
	{
		// calculate new gain
		gain = s_ifthen(cemask, s_db2lin(deltadb * (n->v[2] - SC[S_1_0] )), gain);
	}	
	n->out = INP(COMPEXP_IN) * gain;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TRIGGER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn37")
#endif
void SYNTHCALL TRIGGER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(TRIGGER_IN);

	// process

	// trigger for for 1 sample when signal > 0.5 and not activated yet
	sample_t tmask = (INP(TRIGGER_IN) >= SC[S_0_5]) & (n->v[0] == sample_t::zero());
	n->v[0] = s_ifthen(tmask, SC[S_1_0], n->v[0]);
	sample_t out = s_ifthen(tmask, SC[S_1_0], sample_t::zero());
	// deactivation on signal < 0.5
	n->v[0] = s_ifthen( INP(TRIGGER_IN) <  SC[S_0_5], sample_t::zero(), n->v[0]);

	n->out = n->e = out;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TRIGGERSEQ_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn38")
#endif
void SYNTHCALL TRIGGERSEQ_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(TRIGGERSEQ_STEP);
	NODE_CALL_INPUT(TRIGGERSEQ_RESETSTEP);
	NODE_CALL_INPUT(TRIGGERSEQ_RESETPATTERN);
	NODE_CALL_INPUT(TRIGGERSEQ_PATTERN);
	//NODE_CALL_INPUT(TRIGGERSEQ_MODE);
	// 4 integer inputs = 16 patterns @ 8 bits for 2 channels
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN0_3L);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN4_7L);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN8_11L);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN12_15L);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN0_3R);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN4_7R);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN8_11R);
	//NODE_CALL_INPUT(TRIGGERSEQ_PATTERN12_15R);	

#ifdef COMPILE_VSTI
	int mode = n->input[TRIGGERSEQ_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN   

#ifdef COMPILE_VSTI
		if (_mm_testz_si128(n->v[0].pi, n->v[0].pi))
			n->v[0] = SC[S_1_0];

		// v[0] = current step
		// v[1] = current pattern
		// v[2] and v[3] hold patterns
		int* pattern = n->v[2].i;
		*pattern++ = INP(TRIGGERSEQ_PATTERN0_3L  ).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN4_7L  ).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN8_11L).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN12_15L).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN0_3R).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN4_7R).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN8_11R).i[0];
		*pattern++ = INP(TRIGGERSEQ_PATTERN12_15R).i[0];
#else
		// first time init, set step data pointer to the corresponding offset in the values
		if (_mm_testz_si128(n->v[0].pi, n->v[0].pi))
		{
			n->v[0] = SC[S_1_0];
			// in intro mode get direct pointers to values (right after the mode word)
			int* pdata = (int*)(((short*)(n->modePointer))+1);			
#ifdef _M_X64
			n->v[2].l[0] = (uintptr_t)(pdata+0);
			n->v[3].l[0] = (uintptr_t)(pdata+4);
#else
			n->v[2].i[0] = (uintptr_t)(pdata+0);
			n->v[3].i[0] = (uintptr_t)(pdata+4);
#endif
		}
#endif		

	NODE_UPDATE_END

	// process

	// auto step based on bpm
	sample_t resetstep = INP(TRIGGERSEQ_RESETSTEP) >= SC[S_0_5];
	sample_t bpmstep = sample_t::zero();
	int bpmsync = mode & TRIGGERSEQ_BPMMASK;
	if (bpmsync)
	{
		double frac = DELAY_TIME_FRACTION[bpmsync>>8];
		n->v[4] += SynthGlobalState.CurrentBPM / (SC[S_60_0] * SC[S_SAMPLERATE] * sample_t(frac));
		bpmstep = n->v[4] >= SC[S_1_0]; // signals phase wrap and therefore step froward
		n->v[4] = s_ifthen(resetstep, sample_t::zero(), n->v[4] - s_floor(n->v[4]));
	}
	// step triggered or bpm phase wrap? step forward
	n->v[0] += s_ifthen((INP(TRIGGERSEQ_STEP) >= SC[S_0_5]) | bpmstep, n->v[0], sample_t::zero());
	// reset triggered or next pattern? reset step
	n->v[0] = s_ifthen(resetstep, SC[S_1_0], n->v[0]);
	sample_t nextpattern = n->v[0] >= (SC[S_128_0]+SC[S_128_0]);
	n->v[0] = s_ifthen(nextpattern, SC[S_1_0], n->v[0]);	
	// next pattern if needed
	if (!_mm_testz_si128(nextpattern.pi, nextpattern.pi))
	{
		// random if trigger at pattern input
		sample_t maxpattern = sample_t((double)(mode & TRIGGERSEQ_COUNTMASK));
		n->v[1] += s_ifthen(INP(TRIGGERSEQ_PATTERN) >= SC[S_0_5], s_abs(s_dupleft(s_rand()))*maxpattern, SC[S_1_0]);
		// wrap pattern
		n->v[1] = s_ifthen(n->v[1] >= maxpattern, sample_t::zero(), n->v[1]);
	}
	// reset pattern triggered?
	n->v[1] = s_ifthen(INP(TRIGGERSEQ_RESETPATTERN) >= SC[S_0_5], sample_t::zero(), n->v[1]);
	// event signals next pattern
	n->e = nextpattern;	

	// integer indices
	sample_t pindex = s_toInt(n->v[1]);
	sample_t sindex = s_toInt(n->v[0]);
	// set output to 0 or 1 depending on the respective bit
	n->out = sample_t::zero();
#ifdef COMPILE_VSTI
	if (n->v[2].c[pindex.i[0]] & sindex.i[0])
		n->out.d[0] = SC[S_1_0].d[0];
	if (n->v[3].c[pindex.i[1]] & sindex.i[1])
		n->out.d[1] = SC[S_1_0].d[0];
#else
#ifdef _M_X64
	if (((char*)(n->v[2].l[0]))[pindex.i[0]] & sindex.i[0])
		n->out.d[0] = SC[S_1_0].d[0];
	if (((char*)(n->v[3].l[0]))[pindex.i[1]] & sindex.i[1])
		n->out.d[1] = SC[S_1_0].d[0];
#else
	if (((char*)(n->v[2].i[0]))[pindex.i[0]] & sindex.i[0])
		n->out.d[0] = SC[S_1_0].d[0];
	if (((char*)(n->v[3].i[0]))[pindex.i[1]] & sindex.i[1])
		n->out.d[1] = SC[S_1_0].d[0];
#endif		
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAMPLEREC_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn39")
#endif
void SYNTHCALL SAMPLEREC_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	//NODE_CALL_INPUT(SAMPLEREC_IN);
	//NODE_CALL_INPUT(SAMPLEREC_MODE);

	if (_mm_testz_si128(n->e.pi, n->e.pi))
	{
		// gate signal for wavetable creation and also for no preocessing again
		n->e = SC[S_1_0];

#ifdef COMPILE_VSTI
		if (n->customMem)
			SynthFree(n->customMem);
#endif
		// get requested number of samples (max. is 2^20)

#ifdef COMPILE_VSTI
		DWORD samples = ((DWORD)(n->input[1]->i[0])) >> 12;
#else
		DWORD samples = *(n->modePointer) >> 12;
#endif
		n->v[10] = sample_t((double)samples);
		// create memory
		n->customMem = (DWORD*)SynthMalloc(samples * sizeof(sample_t));
		// buffer start pointer
#ifdef _M_X64
		n->v[11].l[0] = (long long)(n->customMem);
#else
		n->v[11].i[0] = (int)(n->customMem);
#endif		

		// save global tick counter and fill the buffer
		DWORD backupTick = SynthGlobalState.CurrentTick;
		sample_t* buffer = (sample_t*)(n->customMem);
		while (samples--)
		{
			// ensure the input hierarchy is called every sample by changing the global tick counter
			SynthGlobalState.CurrentTick++;
			// adjust our own tick as well for proper recursion
			n->lastTick = SynthGlobalState.CurrentTick;

			NODE_CALL_INPUT(SAMPLEREC_IN);
			// store in wavetable
			*buffer++ = INP(SAMPLEREC_IN);
		}
		// restore global tick counter
		SynthGlobalState.CurrentTick = backupTick;
		n->lastTick = SynthGlobalState.CurrentTick;
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sn3a")
#endif
inline sample_t Sampler_Sample(SynthNode* n, const sample_t& index)
{	
	sample_t i = s_ifthen(sample_t::zero() > index, index + n->v[10], index);
	i = s_ifthen(i >= n->v[10],	i - n->v[10], i);
	sample_t cur   = s_floor(i);
	sample_t next  = i + SC[S_1_0];
	next = s_ifthen(next >= n->v[10], sample_t::zero(), next);
	sample_t curi  = s_toInt(cur);
	sample_t nexti = s_toInt(next);
#ifdef _M_X64
	sample_t scur = sample_t((((sample_t*)(n->v[11].l[0]))[curi.i[0]]).d[0], (((sample_t*)(n->v[11].l[0]))[curi.i[1]]).d[1]);
	sample_t snext = sample_t((((sample_t*)(n->v[11].l[0]))[nexti.i[0]]).d[0], (((sample_t*)(n->v[11].l[0]))[nexti.i[1]]).d[1]);
#else
	sample_t scur  = sample_t((((sample_t*)(n->v[11].i[0]))[curi.i[0]]).d[0], (((sample_t*)(n->v[11].i[0]))[curi.i[1]]).d[1]);
	sample_t snext = sample_t((((sample_t*)(n->v[11].i[0]))[nexti.i[0]]).d[0], (((sample_t*)(n->v[11].i[0]))[nexti.i[1]]).d[1]);
#endif
	return s_lerp(scur, snext, i-cur);
}

#ifndef SAMPLER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn3b")
#endif
void SYNTHCALL SAMPLER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SAMPLER_IN);
	NODE_CALL_INPUT(SAMPLER_POSITION);	
	NODE_CALL_INPUT(SAMPLER_SPEED);
	NODE_CALL_INPUT(SAMPLER_DIRECTION);	
	NODE_CALL_INPUT(SAMPLER_LOOPSTART);	
	NODE_CALL_INPUT(SAMPLER_LOOPEND);
	NODE_CALL_INPUT(SAMPLER_CROSSFADE);	
	//NODE_CALL_INPUT(SAMPLER_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[SAMPLER_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN   
		
#ifdef COMPILE_VSTI
#ifdef _M_X64
		n->v[11].l[0] = 0;
#else
		n->v[11].i[0] = 0;
#endif
#endif

#ifndef STOREDSAMPLES_SKIP
		// using compressed samples?
		if ((mode & SAMPLER_MODEMASK) == SAMPLER_STORED)
		{
			int table = (mode & SAMPLER_SRCMASK) >> SAMPLER_SRCSHIFT;
#ifdef COMPILE_VSTI
			if (SynthGlobalState.RawWaveTable[table])
#endif
			{
				// size of the table
				n->v[10] = SynthGlobalState.RawWaveTable[table][0];
				// buffer start pointer
#ifdef _M_X64
				n->v[11].l[0] = (long long)(&(SynthGlobalState.RawWaveTable[table][1]));
#else
				n->v[11].i[0] = (int)(&(SynthGlobalState.RawWaveTable[table][1]));
#endif
			}
		}
		// using sample input node?
		else
#endif
		{
			SynthNode* innode = ((SynthNode*)(n->input[SAMPLER_IN]));
#ifdef COMPILE_VSTI
			if (innode->id == SAMPLEREC_ID || innode->id == SAPI_ID || innode->id == GMDLS_ID)
#endif
			{
				// size of the table
				n->v[10] = innode->v[10];
				// buffer start pointer
#ifdef _M_X64
				n->v[11].l[0] = innode->v[11].l[0];
#else
				n->v[11].i[0] = innode->v[11].i[0];
#endif
			}
		}
		
		// start sample position
		n->v[5] = s_clamp(INP(SAMPLER_POSITION)*n->v[10], n->v[10]-SC[S_1_0], sample_t::zero());
		// loop start and end pos
		sample_t ts = s_clamp(INP(SAMPLER_LOOPSTART)*n->v[10], n->v[10]-SC[S_1_0], sample_t::zero());
		sample_t te = s_clamp(INP(SAMPLER_LOOPEND)*n->v[10], n->v[10]-SC[S_1_0], sample_t::zero());
		// loop start pos
		n->v[1] = s_min(ts, te);
		// loop end pos
		n->v[2] = s_max(ts, te);
		// crossfade samples
		n->v[3] = s_max((n->v[2]-n->v[1]) * SC[S_0_5] * s_clamp(INP(SAMPLER_CROSSFADE), SC[S_1_0], sample_t::zero()), SC[S_1_0]);
		// playback speed
		n->v[9] = s_exp2(INP(SAMPLER_SPEED)*SC[S_128_0]*SC[S_1_O_12]);

	NODE_UPDATE_END

	// process

	n->out = sample_t::zero();

#ifdef COMPILE_VSTI
	// bail out when no buffer is set or no buffer size given
#ifdef _M_X64
	if (!n->v[11].l[0])
#else
	if (!n->v[11].i[0])
#endif
		return;
#endif

	sample_t forward = INP(SAMPLER_DIRECTION) < SC[S_0_5];
	sample_t direction = s_ifthen(forward, SC[S_1_0], SC[S_M1_0]);
	sample_t step = n->v[9]*direction;

	// get left and right samples (read index)
	n->v[0] += n->v[5];
	sample_t out  = Sampler_Sample(n, n->v[0]);

#ifndef SAMPLER_SKIP_LOOP
	// loop activated?
	if (mode & SAMPLER_LOOP)
	{
		n->v[4] = n->v[4] | ((n->v[0] >= n->v[1]) & (n->v[0] <= n->v[2]));
		if (!_mm_testz_si128(n->v[4].pi, n->v[4].pi))
		{
			// keep in loop borders
			n->v[0] = s_clamp(n->v[0], n->v[2], n->v[1]);
			// get distance to loop boundary and blend factor
			sample_t boundary = s_ifthen(forward, n->v[2], n->v[1]);
			sample_t otherboundary = s_ifthen(forward, n->v[1], n->v[2]) + n->v[3]*direction;
			sample_t bdist = boundary - n->v[0];
			//// blend with sample on the other side of the loop
			sample_t fout = Sampler_Sample(n, otherboundary - bdist);			
			out = s_ifthen(n->v[4], s_lerp(fout, out, s_min(s_abs(bdist), n->v[3]) / n->v[3]), out);
			// go to next sample according to play speed, direction 	
			n->v[0] += step;
			// wrap loop pos if needed	
			bdist = boundary - n->v[0];
			n->v[0] = s_ifthen(n->v[4] & ((bdist*direction) < sample_t::zero()), otherboundary - bdist, n->v[0]);
		}
		else 
		{
			// go to next sample according to play speed, direction 	
			n->v[0] += step;
		}
	}
	else
#endif
	{
		// go to next sample according to play speed, direction 	
		n->v[0] += step;		
	}
	// stay in buffer range (-1 for first sample so we get a 0 from sampler_sample once below beginning
	n->v[0] = s_clamp(n->v[0], n->v[10], SC[S_M1_0]);	
	n->out = s_ifthen((n->v[0] >= n->v[10]) | (n->v[0] < sample_t::zero()), sample_t::zero(), out);
	n->v[0] -= n->v[5];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLITCH_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sninit")
#endif
void SYNTHCALL GLITCH_init(SynthNode* n)
{
	n->customMem = (DWORD*)SynthMalloc(sizeof(sample_t)*(SYNTH_SAMPLERATE)*8);
}

#ifdef CODE_SECTIONS
#pragma code_seg(".sn3d")
#endif
void SYNTHCALL GLITCH_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(GLITCH_IN);
	NODE_CALL_INPUT(GLITCH_ACTIVE);	
	NODE_CALL_INPUT(GLITCH_TIME);
	NODE_CALL_INPUT(GLITCH_P1);	
	NODE_CALL_INPUT(GLITCH_P2);	
	NODE_CALL_INPUT(GLITCH_SPEED);	
	//NODE_CALL_INPUT(GLITCH_MODE);
		
#ifdef COMPILE_VSTI
	int mode = n->input[GLITCH_MODE]->i[0];
#else
	int mode = *(n->modePointer) & 0xffff;
#endif

	// update values
	NODE_UPDATE_BEGIN
				
		// slice time is in fractions per beat (therefore / 4)
		n->v[7] = GetDelayTimeFraction(INP(GLITCH_TIME))*SC[S_0_5]*SC[S_0_5];
		n->v[6] = s_dupleft(s_clamp(INP(GLITCH_P1), SC[S_1_0], sample_t::zero()));
		// base speed
		n->v[2] = s_exp2(INP(GLITCH_SPEED)*SC[S_128_0]*SC[S_1_O_12]);
		
		// available: v[13], v[14], v[15], v[19], v[23], v[24]

		switch (mode)
		{
#ifndef GLITCH_SKIP_TAPESTOP
			case GLITCH_TAPESTOP:
			{		
				// slowdown
				n->v[4] = n->v[6]*n->v[6]/(SC[S_32768_0]*SC[S_0_125]);
				// speedup
				n->v[5] = s_dupleft(INP(GLITCH_TSSPEEDUP)*INP(GLITCH_TSSPEEDUP)/(SC[S_32768_0]*SC[S_0_125]));
				break;
			}
#endif
#ifndef GLITCH_SKIP_RETRIGGER
			case GLITCH_RETRIGGER:
			{
				sample_t sp = s_exp2(SC[S_0_5]*INP(GLITCH_RTSPEEDPITCH));
				// delta speed per loop
				n->v[4] = s_dupleft(sp);
				// delta pitch per loop
				n->v[5] = s_dupright(sp);
				// slice fraction to be used
				//n->v[6] = s_dupleft(s_clamp(INP(GLITCH_RTLENGTH), SC[S_1_0], sample_t::zero()));
				break;
			}
#endif
#ifndef GLITCH_SKIP_SHUFFLE
			case GLITCH_SHUFFLE:
			{				
				sample_t sp = s_clamp(INP(GLITCH_SHREPEATAMOUNT), SC[S_1_0], sample_t::zero());
				// repeat probability
				n->v[4] = s_dupleft(sp);
				// amount of blending
				n->v[5] = s_dupright(sp);				
				// backrange
				//n->v[6] = s_dupleft(s_clamp(INP(GLITCH_SHBACKRANGE), SC[S_1_0], sample_t::zero()))*SC[S_0_5];
				n->v[6] *= SC[S_0_5];				
				break;
			}
#endif
#ifndef GLITCH_SKIP_REVERSE
			case GLITCH_REVERSE:
			{
				break;
			}
#endif
		}

		// maximum buffer size
		n->v[10] = SC[S_SAMPLERATE]*SC[S_8_0];
		// buffer pointer (neded for Sampler_Sample)
#ifdef _M_X64
		n->v[11].l[0] = (long long)(n->customMem);
#else
		n->v[11].i[0] = (int)(n->customMem);
#endif

	NODE_UPDATE_END

	// process

	sample_t out = INP(GLITCH_IN);

#if !((defined GLITCH_SKIP_SHUFFLE) && (defined GLITCH_SKIP_REVERSE)) 
	// circular buffer for shuffle and reverse
	if (mode >= GLITCH_SHUFFLE)
	{
		sample_t bpos = s_toInt(n->v[0]);
		(((sample_t*)n->customMem)[bpos.i[0]]).d[0] = out.d[0];
		(((sample_t*)n->customMem)[bpos.i[1]]).d[1] = out.d[1];
		// increase/wrap buffer indices
		n->v[0] += SC[S_1_0];
		n->v[0] = s_ifthen(n->v[0] >= n->v[10], n->v[0] - n->v[10], n->v[0]);
	}
#endif

	// effect active
	if (!_mm_testz_si128((INP(GLITCH_ACTIVE) > SC[S_0_5]).pi, (INP(GLITCH_ACTIVE) > SC[S_0_5]).pi))
	{
		// init needed?
		if (_mm_testz_si128(n->e.pi, n->e.pi))
		{		
			n->e = SC[S_1_0];
			// buffer fill index for tapestop and retrigger
			if (mode < GLITCH_SHUFFLE)
				n->v[0] = sample_t::zero();
#ifndef GLITCH_SKIP_SHUFFLE			
			// custom rand seed specified?
			sample_t rands = s_dupright(INP(GLITCH_SHBACKRANGESEED));
			if (!_mm_testz_si128(rands.pi, rands.pi))
			{
				n->v[21].pi = _mm_add_epi32(_mm_shuffle_epi32(s_toInt(rands*SC[S_32768_0]), _MM_SHUFFLE(3, 1, 2, 0)), SC[S_EXP2_OFFSET].pi);
			}
			// use current rand seed
			else
			{
				n->v[21] = SC[S_RAND_SEED];
			}
#endif			
			// playback index
			n->v[1] = n->v[0];			
			// state
			n->v[3] = 
			// last out and fadein counter
			n->v[16] = n->v[17] = 
#ifndef GLITCH_SKIP_SHUFFLE			
			// shuffle sample counter
			n->v[9] = 
#endif			
			sample_t::zero();			
			// retrigger slice factor / shuffle repeat counter,  incremental (normalzed) play speed
			n->v[8] = n->v[12] = SC[S_1_0];			
			// fadeout counter once effect is off
			n->v[20] = SC[S_32_0];

		}		

#if !((defined GLITCH_SKIP_TAPESTOP) && (defined GLITCH_SKIP_RETRIGGER))
		// for tapestop and retrigger stop when buffer is full
		if (mode < GLITCH_SHUFFLE && !_mm_testz_si128((n->v[0] < n->v[10]).pi, (n->v[0] < n->v[10]).pi))
		{
			sample_t bpos = s_toInt(n->v[0]);
			(((sample_t*)n->customMem)[bpos.i[0]]).d[0] = out.d[0];
			(((sample_t*)n->customMem)[bpos.i[1]]).d[1] = out.d[1];
			// increase/wrap buffer indices
			n->v[0] += SC[S_1_0];
		}
#endif

		// get current sample
		n->v[16] = Sampler_Sample(n, n->v[1]);
		// fadein counter update
		n->v[17] = s_max(n->v[17] - SC[S_1_0], sample_t::zero());

		switch (mode)
		{
#ifndef GLITCH_SKIP_TAPESTOP
			case GLITCH_TAPESTOP:
			{				
				// done with effect (read index exceeded buffer write index?)
				n->v[16] = s_ifthen(n->v[1] < n->v[0], n->v[16], out);
				// slowdown
				n->v[12] = s_ifthen(n->v[3] == sample_t::zero(), s_max(n->v[12] - n->v[4], sample_t::zero()), n->v[12]);
				// state change?
				n->v[3] = s_ifthen(n->v[12] == sample_t::zero(), SC[S_1_0], n->v[3]);
				// speedup
				n->v[12] = s_ifthen(n->v[3] != sample_t::zero(), n->v[12] + n->v[5], n->v[12]);				
				// next read index				
				n->v[1] = s_min(n->v[1] + n->v[12]*n->v[2], n->v[10]);
				break;
			}
#endif
#ifndef GLITCH_SKIP_RETRIGGER
			case GLITCH_RETRIGGER:
			{
				sample_t slicesize = s_max(s_dupleft(n->v[7]) * n->v[8], SC[S_32_0]);
				// fadeout start when slice fraction is reached
				n->v[16] = s_lerp(n->v[16], sample_t::zero(), s_clamp((n->v[1] - slicesize*n->v[6])/SC[S_32_0], SC[S_1_0], sample_t::zero()));
				// fadein to current slice
				n->v[16] = s_lerp(n->v[16], n->v[18], n->v[17]/SC[S_32_0]);				
				// slice loop done?
				if (!_mm_testz_si128((n->v[1] >= slicesize).pi, (n->v[1] >= slicesize).pi))
				{
					// wrap index
					n->v[1] = n->v[1] - slicesize;
					// change pitch
					n->v[12] = n->v[12] * n->v[5];
					// change slice factor
					n->v[8] = s_clamp(n->v[8] * n->v[4], n->v[0], SC[S_0_125]);				
					// set new fade signal and restore fade counter
					n->v[18] = n->v[16];
					n->v[17] = SC[S_32_0];
				}
				// next read index				
				n->v[1] = s_min(n->v[1] + n->v[12]*n->v[2], n->v[10]);
				break;
			}
#endif
#ifndef GLITCH_SKIP_SHUFFLE
			case GLITCH_SHUFFLE:
			{				
				// mix using shuffle blend
				n->v[16] = s_lerp(n->v[16], out, n->v[3]);
				// fadein to current slice
				n->v[16] = s_lerp(n->v[16], n->v[18], n->v[17]/SC[S_32_0]);				
				// decrement sample counter by speed factor
				n->v[9] -= n->v[2];
				// slice loop done?
				if (!_mm_testz_si128((n->v[9] <= sample_t::zero()).pi, (n->v[9] <= sample_t::zero()).pi))
				{
					// reset read index
					n->v[1] -= n->v[12];
					// reset sample counter to slice size
					n->v[9] = n->v[12];
					// decrement slice loop counter
					n->v[8] -= SC[S_1_0];
					// set new fade signal and restore fade counter
					n->v[18] = n->v[16];
					n->v[17] = SC[S_32_0];
				}				
				// need a new slice?				
				if (!_mm_testz_si128((n->v[8] <= sample_t::zero()).pi, (n->v[8] <= sample_t::zero()).pi))
				{	
					// flip random seed
					sample_t rand = SC[S_RAND_SEED];
					SC[S_RAND_SEED] = n->v[21];
					n->v[21] = rand;
					// new random slice size					
					rand = s_abs(s_rand());					
					sample_t s0 = s_min(n->v[7], s_shuffle(n->v[7], n->v[7]));
					sample_t s1 = s_max(n->v[7], s_shuffle(n->v[7], n->v[7]));
					n->v[12] = n->v[9] = s_lerp(s0, s1, s_dupleft(rand));
					// new random slice start (current buffer write index - random backrange)
					n->v[1] = n->v[0] - s_dupright(rand)*n->v[6]*n->v[10] - SC[S_1_0];					
					// new random loop counter					
					rand = s_abs(s_rand());
					n->v[8] = s_floor(SC[S_32_0]*s_dupleft(rand)*n->v[4] + SC[S_1_0]);
					// new blend factor
					n->v[3] = s_dupright(rand)*n->v[5];
					// flip random seed
					rand = SC[S_RAND_SEED];
					SC[S_RAND_SEED] = n->v[21];
					n->v[21] = rand;
				}
				// next read index with speed factor including wrap
				n->v[1] += n->v[2];
				n->v[1] = s_ifthen(n->v[1] >= n->v[10], n->v[1]-n->v[10], n->v[1]);
				break;
			}
#endif
#ifndef GLITCH_SKIP_REVERSE
			case GLITCH_REVERSE:
			{
				// next read index including wrap
				n->v[1] -= n->v[2];
				n->v[1] = s_ifthen(n->v[1] < sample_t::zero(), n->v[1] + n->v[10], n->v[1]);
				break;
			}
#endif
		}		
	}
	// effect disabled
	else
	{
		n->e = sample_t::zero();
		// perform fadeout when effect goes off
		n->v[20] = s_max(n->v[20] - SC[S_1_0], sample_t::zero());
	}
	// blend incoming with effect according to fade factor
	n->out = s_lerp(out, n->v[16], n->v[20]/SC[S_32_0]);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SVFILTER_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn3e")
#endif
void SYNTHCALL SVFILTER_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SVFILTER_IN);
	NODE_CALL_INPUT(SVFILTER_FREQ);	
	NODE_CALL_INPUT(SVFILTER_Q);
	//NODE_CALL_INPUT(SVFILTER_MODE));
		
#ifdef COMPILE_VSTI
	int mode = n->input[SVFILTER_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN   	

		sample_t temp = Frequency_Map(INP(SVFILTER_FREQ), mode);

		
		//n->v[0] = s_min(temp, SC[S_0_75])*SC[S_PI2]; // this is limited to about 16khz, after that it gets unstable
		// would be a better way for the param space but since we've been using this for a long time it would affect many instruments
		
		// v[0] is frequency 	
		n->v[0] = temp;
		// legacy fix for note sync due to above way of using the param space since the early days
		if (mode & BQFILTER_NOTEMAP)
			n->v[0] *= SC[S_PI2];
		
		// v[1] = q
		n->v[1] = s_clamp(SC[S_1_0] - INP(SVFILTER_Q), SC[NOISEGEN_B0], sample_t::zero()); 

	NODE_UPDATE_END

	sample_t input = INP(SVFILTER_IN);
	sample_t* wrk = &(n->v[4]);
	int loop = 0;
	int stages = ((mode & SVFILTER_STAGEMASK) >> (SVFILTER_STAGESHIFT-1)); // 0, 2, 4, 8
	do 
	{
		// step 1
		wrk[0] += n->v[0]*wrk[2];		
		wrk[1] = input - wrk[0] - n->v[1]*wrk[2];
		wrk[2] += n->v[0]*wrk[1];
		// step 2 (double sample to get more filter precision than just 11khz)
		wrk[0] += n->v[0]*wrk[2];
		wrk[1] = input - wrk[0] - n->v[1]*wrk[2];
		wrk[2] += n->v[0]*wrk[1];
		
		// combine low high and band outputs to get all available combinations
		input = sample_t::zero();
		if (mode & SVFILTER_LOWPASS)
			input += wrk[0];
		if (mode & SVFILTER_HIGHPASS)
			input += wrk[1];
		if (mode & SVFILTER_BANDPASS)
			input += wrk[2];
		if (mode & SVFILTER_PEAK)
			input -= wrk[1];

		// next stage work vars
		wrk += 3;
	} while (++loop < stages);

	n->out = input;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAPI_SKIP
wchar_t			lpSAPITextBuffer[1024*1024];

GUID speechGUIDFormat =
{
	0xC31ADBAE,
	0x527F,
	0x4FF5,
	{ 0xA2, 0x30,
	0xF6, 0x2B, 0xB6, 0x1F, 0xF7, 0x0C }
};
WAVEFORMATEX speechWaveFormatEx =
{
	WAVE_FORMAT_PCM,
	1,      // WORD        nChannels;          
	44100,	// DWORD       nSamplesPerSec;     
	88200,	// DWORD       nAvgBytesPerSec;    
	2,		// WORD        nBlockAlign;        
	16,     // WORD        wBitsPerSample;     
	0,		// WORD        cbSize;       // extra bytes after wfx struct, UNUSED
};
#ifdef _M_X64
#include <sapi.h>
IStream		*pBaseStream;
ISpVoice	*pVoice;
ISpStream	*pStream;
#else
void	*pVoice;	
void	*pStream;	
void	*pBaseStream;
#endif

//96749377-3391-11D2-9EE3-00C04F797396
const CLSID CLSID_SpVoice64k =
{
	0x96749377,
	0x3391,
	0x11D2,
	{ 0x9E, 0xE3,
	0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96 }
};

//6C44DF74-72B9-4992-A1EC-EF996E0422D4
const IID IID_ISpVoice64k =
{
	0x6C44DF74,
	0x72B9,
	0x4992,
	{ 0xA1, 0xEC,
	0xEF, 0x99, 0x6E, 0x04, 0x22, 0xD4 }
};

//715D9C59-4442-11D2-9605-00C04F8EE628
const CLSID CLSID_SpStream64k =
{
	0x715D9C59,
	0x4442,
	0x11D2,
	{ 0x96, 0x05,
	0x00, 0xC0, 0x4F, 0x8E, 0xE6, 0x28 }
};

//12E3CCA9-7518-44C5-A5E7-BA5A79CB929E
const IID IID_ISpStream64k =
{
	0x12E3CCA9,
	0x7518,
	0x44C5,
	{ 0xA5, 0xE7,
	0xBA, 0x5A, 0x79, 0xCB, 0x92, 0x9E }
};

#ifdef CODE_SECTIONS
#pragma code_seg(".sn3f")
#endif
void SYNTHCALL SAPI_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	if (_mm_testz_si128(n->e.pi, n->e.pi))
	{
		// gate signal for wavetable creation and also for no preocessing again
		n->e = SC[S_1_0];

		STATSTG stats;
		char* ttsText = (char*)(n->specialData); //"<volume level='100'/>This sounds <emph>normal</emph><pitch absmiddle='-10'/>but the pitch drops half way through";
#ifdef COMPILE_VSTI
		if (ttsText == 0)
			return;
		if (n->customMem)
			SynthFree(n->customMem);
#endif
		int tlen = (int)strlen(ttsText) + 1;
		MultiByteToWideChar(0, 0, ttsText, tlen, lpSAPITextBuffer, tlen);

		CoInitialize(NULL);
		CoCreateInstance(CLSID_SpVoice64k, NULL, CLSCTX_ALL, IID_ISpVoice64k, (void **)&pVoice);
		CoCreateInstance(CLSID_SpStream64k, NULL, CLSCTX_ALL, IID_ISpStream64k, (void **)&pStream);
		CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&pBaseStream);
#ifdef _M_X64
		//SpConvertStreamFormatEnum(SPSF_44kHz16BitMono, &speechGUIDFormat, &speechWaveFormatEx); //using direct constants	
		pStream->SetBaseStream(pBaseStream, speechGUIDFormat, &speechWaveFormatEx);
		pVoice->SetOutput(pStream, TRUE);
		pVoice->Speak(lpSAPITextBuffer, 8, NULL); // 8 = SPF_IS_XML
		_LARGE_INTEGER a = { 0 };
		pStream->Seek(a, STREAM_SEEK_SET, NULL);
		pBaseStream->Stat(&stats, STATFLAG_NONAME);
#else
		//SpConvertStreamFormatEnum(SPSF_44kHz16BitMono, &speechGUIDFormat, &speechWaveFormatEx); //using direct constants	
		_asm
		{
			//pStream->SetBaseStream(pBaseStream, speechGUIDFormat, speechWaveFormatEx);
			push	offset speechWaveFormatEx
			push	offset speechGUIDFormat
			push	dword ptr[pBaseStream]
			mov     eax, dword ptr[pStream]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 3Ch]
			call	eax

			//pVoice->SetOutput(pStream, TRUE);
			push	1
			push	dword ptr[pStream]
			mov     eax, dword ptr[pVoice]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 34h]
			call	eax

			//pVoice->Speak(text, SPF_IS_XML, NULL );
			push	0
			push	8
			push	offset lpSAPITextBuffer
			mov     eax, dword ptr[pVoice]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 50h]
			call	eax

			//_LARGE_INTEGER a = { 0 };
			//pStream->Seek(a, STREAM_SEEK_SET, NULL);
			push	0
			push	0
			push	0
			push	0
			mov     eax, dword ptr[pStream]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 14h]
			call	eax

			//pBaseStream->Stat(&stats, STATFLAG_NONAME);*/
			push	1
			lea		eax, stats
			push	eax
			mov     eax, dword ptr[pBaseStream]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 30h]
			call	eax
		}
#endif

		short *pBuffer = (short*)SynthMalloc(stats.cbSize.QuadPart);	//buffer to read the data
		ULONG bytesRead;
		ULONG bsize = stats.cbSize.QuadPart;
#ifdef _M_X64
		pBaseStream->Read(pBuffer, stats.cbSize.QuadPart, &bytesRead);
#else
		_asm
		{
			//pBaseStream->Read(pBuffer, stats.cbSize.QuadPart, &bytesRead);
			lea		eax, bytesRead
			push	eax
			push	dword ptr[bsize]
			push	dword ptr[pBuffer]
			mov     eax, dword ptr[pBaseStream]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 0Ch]
			call	eax
		}
#endif
		// get requested number of samples (max. is 2^20)
		DWORD samples = bytesRead / 2;
		n->v[10] = sample_t((double)samples);
		// create memory
		n->customMem = (DWORD*)SynthMalloc(samples * sizeof(sample_t));
		// buffer start pointer
#ifdef _M_X64
		n->v[11].l[0] = (long long)(n->customMem);
#else
		n->v[11].i[0] = (int)(n->customMem);
#endif

		// copy/convert sapi samples
		sample_t* buffer = (sample_t*)(n->customMem);
		short* srcbuf = pBuffer;
		while (samples--)
		{
			*buffer++ = sample_t((double)(*srcbuf++)) / SC[S_32768_0];
		}

		// cleanup
		SynthFree(pBuffer);
#ifdef _M_X64
		pStream->Release();
		pBaseStream->Release();
		pVoice->Release();
#else
		_asm
		{
			//pStream->Release();
			mov     eax, dword ptr[pStream]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 8h]
			call	eax

			//pBaseStream->Release();
			mov     eax, dword ptr[pBaseStream]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 8h]
			call	eax

			//pVoice->Release();
			mov     eax, dword ptr[pVoice]
			push	eax
			mov		eax, dword ptr[eax]
			mov     eax, dword ptr[eax + 8h]
			call	eax
		}
#endif
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD		GMDLS_NumSamples[512];
sample_t*	GMDLS_SampleBuffer[512];
#ifndef GMDLS_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn40")
#endif
void SYNTHCALL GMDLS_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	//NODE_CALL_INPUT(GMDLS_MODE)
		
#ifdef COMPILE_VSTI
	int mode = n->input[GMDLS_MODE]->i[0];
#else
	int mode = *(n->modePointer) & 0xffff;
#endif

	// set number of samples
	n->v[10] = sample_t((double)GMDLS_NumSamples[mode]);
	// set buffer start pointer
#ifdef _M_X64
	n->v[11].l[0] = (long long)(GMDLS_SampleBuffer[mode]);
#else
	n->v[11].i[0] = (int)(GMDLS_SampleBuffer[mode]);
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BOWED_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn41")
#endif
void SYNTHCALL BOWED_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(BOWED_POSITION);
	NODE_CALL_INPUT(BOWED_PRESSURE);
	NODE_CALL_INPUT(BOWED_VELOCITY);
	NODE_CALL_INPUT(BOWED_VIBRATO);
	NODE_CALL_INPUT(BOWED_FRICTIONSYM);
	
	// update values
	NODE_UPDATE_BEGIN   

#ifndef SONG_AFTERTOUCH_SKIP
		// max velocity (attack velocity or aftertouch if available)
		n->v[4] = s_ifthen(SynthGlobalState.CurrentVoice->Aftertouch != sample_t::zero(), SynthGlobalState.CurrentVoice->Aftertouch, SynthGlobalState.CurrentVoice->AttackVelocity)*SC[S_0_2]+SC[BOWED_VELOFFSET];
#else
		n->v[4] = SynthGlobalState.CurrentVoice->AttackVelocity*SC[S_0_2]+SC[BOWED_VELOFFSET];
#endif
		// friction symmetry offset
		n->v[5] = INP(BOWED_FRICTIONSYM)*SC[REVERB_RSF];
		// bow pressure
		n->v[6] = SC[S_4_0]+SC[S_1_0] - s_clamp(INP(BOWED_PRESSURE), SC[S_1_0], sample_t::zero())*SC[S_4_0];

		// max delay length = REVERB_MAXCOMBSIZE*6 per delay = 9840*4 DWORD
		n->v[10] = SC[REVERB_MAXCOMBSIZE]*SC[S_6_0];
		// base delay length
		sample_t baseDelay = s_clamp(SC[S_1_0]/SynthGlobalState.CurrentVoice->Frequency - SC[S_1_0] - SC[S_0_5], n->v[10], SC[S_0_5]);
		sample_t ratio = s_clamp(INP(BOWED_POSITION), SC[S_1_0], sample_t::zero())*SC[S_0_2]+SC[BOWED_VELOFFSET];
		// bridge delay
		n->v[8] = baseDelay*ratio;
		// neck delay
		n->v[9] = baseDelay*(s_clamp(INP(BOWED_VIBRATO), SC[S_1_0], sample_t::zero())*SC[S_1_O_12] + SC[S_1_0]-ratio);
		
		// delay pointers
#ifdef _M_X64
		n->v[12].l[0] = (long long)(n->customMem);
		n->v[13].l[0] = (long long)(n->customMem + 9840 * 4);
#else
		n->v[12].i[0] = (int)(n->customMem);
		n->v[13].i[0] = (int)(n->customMem + 9840 * 4);
#endif

	NODE_UPDATE_END

	// process

	// read from bridge delay = output
#ifdef _M_X64
	n->v[11].l[0] = n->v[12].l[0];
#else
	n->v[11].i[0] = n->v[12].i[0];
#endif
	sample_t bridgeReflection = Sampler_Sample(n, n->v[0] - n->v[8]);	
	
	// read from neck delay
#ifdef _M_X64
	n->v[11].l[0] = n->v[13].l[0];
#else
	n->v[11].i[0] = n->v[13].i[0];
#endif
	sample_t neckReflection = -Sampler_Sample(n, n->v[1] - n->v[9]);

	// apply filter to bridgereflection (1pole filter, pole at: 0.75 - (0.2 * 22050.0 / SAMPLE_RATE) with gain 0.95
	n->out = bridgeReflection;
	n->v[7] = SC[BOWED_FILTERB0]*bridgeReflection + SC[BOWED_FILTERA1]*n->v[7];//sample_t(0.4275)*bridgeReflection + sample_t(0.55)*n->v[7];
	bridgeReflection = -n->v[7];

	// calculate velocity and deltas including friction table	
	sample_t deltaVelocity = n->v[4]*INP(BOWED_VELOCITY) - (bridgeReflection + neckReflection);
	// nonlinear bow function of the delta velocity
	sample_t bowfunc = s_abs((deltaVelocity + n->v[5])*n->v[6]) + SC[S_0_75];
	bowfunc = s_clamp(SynthGlobalState.CurrentVoice->Gate / (bowfunc*bowfunc*bowfunc*bowfunc), SC[NOISEGEN_B0], SC[S_0_001]);
	// new velocity only when voice still on (gate = 1.0)
	sample_t newVelocity = deltaVelocity * bowfunc;
		
	neckReflection = newVelocity + neckReflection;
	bridgeReflection = newVelocity + bridgeReflection;

	// bridge delay new input
	sample_t bpos = s_toInt(n->v[0]);
#ifdef _M_X64
	(((sample_t*)n->v[12].l[0])[bpos.i[0]]).d[0] = neckReflection.d[0];
	(((sample_t*)n->v[12].l[0])[bpos.i[1]]).d[1] = neckReflection.d[1];
#else
	(((sample_t*)n->v[12].i[0])[bpos.i[0]]).d[0] = neckReflection.d[0];
	(((sample_t*)n->v[12].i[0])[bpos.i[1]]).d[1] = neckReflection.d[1];
#endif

	// neck delay new input	
	bpos = s_toInt(n->v[1]);
#ifdef _M_X64
	(((sample_t*)n->v[13].l[0])[bpos.i[0]]).d[0] = bridgeReflection.d[0];
	(((sample_t*)n->v[13].l[0])[bpos.i[1]]).d[1] = bridgeReflection.d[1];
#else
	(((sample_t*)n->v[13].i[0])[bpos.i[0]]).d[0] = bridgeReflection.d[0];
	(((sample_t*)n->v[13].i[0])[bpos.i[1]]).d[1] = bridgeReflection.d[1];
#endif

	// increase/wrap buffer indices
	n->v[0] += SC[S_1_0];
	n->v[0] = s_ifthen(n->v[0] >= n->v[10], n->v[0] - n->v[10], n->v[0]);
	n->v[1] += SC[S_1_0];
	n->v[1] = s_ifthen(n->v[1] >= n->v[10], n->v[1] - n->v[10], n->v[1]);	
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// code injection test, use compiled cpu instructions, inject to memory and execute
//int func_add(int a, int b)
//{
//	return a + b;
//}
//BYTE func[] = { 0x8B,0x44,0x24,0x04,0x03,0x44,0x24,0x08,0xC3 };
//
//typedef int(*func_copy)(int, int);
// code injection test
/*HANDLE hMMFile = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_EXECUTE_READWRITE, 0, 256, NULL);
BYTE* lpMMFile = (BYTE*)MapViewOfFile(hMMFile, FILE_MAP_WRITE, 0, 0, 0);
memcpy(lpMMFile, func, sizeof(func));
UnmapViewOfFile(lpMMFile);
lpMMFile = 0;
lpMMFile = (BYTE*)MapViewOfFile(hMMFile, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, 0);
func_copy testfunc = (func_copy)lpMMFile;
int b = testfunc(2, 3);
CloseHandle(hMMFile);*/

#ifndef FORMULA_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn42")
#endif
void SYNTHCALL FORMULA_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(FORMULA_IN0);
	NODE_CALL_INPUT(FORMULA_IN1);
	
#ifdef COMPILE_VSTI
	BYTE* command = (BYTE*)(SynthGlobalState.SpecialDataPointer[n->valueOffset]); // in vsti the command buffer is used by all nodes of the same offset (to allow voice nodes as well)
	if (!command)
		return;
#else
	BYTE* command = (BYTE*)(n->modePointer); // for exe inlined in node definition as "mode"
#endif
	sample_t* stack = (sample_t*)(n->customMem);
	stack--;
	while (*command != FORMULA_DONE)
	{
		// increase stack if needed
		if (*command == FORMULA_CMD_CONSTANT || *command > FORMULA_KEEP_STACK)
			stack++;
		else if (*command <= FORMULA_DEC_STACK)
			stack--;

		switch (*command)
		{		
			case FORMULA_CMD_CONSTANT:	*stack = sample_t((double)(*((float*)command))); command += 3;		break;
			// 2 param (decrease stack)
			case FORMULA_CMD_ADD:		*stack = *stack  + *(stack + 1);		break;
			case FORMULA_CMD_MUL:		*stack = *stack  * *(stack + 1);		break;			
			case FORMULA_CMD_E:			*stack = *stack == *(stack + 1);		break;
			case FORMULA_CMD_NE:		*stack = *stack != *(stack + 1);		break;
			case FORMULA_CMD_GT:		*stack = *stack  > *(stack + 1);		break;			
			case FORMULA_CMD_GTE:		*stack = *stack >= *(stack + 1);		break;			
			case FORMULA_CMD_AND:		*stack = *stack  & *(stack + 1);		break;
			case FORMULA_CMD_OR:		*stack = *stack  | *(stack + 1);		break;
			case FORMULA_CMD_LT:		*stack = *stack  < *(stack + 1);		break;
			case FORMULA_CMD_LTE:		*stack = *stack <= *(stack + 1);		break;
			case FORMULA_CMD_SUB:		*stack = *stack - *(stack + 1);			break;
			case FORMULA_CMD_DIV:		*stack = *stack / *(stack + 1);			break;
			case FORMULA_CMD_MIN:		*stack = s_min(*stack, *(stack + 1));	break;
			case FORMULA_CMD_MAX:		*stack = s_max(*stack, *(stack + 1));	break;
			case FORMULA_CMD_MOD:		*stack = s_mod(*stack, *(stack + 1));	break;
			case FORMULA_CMD_TRISAW:	*stack = TriSaw(*stack, *(stack + 1));	break;
			case FORMULA_CMD_PULSE:		*stack = s_ifthen(*stack < *(stack + 1), SC[S_1_0], SC[S_M1_0]);	break;
			case FORMULA_CMD_LERP:		stack--;	*stack = s_lerp(*stack, *(stack + 1), *(stack + 2));	break;
			case FORMULA_CMD_IFTHEN:	stack--;	*stack = s_ifthen(*stack, *(stack + 1), *(stack + 2));	break;
			case FORMULA_CMD_MAXTIME:	n->e = n->v[1] < *stack; *stack = s_ifthen(n->e, *(stack + 1), sample_t::zero());	break;
			// 1 param (keep stack)
			case FORMULA_CMD_NEG:		*stack = s_neg(*stack);					break;
			case FORMULA_CMD_ABS:		*stack = s_abs(*stack);					break;
			case FORMULA_CMD_SQRT:		*stack = s_sqrt(*stack);				break;
			case FORMULA_CMD_CEIL:		*stack = s_ceil(*stack);				break;
			case FORMULA_CMD_FLOOR:		*stack = s_floor(*stack);				break;
			case FORMULA_CMD_SQR:		*stack = *stack * *stack;				break;
			case FORMULA_CMD_COS:		*stack = *stack + SC[S_PI2];			// continue with sin below
			case FORMULA_CMD_SIN:		*stack = s_sin(*stack);					break;
			case FORMULA_CMD_EXP2:		*stack = s_exp2(*stack);				break;
			case FORMULA_CMD_LOG2:		*stack = s_log2(*stack);				break;
			// 0 param (inc stack)
			case FORMULA_CMD_RAND:		*stack = s_rand();						break;						
			case FORMULA_CMD_PI:		*stack = SC[S_PI];						break;			
			case FORMULA_CMD_TAU:		*stack = SC[S_2PI];						break;
			case FORMULA_CMD_TAUTIME:	*stack = SC[S_2PI] * n->v[1];			break;
			case FORMULA_CMD_IN0:		*stack = INP(FORMULA_IN0);				break;
			case FORMULA_CMD_IN1:		*stack = INP(FORMULA_IN1);				break;
			case FORMULA_CMD_VFREQUENCY:*stack = SC[S_SAMPLERATE] * ((sample_t*)(SynthGlobalState.CurrentVoice))[*command - FORMULA_CMD_VFREQUENCY + 3]; break;
			case FORMULA_CMD_VNOTE:
			case FORMULA_CMD_VVELOCITY:
			case FORMULA_CMD_VTRIGGER:
			case FORMULA_CMD_VGATE:
			case FORMULA_CMD_VAFTERTOUCH:*stack = ((sample_t*)(SynthGlobalState.CurrentVoice))[*command - FORMULA_CMD_VFREQUENCY + 3]; break;			
			default:
			{
				// set variable (needs stack reduction)
				if (*command >= FORMULA_CMD_SETVAR)
				{
					stack--; // to compensate for increment from above)
					n->v[*command - FORMULA_CMD_SETVAR] = *stack; 
					stack--;
				}
				// get variable (stack already correct)
				else
				{
					*stack = n->v[*command - FORMULA_CMD_GETVAR]; break;
				}
				break;
			}
		}
		// go to next command
		command++;
	}

	// increase time counter (given in seconds)
	n->v[1] += SC[S_1_0] / SC[S_SAMPLERATE];
	n->out = n->v[0];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SNH_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn43")
#endif
void SYNTHCALL SNH_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(SNH_IN);
	NODE_CALL_INPUT(SNH_SNH);
#ifndef SNH_SKIP_SNHSMOOTH
	NODE_CALL_INPUT(SNH_SNHSMOOTH);
#endif
	//NODE_CALL_INPUT(SNH_MODE);

#ifdef COMPILE_VSTI
	int mode = n->input[SNH_MODE]->i[0];
#else
	int mode = *(n->modePointer);
#endif

	// update values
	NODE_UPDATE_BEGIN
		
		n->v[0] = Frequency_Map(INP(SNH_SNH), mode);
		// slightly different handling for notemap in snh (transpose frequency)	
		if (mode & SNH_NOTEMAP)
		{
			n->v[0] = SC[S_0_5] * n->v[0] * s_exp2((SC[S_0_75] - SC[S_1_0] + INP(SNH_SNH))*SC[S_128_0] * SC[S_1_O_12]);
		}

	NODE_UPDATE_END

	// process

	sample_t tmask;
	if (mode & SNH_TRIGGER)
	{
		tmask = INP(SNH_SNH) < SC[S_0_5];
		n->v[1] = sample_t::zero(); // no smoothing in trigger mode
	}
	else
	{
		// snh phase integration and wrapping
		n->v[1] -= n->v[0];
		tmask = n->v[1] < sample_t::zero();
		n->v[1] -= s_floor(n->v[1]);
	}
	// at least one channel needs tick?
	if (!_mm_testz_si128(tmask.pi, tmask.pi))
	{
		n->v[3] = s_ifthen(tmask, n->v[2], n->v[3]);
		n->v[2] = s_ifthen(tmask, INP(SNH_IN), n->v[2]);
	}
#ifndef SNH_SKIP_SNHSMOOTH
	// snhsmooth between current signal and previous signal via cosine interpolation
	n->out = s_lerp(n->v[2], s_lerp(n->v[2], n->v[3], (SC[S_1_0] - s_cos(n->v[1] * SC[S_PI]))*SC[S_0_5]), INP(SNH_SNHSMOOTH));
#else
	// no smooth
	n->out = n->v[2];
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WTFOSC_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn44")
#endif
inline bool GetNextZeroCrossing(SynthNode* n, int& startIndex)
{
	sample_t bufSize = s_toInt(n->v[10]);
#ifdef _M_X64
	sample_t* buf = (sample_t*)(n->v[11].l[0]);
#else
	sample_t* buf = (sample_t*)(n->v[11].i[0]);
#endif
	// start at next zero crossing
	int i = startIndex;
	while (i < bufSize.i[0])
	{
		// require wave going UP
		sample_t waveup = (buf[i] < sample_t::zero()) & (buf[i + 1] > sample_t::zero());
		if (!_mm_testz_si128(waveup.pi, waveup.pi))
		{
			startIndex = i;
			return true;
		}
		i++;
	}
	return false;
}

#ifdef CODE_SECTIONS
#pragma code_seg(".sn45")
#endif
void SYNTHCALL WTFOSC_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(WTFOSC_IN);
	NODE_CALL_INPUT(WTFOSC_POSITION);
	NODE_CALL_INPUT(WTFOSC_SPEED);
	NODE_CALL_INPUT(WTFOSC_TARGETSEARCHLEN);
	NODE_CALL_INPUT(WTFOSC_COUNTSKIP);
	//NODE_CALL_INPUT(WTFOSC_MODE);

#ifdef COMPILE_VSTI
int mode = n->input[WTFOSC_MODE]->i[0];
#else
int mode = *(n->modePointer);
#endif

	// update/(re)scan the wave input only once
	if (_mm_testz_si128(n->e.pi, n->e.pi))
	{
		n->e = SC[S_1_0];

#ifdef COMPILE_VSTI
#ifdef _M_X64
		n->v[11].l[0] = 0;
#else
		n->v[11].i[0] = 0;
#endif
#endif

#ifndef STOREDSAMPLES_SKIP
		// using compressed samples?
		if ((mode & SAMPLER_MODEMASK) == SAMPLER_STORED)
		{
			int table = (mode & WTFOSC_SRCMASK) >> WTFOSC_SRCSHIFT;
#ifdef COMPILE_VSTI
			if (SynthGlobalState.RawWaveTable[table])
#endif
			{
				// size of the table
				n->v[10] = SynthGlobalState.RawWaveTable[table][0];
				// buffer start pointer
#ifdef _M_X64
				n->v[11].l[0] = (long long)(&(SynthGlobalState.RawWaveTable[table][1]));
#else
				n->v[11].i[0] = (int)(&(SynthGlobalState.RawWaveTable[table][1]));
#endif
			}
		}
		// using sample input node?
		else
#endif
		{
			SynthNode* innode = ((SynthNode*)(n->input[WTFOSC_IN]));
#ifdef COMPILE_VSTI
			if (innode->id == SAMPLEREC_ID || innode->id == SAPI_ID || innode->id == GMDLS_ID)
#endif
			{
				// size of the table
				n->v[10] = innode->v[10];
				// buffer start pointer
#ifdef _M_X64
				n->v[11].l[0] = innode->v[11].l[0];
#else
				n->v[11].i[0] = innode->v[11].i[0];
#endif
			}
		}
		
		// playback speed
		n->v[3] = s_exp2(INP(WTFOSC_SPEED)*SC[S_128_0] * SC[S_1_O_12]);

		
#ifdef COMPILE_VSTI
		// bail out when no buffer is set or no buffer size given
#ifdef _M_X64
		if (!n->v[11].l[0])
#else
		if (!n->v[11].i[0])
#endif
			return;
		if (n->customMem)
			SynthFree(n->customMem);
#endif
		// allocate new buffer
		sample_t targetSearch = s_floor(s_max(INP(WTFOSC_TARGETSEARCHLEN)*SC[S_32_0] * SC[S_32_0], SC[S_2_0]));
		n->v[1] = s_dupleft(targetSearch);
		targetSearch = s_toInt(targetSearch);
		sample_t countSkip = s_toInt(s_max(INP(WTFOSC_COUNTSKIP)*SC[S_32_0] * SC[S_32_0], sample_t::zero()));
#ifdef _M_X64
		sample_t* buf = (sample_t*)n->v[11].l[0];
#else
		sample_t* buf = (sample_t*)n->v[11].i[0];
#endif
		n->customMem = (DWORD*)SynthMalloc(countSkip.i[0] * targetSearch.i[0] * sizeof(sample_t));

		n->v[2] = sample_t::zero();
		sample_t* wtBuf = (sample_t*)(n->customMem);
		int seekStartIdx = 0;
		while (countSkip.i[0]-- > 0)
		{
			int seekEndIdx;
			if (mode & WTFOSC_FIXEDGRAINS)
			{
				// end is always start + searchlen
				seekEndIdx = seekStartIdx + targetSearch.i[1];
				if (seekEndIdx >= s_toInt(n->v[10]).m128i_i32[0])
					break;
			}
			else
			{
				// start at next zero crossing
				if (!GetNextZeroCrossing(n, seekStartIdx))
					break;
				// increase by searchlen and look for next zero crossing
				seekEndIdx = seekStartIdx + targetSearch.i[1];
				if (!GetNextZeroCrossing(n, seekEndIdx))
					break;
			}
			// now stretch the found section into our wavetable
			sample_t pos = sample_t((double)seekStartIdx);
			sample_t stepSize = (sample_t((double)(seekEndIdx - seekStartIdx)) / n->v[1]);
			int samples = targetSearch.i[0];
			while (samples--)
			{
				sample_t ipos = s_floor(pos);
				*wtBuf++ = s_lerp(buf[s_toInt(ipos).m128i_i32[0]], buf[s_toInt(s_floor(pos + stepSize)).m128i_i32[0]], pos - ipos);
				pos += stepSize;
			}
			// go to next start index including an optional skip
			seekStartIdx = seekEndIdx + countSkip.i[1];
			n->v[2] += SC[S_1_0];
		}

		// size of the final table
		n->v[10] = n->v[2] * n->v[1];
		// buffer start pointer
#ifdef _M_X64
		n->v[11].l[0] = (long long)(n->customMem);
#else
		n->v[11].i[0] = (int)(n->customMem);
#endif
	}

	// process	

#ifdef COMPILE_VSTI
	// bail out when no buffer is set or no buffer size given
#ifdef _M_X64
	if (!n->v[11].l[0] || n->v[10].d[0] < 0.0)
#else
	if (!n->v[11].i[0] || n->v[10].d[0] < 0.0)
#endif
		return;
#endif	

	sample_t fIndex = s_clamp(n->v[2] * INP(WTFOSC_POSITION), n->v[2] - SC[S_1_0], sample_t::zero());
	sample_t iIndex = s_floor(fIndex);
	sample_t wtOffset = n->v[1] * iIndex;
	sample_t out1 = Sampler_Sample(n, n->v[0] + wtOffset);
	sample_t out2 = Sampler_Sample(n, n->v[0] + wtOffset + n->v[1]);
	n->out = s_equalp(out1, out2, fIndex - iIndex);
	// go to next sample according to play speed 	
	n->v[0] += n->v[3];
	sample_t wrap = n->v[0] >= n->v[1];
	n->v[0] = s_ifthen(wrap, n->v[0] - n->v[1], n->v[0]);
	// click reduction
#ifndef WTFOSC_SKIP_REDUCECLICK
	if (mode & WTFOSC_REDUCECLICK)
	{
		// fadeout last cycles last output
		n->out = s_lerp(n->out, n->v[5], n->v[4] / SC[S_10_0]);
		// update fadecounter
		n->v[4] = s_max(n->v[4] - SC[S_1_0], sample_t::zero());
		// on wrap reset fade counter and set last output
		if (!_mm_testz_si128(wrap.pi, wrap.pi))
		{
			n->v[4] = s_ifthen(wrap, SC[S_10_0], n->v[4]);
			n->v[5] = s_ifthen(wrap, n->out, n->v[5]);
		}
	}
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FORMANT_SKIP			
// vowel formants: 5x amplitude(-dB), 5x bandwidth(Hz), 5x frequency(Hz)
// see http://www.csounds.com/manual/html/MiscFormants.html
#define FORMANT_NUM_VOICES 5
#define FORMANT_NUM_VOWELS (FORMANT_NUM_VOICES*5)
#define FORMANT_NUM_FILTERS_PER_VOWEL 5
#define FORMANT_NUM_FILTERS (FORMANT_NUM_VOWELS*FORMANT_NUM_FILTERS_PER_VOWEL)
#define FORMANT_NUM_COEFFS (FORMANT_NUM_FILTERS*3)
short FORMANT_Coeff[FORMANT_NUM_COEFFS] =
{
// soprano
	   0, 	   6, 	  32, 	  20, 	  50, 
	  80, 	  90, 	 120, 	 130, 	 140, 
	 800, 	1150, 	2900, 	3900, 	4950, 
	   0, 	  20, 	  15, 	  40, 	  56, 
	  60, 	 100, 	 120, 	 150, 	 200, 
	 350, 	2000, 	2800, 	3600, 	4950, 
	   0, 	  12, 	  26, 	  26, 	  44, 
	  60, 	  90, 	 100, 	 120, 	 120, 
	 270, 	2140, 	2950, 	3900, 	4950, 
	   0, 	  11, 	  22, 	  22, 	  50, 
	  70, 	  80, 	 100, 	 130, 	 135, 
	 450, 	 800, 	2830, 	3800, 	4950, 
	   0, 	  16, 	  35, 	  40, 	  60, 
	  50, 	  60, 	 170, 	 180, 	 200, 
	 325, 	 700, 	2700, 	3800, 	4950, 
// alto
	   0, 	   4, 	  20, 	  36, 	  60, 
	  80, 	  90, 	 120, 	 130, 	 140, 
	 800, 	1150, 	2800, 	3500, 	4950, 
	   0, 	  24, 	  30, 	  35, 	  60, 
	  60, 	  80, 	 120, 	 150, 	 200, 
	 400, 	1600, 	2700, 	3300, 	4950, 
	   0, 	  20, 	  30, 	  36, 	  60, 
	  50, 	 100, 	 120, 	 150, 	 200, 
	 350, 	1700, 	2700, 	3700, 	4950, 
	   0, 	   9, 	  16, 	  28, 	  55, 
	  70, 	  80, 	 100, 	 130, 	 135, 
	 450, 	 800, 	2830, 	3500, 	4950, 
	   0, 	  12, 	  30, 	  40, 	  64, 
	  50, 	  60, 	 170, 	 180, 	 200, 
	 325, 	 700, 	2530, 	3500, 	4950, 
// countertenor
	   0, 	   6, 	  23, 	  24, 	  38, 
	  80, 	  90, 	 120, 	 130, 	 140, 
	 660, 	1120, 	2750, 	3000, 	3350, 
	   0, 	  14, 	  18, 	  20, 	  20, 
	  70, 	  80, 	 100, 	 120, 	 120, 
	 440, 	1800, 	2700, 	3000, 	3300, 
	   0, 	  24, 	  24, 	  36, 	  36, 
	  40, 	  90, 	 100, 	 120, 	 120, 
	 270, 	1850, 	2900, 	3350, 	3590, 
	   0, 	  10, 	  26, 	  22, 	  34, 
	  40, 	  80, 	 100, 	 120, 	 120, 
	 430, 	 820, 	2700, 	3000, 	3300, 
	   0, 	  20, 	  23, 	  30, 	  34, 
	  40, 	  60, 	 100, 	 120, 	 120, 
	 370, 	 630, 	2750, 	3000, 	3400, 
// tenor
	   0, 	   6, 	   7, 	   8, 	  22, 
	  80, 	  90, 	 120, 	 130, 	 140, 
	 650, 	1080, 	2650, 	2900, 	3250, 
	   0, 	  14, 	  12, 	  14, 	  20, 
	  70, 	  80, 	 100, 	 120, 	 120, 
	 400, 	1700, 	2600, 	3200, 	3850, 
	   0, 	  15, 	  18, 	  20, 	  30, 
	  40, 	  90, 	 100, 	 120, 	 120, 
	 290, 	1870, 	2800, 	3250, 	3540, 
	   0, 	  10, 	  12, 	  12, 	  26, 
	  40, 	  80, 	 100, 	 120, 	 120, 
	 400, 	 800, 	2600, 	2800, 	3000, 
	   0, 	  20, 	  17, 	  14, 	  26, 
	  40, 	  60, 	 100, 	 120, 	 120, 
	 350, 	 600, 	2700, 	2900, 	3300, 
// bass
	   0, 	   7, 	   9, 	   9, 	  20, 
	  60, 	  70, 	 110, 	 120, 	 130, 
	 600, 	1040, 	2250, 	2450, 	2750, 
	   0, 	  12, 	   9, 	  12, 	  18, 
	  40, 	  80, 	 100, 	 120, 	 120, 
	 400, 	1620, 	2400, 	2800, 	3100, 
	   0, 	  30, 	  16, 	  22, 	  28, 
	  60, 	  90, 	 100, 	 120, 	 120, 
	 250, 	1750, 	2600, 	3050, 	3340, 
	   0, 	  11, 	  21, 	  20, 	  40, 
	  40, 	  80, 	 100, 	 120, 	 120, 
	 400, 	 750, 	2400, 	2600, 	2900, 
	   0, 	  20, 	  32, 	  28, 	  36, 
	  40, 	  80, 	 100, 	 120, 	 120, 
	 350, 	 600, 	2400, 	2675, 	2950, 
};

#ifdef CODE_SECTIONS
#pragma code_seg(".sn49")
#endif
void SYNTHCALL FORMANT_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(FORMANT_IN);
	NODE_CALL_INPUT(FORMANT_GAIN);
	//NODE_CALL_INPUT(FORMANT_MODE);

#ifdef COMPILE_VSTI
int mode = n->input[FORMANT_MODE]->i[0];
#else
int mode = *(n->modePointer)&0xffff;
#endif

	// calculate all formant coeffs once
	if (_mm_testz_si128(n->e.pi, n->e.pi))
	{
		n->e = SC[S_1_0];
		n->customMem = (DWORD*)SynthMalloc(FORMANT_NUM_FILTERS * 5 * sizeof(sample_t));
		sample_t* wrkbuf = ((sample_t*)(n->customMem));
		int ofs = 0;
		while (ofs < FORMANT_NUM_COEFFS)
		{
			int fpv = FORMANT_NUM_FILTERS_PER_VOWEL;
			while (fpv--)
			{
				sample_t amp((double)FORMANT_Coeff[ofs]);
				sample_t bw((double)FORMANT_Coeff[ofs + FORMANT_NUM_FILTERS_PER_VOWEL]);
				sample_t f((double)FORMANT_Coeff[ofs + FORMANT_NUM_FILTERS_PER_VOWEL + FORMANT_NUM_FILTERS_PER_VOWEL]);
				sample_t r = s_exp(s_neg(bw*SC[S_PI]/SC[S_SAMPLERATE]));
				sample_t c = s_neg(r*r);
				sample_t b = r*SC[S_2_0]*s_cos(f*SC[S_2PI]/SC[S_SAMPLERATE]);
				wrkbuf[0] = (SC[S_1_0] - b - c) * s_exp2(s_neg(amp) / SC[S_6_0]); // caution, amp is stored negated in FORMANT_Coeff, therefore the s_neg
				wrkbuf[1] = b;
				wrkbuf[2] = c;
				//wrkbuf[3] = x1
				//wrkbuf[4] = x2

				wrkbuf += 5;
				ofs++;
			}
			ofs += 10;
		}		
	}

	// process	
		
	n->out = sample_t::zero();
	sample_t* wrkbuf = ((sample_t*)(n->customMem)) + mode * FORMANT_NUM_FILTERS_PER_VOWEL*5;
	// loop all formant filters	
	int fpv = FORMANT_NUM_FILTERS_PER_VOWEL;
	while (fpv--)
	{
		sample_t y = INP(FORMANT_IN)*wrkbuf[0] + wrkbuf[3] * wrkbuf[1] + wrkbuf[4] * wrkbuf[2];
		wrkbuf[4] = wrkbuf[3];
		wrkbuf[3] = y;		
		wrkbuf += 5;
		n->out += y*INP(FORMANT_GAIN);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef EQ3_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn50")
#endif
void SYNTHCALL EQ3_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(EQ3_IN);
	NODE_CALL_INPUT(EQ3_LGAIN);
	NODE_CALL_INPUT(EQ3_MGAIN);
	NODE_CALL_INPUT(EQ3_HGAIN);

	// update values
	NODE_UPDATE_BEGIN

		n->v[0] = s_db2lin((INP(EQ3_LGAIN) - SC[S_0_75]) * SC[S_128_0]);
		n->v[1] = s_db2lin((INP(EQ3_MGAIN) - SC[S_0_75]) * SC[S_128_0]);
		n->v[2] = s_db2lin((INP(EQ3_HGAIN) - SC[S_0_75]) * SC[S_128_0]);

		n->v[3] = sample_t(0.11512805391913456716493084746726);// 2 * sin(M_PI * ((double)880 / (double)48000)); 		
		n->v[8] = sample_t(0.6428789306063231614021152481578);// 2 * sin(M_PI * ((double)5000 / (double)48000)); 

	NODE_UPDATE_END

	// process

	sample_t* b = &(n->v[3]);

	// Filter #1 (lowpass)	
	b[1]  += b[0] * (INP(EQ3_IN) - b[1]);
	b[2]  += b[0] * (b[1] - b[2]);
	b[3]  += b[0] * (b[2] - b[3]);
	b[4]  += b[0] * (b[3] - b[4]);
	b += 5;

	// Filter #2 (highpass)
	b[1]  += b[0] * (INP(EQ3_IN) - b[1]);
	b[2]  += b[0] * (b[1] - b[2]);
	b[3]  += b[0] * (b[2] - b[3]);
	b[4]  += b[0] * (b[3] - b[4]);
	b += 5;

	// output
	n->out = n->v[7]*n->v[0] + (n->v[12] - n->v[7])*n->v[1] + (b[2] - n->v[12])*n->v[2];

	// Shuffle history buffer 
	b[2] = b[1];
	b[1] = b[0];
	b[0] = INP(EQ3_IN);                
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef OSCSYNC_SKIP
#ifdef CODE_SECTIONS
#pragma code_seg(".sn51")
#endif
void SYNTHCALL OSCSYNC_tick(SynthNode* n)
{
	NODE_STATEFUL_PROLOG;

	NODE_CALL_INPUT(OSCSYNC_IN);

	SynthNode* in  = (SynthNode*)(n->input[OSCSYNC_IN]);
	SynthNode* osc = (SynthNode*)(n->input[OSCSYNC_OSC]);
	// reset current workphase and previous workphase
	osc->v[2] = s_ifthen(in->e != sample_t::zero(), sample_t::zero(), osc->v[2]);
	osc->v[3] = s_ifthen(in->e != sample_t::zero(), sample_t::zero(), osc->v[3]);

	// now call the synced osc
	NODE_CALL_INPUT(OSCSYNC_OSC);

	// output
	n->out = INP(OSCSYNC_OSC);  // s_ifthen(in->e != sample_t::zero(), SC[S_1_0], sample_t::zero());
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sn46")
#endif
void SYNTHCALL CONSTANT_tick(SynthNode* n)
{
	NODE_STATELESS_PROLOG
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sn47")
#endif
void SYNTHCALL VOICEPARAM_tick(SynthNode* n)
{
	NODE_STATELESS_PROLOG

	// NODE_CALL_INPUT(VOICEPARAM_SCALE);

	n->out = INP(VOICEPARAM_SCALE) * ((sample_t*)(SynthGlobalState.CurrentVoice))[n->id - CONSTANT_ID + 2];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// node hierarhy create/delete functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sncdn")
#endif
SynthNode* CreateNode(DWORD id, DWORD refnodes, DWORD offset, DWORD isGlobal)
{
	// create and clear the node
	SynthNode* node = (SynthNode*)SynthMalloc(sizeof(SynthNode));
	// special case for constants. they have 1 or 2 values after the type value which are no node references, so copy the values
	if (id == CONSTANT_ID)
	{
#ifdef COMPILE_VSTI
		// in plugin mode, nothing to do for constants, they are set in the caller of this function
#else		
		// in intro  mode, NodeValues = WORD  width        no status			
		// float constant (1 or 2 values, triggered via 1 or 2 refnodes)
		node->out.d[0] = node->out.d[1] = *(float*)(&(SynthGlobalState.NodeValues[offset]));
		if (refnodes == 2)
			node->out.d[1] = *(float*)(&(SynthGlobalState.NodeValues[offset+2]));
#endif		
	}
	SynthGlobalState.GlobalNodes[offset] = node;

	// set remaining parameters
	node->isGlobal = isGlobal;
	node->valueOffset = offset;
	node->numInputs = refnodes;
	node->id = id;	
	node->tick = NodeTickFunctions[id];
	node->init = NodeInitFunctions[id];	
	if (node->init)
		node->init(node);
#ifdef COMPILE_VSTI
	node->updateRate = NODE_UPDATE_RATE;
#endif

	return node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sncdn")
#endif
SynthNode* CreateNodes(DWORD offset, BOOL global)
{
	if (global)
	{
		// check global nodes array if the node already exists
		if (SynthGlobalState.GlobalNodes[offset])
			return SynthGlobalState.GlobalNodes[offset];
	}
	else
	{
		// check creation tick for voice nodes (to prevent loops during current creation tick)
		if (
#ifndef COMPILE_VSTI
			(offset >= SynthGlobalState.ConstantOffset) ||
#endif
			(SynthGlobalState.CreatedNodeTicks[offset] == SynthGlobalState.CreateNodeTick))
		{
			goto CreateNodes_ReturnExisting;
		}
		SynthGlobalState.CreatedNodeTicks[offset] = SynthGlobalState.CreateNodeTick;
	}

	// scope fix for VS2017 C2362
	{
	// get the info
	SYNTH_WORD* values = &(SynthGlobalState.NodeValues[offset]);
	DWORD info = *values++;
	// only create nodes of voice/global type depending on global flag
	if (((info & NODEINFO_GLOBAL) != 0) == global)
	{
		// type id, number of parametersn, number of required inputs and number of input references
		DWORD id = info & 0x7f;
		DWORD refnodes = (info >> 8) & 0x7f;
		// create and init the new voice node and put the new global node in the global node array to prevent recreation
		SynthNode* node = CreateNode(id, refnodes, offset, global);
		// recurse all required input signals
		id = 0;
		while (id < refnodes)
			node->input[id++] = (sample_t*)CreateNodes(*values++, global);
		// store the pointer to the potential mode value(s)
		node->modePointer = (DWORD*)values;
	}
	else
	{
		// global node creation, but no global node, so return a 0 pointer
		if (global)
			return 0;
	}
	}

CreateNodes_ReturnExisting:
	return CreateNodes(offset, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CODE_SECTIONS
#pragma code_seg(".sncdn")
#endif
void DestroyVoiceNodes(SynthNode* node)
{
	// only delete existing nodes, which are not global
	if (node && !node->isGlobal)
	{
#ifdef COMPILE_VSTI
		SynthGlobalState.GlobalNodes[node->valueOffset] = NULL;
#endif
		// mark processed voice nodes by the global flag, so they wont be processed again
		node->isGlobal = 1;
		// free custom mem if it was allocated
		if (node->customMem)
			SynthFree(node->customMem);
		// destroy all inputs
		SynthNode** input = (SynthNode**)(node->input);
		while (node->numInputs--)
			DestroyVoiceNodes(*input++);
		// delete the current node
		SynthFree(node);
	}
}
