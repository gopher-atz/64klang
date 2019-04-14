///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// synth node classes/structs defining and implementing all available node types of 64klang
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SYNTH_NODE_H_
#define _SYNTH_NODE_H_

#include "windows.h"
#include "sample_t.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// defines and helpers
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SYNTH_SAMPLERATE	44100
#ifdef COMPILE_VSTI
	#include <string>
	#define MAX_CHANNELS		16
#else
	#define MAX_CHANNELS		16
#endif

#define NODE_MAX_WORKVARS	32
#define NODE_MAX_INPUTS		16

struct SynthNode;
typedef void(__fastcall *SynthFunction)(SynthNode*);
#define SYNTHCALL __fastcall

// node info looks like this:
// byte0: nodeid
// byte1: number of referenced nodes (bits 0..6), global flag (bit 7)
#define INFO(__u, __n, __g)	((((DWORD)__g) << 15) | (((DWORD)__n) << 8) | ((DWORD)__u))

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// node base class
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SynthNode
{
	// 0x000: output sample 
	sample_t		out;

	// 0x010: pointer to tick function
	SynthFunction	tick;
	// last tick we were called (indicates node update needed)
	DWORD			lastTick;
	// pointer to init function
	SynthFunction	init;
	// pointer to an additional work set if a unit needs it
	DWORD*			customMem;

	// 0x020: 
	// pointer to mode
	DWORD*			modePointer;	
	// number of used inputs
	DWORD			numInputs;	
	// flag telling if this unit is a global one
	DWORD			isGlobal;
	// offset to values where that node starts
	DWORD			valueOffset;	
		
	// 0x030: 
	// event signal for special use.
	sample_t		e;
	
	// 0x040: 
	// space reserved for unit work set
	sample_t		v[NODE_MAX_WORKVARS];

	// 0x240 
	// pointer to the node/out used as inputs for this node
	sample_t*		input[NODE_MAX_INPUTS];	

	// 0x280 	
	// node id
	DWORD			id;
	// special data assigned to the node
	void*			specialData;

	// vsti specific 
#ifdef COMPILE_VSTI
	std::string		specialDataText;
	std::string		specialDataText2;
	// the update counter
	int				currentUpdateStep;
	// the update rate at which input changes are applied (if the watchlist has differences)
	int				updateRate;
	// special processing flag for channel root etc.
	DWORD			processingFlags;

	static int numActiveVoices;
#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum NodeIDs
{
	SYNTHROOT_ID,
	CHANNELROOT_ID,
	NOTECONTROLLER_ID,
	VOICEMANAGER_ID,
	VOICEROOT_ID,
	ADSR_ID,
	LFO_ID,
	OSCILLATOR_ID,
	NOISEGEN_ID,
	BQFILTER_ID,
	ONEPOLEFILTER_ID,
	ONEZEROFILTER_ID,
	SHAPER_ID,
	PANNING_ID,
	SCALE_ID,
	MUL_ID,
	DIV_ID,
	ADD_ID,
	SUB_ID,
	CLIP_ID,
	MIX_ID,
	MULTIADD_ID,
	MONO_ID,
	ENVFOLLOWER_ID,
	LOGIC_ID,
	COMPARE_ID,
	SELECT_ID,
	EVENTSIGNAL_ID,
	PROCESS_ID,
	MIDISIGNAL_ID,
	DISTORTION_ID,
	CROSSMIX_ID,
	DELAY_ID,
	FBDELAY_ID,
	DCFILTER_ID,
	OSRAND_ID,
	REVERB_ID,
	EQ_ID,
	COMPEXP_ID,	
	TRIGGER_ID,
	TRIGGERSEQ_ID,
	SAMPLEREC_ID,
	SAMPLER_ID,
	SVFILTER_ID,
	ABS_ID,
	NEG_ID,
	SQRT_ID,
	MIN_ID,
	MAX_ID,
	GLITCH_ID,
	SAPI_ID,
	COMBDELAY_ID,
	GMDLS_ID,
	BOWED_ID,
	FORMULA_ID,
	SNH_ID,
	WTFOSC_ID,
	FORMANT_ID,
	EQ3_ID,
	OSCSYNC_ID,
	// reserved slots from here to 63 for future nodes
	RESERVED_ID,

	CONSTANT_ID=64,

	VOICE_FREQUENCY_ID,		
	VOICE_NOTE_ID,
	VOICE_ATTACKVELOCITY_ID,	
	VOICE_TRIGGER_ID,			
	VOICE_GATE_ID,		
	VOICE_AFTERTOUCH_ID,	

	MAXIMUM_ID
};

extern SynthFunction NodeTickFunctions[MAXIMUM_ID];
extern SynthFunction NodeInitFunctions[MAXIMUM_ID];

enum NODE_PROCESSING_FLAGS
{
	NODE_PROCESSING_DEFAULT = 0,
	NODE_PROCESSING_MUTE = 1,
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the node definitions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SYNTHROOT_INPUT
{
	SYNTHROOT_IN = 0,
	SYNTHROOT_GAIN,
	SYNTHROOT_MAX,
	SYNTHROOT_REQ_GUI_SIGNALS = 1,
	SYNTHROOT_MAX_GUI_SIGNALS = SYNTHROOT_MAX
};
#ifdef COMPILE_VSTI
	void SYNTHCALL SYNTHROOT_tick(SynthNode* n);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum CHANNELROOT_INPUT
{
	CHANNELROOT_IN = 0,
	CHANNELROOT_GAIN,
	CHANNELROOT_MAX,
	CHANNELROOT_REQ_GUI_SIGNALS = 1,
	CHANNELROOT_MAX_GUI_SIGNALS = CHANNELROOT_MAX
};
void SYNTHCALL CHANNELROOT_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum NOTECONTROLLER_INPUT
{
	NOTECONTROLLER_MAX = 0,
	NOTECONTROLLER_REQ_GUI_SIGNALS = 1,
	NOTECONTROLLER_MAX_GUI_SIGNALS = NOTECONTROLLER_MAX
};
void SYNTHCALL NOTECONTROLLER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct VMVoice
{
	SynthNode*	VoiceRoot;
	int			NoteValue;
	DWORD		VoiceTime;
#ifndef _M_X64
	int			_padding1_;
#endif

	sample_t	BaseFrequency;
	sample_t	BaseNote;
	sample_t	Frequency;	
	sample_t	Note;
	sample_t	AttackVelocity;	
	sample_t	Trigger;
	sample_t	Gate;		
	sample_t	Aftertouch;

	sample_t	GlideBaseValue;
	sample_t	GlideTargetValue;
	sample_t	GlideCurrentValue;
	sample_t	GlideStep;

	// padding for 256 byte size
	sample_t	_padding2_;
	sample_t	_padding3_;
	sample_t	_padding4_;
};
struct VMWork
{
	// voice states
	int			NumActiveVoices;
	// the current glider if available
	VMVoice*	Glider;
	int			_padding1_;
	int			_padding2_;
	// key tracking	
	int			Rescan;
	int			NumKeys;
	int			LowestKey;
	int			HighestKey;
	// arpeggiator stuff	
	int			ArpCurrentKey;
	int			ArpCurrentOct;
	int			ArpSequenceLoopIndex;
	WORD*		ArpSequencePtr;
	// padding for 256 byte alignment
	DWORD		_padding3_[52];
	// key state array
	DWORD		Keys[128];
	// active voice array
	int			ActiveVoices[128];
	// voice array
	VMVoice		Voice[128];

#ifdef COMPILE_VSTI
	WORD		ArpSequence[32];

	// arpeggiator settings (32 steps a 2 bytes)
	// byte 1: semitones and gate mask 

	// semitone mask: 00111111
	//		+2 and -2 octaves a 12 semitones = 48 semitones, center is 32

	// gate mask	: 11000000
	//		arptime/3
	//		arptime/2
	//		arptime/1.5
	//		arptime/1

	// byte 2: velocity

	// special states:
	// NEW STEP NOTE:								byte 2 value != 0x00000000
	// HOLD			: byte 1 value != 0x00000000 && byte 2 value == 0x00000000
	// EMPTY		: byte 1 value == 0x00000000 && byte 2 value == 0x00000000
#endif
};
enum VOICEMANAGER_INPUT
{
	VOICEMANAGER_VOICEROOT = 0,
	VOICEMANAGER_TRANSPOSE,
	VOICEMANAGER_GLIDE,
	VOICEMANAGER_ARPSPEED,
	VOICEMANAGER_MODE,
	VOICEMANAGER_MAX,
	VOICEMANAGER_REQ_GUI_SIGNALS = 1,
	VOICEMANAGER_MAX_GUI_SIGNALS = VOICEMANAGER_MODE
};
void SYNTHCALL VOICEMANAGER_init(SynthNode* n);
void SYNTHCALL VOICEMANAGER_tick(SynthNode* n);
enum VOICEMANAGER_MODE
{
	VOICEMANAGER_MINNOTEMASK		= 0x000000ff,
	VOICEMANAGER_MAXNOTEMASK		= 0x0000ff00,	
	VOICEMANAGER_ARP_OFF			= 0x00000000,
	VOICEMANAGER_ARP_UP				= 0x00010000,
	VOICEMANAGER_ARP_DOWN			= 0x00020000,
	VOICEMANAGER_ARP_POLY			= 0x00030000,
	VOICEMANAGER_ARP_RUNMASK		= 0x000f0000,
	VOICEMANAGER_ARP_RUNSHIFT		= 16,
	VOICEMANAGER_ARP_1OCT			= 0x00100000,
	VOICEMANAGER_ARP_2OCT			= 0x00200000,
	VOICEMANAGER_ARP_3OCT			= 0x00300000,
	VOICEMANAGER_ARP_4OCT			= 0x00400000,
	VOICEMANAGER_ARP_OCTMASK		= 0x00f00000,
	VOICEMANAGER_ARP_OCTSHIFT		= 20,
	VOICEMANAGER_ARP_INDEXMASK		= 0x1f000000, // only used in intro mode (max unique arp sequences is 32)
	VOICEMANAGER_ARP_INDEXSHIFT		= 24,
	VOICEMANAGER_ARP_RETRIGGER		= 0x80000000,
	VOICEMANAGER_MAXPOLY_64			= 0x00000000,
	VOICEMANAGER_MAXPOLY_32			= 0x20000000,
	VOICEMANAGER_MAXPOLY_16			= 0x40000000,
	VOICEMANAGER_MAXPOLY_8			= 0x60000000,
	VOICEMANAGER_MAXPOLY_RUNMASK	= 0x60000000,
	VOICEMANAGER_MAXPOLY_RUNSHIFT	= 29,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum VOICEROOT_INPUT
{
	VOICEROOT_IN = 0,
	VOICEROOT_GAIN,
	VOICEROOT_GATE_OUT,		// input which signals voice activity
	VOICEROOT_MAX,
	VOICEROOT_REQ_GUI_SIGNALS = 3,
	VOICEROOT_MAX_GUI_SIGNALS = VOICEROOT_MAX
};
void SYNTHCALL VOICEROOT_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ADSR_INPUT
{
#ifndef ADSR_SKIP_TRIGGER_GATE
	ADSR_TRIGGER = 0,			
	ADSR_GATE,
	ADSR_ATTACK,
#else
	ADSR_ATTACK = 0,
#endif
	ADSR_DECAY,
	ADSR_SUSTAIN,
	ADSR_RELEASE,
	ADSR_GAIN,
	ADSR_MODE,
	ADSR_MAX,
	ADSR_REQ_GUI_SIGNALS = 2,
	ADSR_MAX_GUI_SIGNALS = ADSR_MODE
};
void SYNTHCALL ADSR_tick(SynthNode* n);
enum ADSR_MODE
{
	ADSR_VOICETRIGGER	= 0x10,
	ADSR_VOICEGATE		= 0x20,
	ADSR_EXP			= 0x40,
	ADSR_DBGAIN			= 0x80,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum LFO_INPUT
{
	LFO_FREQ,
	LFO_PHASE,
	LFO_COLOR,
	LFO_GAIN,
	LFO_MODE,
	LFO_MAX,
	LFO_REQ_GUI_SIGNALS = 0,
	LFO_MAX_GUI_SIGNALS = LFO_MODE
};
void SYNTHCALL LFO_tick(SynthNode* n);
enum LFO_MODE
{
	LFO_SINE=0,
	LFO_SAW,
	LFO_PULSE,
	LFO_AATRISAW,
	LFO_AAPULSE,
	LFO_AABITRISAW,
	LFO_AABIPULSE,
	LFO_RPSINE,
	LFO_PNOISE,
	LFO_AATRISAWSEQ,
	LFO_SINESEQ,
#ifdef COMPILE_VSTI
	LFO_TEST,
#endif
	LFO_WAVEMASK	= 0x000000ff,	
	LFO_UNIPOLAR_P	= 0x00000000,
	LFO_UNIPOLAR_N	= 0x00000100,
	LFO_BIPOLAR		= 0x00000200,
	LFO_POLARITYMASK= 0x00000300,
	LFO_BPMSYNC		= 0x00000800,
	LFO_RANDPHASE	= 0x00001000,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum OSCILLATOR_INPUT
{
	OSCILLATOR_FREQ = 0,
	OSCILLATOR_PHASE,
	OSCILLATOR_COLOR,
	OSCILLATOR_TRANSPOSE,
	OSCILLATOR_DETUNE,	
	OSCILLATOR_GAIN,
	OSCILLATOR_UDETUNE,
	OSCILLATOR_MODE,
	OSCILLATOR_MAX,
	OSCILLATOR_REQ_GUI_SIGNALS = 0,
	OSCILLATOR_MAX_GUI_SIGNALS = OSCILLATOR_MODE
};
void SYNTHCALL OSCILLATOR_tick(SynthNode* n);
enum OSCILLATOR_MODE
{
	OSCILLATOR_SINE=0,
	OSCILLATOR_SAW,
	OSCILLATOR_PULSE,
	OSCILLATOR_AATRISAW,
	OSCILLATOR_AAPULSE,
	OSCILLATOR_AABITRISAW,
	OSCILLATOR_AABIPULSE,
	OSCILLATOR_RPSINE,
	OSCILLATOR_PNOISE,		
	OSCILLATOR_AATRISAWSEQ,
	OSCILLATOR_SINESEQ,
#ifdef COMPILE_VSTI
	OSCILLATOR_TEST,	
#endif
	OSCILLATOR_WAVEMASK	= 0x000000ff,	
	OSCILLATOR_LOOPMASK	= 0x00000700,
	OSCILLATOR_LOOPSHIFT= 0x00000008,
	OSCILLATOR_VOICEFREQ= 0x00000800,
	OSCILLATOR_RANDPHASE= 0x00001000,
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum NOISEGEN_INPUT
{
	NOISEGEN_MIX = 0,				// noise mix
	NOISEGEN_GAIN,
	NOISEGEN_MAX,
	NOISEGEN_REQ_GUI_SIGNALS = 0,
	NOISEGEN_MAX_GUI_SIGNALS = NOISEGEN_MAX
};
void SYNTHCALL NOISEGEN_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct BQFilterState
{
	sample_t	b0, b1, b2;
	sample_t	a0, a1, a2;	
	sample_t	inout[8*2]; // inout for 8 stages
};
enum BQFILTER_INPUT
{
	BQFILTER_IN = 0,		// input signal	
	BQFILTER_FREQ,			// frequency
	BQFILTER_Q,				// q
	BQFILTER_DBGAIN,		// dbgain for shelving filters
	BQFILTER_MODE,			// type of filter
	BQFILTER_MAX,
	BQFILTER_REQ_GUI_SIGNALS = 1,
	BQFILTER_MAX_GUI_SIGNALS = BQFILTER_MODE
};
void SYNTHCALL BQFILTER_tick(SynthNode* n);
enum BQFILTER_MODE
{
	BQFILTER_LOWPASS = 0,
	BQFILTER_HIGHPASS,
	BQFILTER_BANDPASSQ,
	BQFILTER_BANDPASSBW,
	BQFILTER_NOTCH,
	BQFILTER_ALLPASS,
	BQFILTER_PEAK,
	BQFILTER_LOWSHELF,
	BQFILTER_HIGHSHELF,
	BQFILTER_RESONANCE,
	BQFILTER_RESONANCENORM,
	BQFILTER_MODE_MAX,
	BQFILTER_MODEMASK	= 0x0000000f,
	BQFILTER_NOTEMAP	= 0x00000080,
	BQFILTER_QUADRATIC	= 0x00000040,
	BQFILTER_STAGEMASK	= 0x00000f00,
	BQFILTER_STAGESHIFT = 8
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ONEPOLEFILTER_INPUT
{
	ONEPOLEFILTER_IN = 0,			// input signal
	ONEPOLEFILTER_POLE,			// pole position
	ONEPOLEFILTER_MAX,
	ONEPOLEFILTER_REQ_GUI_SIGNALS = 1,
	ONEPOLEFILTER_MAX_GUI_SIGNALS = ONEPOLEFILTER_MAX
};
void SYNTHCALL ONEPOLEFILTER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ONEZEROFILTER_INPUT
{
	ONEZEROFILTER_IN = 0,			// input signal
	ONEZEROFILTER_ZERO,			// zero position
	ONEZEROFILTER_MAX,
	ONEZEROFILTER_REQ_GUI_SIGNALS = 1,
	ONEZEROFILTER_MAX_GUI_SIGNALS = ONEZEROFILTER_MAX
};
void SYNTHCALL ONEZEROFILTER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SHAPER_INPUT
{
	SHAPER_IN = 0,			// input signal
	SHAPER_DRIVE,			// drive
	SHAPER_MAX,
	SHAPER_REQ_GUI_SIGNALS = 1,
	SHAPER_MAX_GUI_SIGNALS = SHAPER_MAX
};
void SYNTHCALL SHAPER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum PANNING_INPUT
{
	PANNING_IN = 0,			// input signal
	PANNING_PAN,			// panning position
	PANNING_MAX,
	PANNING_REQ_GUI_SIGNALS = 1,
	PANNING_MAX_GUI_SIGNALS = PANNING_MAX
};
void SYNTHCALL PANNING_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SCALE_INPUT
{
	SCALE_IN = 0,					// input signal 1
	SCALE_SCALE,
	SCALE_MAX,
	SCALE_REQ_GUI_SIGNALS = 1,
	SCALE_MAX_GUI_SIGNALS = 1
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MUL_INPUT
{
	MUL_IN1 = 0,					// input signal 1
	MUL_IN2,						// input signal 2
	MUL_MAX,
	MUL_REQ_GUI_SIGNALS = 2,
	MUL_MAX_GUI_SIGNALS = MUL_MAX
};
void SYNTHCALL MUL_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum DIV_INPUT
{
	DIV_IN1 = 0,					// input signal 1
	DIV_IN2,						// input signal 2
	DIV_MAX,
	DIV_REQ_GUI_SIGNALS = DIV_MAX,
	DIV_MAX_GUI_SIGNALS = DIV_MAX
};
void SYNTHCALL DIV_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ADD_INPUT
{
	ADD_IN1 = 0,					// input signal 1
	ADD_IN2,						// input signal 2
	ADD_MAX,
	ADD_REQ_GUI_SIGNALS = 2,
	ADD_MAX_GUI_SIGNALS = ADD_MAX
};
void SYNTHCALL ADD_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SUB_INPUT
{
	SUB_IN1 = 0,					// input signal 1
	SUB_IN2,						// input signal 2
	SUB_MAX,
	SUB_REQ_GUI_SIGNALS = 2,
	SUB_MAX_GUI_SIGNALS = SUB_MAX
};
void SYNTHCALL SUB_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum CLIP_INPUT
{
	CLIP_IN = 0,			// input signal
	CLIP_LEVEL,			// clip level
	CLIP_MAX,
	CLIP_REQ_GUI_SIGNALS = 1,
	CLIP_MAX_GUI_SIGNALS = CLIP_MAX
};
void SYNTHCALL CLIP_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MIX_INPUT
{
	MIX_IN1 = 0,			// input 1
	MIX_IN2,				// input 2
	MIX_MIX,				// mix
	MIX_MAX,
	MIX_REQ_GUI_SIGNALS = 2,
	MIX_MAX_GUI_SIGNALS = MIX_MAX
};
void SYNTHCALL MIX_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MULTIADD_INPUT
{
	MULTIADD_MAX=0,
	MULTIADD_REQ_GUI_SIGNALS = 1,
	MULTIADD_MAX_GUI_SIGNALS = MULTIADD_MAX
};
void SYNTHCALL MULTIADD_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MONO_INPUT
{
	MONO_IN = 0,
	MONO_MODE,
	MONO_MAX,
	MONO_REQ_GUI_SIGNALS = 1,
	MONO_MAX_GUI_SIGNALS = MONO_MODE
};
void SYNTHCALL MONO_tick(SynthNode* n);
enum MONO_MODE
{
	MONO_LEFT = 0,
	MONO_RIGHT
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ENVFOLLOWER_INPUT
{
	ENVFOLLOWER_IN = 0,
	ENVFOLLOWER_ATTACK,
	ENVFOLLOWER_RELEASE,
	ENVFOLLOWER_MAX,
	ENVFOLLOWER_REQ_GUI_SIGNALS = 1,
	ENVFOLLOWER_MAX_GUI_SIGNALS = ENVFOLLOWER_MAX
};
void SYNTHCALL ENVFOLLOWER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum LOGIC_INPUT
{
	LOGIC_IN1 = 0,			// input 1
	LOGIC_IN2,				// input 2
	LOGIC_MODE,				// mode
	LOGIC_MAX,
	LOGIC_REQ_GUI_SIGNALS = 2,
	LOGIC_MAX_GUI_SIGNALS = LOGIC_MODE
};
void SYNTHCALL LOGIC_tick(SynthNode* n);
enum LOGIC_MODE
{
	LOGIC_AND = 0,
	LOGIC_OR,
	LOGIC_XOR,
	LOGIC_NOT
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum COMPARE_INPUT
{	
	COMPARE_IN1 = 0,
	COMPARE_IN2,
	COMPARE_MODE,
	COMPARE_MAX,
	COMPARE_REQ_GUI_SIGNALS = 2,
	COMPARE_MAX_GUI_SIGNALS = COMPARE_MODE
};
void SYNTHCALL COMPARE_tick(SynthNode* n);
enum COMPARE_MODE
{
	COMPARE_GT = 0,
	COMPARE_GTE,
	COMPARE_E,
	COMPARE_NE
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SELECT_INPUT
{	
	SELECT_IN1 = 0,
	SELECT_IN2,
	SELECT_CONDITION,
	SELECT_MAX,
	SELECT_REQ_GUI_SIGNALS = SELECT_MAX,
	SELECT_MAX_GUI_SIGNALS = SELECT_MAX
};
void SYNTHCALL SELECT_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EVENTSIGNAL_INPUT
{
	EVENTSIGNAL_IN = 0,
	EVENTSIGNAL_MAX,
	EVENTSIGNAL_REQ_GUI_SIGNALS = 1,
	EVENTSIGNAL_MAX_GUI_SIGNALS = EVENTSIGNAL_MAX
};
void SYNTHCALL EVENTSIGNAL_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum PROCESS_INPUT
{
	PROCESS_IN = 0,
	PROCESS_CONDITION,
	PROCESS_MAX,
	PROCESS_REQ_GUI_SIGNALS = PROCESS_MAX,
	PROCESS_MAX_GUI_SIGNALS = PROCESS_MAX
};
void SYNTHCALL PROCESS_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MIDISIGNAL_INPUT
{
	MIDISIGNAL_SCALE=0,
	MIDISIGNAL_MODE,
	MIDISIGNAL_MAX,
	MIDISIGNAL_REQ_GUI_SIGNALS = 0,
	MIDISIGNAL_MAX_GUI_SIGNALS = 0
};
void SYNTHCALL MIDISIGNAL_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum DISTORTION_INPUT
{
	DISTORTION_IN=0,
	DISTORTION_DRIVE,		
	DISTORTION_THRESHOLD,		
	DISTORTION_MODE,
	DISTORTION_MAX,
	DISTORTION_REQ_GUI_SIGNALS = 1,
	DISTORTION_MAX_GUI_SIGNALS = DISTORTION_MODE
};
void SYNTHCALL DISTORTION_tick(SynthNode* n);
enum DISTORTION_MODE
{
	DISTORTION_OVERDRIVE = 0,
	DISTORTION_DAMP,
	DISTORTION_QUANT,
	DISTORTION_LIMIT,
	DISTORTION_ASYM,
	DISTORTION_SIN
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum CROSSMIX_INPUT
{
	CROSSMIX_IN=0,
	CROSSMIX_MIX,
	CROSSMIX_MAX,
	CROSSMIX_REQ_GUI_SIGNALS = 1,
	CROSSMIX_MAX_GUI_SIGNALS = CROSSMIX_MAX
};
void SYNTHCALL CROSSMIX_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum COMBDELAY_INPUT
{
	COMBDELAY_IN=0,
	COMBDELAY_TIME,
	COMBDELAY_MODE,
	COMBDELAY_MAX,
	COMBDELAY_REQ_GUI_SIGNALS = 1,
	COMBDELAY_MAX_GUI_SIGNALS = COMBDELAY_MODE
};
void SYNTHCALL DELAY_init(SynthNode* n);
void SYNTHCALL COMBDELAY_tick(SynthNode* n);
enum DELAY_MODE
{
	DELAY_BPMSYNC = 0,
	DELAY_SHORT,
	DELAY_MIDDLE,
	DELAY_LONG,
	DELAY_NOTEMAP,
	DELAY_NOTEMAP2,
	DELAY_TIMEMASK = 0x0f,
	DELAY_CROSS = 0x80
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum DELAY_INPUT
{
	DELAY_IN=0,
	DELAY_TIME,
	DELAY_MODE,
	DELAY_MAX,
	DELAY_REQ_GUI_SIGNALS = 1,
	DELAY_MAX_GUI_SIGNALS = DELAY_MODE
};
void SYNTHCALL DELAY_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum FBDELAY_INPUT
{
	FBDELAY_IN=0,
	FBDELAY_TIME,
	FBDELAY_FEEDBACK,
	FBDELAY_DAMP,
	FBDELAY_MIX,
	FBDELAY_MODE,
	FBDELAY_MAX,
	FBDELAY_REQ_GUI_SIGNALS = 1,
	FBDELAY_MAX_GUI_SIGNALS = FBDELAY_MODE
};
void SYNTHCALL FBDELAY_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum DCFILTER_INPUT
{
	DCFILTER_IN=0,
	DCFILTER_POLE,
	DCFILTER_MAX,
	DCFILTER_REQ_GUI_SIGNALS = 1,
	DCFILTER_MAX_GUI_SIGNALS = DCFILTER_MAX
};
void SYNTHCALL DCFILTER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum OSRAND_INPUT
{
	OSRAND_SCALE = 0,
	OSRAND_MAX,
	OSRAND_REQ_GUI_SIGNALS = 0,
	OSRAND_MAX_GUI_SIGNALS = 0
};
void SYNTHCALL OSRAND_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum REVERB_INPUT
{
	REVERB_IN = 0,
	REVERB_GAIN,
	REVERB_ROOMSIZE,
	REVERB_DAMP,	
	REVERB_MIX,
	REVERB_WIDTH,
	REVERB_MAX,
	REVERB_REQ_GUI_SIGNALS = 1,
	REVERB_MAX_GUI_SIGNALS = REVERB_MAX
};
void SYNTHCALL REVERB_init(SynthNode* n);
void SYNTHCALL REVERB_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EQ_INPUT
{
	EQ_IN = 0,
	EQ_B1,
	EQ_B2,
	EQ_B3,
	EQ_B4,
	EQ_B5,
	EQ_B6,
	EQ_B7,
	EQ_B8,
	EQ_B9,
	EQ_B10,
	EQ_MAX,
	EQ_REQ_GUI_SIGNALS = 1,
	EQ_MAX_GUI_SIGNALS = EQ_MAX
};
void SYNTHCALL EQ_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum COMPEXP_INPUT
{
	COMPEXP_IN = 0,
	COMPEXP_CONTROL,
	COMPEXP_THRESHOLD,
	COMPEXP_RATIO,
	COMPEXP_ATTACK,	
	COMPEXP_RELEASE,
	COMPEXP_MAX,
	COMPEXP_REQ_GUI_SIGNALS = 2,
	COMPEXP_MAX_GUI_SIGNALS = COMPEXP_MAX
};
void SYNTHCALL COMPEXP_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum TRIGGER_INPUT
{
	TRIGGER_IN = 0,
	TRIGGER_MAX,
	TRIGGER_REQ_GUI_SIGNALS = TRIGGER_MAX,
	TRIGGER_MAX_GUI_SIGNALS = TRIGGER_MAX
};
void SYNTHCALL TRIGGER_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum TRIGGERSEQ_INPUT
{
	TRIGGERSEQ_STEP,
	TRIGGERSEQ_RESETSTEP,
	TRIGGERSEQ_RESETPATTERN,
	TRIGGERSEQ_PATTERN,
	TRIGGERSEQ_MODE,
	TRIGGERSEQ_PATTERN0_3L,
	TRIGGERSEQ_PATTERN4_7L,
	TRIGGERSEQ_PATTERN8_11L,
	TRIGGERSEQ_PATTERN12_15L,
	TRIGGERSEQ_PATTERN0_3R,
	TRIGGERSEQ_PATTERN4_7R,
	TRIGGERSEQ_PATTERN8_11R,
	TRIGGERSEQ_PATTERN12_15R,	
	TRIGGERSEQ_MAX,
	TRIGGERSEQ_REQ_GUI_SIGNALS = 4,
	TRIGGERSEQ_MAX_GUI_SIGNALS = TRIGGERSEQ_MODE
};
void SYNTHCALL TRIGGERSEQ_tick(SynthNode* n);
enum TRIGGERSEQ_MODE
{
	TRIGGERSEQ_COUNTMASK	= 0x000000ff,	
	TRIGGERSEQ_BPMMASK		= 0x0000ff00,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SAMPLEREC_INPUT
{
	SAMPLEREC_IN,
	SAMPLEREC_MODE,
	SAMPLEREC_MAX,
	SAMPLEREC_REQ_GUI_SIGNALS = 1,
	SAMPLEREC_MAX_GUI_SIGNALS = SAMPLEREC_MODE
};
void SYNTHCALL SAMPLEREC_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SAMPLER_INPUT
{
	SAMPLER_IN,
	SAMPLER_POSITION,
	SAMPLER_SPEED,
	SAMPLER_DIRECTION,
	SAMPLER_LOOPSTART,
	SAMPLER_LOOPEND,
	SAMPLER_CROSSFADE,
	SAMPLER_MODE,
	SAMPLER_MAX,
	SAMPLER_REQ_GUI_SIGNALS = 1,
	SAMPLER_MAX_GUI_SIGNALS = SAMPLER_MODE
};
void SYNTHCALL SAMPLER_tick(SynthNode* n);
enum SAMPLER_MODE
{
	SAMPLER_MODEMASK	= 0x00000007,	
	SAMPLER_STORED		= 0x00000000,
	SAMPLER_INPUT		= 0x00000001,
	SAMPLER_LOOP		= 0x00000008,
	SAMPLER_SRCMASK		= 0x00000ff0,
	SAMPLER_SRCSHIFT	= 4,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SVFILTER_INPUT
{
	SVFILTER_IN = 0,		// input signal	
	SVFILTER_FREQ,			// frequency
	SVFILTER_Q,				// q
	SVFILTER_MODE,			// type of filter and stages
	SVFILTER_MAX,
	SVFILTER_REQ_GUI_SIGNALS = 1,
	SVFILTER_MAX_GUI_SIGNALS = SVFILTER_MODE
};
void SYNTHCALL SVFILTER_tick(SynthNode* n);
enum SVFILTER_MODE
{
	SVFILTER_LOWPASS	= 0x00000001,
	SVFILTER_HIGHPASS	= 0x00000002,
	SVFILTER_BANDPASS	= 0x00000004,
	SVFILTER_PEAK		= 0x00000008,
	SVFILTER_MODE_MAX,
	SVFILTER_MODEMASK	= 0x0000000f,
	SVFILTER_NOTEMAP	= 0x00000080,
	SVFILTER_QUADRATIC	= 0x00000040,
	SVFILTER_STAGEMASK	= 0x00000f00,
	SVFILTER_STAGESHIFT = 8
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ABS_INPUT
{
	ABS_IN = 0,
	ABS_MAX,
	ABS_REQ_GUI_SIGNALS = ABS_MAX,
	ABS_MAX_GUI_SIGNALS = ABS_MAX
};
void SYNTHCALL ABS_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum NEG_INPUT
{
	NEG_IN = 0,
	NEG_MAX,
	NEG_REQ_GUI_SIGNALS = NEG_MAX,
	NEG_MAX_GUI_SIGNALS = NEG_MAX
};
void SYNTHCALL NEG_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SQRT_INPUT
{
	SQRT_IN = 0,					
	SQRT_MAX,
	SQRT_REQ_GUI_SIGNALS = SQRT_MAX,
	SQRT_MAX_GUI_SIGNALS = SQRT_MAX
};
void SYNTHCALL SQRT_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MIN_INPUT
{
	MIN_IN1 = 0,
	MIN_IN2,
	MIN_MAX,
	MIN_REQ_GUI_SIGNALS = MIN_MAX,
	MIN_MAX_GUI_SIGNALS = MIN_MAX
};
void SYNTHCALL MIN_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MAX_INPUT
{
	MAX_IN1 = 0,
	MAX_IN2,
	MAX_MAX,
	MAX_REQ_GUI_SIGNALS = MAX_MAX,
	MAX_MAX_GUI_SIGNALS = MAX_MAX
};
void SYNTHCALL MAX_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum GLITCH_INPUT
{
	GLITCH_IN = 0,
	GLITCH_ACTIVE,
	GLITCH_TIME,
	GLITCH_P1, 
	GLITCH_P2,	
	GLITCH_SPEED,
	GLITCH_MODE,
	GLITCH_MAX,
	GLITCH_REQ_GUI_SIGNALS = 1,
	GLITCH_MAX_GUI_SIGNALS = GLITCH_MODE,
	
	GLITCH_TSSLOWDOWN = GLITCH_P1,
	GLITCH_TSSPEEDUP = GLITCH_P2,
	
	GLITCH_RTLENGTH = GLITCH_P1,
	GLITCH_RTSPEEDPITCH = GLITCH_P2,

	GLITCH_SHBACKRANGESEED = GLITCH_P1,
	GLITCH_SHREPEATAMOUNT = GLITCH_P2,
};
void SYNTHCALL GLITCH_tick(SynthNode* n);
void SYNTHCALL GLITCH_init(SynthNode* n);
enum GLITCH_MODE
{
	GLITCH_TAPESTOP		= 0x00,	
	GLITCH_RETRIGGER	= 0x01,
	GLITCH_SHUFFLE		= 0x02,
	GLITCH_REVERSE		= 0x03,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SAPI_INPUT
{
	SAPI_MAX,
	SAPI_REQ_GUI_SIGNALS = 0,
	SAPI_MAX_GUI_SIGNALS = SAPI_MAX,
};
void SYNTHCALL SAPI_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern DWORD		GMDLS_NumSamples[512];
extern sample_t*	GMDLS_SampleBuffer[512];

enum GMDLS_INPUT
{
	GMDLS_MODE,
	GMDLS_MAX,
	GMDLS_REQ_GUI_SIGNALS = 0,
	GMDLS_MAX_GUI_SIGNALS = 0,
};
void SYNTHCALL GMDLS_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum BOWED_INPUT
{
	BOWED_POSITION,
	BOWED_PRESSURE,
	BOWED_VELOCITY,
	BOWED_VIBRATO,
	BOWED_FRICTIONSYM,
	BOWED_MAX,
	BOWED_REQ_GUI_SIGNALS = 0,
	BOWED_MAX_GUI_SIGNALS = BOWED_MAX,
};
void SYNTHCALL BOWED_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum FORMULA_INPUT
{
	FORMULA_IN0,
	FORMULA_IN1,
	FORMULA_MAX,
	FORMULA_REQ_GUI_SIGNALS = FORMULA_MAX,
	FORMULA_MAX_GUI_SIGNALS = FORMULA_MAX,
};
void SYNTHCALL FORMULA_tick(SynthNode* n);
enum FORUMLA_CMD
{	
// get constant (stack++ op)
	FORMULA_CMD_CONSTANT = 0,
	// stack-- ops
	FORMULA_CMD_ADD,
	FORMULA_CMD_MUL,
	FORMULA_CMD_E,
	FORMULA_CMD_NE,
	FORMULA_CMD_GT,
	FORMULA_CMD_GTE,
	FORMULA_CMD_AND,
	FORMULA_CMD_OR,
	FORMULA_CMD_LT,
	FORMULA_CMD_LTE,
	FORMULA_CMD_SUB,
	FORMULA_CMD_DIV,
	FORMULA_CMD_MIN,
	FORMULA_CMD_MAX,
	FORMULA_CMD_MOD,
	FORMULA_CMD_TRISAW,
	FORMULA_CMD_PULSE,
	FORMULA_CMD_LERP,
	FORMULA_CMD_IFTHEN,
	FORMULA_CMD_MAXTIME,
	FORMULA_DEC_STACK = FORMULA_CMD_MAXTIME,
	// stack processing ops
	FORMULA_CMD_NEG,
	FORMULA_CMD_ABS,
	FORMULA_CMD_SQRT,
	FORMULA_CMD_CEIL,
	FORMULA_CMD_FLOOR,
	FORMULA_CMD_SQR,
	FORMULA_CMD_COS,
	FORMULA_CMD_SIN,
	FORMULA_CMD_EXP2,
	FORMULA_CMD_LOG2,
	FORMULA_KEEP_STACK = FORMULA_CMD_LOG2,
	// stack++ ops
	FORMULA_CMD_RAND,
	FORMULA_CMD_PI,
	FORMULA_CMD_TAU,
	FORMULA_CMD_TAUTIME,
	FORMULA_CMD_IN0,
	FORMULA_CMD_IN1,
	FORMULA_CMD_VFREQUENCY,
	FORMULA_CMD_VNOTE,
	FORMULA_CMD_VVELOCITY,
	FORMULA_CMD_VTRIGGER,
	FORMULA_CMD_VGATE,
	FORMULA_CMD_VAFTERTOUCH,
// variables v[0] = out, v[1] = time, v[2] = a, ..., v[23] = v
// get variable (access n->v[])
	FORMULA_CMD_GETVAR = 64,
// set var (access n->v[])
	FORMULA_CMD_SETVAR = 96,
	FORMULA_DONE = 128,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SNH_INPUT
{
	SNH_IN = 0,
	SNH_SNH,
	SNH_SNHSMOOTH,
	SNH_MODE,
	SNH_MAX,
	SNH_REQ_GUI_SIGNALS = 1,
	SNH_MAX_GUI_SIGNALS = SNH_MODE
};
void SYNTHCALL SNH_tick(SynthNode* n);
enum SNH_MODE
{
	SNH_TRIGGER = 0x00000010,
	SNH_QUADRATIC = 0x00000040,
	SNH_NOTEMAP = 0x00000080,	
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum WTFOSC_INPUT
{
	WTFOSC_IN,
	WTFOSC_POSITION,
	WTFOSC_SPEED,
	WTFOSC_TARGETSEARCHLEN,
	WTFOSC_COUNTSKIP,
	WTFOSC_MODE,
	WTFOSC_MAX,
	WTFOSC_REQ_GUI_SIGNALS = 1,
	WTFOSC_MAX_GUI_SIGNALS = WTFOSC_MODE
};
void SYNTHCALL WTFOSC_tick(SynthNode* n);
enum WTFOSC_MODE
{
	WTFOSC_MODEMASK		= 0x00000007,
	WTFOSC_STORED		= 0x00000000,
	WTFOSC_INPUT		= 0x00000001,	
	WTFOSC_SRCMASK		= 0x00000ff0,
	WTFOSC_SRCSHIFT		= 4,
	WTFOSC_REDUCECLICK	= 0x00001000,
	WTFOSC_FIXEDGRAINS	= 0x00002000
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum FORMANT_INPUT
{
	FORMANT_IN,
	FORMANT_GAIN,
	FORMANT_MODE,
	FORMANT_MAX,
	FORMANT_REQ_GUI_SIGNALS = 1,
	FORMANT_MAX_GUI_SIGNALS = FORMANT_MODE
};
void SYNTHCALL FORMANT_tick(SynthNode* n);
enum FORMANT_MODE
{
	FORMANT_SOPRANO_A = 0,
	FORMANT_SOPRANO_E,
	FORMANT_SOPRANO_I,
	FORMANT_SOPRANO_O,
	FORMANT_SOPRANO_U,
	FORMANT_ALTO_A,
	FORMANT_ALTO_E,
	FORMANT_ALTO_I,
	FORMANT_ALTO_O,
	FORMANT_ALTO_U,
	FORMANT_TENOR_A,
	FORMANT_TENOR_E,
	FORMANT_TENOR_I,
	FORMANT_TENOR_O,
	FORMANT_TENOR_U,
	FORMANT_COUNTERTENOR_A,
	FORMANT_COUNTERTENOR_E,
	FORMANT_COUNTERTENOR_I,
	FORMANT_COUNTERTENOR_O,
	FORMANT_COUNTERTENOR_U,
	FORMANT_BASS_A,
	FORMANT_BASS_E,
	FORMANT_BASS_I,
	FORMANT_BASS_O,
	FORMANT_BASS_U,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EQ3_INPUT
{
	EQ3_IN,
	EQ3_LGAIN,
	EQ3_MGAIN,
	EQ3_HGAIN,
	EQ3_MAX,
	EQ3_REQ_GUI_SIGNALS = 1,
	EQ3_MAX_GUI_SIGNALS = EQ3_MAX
};
void SYNTHCALL EQ3_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum OSCSYNC_INPUT
{
	OSCSYNC_IN,
	OSCSYNC_OSC,
	OSCSYNC_MAX,
	OSCSYNC_REQ_GUI_SIGNALS = 2,
	OSCSYNC_MAX_GUI_SIGNALS = OSCSYNC_MAX
};
void SYNTHCALL OSCSYNC_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SYNTHCALL CONSTANT_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum VOICEPARAM_INPUT
{
	VOICEPARAM_SCALE = 0,
	VOICEPARAM_MAX,
	VOICEPARAM_REQ_GUI_SIGNALS = 0,
	VOICEPARAM_MAX_GUI_SIGNALS = 0
};
void SYNTHCALL VOICEPARAM_tick(SynthNode* n);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// global synth info struct
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef COMPILE_VSTI
#define SYNTH_WORD DWORD
#else
#define SYNTH_WORD WORD
#endif
#define NODEINFO_GLOBAL 0x8000 

struct SynthGlobalStateStruct
{
	// the current tick for sample processing
	DWORD		CurrentTick;
	// the raw input data for the synth used for creating the node hierarchy
	SYNTH_WORD* NodeValues;
	// array (same dimension as NodeValues) storing the tick at which an node was created to ensure only one time creation (voice level)
	DWORD*		CreatedNodeTicks;
	// array (same dimension as NodeValues) storing the global nodes to ensure only one time creation (global level)
	SynthNode**	GlobalNodes;
	// the current tick for creating nodes
	DWORD		CreateNodeTick;	
	// the current voice
	VMVoice*	CurrentVoice;
	// the offset where the constants start
	DWORD		ConstantOffset;
	// the end offset for constants
	DWORD		MaxOffset;
	// current BPM used
	sample_t	CurrentBPM;
	// array for all channels midi controller values
	sample_t	MidiSignals[16*128];
	// array for all channels aftertouch values (current state)
	sample_t	NoteAftertouch[16*128];			
	// array of raw wave tables filled with the uncompressed stored samples provided. first sample is the buffer size
	sample_t*	RawWaveTable[32]; // 32 slots for wavetables
#ifdef COMPILE_VSTI
	int			RawWaveFormat[32]; // fromats for the rawwavetables
	int			RawWaveFrequency[32]; // frequencies for the rawwavetables
	std::string	RawWaveFileName[32]; // full path of filenames used as source for the rawwavetables
	int			RawWavePackedSize[32];
	int			CompSampleRate[32];
	int			CompAvgBytes[32];
	int			CompWaveTableSize[32];
	BYTE*		CompWaveTable[32];
	void**		SpecialDataPointer;
#endif	
	// the song stream pointer
	BYTE*		SongStream;
	// the current noteon tick
	DWORD		CurrentNoteTick;
	
};
extern SynthGlobalStateStruct SynthGlobalState;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// node hierarhy create/delete functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthNode* CreateNode(DWORD id, DWORD refnodes, DWORD offset, DWORD isGlobal);
SynthNode* CreateNodes(DWORD offset, BOOL global);
void DestroyVoiceNodes(SynthNode* node);

#endif