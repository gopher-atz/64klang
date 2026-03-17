#include "SynthController.h"
#include "Synth.h"
#include "SynthNode.h"
#include "SynthAllocator.h"

#ifdef _WIN32
#include <mmreg.h>
#include <msacm.h>
#include <wmsdk.h>
#pragma comment(lib, "msacm32.lib")
#pragma comment(lib, "wmvcore.lib")
#endif

#include "tinyxml.h"
#include <fstream>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// vsti specific
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define	NODE_SLOTS	(NODE_MAX_INPUTS+1)
#define CONSTANT_0	((MAX_NODES-1)*NODE_SLOTS)

SynthController*	SynthController::_instance = 0;
K64_API void*				SynthController::ModuleInstance = 0;
std::timed_mutex			SynthController::DataAccessMutex;

// id based total number of inputs for a node
DWORD NodeInputs[MAXIMUM_ID] =
{
	SYNTHROOT_MAX,
	CHANNELROOT_MAX,
	NOTECONTROLLER_MAX,
	VOICEMANAGER_MAX,
	VOICEROOT_MAX,
	ADSR_MAX,
	LFO_MAX,
	OSCILLATOR_MAX,
	NOISEGEN_MAX,
	BQFILTER_MAX,
	ONEPOLEFILTER_MAX,
	ONEZEROFILTER_MAX,
	SHAPER_MAX,
	PANNING_MAX,
	SCALE_MAX,
	MUL_MAX,
	DIV_MAX,
	ADD_MAX,
	SUB_MAX,
	CLIP_MAX,
	MIX_MAX,
	MULTIADD_MAX,
	MONO_MAX,
	ENVFOLLOWER_MAX,
	LOGIC_MAX,
	COMPARE_MAX,
	SELECT_MAX,
	EVENTSIGNAL_MAX,
	PROCESS_MAX,
	MIDISIGNAL_MAX,
	DISTORTION_MAX,
	CROSSMIX_MAX,
	DELAY_MAX,
	FBDELAY_MAX,
	DCFILTER_MAX,
	OSRAND_MAX,
	REVERB_MAX,
	EQ_MAX,
	COMPEXP_MAX,
	TRIGGER_MAX,
	TRIGGERSEQ_MAX,
	SAMPLEREC_MAX,
	SAMPLER_MAX,
	SVFILTER_MAX,
	ABS_MAX,
	NEG_MAX,
	SQRT_MAX,
	MIN_MAX,
	MAX_MAX,
	GLITCH_MAX,
	SAPI_MAX,
	COMBDELAY_MAX,
	GMDLS_MAX,
	BOWED_MAX,
	FORMULA_MAX,
	SNH_MAX,
	WTFOSC_MAX,
	FORMANT_MAX,
	EQ3_MAX,
	OSCSYNC_MAX,
	// reserved slots
	0, 0, 0, 0,

	0, // CONSTANT_MAX

	VOICEPARAM_MAX, //VOICE_FREQUENCY_MAX,
	VOICEPARAM_MAX, //VOICE_NOTE_MAX,
	VOICEPARAM_MAX, //VOICE_ATTACKVELOCITY_MAX,
	VOICEPARAM_MAX, //VOICE_TRIGGER_MAX,
	VOICEPARAM_MAX, //VOICE_GATE_MAX,
	VOICEPARAM_MAX, //VOICE_RELEASEVELOCITY_MAX,
};

// id based number of required connections (gui side) for a node
DWORD NodeReqGUISignals[MAXIMUM_ID] =
{
	SYNTHROOT_REQ_GUI_SIGNALS,
	CHANNELROOT_REQ_GUI_SIGNALS,
	NOTECONTROLLER_REQ_GUI_SIGNALS,
	VOICEMANAGER_REQ_GUI_SIGNALS,
	VOICEROOT_REQ_GUI_SIGNALS,
	ADSR_REQ_GUI_SIGNALS,
	LFO_REQ_GUI_SIGNALS,
	OSCILLATOR_REQ_GUI_SIGNALS,
	NOISEGEN_REQ_GUI_SIGNALS,
	BQFILTER_REQ_GUI_SIGNALS,
	ONEPOLEFILTER_REQ_GUI_SIGNALS,
	ONEZEROFILTER_REQ_GUI_SIGNALS,
	SHAPER_REQ_GUI_SIGNALS,
	PANNING_REQ_GUI_SIGNALS,
	SCALE_REQ_GUI_SIGNALS,
	MUL_REQ_GUI_SIGNALS,
	DIV_REQ_GUI_SIGNALS,
	ADD_REQ_GUI_SIGNALS,
	SUB_REQ_GUI_SIGNALS,
	CLIP_REQ_GUI_SIGNALS,
	MIX_REQ_GUI_SIGNALS,
	MULTIADD_REQ_GUI_SIGNALS,
	MONO_REQ_GUI_SIGNALS,
	ENVFOLLOWER_REQ_GUI_SIGNALS,
	LOGIC_REQ_GUI_SIGNALS,
	COMPARE_REQ_GUI_SIGNALS,
	SELECT_REQ_GUI_SIGNALS,
	EVENTSIGNAL_REQ_GUI_SIGNALS,
	PROCESS_REQ_GUI_SIGNALS,
	MIDISIGNAL_REQ_GUI_SIGNALS,
	DISTORTION_REQ_GUI_SIGNALS,
	CROSSMIX_REQ_GUI_SIGNALS,
	DELAY_REQ_GUI_SIGNALS,
	FBDELAY_REQ_GUI_SIGNALS,
	DCFILTER_REQ_GUI_SIGNALS,
	OSRAND_MAX_GUI_SIGNALS,
	REVERB_REQ_GUI_SIGNALS,
	EQ_REQ_GUI_SIGNALS,
	COMPEXP_REQ_GUI_SIGNALS,
	TRIGGER_REQ_GUI_SIGNALS,
	TRIGGERSEQ_REQ_GUI_SIGNALS,
	SAMPLEREC_REQ_GUI_SIGNALS,
	SAMPLER_REQ_GUI_SIGNALS,
	SVFILTER_REQ_GUI_SIGNALS,
	ABS_REQ_GUI_SIGNALS,
	NEG_REQ_GUI_SIGNALS,
	SQRT_REQ_GUI_SIGNALS,
	MIN_REQ_GUI_SIGNALS,
	MAX_REQ_GUI_SIGNALS,
	GLITCH_REQ_GUI_SIGNALS,
	SAPI_REQ_GUI_SIGNALS,
	COMBDELAY_REQ_GUI_SIGNALS,
	GMDLS_REQ_GUI_SIGNALS,
	BOWED_REQ_GUI_SIGNALS,
	FORMULA_REQ_GUI_SIGNALS,
	SNH_REQ_GUI_SIGNALS,
	WTFOSC_REQ_GUI_SIGNALS,
	FORMANT_REQ_GUI_SIGNALS,
	EQ3_REQ_GUI_SIGNALS,
	OSCSYNC_REQ_GUI_SIGNALS,
	// reserved slots
	0, 0, 0, 0,

	0, // CONSTANT_REQ_GUI_SIGNALS

	VOICEPARAM_REQ_GUI_SIGNALS, //VOICE_FREQUENCY_MAX,
	VOICEPARAM_REQ_GUI_SIGNALS, //VOICE_NOTE_MAX,
	VOICEPARAM_REQ_GUI_SIGNALS, //VOICE_ATTACKVELOCITY_MAX,
	VOICEPARAM_REQ_GUI_SIGNALS, //VOICE_TRIGGER_MAX,
	VOICEPARAM_REQ_GUI_SIGNALS, //VOICE_GATE_MAX,
	VOICEPARAM_REQ_GUI_SIGNALS, //VOICE_RELEASEVELOCITY_MAX,
};

// id based number of maximum connections you can add in the gui
DWORD NodeMaxGUISignals[MAXIMUM_ID] =
{
	SYNTHROOT_MAX_GUI_SIGNALS,
	CHANNELROOT_MAX_GUI_SIGNALS,
	NOTECONTROLLER_MAX_GUI_SIGNALS,
	VOICEMANAGER_MAX_GUI_SIGNALS,
	VOICEROOT_MAX_GUI_SIGNALS,
	ADSR_MAX_GUI_SIGNALS,
	LFO_MAX_GUI_SIGNALS,
	OSCILLATOR_MAX_GUI_SIGNALS,
	NOISEGEN_MAX_GUI_SIGNALS,
	BQFILTER_MAX_GUI_SIGNALS,
	ONEPOLEFILTER_MAX_GUI_SIGNALS,
	ONEZEROFILTER_MAX_GUI_SIGNALS,
	SHAPER_MAX_GUI_SIGNALS,
	PANNING_MAX_GUI_SIGNALS,
	SCALE_MAX_GUI_SIGNALS,
	MUL_MAX_GUI_SIGNALS,
	DIV_MAX_GUI_SIGNALS,
	ADD_MAX_GUI_SIGNALS,
	SUB_MAX_GUI_SIGNALS,
	CLIP_MAX_GUI_SIGNALS,
	MIX_MAX_GUI_SIGNALS,
	MULTIADD_MAX_GUI_SIGNALS,
	MONO_MAX_GUI_SIGNALS,
	ENVFOLLOWER_MAX_GUI_SIGNALS,
	LOGIC_MAX_GUI_SIGNALS,
	COMPARE_MAX_GUI_SIGNALS,
	SELECT_MAX_GUI_SIGNALS,
	EVENTSIGNAL_MAX_GUI_SIGNALS,
	PROCESS_MAX_GUI_SIGNALS,
	MIDISIGNAL_MAX_GUI_SIGNALS,
	DISTORTION_MAX_GUI_SIGNALS,
	CROSSMIX_MAX_GUI_SIGNALS,
	DELAY_MAX_GUI_SIGNALS,
	FBDELAY_MAX_GUI_SIGNALS,
	DCFILTER_MAX_GUI_SIGNALS,
	OSRAND_MAX_GUI_SIGNALS,
	REVERB_MAX_GUI_SIGNALS,
	EQ_MAX_GUI_SIGNALS,
	COMPEXP_MAX_GUI_SIGNALS,
	TRIGGER_MAX_GUI_SIGNALS,
	TRIGGERSEQ_MAX_GUI_SIGNALS,
	SAMPLEREC_MAX_GUI_SIGNALS,
	SAMPLER_MAX_GUI_SIGNALS,
	SVFILTER_MAX_GUI_SIGNALS,
	ABS_MAX_GUI_SIGNALS,
	NEG_MAX_GUI_SIGNALS,
	SQRT_MAX_GUI_SIGNALS,
	MIN_MAX_GUI_SIGNALS,
	MAX_MAX_GUI_SIGNALS,
	GLITCH_MAX_GUI_SIGNALS,
	SAPI_MAX_GUI_SIGNALS,
	COMBDELAY_MAX_GUI_SIGNALS,
	GMDLS_MAX_GUI_SIGNALS,
	BOWED_MAX_GUI_SIGNALS,
	FORMULA_MAX_GUI_SIGNALS,
	SNH_MAX_GUI_SIGNALS,
	WTFOSC_MAX_GUI_SIGNALS,
	FORMANT_MAX_GUI_SIGNALS,
	EQ3_MAX_GUI_SIGNALS,
	OSCSYNC_MAX_GUI_SIGNALS,
	// reserved slots
	0, 0, 0, 0,

	0, // CONSTANT_MAX_GUI_SIGNALS

	VOICEPARAM_MAX_GUI_SIGNALS, //VOICE_FREQUENCY_MAX,
	VOICEPARAM_MAX_GUI_SIGNALS, //VOICE_NOTE_MAX,
	VOICEPARAM_MAX_GUI_SIGNALS, //VOICE_ATTACKVELOCITY_MAX,
	VOICEPARAM_MAX_GUI_SIGNALS, //VOICE_TRIGGER_MAX,
	VOICEPARAM_MAX_GUI_SIGNALS, //VOICE_GATE_MAX,
	VOICEPARAM_MAX_GUI_SIGNALS, //VOICE_RELEASEVELOCITY_MAX,
};

// id based total number of inputs for a node
char* NodeNames[MAXIMUM_ID] =
{
	"SYNTHROOT",
	"CHANNELROOT",
	"NOTECONTROLLER",
	"VOICEMANAGER",
	"VOICEROOT",
	"ADSR",
	"LFO",
	"OSCILLATOR",
	"NOISEGEN",
	"BQFILTER",
	"ONEPOLEFILTER",
	"ONEZEROFILTER",
	"SHAPER",
	"PANNING",
	"SCALE",
	"MUL",
	"DIV",
	"ADD",
	"SUB",
	"CLIP",
	"MIX",
	"MULTIADD",
	"MONO",
	"ENVFOLLOWER",
	"LOGIC",
	"COMPARE",
	"SELECT",
	"EVENTSIGNAL",
	"PROCESS",
	"MIDISIGNAL",
	"DISTORTION",
	"CROSSMIX",
	"DELAY",
	"FBDELAY",
	"DCFILTER",
	"OSRAND",
	"REVERB",
	"EQ",
	"COMPEXP",
	"TRIGGER",
	"TRIGGERSEQ",
	"SAMPLEREC",
	"SAMPLER",
	"SVFILTER",
	"ABS",
	"NEG",
	"SQRT",
	"MIN",
	"MAX",
	"GLITCH",
	"SAPI",
	"COMBDELAY",
	"GMDLS",
	"BOWED",
	"FORMULA",
	"SNH",
	"WTFOSC",
	"FORMANT",
	"EQ3",
	"OSCSYNC",
	// reserved slots
	"R0", "R1", "R2", "R3",

	"CONSTANT", // CONSTANT_MAX

	"VOICE_FREQUENCY",
	"VOICE_NOTE",
	"VOICE_ATTACKVELOCITY",
	"VOICE_TRIGGER",
	"VOICE_GATE",
	"VOICE_RELEASEVELOCITY",
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthController* SynthController::instance()
{
	if (_instance == 0)
		_instance = new SynthController();
	return _instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthController::SynthController() : _nodes(0)
{
	_64klang_Init(NULL, NULL, 0, 0, MAX_NODES*NODE_SLOTS);
	_nodes = (SynthNode**)SynthMalloc(MAX_NODES*sizeof(SynthNode*));
	// init free nodes index list
	for (DWORD i = 0; i < MAX_NODES; i++)
		_freeSlots[i] = 1;
	// reset patch to default
	resetPatch(true, false);

	if (!checkCPUSupport())
	{
		fprintf(stderr, "64klang: %s\n", "Your CPU does not support SSE4.1 instructions\n64klang will not work without!");
		_initialized = false;
	}
	else
	{
		_initialized = true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthController::~SynthController()
{
	SynthFree(_nodes);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthNode* SynthController::createNode(DWORD id, DWORD channel, DWORD isGlobal)
{
	// find a free slot
	DWORD slot;
	std::map<DWORD, DWORD>::iterator it;
	// constants start at the end
	if (id == CONSTANT_ID && isGlobal)
	{
		isGlobal = 1; // just in case
		std::map<DWORD, DWORD>::reverse_iterator cit = _freeSlots.rbegin();
		slot = cit->first;
		it = _freeSlots.find(slot);
	}
	// rest of the nodes start at the beginning
	else
	{
		// synthroot (mul) must always slot 0
		if (id == SYNTHROOT_ID)
			it = _freeSlots.find(0);
		// channelroot must always get slot 1-16
		else if (id == CHANNELROOT_ID)
			it = _freeSlots.find(1+channel);
		// notecontroller must always get slot 17-32
		else if (id == NOTECONTROLLER_ID)
			it = _freeSlots.find(1+16+channel);
		// other node types may just use the next free slot
		else
		{
			it = _freeSlots.begin();
			while (it->first < (1+16+16)) it++;
		}

		slot = it->first;
	}
	// mark slot as used
	_freeSlots.erase(it);

	// create the node
	DWORD offset = slot*NODE_SLOTS;
	_nodes[slot] = CreateNode(id, NodeInputs[id], offset, isGlobal);

	// set core values
	SynthGlobalState.NodeValues[offset] = INFO(id, NodeInputs[id], isGlobal);
	// always put the node in the global nodes map
	SynthGlobalState.GlobalNodes[offset] = _nodes[slot];

	NodeGUIInfo info;

	// set all inputs to default 0
	for (int i = 0; i < NODE_MAX_INPUTS; i++)
	{
		info.ModAdder[i] = 0;
		_nodes[slot]->input[i] = (sample_t*)constant0();
		int ofsconstz = CONSTANT_0;
		SynthGlobalState.NodeValues[++offset] = ofsconstz;
	}

	info.Node = _nodes[slot];
	// set fixed channel
	if (id == SYNTHROOT_ID)
		info.FixedChannel = -1;
	else
		info.FixedChannel = channel;
	info.Outputs.clear();
	_nodeGUIInfo[slot*NODE_SLOTS] = info;

	return _nodes[slot];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SETCONSTANT(x, y) ((SynthNode*)(node->input[x]))->out = y; ((SynthNode*)(node->input[x]))->numInputs = 2; SynthGlobalState.NodeValues[((SynthNode*)(node->input[x]))->valueOffset] = INFO(CONSTANT_ID, 2, 1);
#define SETMODE(x, y) ((SynthNode*)(node->input[x]))->out = y; ((SynthNode*)(node->input[x]))->numInputs = 0; SynthGlobalState.NodeValues[((SynthNode*)(node->input[x]))->valueOffset] = INFO(CONSTANT_ID, 0, 1);

SynthNode* SynthController::createGUINode(DWORD id, DWORD channel, DWORD isGlobal, double x, double y)
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	// force global flag for some nodes
	if (id <= VOICEMANAGER_ID ||
		id == MIDISIGNAL_ID ||
		id == CONSTANT_ID)
	{
		isGlobal = 1;
	}

	// user created constant? always global channel
	if (id == CONSTANT_ID)
		channel = -1;
	// voice constants? always unassigned
	if (id > CONSTANT_ID)
		channel = -2;

	// create the requested node
	SynthNode* node = createNode(id, channel, isGlobal);

	// user created constant tag as stereo constant
	if (id == CONSTANT_ID)
		node->numInputs = 2;

	// set position in info
	_nodeGUIInfo[node->valueOffset].X = x;
	_nodeGUIInfo[node->valueOffset].Y = y;
	_nodeGUIInfo[node->valueOffset].Visible = true;

	// create the modulation parameter constants and adders
	for (DWORD i = NodeReqGUISignals[id]; i < NodeMaxGUISignals[id]; i++)
	{
		SynthNode* constant = createNode(CONSTANT_ID, -2, 1);
		// connect to the node input
		node->input[i] = (sample_t*)constant;
		SynthGlobalState.NodeValues[node->valueOffset+1+i] = constant->valueOffset;
		_nodeGUIInfo[constant->valueOffset].IsParameter = true;

		// create modulation adder and store in info for later use when connecting modulations
		SynthNode* adder = createNode(ADD_ID, -2, isGlobal);
		// connect constant to input 0 of adder and 0 to input 1
		adder->input[0] = (sample_t*)constant;
		adder->input[1] = (sample_t*)constant0();
		SynthGlobalState.NodeValues[adder->valueOffset+1+0] = constant->valueOffset;
		SynthGlobalState.NodeValues[adder->valueOffset+1+1] = constant0()->valueOffset;
		_nodeGUIInfo[adder->valueOffset].IsModAdder = true;

		_nodeGUIInfo[node->valueOffset].ModAdder[i] = adder;
	}
	// create mode constants
	bool oldMassDataUpdate = _massDataUpdate;
	_massDataUpdate = true; // prevent mutex deadlock
	for (DWORD i = NodeMaxGUISignals[id]; i < NodeInputs[id]; i++)
	{
		SynthNode* constant = createNode(CONSTANT_ID, -2, 1);
		// connect to the node input
		node->input[i] = (sample_t*)constant;
		SynthGlobalState.NodeValues[node->valueOffset+1+i] = constant->valueOffset;
		_nodeGUIInfo[constant->valueOffset].IsParameter = true;
	}
	_massDataUpdate = oldMassDataUpdate; // restore massdataupdate flag

	// Set default values for parameter and mode inputs.
	// CONSTANT_ID is special: its "value" is node->out directly (no parameter inputs).
	// For all other nodes, delegate to resetNodeToDefaults which calls setInputValue/setInputMode.
	// setInputValue/setInputMode check _massDataUpdate internally, so set it true here to
	// avoid re-acquiring the mutex we already hold.
	if (id == CONSTANT_ID)
	{
		node->out = sample_t(0.0);
	}
	else
	{
		bool saved = _massDataUpdate;
		_massDataUpdate = true;
		resetNodeToDefaults(node->valueOffset, id, isGlobal != 0);
		_massDataUpdate = saved;
	}

	if (!_massDataUpdate)
		DataAccessMutex.unlock();

	return node;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the factory-default raw normalized value for a specific input of a given node type.
// createGUINode delegates to resetNodeToDefaults which uses this function as the single source of truth.

void SynthController::getNodeInputDefault(DWORD typeID, DWORD inputIdx, bool isGlobal, double& outL, double& outR)
{
	outL = outR = 0.0;
	int idx = (int)inputIdx;

#define DEFCONST(x, val)       if (idx == (x)) { outL = outR = (double)(val); return; }
#define DEFCONST2(x, vl, vr)   if (idx == (x)) { outL = (double)(vl); outR = (double)(vr); return; }
#define DEFMODE(x, val)        if (idx == (x)) { outL = outR = (double)(int)(val); return; }

	switch (typeID)
	{
	case SYNTHROOT_ID:      DEFCONST(SYNTHROOT_GAIN, 0.5); break;
	case CHANNELROOT_ID:    DEFCONST(CHANNELROOT_GAIN, 0.5); break;
	case VOICEMANAGER_ID:
		DEFCONST(VOICEMANAGER_TRANSPOSE, 0.0);
		DEFCONST(VOICEMANAGER_GLIDE,     0.0);
		DEFCONST(VOICEMANAGER_ARPSPEED,  0.140625);
		DEFMODE (VOICEMANAGER_MODE,      0x00107f00);
		break;
	case ADSR_ID:
		DEFCONST(ADSR_ATTACK,  0.375);
		DEFCONST(ADSR_DECAY,   0.375);
		DEFCONST(ADSR_SUSTAIN, 0.5);
		DEFCONST(ADSR_RELEASE, 0.375);
		DEFCONST(ADSR_GAIN,    0.75);
		if (!isGlobal) { DEFMODE(ADSR_MODE, (int)(ADSR_VOICETRIGGER | ADSR_VOICEGATE)); }
		else           { DEFMODE(ADSR_MODE, 0); }
		break;
	case LFO_ID:
		DEFCONST(LFO_FREQ,  0.5);
		DEFCONST(LFO_PHASE, 0.0);
		DEFCONST(LFO_COLOR, 1.0);
		DEFCONST(LFO_GAIN,  1.0);
		DEFMODE (LFO_MODE,  LFO_SINE);
		break;
	case OSCILLATOR_ID:
		DEFCONST(OSCILLATOR_FREQ,      0.0);
		DEFCONST(OSCILLATOR_PHASE,     0.0);
		DEFCONST(OSCILLATOR_COLOR,     0.5);
		DEFCONST(OSCILLATOR_TRANSPOSE, 0.0);
		DEFCONST(OSCILLATOR_DETUNE,    0.0);
		DEFCONST(OSCILLATOR_GAIN,      1.0);
		DEFCONST(OSCILLATOR_UDETUNE,  -0.5);
		if (!isGlobal) {DEFMODE(OSCILLATOR_MODE, (int)(OSCILLATOR_SINE | OSCILLATOR_VOICEFREQ)); }
		else           {DEFMODE(OSCILLATOR_MODE, (int)(OSCILLATOR_SINE)); }
		break;
	case NOISEGEN_ID:
		DEFCONST(NOISEGEN_MIX,  0.0);
		DEFCONST(NOISEGEN_GAIN, 1.0);
		break;
	case BQFILTER_ID:
		DEFCONST(BQFILTER_FREQ,   1.0);
		DEFCONST(BQFILTER_Q,      0.0);
		DEFCONST(BQFILTER_DBGAIN, 0.75);
		DEFMODE (BQFILTER_MODE,   BQFILTER_LOWPASS);
		break;
	case ONEPOLEFILTER_ID:  DEFCONST(ONEPOLEFILTER_POLE, 0.0); break;
	case ONEZEROFILTER_ID:  DEFCONST(ONEZEROFILTER_ZERO, 0.0); break;
	case SHAPER_ID:         DEFCONST(SHAPER_DRIVE, 0.0); break;
	case PANNING_ID:        DEFCONST(PANNING_PAN, 0.5); break;
	case SCALE_ID:          DEFCONST(SCALE_SCALE, 0.0); break;
	case CLIP_ID:           DEFCONST(CLIP_LEVEL, 1.0); break;
	case MIX_ID:            DEFCONST(MIX_MIX, 0.5); break;
	case MONO_ID:           DEFMODE (MONO_MODE, 0); break;
	case ENVFOLLOWER_ID:
		DEFCONST(ENVFOLLOWER_ATTACK,  0.125);
		DEFCONST(ENVFOLLOWER_RELEASE, 0.375);
		break;
	case LOGIC_ID:          DEFMODE(LOGIC_MODE,   LOGIC_AND); break;
	case COMPARE_ID:        DEFMODE(COMPARE_MODE, COMPARE_GT); break;
	case MIDISIGNAL_ID:
		DEFCONST(MIDISIGNAL_SCALE, 1.0);
		DEFMODE (MIDISIGNAL_MODE,  0);
		break;
	case DISTORTION_ID:
		DEFCONST(DISTORTION_DRIVE,     0.0);
		DEFCONST(DISTORTION_THRESHOLD, 0.75);
		DEFMODE (DISTORTION_MODE,      DISTORTION_OVERDRIVE);
		break;
	case CROSSMIX_ID:  DEFCONST(CROSSMIX_MIX, 0.0); break;
	case DELAY_ID:
		DEFCONST(DELAY_TIME, 0.5);
		DEFMODE (DELAY_MODE, DELAY_BPMSYNC);
		break;
	case FBDELAY_ID:
		DEFCONST(FBDELAY_TIME,     0.3125);
		DEFCONST(FBDELAY_FEEDBACK, 0.5);
		DEFCONST(FBDELAY_DAMP,     0.5);
		DEFCONST(FBDELAY_MIX,      0.5);
		DEFMODE (FBDELAY_MODE,     DELAY_BPMSYNC);
		break;
	case DCFILTER_ID:  DEFCONST(DCFILTER_POLE, 0.99765014648437500); break;
	case OSRAND_ID:    DEFCONST(OSRAND_SCALE,  0.0); break;
	case REVERB_ID:
		DEFCONST(REVERB_GAIN,     0.5);
		DEFCONST(REVERB_ROOMSIZE, 0.75);
		DEFCONST(REVERB_DAMP,     0.5);
		DEFCONST(REVERB_WIDTH,    0.5);
		DEFCONST(REVERB_MIX,      0.5);
		break;
	case EQ_ID:
		DEFCONST(EQ_B1,  0.75); DEFCONST(EQ_B2,  0.75); DEFCONST(EQ_B3,  0.75);
		DEFCONST(EQ_B4,  0.75); DEFCONST(EQ_B5,  0.75); DEFCONST(EQ_B6,  0.75);
		DEFCONST(EQ_B7,  0.75); DEFCONST(EQ_B8,  0.75); DEFCONST(EQ_B9,  0.75);
		DEFCONST(EQ_B10, 0.75);
		break;
	case COMPEXP_ID:
		DEFCONST(COMPEXP_THRESHOLD, 0.75);
		DEFCONST(COMPEXP_RATIO,     0.0);
		DEFCONST(COMPEXP_ATTACK,    0.0);
		DEFCONST(COMPEXP_RELEASE,   0.75);
		break;
	case TRIGGERSEQ_ID:
		DEFMODE(TRIGGERSEQ_MODE,          0x0400 | 0x1);
		DEFMODE(TRIGGERSEQ_PATTERN0_3L,   0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN4_7L,   0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN8_11L,  0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN12_15L, 0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN0_3R,   0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN4_7R,   0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN8_11R,  0x11111111);
		DEFMODE(TRIGGERSEQ_PATTERN12_15R, 0x11111111);
		break;
	case SAMPLEREC_ID:  DEFMODE(SAMPLEREC_MODE, 0x0ffff000); break;
	case SAMPLER_ID:
		DEFCONST(SAMPLER_POSITION,  0.0);
		DEFCONST(SAMPLER_SPEED,     0.0);
		DEFCONST(SAMPLER_DIRECTION, 0.0);
		DEFCONST(SAMPLER_LOOPSTART, 0.0);
		DEFCONST(SAMPLER_LOOPEND,   1.0);
		DEFCONST(SAMPLER_CROSSFADE, 0.0);
		DEFMODE (SAMPLER_MODE,      SAMPLER_STORED);
		break;
	case SVFILTER_ID:
		DEFCONST(SVFILTER_FREQ, 1.0);
		DEFCONST(SVFILTER_Q,    0.0);
		DEFMODE (SVFILTER_MODE, SVFILTER_LOWPASS);
		break;
	case GLITCH_ID:
		DEFCONST(GLITCH_ACTIVE, 0.0);
		DEFCONST(GLITCH_TIME,   0.3125);
		DEFCONST(GLITCH_P1,     0.125);
		DEFCONST(GLITCH_P2,     0.125);
		DEFCONST(GLITCH_SPEED,  0.0);
		DEFMODE (GLITCH_MODE,   0);
		break;
	case COMBDELAY_ID:
		DEFCONST(COMBDELAY_TIME, 0.5);
		DEFMODE (COMBDELAY_MODE, DELAY_BPMSYNC);
		break;
	case GMDLS_ID:  DEFMODE(GMDLS_MODE, 0); break;
	case BOWED_ID:
		DEFCONST(BOWED_POSITION,    0.5);
		DEFCONST(BOWED_PRESSURE,    0.5);
		DEFCONST(BOWED_VELOCITY,    0.75);
		DEFCONST(BOWED_VIBRATO,     0.0);
		DEFCONST(BOWED_FRICTIONSYM, 0.0);
		break;
	case SNH_ID:
		DEFCONST(SNH_SNH,       1.0);
		DEFCONST(SNH_SNHSMOOTH, 0.0);
		DEFMODE (SNH_MODE,      SNH_QUADRATIC);
		break;
	case WTFOSC_ID:
		DEFCONST (WTFOSC_POSITION,        0.0);
		DEFCONST (WTFOSC_SPEED,           0.0);
		DEFCONST2(WTFOSC_TARGETSEARCHLEN, 0.125, 0.0625);
		DEFCONST2(WTFOSC_COUNTSKIP,       0.125, 0.0);
		DEFMODE  (WTFOSC_MODE,            SAMPLER_STORED);
		break;
	case FORMANT_ID:
		DEFCONST(FORMANT_GAIN, 1.0);
		DEFMODE (FORMANT_MODE, 0);
		break;
	case EQ3_ID:
		DEFCONST(EQ3_LGAIN, 0.75);
		DEFCONST(EQ3_MGAIN, 0.75);
		DEFCONST(EQ3_HGAIN, 0.75);
		break;
	case VOICE_FREQUENCY_ID:
	case VOICE_NOTE_ID:
	case VOICE_ATTACKVELOCITY_ID:
	case VOICE_TRIGGER_ID:
	case VOICE_GATE_ID:
	case VOICE_AFTERTOUCH_ID:
		DEFCONST(VOICEPARAM_SCALE, 1.0);
		break;
	default: break;
	}

#undef DEFCONST
#undef DEFCONST2
#undef DEFMODE
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::resetNodeToDefaults(DWORD nodeID, DWORD typeID, bool isGlobal)
{
	// Mirror the same special case used in savePatch: SCALE, MIDISIGNAL, OSRAND, and all
	// voice-param node types (id > CONSTANT_ID) store a float "Scale" constant at the
	// index just above NodeMaxGUISignals, even though MAX_GUI_SIGNALS is left at 0 for
	// these node types.  Without the bump that constant would be initialised as an integer
	// mode (via setInputMode) rather than a float value (via setInputValue), leaving it
	// at ~0 instead of the intended 1.0 default.
	DWORD maxguisignals = NodeMaxGUISignals[typeID];
	if (typeID == SCALE_ID     ||
	    typeID == MIDISIGNAL_ID ||
	    typeID == OSRAND_ID    ||
	    typeID > CONSTANT_ID)
		maxguisignals++;

	// Reset parameter constant inputs to factory defaults
	for (DWORD i = NodeReqGUISignals[typeID]; i < maxguisignals; i++)
	{
		double defL, defR;
		getNodeInputDefault(typeID, i, isGlobal, defL, defR);
		setInputValue(nodeID, i, defL, defR);
	}
	// Reset mode inputs to factory defaults
	for (DWORD i = maxguisignals; i < NodeInputs[typeID]; i++)
	{
		double defVal, dummy;
		getNodeInputDefault(typeID, i, isGlobal, defVal, dummy);
		setInputMode(nodeID, i, (DWORD)(long long)defVal, 0xffffffff);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::deleteNode(DWORD node)
{
	// Suppress nested locking: disconnectInput also checks _massDataUpdate.
	// If we held the mutex but left _massDataUpdate false, every inner call
	// would try to lock a std::timed_mutex we already own → deadlock.
	bool wasUpdating = _massDataUpdate;
	if (!wasUpdating)
	{
		_massDataUpdate = true;
		DataAccessMutex.lock();
	}

	// parameters and modadders are destroyed when their owner nodes are destroyed
	if (_nodeGUIInfo[node].IsParameter || _nodeGUIInfo[node].IsModAdder)
	{
		if (!wasUpdating)
		{
			_massDataUpdate = false;
			DataAccessMutex.unlock();
		}
		return;
	}

	// the gui node to be deleted
	SynthNode* dnode = _nodeGUIInfo[node].Node;

	// disconnect from all dependent nodes and reestablish default 0 for them
	for (std::set<SynthNode*>::iterator it = _nodeGUIInfo[node].Outputs.begin(); it != _nodeGUIInfo[node].Outputs.end(); it++)
	{
		// loop outputs inputs
		for (DWORD i = 0; i < (*it)->numInputs; i++)
		{
			SynthNode* input = (SynthNode*)((*it)->input[i]);
			// input is a modadder, so refer to its connected input
			if (input == _nodeGUIInfo[(*it)->valueOffset].ModAdder[i])
				input = (SynthNode*)(input->input[1]);
			// found our node, so disconnect
			if (input == dnode)
				disconnectInput((*it)->valueOffset, i, false);
		}
	}

	// constants dont have inputs (and also use numInputs field for constant mode)
	if (dnode->id != CONSTANT_ID)
	{
		// disconnect from our inputs
		for (int i = dnode->numInputs-1; i >= 0; i--)
		{
			// disconnect required inputs
			if (i < NodeReqGUISignals[dnode->id])
			{
				disconnectInput(dnode->valueOffset, i);
			}
			// disconnect modulations
			else if (i < NodeMaxGUISignals[dnode->id])
			{
				SynthNode* input = (SynthNode*)(dnode->input[i]);
				if (input == _nodeGUIInfo[dnode->valueOffset].ModAdder[i])
					disconnectInput(dnode->valueOffset, i);
			}
			// disconnect multiadd inputs
			else if (dnode->id == MULTIADD_ID || dnode->id == NOTECONTROLLER_ID)
			{
				disconnectInput(dnode->valueOffset, i);
			}
		}

		// remove all constants and modadders created by this node
		if (dnode->id != MULTIADD_ID && dnode->id != NOTECONTROLLER_ID)
		{
			for (DWORD i = NodeReqGUISignals[dnode->id]; i < dnode->numInputs; i++)
			{
				if (_nodeGUIInfo[dnode->valueOffset].ModAdder[i])
				{
					SynthNode* modadder = _nodeGUIInfo[dnode->valueOffset].ModAdder[i];
					// free the modadder constant
					SynthNode* constant = (SynthNode*)(modadder->input[0]);
					_freeSlots[constant->valueOffset/NODE_SLOTS] = 1;
					_nodes[constant->valueOffset/NODE_SLOTS] = 0;
					_nodeGUIInfo.erase(constant->valueOffset);
					SynthFree(constant);
					// free the modadder itself
					_freeSlots[modadder->valueOffset/NODE_SLOTS] = 1;
					_nodes[modadder->valueOffset/NODE_SLOTS] = 0;
					_nodeGUIInfo.erase(modadder->valueOffset);
					SynthFree(modadder);
				}
				// mode constants
				else
				{
					SynthNode* constant = (SynthNode*)(dnode->input[i]);
					_freeSlots[constant->valueOffset/NODE_SLOTS] = 1;
					_nodes[constant->valueOffset/NODE_SLOTS] = 0;
					_nodeGUIInfo.erase(constant->valueOffset);
					SynthFree(constant);
				}
			}
		}
	}

	// mark the slot as free again
	_freeSlots[dnode->valueOffset/NODE_SLOTS] = 1;
	_nodes[dnode->valueOffset/NODE_SLOTS] = 0;
	SynthGlobalState.GlobalNodes[dnode->valueOffset] = 0;
	_nodeGUIInfo.erase(dnode->valueOffset);
	if (dnode->customMem)
		SynthFree(dnode->customMem);
	SynthFree(dnode);

	if (!wasUpdating)
	{
		_massDataUpdate = false;
		DataAccessMutex.unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthNode* SynthController::getNode(DWORD node)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(node);
	if (it != _nodeGUIInfo.end())
		return it->second.Node;
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::connectInput(DWORD inputid, DWORD targetid, DWORD index)
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	SynthNode* input = _nodeGUIInfo[inputid].Node;
	SynthNode* target = _nodeGUIInfo[targetid].Node;

	// special case for multiadd based nodes with variable inputs
	if (target->id == MULTIADD_ID || target->id == NOTECONTROLLER_ID)
	{
		// gui connection
		_nodeGUIInfo[input->valueOffset].Outputs.insert(target);
		target->input[target->numInputs] = (sample_t*)input;
		// core connection
		SynthGlobalState.NodeValues[target->valueOffset+1+target->numInputs] = input->valueOffset;
		// increase number of inputs
		target->numInputs++;
		SynthGlobalState.NodeValues[target->valueOffset] = INFO(target->id, target->numInputs, target->isGlobal);
	}
	// normal connection
	else
	{
		// no checks for required signals
		if (index < NodeReqGUISignals[target->id])
		{
			// gui connection
			_nodeGUIInfo[input->valueOffset].Outputs.insert(target);
			target->input[index] = (sample_t*)input;
			// core connection
			SynthGlobalState.NodeValues[target->valueOffset+1+index] = input->valueOffset;
		}
		// something connected from the gui as a modulator
		else if (index < NodeMaxGUISignals[target->id])
		{
			SynthNode* modadder = _nodeGUIInfo[target->valueOffset].ModAdder[index];
			// gui connection (hidden by modadder)
			_nodeGUIInfo[input->valueOffset].Outputs.insert(target);
			modadder->input[1] = (sample_t*)input;
			target->input[index] = (sample_t*)modadder;
			// core connection
			SynthGlobalState.NodeValues[target->valueOffset+1+index] = modadder->valueOffset;
			SynthGlobalState.NodeValues[modadder->valueOffset+1+1] = input->valueOffset;
		}
		// mode inputs cannot be connected manually
		else
		{
		}
	}

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::RemoveOutput(SynthNode* node, SynthNode* target)
{
	int count = 0;
	// count targets inputs from node
	for (DWORD i = 0; i < target->numInputs; i++)
	{
		SynthNode* input = (SynthNode*)(target->input[i]);
		// input is a modadder, so refer to its connected input
		if (input == _nodeGUIInfo[target->valueOffset].ModAdder[i])
			input = (SynthNode*)(input->input[1]);
		// increase counter on input match
		if (input == node)
			count++;
	}
	if (count == 1)
		_nodeGUIInfo[node->valueOffset].Outputs.erase(target);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::disconnectInput(DWORD targetid, DWORD index, bool removeOutput)
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	SynthNode* target = _nodeGUIInfo[targetid].Node;

	// special case for multiadd based nodes with variable inputs
	if (target->id == MULTIADD_ID || target->id == NOTECONTROLLER_ID)
	{
		if (removeOutput)
			RemoveOutput((SynthNode*)(target->input[index]), target);
		// need to swap index with lastindex keep inputs gap free?
		if (index < target->numInputs-1)
		{
			// gui swap
			target->input[index] = target->input[target->numInputs-1];
			// core swap
			SynthGlobalState.NodeValues[target->valueOffset+1+index] = SynthGlobalState.NodeValues[target->valueOffset+1+target->numInputs-1];
		}
		// decrease number of inputs
		target->numInputs--;
		SynthGlobalState.NodeValues[target->valueOffset] = INFO(target->id, target->numInputs, target->isGlobal);
	}
	// normal disconnect
	else
	{
		// gui disconnect for required signals
		if (index < NodeReqGUISignals[target->id])
		{
			if (removeOutput)
				RemoveOutput((SynthNode*)(target->input[index]), target);
			// for gui set default 0 constant
			target->input[index] = (sample_t*)constant0();
			// core disconnect
			SynthGlobalState.NodeValues[target->valueOffset+1+index] = CONSTANT_0;
		}
		// a modulation slot was disconnected, so reconnect constant directly to the input
		else if (index < NodeMaxGUISignals[target->id])
		{
			SynthNode* modadder = _nodeGUIInfo[target->valueOffset].ModAdder[index];
			// get the node to disconnect from the modadder at index and remove target from the outputs
			SynthNode* input = (SynthNode*)(modadder->input[1]);
			if (removeOutput)
				RemoveOutput(input, target);
			// reconnect the constant
			SynthNode* constant = (SynthNode*)(modadder->input[0]);
			target->input[index] = (sample_t*)constant;
			// core reconnect
			SynthGlobalState.NodeValues[target->valueOffset+1+index] = constant->valueOffset;
			// set modadder input slot to default 0 again (mark as unused)
			modadder->input[1] = (sample_t*)constant0();
		}
		// mode inputs cannot be disconnected
		else
		{
		}
	}

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setInputValue(DWORD nodeid, DWORD index, double value1, double value2)
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	SynthNode* node = _nodeGUIInfo[nodeid].Node;

	if (index == -1)
	{
		node->out = sample_t(value1, value2);
	}
	else
	{
		SynthNode* store = (SynthNode*)(node->input[index]);
		SynthNode* modadder = _nodeGUIInfo[node->valueOffset].ModAdder[index];
		// if modadder at index is connected store in the constant at modadders index 0
		if (store == modadder)
			*(modadder->input[0]) = sample_t(value1, value2);
		// no modadder, so directly store constant
		else
			*(node->input[index]) = sample_t(value1, value2);
	}

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setInputMode(DWORD nodeid, DWORD index, DWORD mode, DWORD modemask)
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	SynthNode* node = _nodeGUIInfo[nodeid].Node;

	SynthNode* store = (SynthNode*)(node->input[index]);
	SynthNode* modadder = _nodeGUIInfo[node->valueOffset].ModAdder[index];
	// if modadder at index is connected store in the constant at modadders index 0
	if (store == modadder)
	{
		DWORD oldmode = modadder->input[0]->i[0] & ~modemask;
		modadder->input[0]->i[0] = (mode & modemask) | oldmode;
	}
	// no modadder, so directly store constant
	else
	{
		DWORD oldmode = node->input[index]->i[0] & ~modemask;
		node->input[index]->i[0] = (mode & modemask) | oldmode;
	}

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double SynthController::getInputValue(DWORD nodeid, DWORD inputIndex, DWORD channel)
{
	SynthNode* node = _nodeGUIInfo[nodeid].Node;

	// special case for constant
	if (inputIndex == -1)
	{
		return node->out.d[channel];
	}
	else
	{
		// required input? return input
		if (inputIndex < NodeReqGUISignals[node->id])
		{
			return 0.0;
		}
		// potential modadder?
		else if (inputIndex < NodeMaxGUISignals[node->id])
		{
			// modadder not used? return constant
			if (node->id == CONSTANT_ID)
			{
				return node->input[inputIndex]->d[channel];
			}
			// modadder used? return referenced input
			else
			{
				NodeGUIInfo* info = &(_nodeGUIInfo[node->valueOffset]);
				SynthNode* modadder = info->ModAdder[inputIndex];
				return modadder->input[0]->d[channel];
			}
		}
		// mode constant or scaler input
		else
		{
			if (node->id == SCALE_ID ||
				node->id == MIDISIGNAL_ID ||
				node->id == OSRAND_ID ||
				node->id > CONSTANT_ID)
			{
				return node->input[inputIndex]->d[channel];
			}
			return 0.0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getInputMode(DWORD nodeid, DWORD inputIndex)
{
	SynthNode* node = _nodeGUIInfo[nodeid].Node;

	// required input? return input
	if (inputIndex < NodeReqGUISignals[node->id])
	{
		return 0;
	}
	// potential modadder?
	else if (inputIndex < NodeMaxGUISignals[node->id])
	{
		return 0;
	}
	// mode constant
	else
	{
		return node->input[inputIndex]->i[0];
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::resetEventSignal(DWORD nodeid)
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	SynthNode* node = _nodeGUIInfo[nodeid].Node;
	node->e = sample_t::zero();

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getArpStepData(DWORD nodeid, DWORD step)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		SynthNode* node = it->second.Node;

		if (node->id != VOICEMANAGER_ID)
			return 0;

		int ret;
		if (step == 0xffffffff)
			ret = ((VMWork*)(node->customMem))->ArpSequenceLoopIndex;
		else
			ret = ((VMWork*)(node->customMem))->ArpSequence[step];
		return ret;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getArpPlayPos(DWORD nodeid)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		SynthNode* node = it->second.Node;

		if (node->id != VOICEMANAGER_ID)
			return 0;

		int step=node->v[19].i[0];
		int loopStart = ((VMWork*)(node->customMem))->ArpSequenceLoopIndex;
		step=(step+31) % 32;
		// note: this does not display correctly when loopStart > 0. tick 31 always wraps to loopStart-1 then
		return step;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getTriggerSeqPlayPos(DWORD nodeid)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		SynthNode* node = it->second.Node;
		if (node->id != TRIGGERSEQ_ID)
			return -1;
		// v[0] doubles each step: 1→tick0, 2→tick1, 4→tick2, ..., 128→tick7
		// v[1] = current pattern (0-based float)
		int step = (int)node->v[0].d[0];
		int tick = 0;
		while (step > 1 && tick < 7) { step >>= 1; tick++; }
		int pattern = (int)node->v[1].d[0];
		if (pattern < 0) pattern = 0;
		return (pattern << 8) | tick;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setArpStepData(DWORD nodeid, DWORD step, DWORD value)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		SynthNode* node = it->second.Node;

		if (node->id != VOICEMANAGER_ID)
			return;

		if (step == 0xffffffff)
			((VMWork*)(node->customMem))->ArpSequenceLoopIndex = value;
		else
			((VMWork*)(node->customMem))->ArpSequence[step] = (WORD)value;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setX(DWORD nodeid, double x)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		it->second.X = x;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setY(DWORD nodeid, double y)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		it->second.Y = y;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setName(DWORD nodeid, std::string name)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		it->second.Name = name;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SynthController::getName(DWORD nodeid)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		return it->second.Name;
	}
	else
		return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setSAPIText(DWORD nodeid, std::string text)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		if (it->second.Node->specialData)
			free(it->second.Node->specialData);
		it->second.Node->specialData = (char*)malloc(text.length()+1);
		char* specialData = (char*)(it->second.Node->specialData);
		memcpy(specialData, text.c_str(), text.length());
		specialData[text.length()] = 0; // terminator
		it->second.Node->e = sample_t::zero(); // activate processing again
		SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset] = specialData;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SynthController::getSAPIText(DWORD nodeid)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		if (it->second.Node->specialData == 0)
			return "";
		return (char*)(it->second.Node->specialData);
	}
	else
		return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setFormulaText(DWORD nodeid, std::string text, std::string rpn)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		// store the actual input formula
		it->second.Node->specialDataText = text;
		it->second.Node->specialDataText2 = rpn;

		std::vector<BYTE> comseq;
		char* token = (char*)(rpn.c_str());
		char* ct = token;
		while (*token != 0)
		{
			// token finished?
			if (*token == ':')
			{
				// terminate string
				*token = 0;
				// constant (positive or negative)
				if ((*ct >= '0' && *ct <= '9') || ((*ct == '-') && (*(ct+1) >= '0' && *(ct+1) <= '9')))
				{
					float constant = atof(ct);
					DWORD iv = *((DWORD*)(&constant)) & 0xffffff00;
					comseq.push_back(0);
					comseq.push_back((iv >> 8) & 0xff);
					comseq.push_back((iv >> 16) & 0xff);
					comseq.push_back((iv >> 24) & 0xff);
				}
				// token
				else
				{
					std::string name = ct;
					if (name == "+")		comseq.push_back(FORMULA_CMD_ADD);
					if (name == "*")		comseq.push_back(FORMULA_CMD_MUL);
					if (name == "==")		comseq.push_back(FORMULA_CMD_E);
					if (name == "!=")		comseq.push_back(FORMULA_CMD_NE);
					if (name == ">")		comseq.push_back(FORMULA_CMD_GT);
					if (name == ">=")		comseq.push_back(FORMULA_CMD_GTE);
					if (name == "&&")		comseq.push_back(FORMULA_CMD_AND);
					if (name == "||")		comseq.push_back(FORMULA_CMD_OR);
					if (name == "<")		comseq.push_back(FORMULA_CMD_LT);
					if (name == "<=")		comseq.push_back(FORMULA_CMD_LTE);
					if (name == "-")		comseq.push_back(FORMULA_CMD_SUB);
					if (name == "/")		comseq.push_back(FORMULA_CMD_DIV);
					if (name == "[mM]in")	comseq.push_back(FORMULA_CMD_MIN);
					if (name == "[mM]ax")	comseq.push_back(FORMULA_CMD_MAX);
					if (name == "%")		comseq.push_back(FORMULA_CMD_MOD);
					if (name == "[lL]erp")	comseq.push_back(FORMULA_CMD_LERP);
					if (name == "[iI]fthen")	comseq.push_back(FORMULA_CMD_IFTHEN);
					if (name == "[mM]axtime")	comseq.push_back(FORMULA_CMD_MAXTIME);
					if (name == "Neg")		comseq.push_back(FORMULA_CMD_NEG);
					if (name == "[aA]bs")	comseq.push_back(FORMULA_CMD_ABS);
					if (name == "[sS]qrt")	comseq.push_back(FORMULA_CMD_SQRT);
					if (name == "[cC]eil")	comseq.push_back(FORMULA_CMD_CEIL);
					if (name == "[fF]loor")	comseq.push_back(FORMULA_CMD_FLOOR);
					if (name == "[sS]qr")	comseq.push_back(FORMULA_CMD_SQR);
					if (name == "[cC]os")	comseq.push_back(FORMULA_CMD_COS);
					if (name == "[sS]in")	comseq.push_back(FORMULA_CMD_SIN);
					if (name == "[eE]xp2")	comseq.push_back(FORMULA_CMD_EXP2);
					if (name == "[lL]og2")	comseq.push_back(FORMULA_CMD_LOG2);
					if (name == "[rR]and")	comseq.push_back(FORMULA_CMD_RAND);
					if (name == "[pP]i")	comseq.push_back(FORMULA_CMD_PI);
					if (name == "[tT]au")	comseq.push_back(FORMULA_CMD_TAU);
					if (name == "[tT]autime")	comseq.push_back(FORMULA_CMD_TAUTIME);
					if (name == "[iI]n0")	comseq.push_back(FORMULA_CMD_IN0);
					if (name == "[iI]n1")	comseq.push_back(FORMULA_CMD_IN1);
					if (name == "[vV]frequency")	comseq.push_back(FORMULA_CMD_VFREQUENCY);
					if (name == "[vV]note")		comseq.push_back(FORMULA_CMD_VNOTE);
					if (name == "[vV]velocity")	comseq.push_back(FORMULA_CMD_VVELOCITY);
					if (name == "[vV]trigger")	comseq.push_back(FORMULA_CMD_VTRIGGER);
					if (name == "[vV]gate")		comseq.push_back(FORMULA_CMD_VGATE);
					if (name == "[vV]aftertouch")	comseq.push_back(FORMULA_CMD_VAFTERTOUCH);
					if (name == "[tT]risaw")	comseq.push_back(FORMULA_CMD_TRISAW);
					if (name == "[pP]ulse")		comseq.push_back(FORMULA_CMD_PULSE);
					// get variables
					if (name == "out")	comseq.push_back(FORMULA_CMD_GETVAR + 0);
					if (name == "time")	comseq.push_back(FORMULA_CMD_GETVAR + 1);
					if (name == "a")	comseq.push_back(FORMULA_CMD_GETVAR + 2);
					if (name == "b")	comseq.push_back(FORMULA_CMD_GETVAR + 3);
					if (name == "c")	comseq.push_back(FORMULA_CMD_GETVAR + 4);
					if (name == "d")	comseq.push_back(FORMULA_CMD_GETVAR + 5);
					if (name == "e")	comseq.push_back(FORMULA_CMD_GETVAR + 6);
					if (name == "f")	comseq.push_back(FORMULA_CMD_GETVAR + 7);
					if (name == "g")	comseq.push_back(FORMULA_CMD_GETVAR + 8);
					if (name == "h")	comseq.push_back(FORMULA_CMD_GETVAR + 9);
					if (name == "i")	comseq.push_back(FORMULA_CMD_GETVAR + 10);
					if (name == "j")	comseq.push_back(FORMULA_CMD_GETVAR + 11);
					if (name == "k")	comseq.push_back(FORMULA_CMD_GETVAR + 12);
					if (name == "l")	comseq.push_back(FORMULA_CMD_GETVAR + 13);
					if (name == "m")	comseq.push_back(FORMULA_CMD_GETVAR + 14);
					if (name == "n")	comseq.push_back(FORMULA_CMD_GETVAR + 15);
					if (name == "o")	comseq.push_back(FORMULA_CMD_GETVAR + 16);
					if (name == "p")	comseq.push_back(FORMULA_CMD_GETVAR + 17);
					if (name == "q")	comseq.push_back(FORMULA_CMD_GETVAR + 18);
					if (name == "r")	comseq.push_back(FORMULA_CMD_GETVAR + 19);
					if (name == "s")	comseq.push_back(FORMULA_CMD_GETVAR + 20);
					if (name == "t")	comseq.push_back(FORMULA_CMD_GETVAR + 21);
					if (name == "u")	comseq.push_back(FORMULA_CMD_GETVAR + 22);
					if (name == "v")	comseq.push_back(FORMULA_CMD_GETVAR + 23);
					// set variables
					if (name == "out=")		comseq.push_back(FORMULA_CMD_SETVAR + 0);
					if (name == "time=")	comseq.push_back(FORMULA_CMD_SETVAR + 1);
					if (name == "a=")	comseq.push_back(FORMULA_CMD_SETVAR + 2);
					if (name == "b=")	comseq.push_back(FORMULA_CMD_SETVAR + 3);
					if (name == "c=")	comseq.push_back(FORMULA_CMD_SETVAR + 4);
					if (name == "d=")	comseq.push_back(FORMULA_CMD_SETVAR + 5);
					if (name == "e=")	comseq.push_back(FORMULA_CMD_SETVAR + 6);
					if (name == "f=")	comseq.push_back(FORMULA_CMD_SETVAR + 7);
					if (name == "g=")	comseq.push_back(FORMULA_CMD_SETVAR + 8);
					if (name == "h=")	comseq.push_back(FORMULA_CMD_SETVAR + 9);
					if (name == "i=")	comseq.push_back(FORMULA_CMD_SETVAR + 10);
					if (name == "j=")	comseq.push_back(FORMULA_CMD_SETVAR + 11);
					if (name == "k=")	comseq.push_back(FORMULA_CMD_SETVAR + 12);
					if (name == "l=")	comseq.push_back(FORMULA_CMD_SETVAR + 13);
					if (name == "m=")	comseq.push_back(FORMULA_CMD_SETVAR + 14);
					if (name == "n=")	comseq.push_back(FORMULA_CMD_SETVAR + 15);
					if (name == "o=")	comseq.push_back(FORMULA_CMD_SETVAR + 16);
					if (name == "p=")	comseq.push_back(FORMULA_CMD_SETVAR + 17);
					if (name == "q=")	comseq.push_back(FORMULA_CMD_SETVAR + 18);
					if (name == "r=")	comseq.push_back(FORMULA_CMD_SETVAR + 19);
					if (name == "s=")	comseq.push_back(FORMULA_CMD_SETVAR + 20);
					if (name == "t=")	comseq.push_back(FORMULA_CMD_SETVAR + 21);
					if (name == "u=")	comseq.push_back(FORMULA_CMD_SETVAR + 22);
					if (name == "v=")	comseq.push_back(FORMULA_CMD_SETVAR + 23);
				}
				ct = token + 1;
			}
			token++;
		}
		// always push done byte (0xff)
		comseq.push_back(FORMULA_DONE);

		// first assignment? (mem stays constant and is reused)
		if (!SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset])
			SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset] = (char*)malloc(65536);
		BYTE* buf = (BYTE*)SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset];

		// fill the buffer
		for (int i = 0; i < (int)comseq.size(); i++)
		{
			*buf++ = comseq[i];
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SynthController::getFormulaText(DWORD nodeid)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		return it->second.Node->specialDataText;
	}
	else
		return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::clearSelection()
{
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it = _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		it->second.IsSelected = false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setSelected(DWORD nodeid, int selected)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(nodeid);
	if (it != _nodeGUIInfo.end())
	{
		it->second.IsSelected = selected;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double SynthController::getNodeValue(DWORD nodeid, DWORD index, DWORD channel)
{
	SynthNode* useNode = SynthGlobalState.GlobalNodes[nodeid];
	if (useNode)
	{
		if (index >= 0 && index <= NODE_MAX_WORKVARS)
			return useNode->v[index].d[channel];
	}
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setNodeProcessingFlags(DWORD nodeid, DWORD flags)
{
	SynthNode* useNode = SynthGlobalState.GlobalNodes[nodeid];
	if (useNode)
	{
		useNode->processingFlags = flags;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// indexed accessors for mass update in gui
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::numGUINodes()
{
	// rebuild accessor list
	_nodesGUIAccessor.clear();
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it = _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		_nodesGUIAccessor.push_back(&(it->second));
	}
	return (int)_nodesGUIAccessor.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::numSelectedGUINodes()
{
	// rebuild accessor list
	_nodesGUIAccessor.clear();
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it = _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		if (it->second.IsSelected)
			_nodesGUIAccessor.push_back(&(it->second));
	}
	return (int)_nodesGUIAccessor.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthNode* SynthController::gnNode(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return NULL;
	return _nodesGUIAccessor[index]->Node;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::gnIsVisible(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return false;
	return _nodesGUIAccessor[index]->Visible;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnType(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->id;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnID(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->valueOffset;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnNodeInputs(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->numInputs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnNodeReqSignals(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return NodeReqGUISignals[_nodesGUIAccessor[index]->Node->id];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnNodeMaxSignals(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return NodeMaxGUISignals[_nodesGUIAccessor[index]->Node->id];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double SynthController::gnX(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->X;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double SynthController::gnY(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SynthController::gnName(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return "";
	return _nodesGUIAccessor[index]->Name;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnChannel(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->FixedChannel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnInput(DWORD index, DWORD inputIndex)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;

	SynthNode* node = _nodesGUIAccessor[index]->Node;
	// required input? return input
	if (inputIndex < NodeReqGUISignals[node->id])
	{
		SynthNode* inode = (SynthNode*)(node->input[inputIndex]);
		return inode->valueOffset;
	}
	// potential modadder?
	else if (inputIndex < NodeMaxGUISignals[node->id])
	{
		// modadder not used? return constant
		if (node->id == CONSTANT_ID)
		{
			SynthNode* inode = (SynthNode*)(node->input[inputIndex]);
			return inode->valueOffset;
		}
		// modadder used? return referenced input
		else
		{
			NodeGUIInfo* info = &(_nodeGUIInfo[node->valueOffset]);
			SynthNode* modadder = info->ModAdder[inputIndex];
			SynthNode* inode = (SynthNode*)(modadder->input[1]);
			return inode->valueOffset;
		}
	}
	// mode constant
	else
	{
		SynthNode* inode = (SynthNode*)(node->input[inputIndex]);
		return inode->valueOffset;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double SynthController::gnInputValue(DWORD index, DWORD inputIndex, DWORD channel)
{
	if (index >= _nodesGUIAccessor.size())
		return 0.0;
	SynthNode* node = _nodesGUIAccessor[index]->Node;
	return getInputValue(node->valueOffset, inputIndex, channel);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::gnInputMode(DWORD index, DWORD inputIndex)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	SynthNode* node = _nodesGUIAccessor[index]->Node;
	return getInputMode(node->valueOffset, inputIndex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::gnIsGlobal(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->isGlobal;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getNumActiveVoices()
{
	return SynthNode::numActiveVoices;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getNumActiveVoices(DWORD node)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(node);
	if (it->second.Node->id == VOICEMANAGER_ID)
		return ((VMWork*)(it->second.Node->customMem))->NumActiveVoices;
	else
		return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::inputIsModulated(DWORD nodeID, DWORD inputIdx)
{
	auto it = _nodeGUIInfo.find(nodeID);
	if (it == _nodeGUIInfo.end()) return false;
	SynthNode* modadder = it->second.ModAdder[inputIdx];
	if (!modadder) return false;
	// input[1] of the ModAdder is reset to constant0() by disconnectInput when no wire is connected.
	return modadder->input[1] != (sample_t*)constant0();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double SynthController::getNodeSignal(DWORD nodeid, int left, int inp)
{
	SynthNode* useNode = SynthGlobalState.GlobalNodes[nodeid];
	if (useNode)
	{
		// special signal for voice signals
		if (inp == -4)
		{
			if (SynthGlobalState.CurrentVoice != 0 && (useNode->id - CONSTANT_ID) < 6)
			{
				sample_t out = useNode->input[VOICEPARAM_SCALE] * ((sample_t*)(SynthGlobalState.CurrentVoice))[useNode->id - CONSTANT_ID + 2];
				if (left == 0)
					return out.d[0];
				else
					return out.d[1];
			}
		}
		// special signal from midi
		if (inp == -3)
		{
			int mode = useNode->input[MIDISIGNAL_MODE]->i[0];
			sample_t out = SynthGlobalState.MidiSignals[mode & 0xffff] * useNode->input[MIDISIGNAL_SCALE];
			if (left == 0)
				return out.d[0];
			else
				return out.d[1];
		}
		// special signal from channel and synth root (envelope follower state)
		if (inp == -2)
		{
			if (left == 0)
				return useNode->v[1].d[0];
			else
				return useNode->v[1].d[1];
		}
		// generic output signal
		if (inp == -1)
		{
			if (left == 0)
				return useNode->out.d[0];
			else
				return useNode->out.d[1];
		}
		// generic input signal
		else if (inp < useNode->numInputs)
		{
			if (left == 0)
				return useNode->input[inp]->d[0];
			else
				return useNode->input[inp]->d[1];
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::killVoices()
{
	if (!_massDataUpdate)
		DataAccessMutex.lock();

#define W ((VMWork*)(node->customMem))
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it = _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		SynthNode* node = it->second.Node;
		if (node->id == VOICEMANAGER_ID)
		{
			int numActive = W->NumActiveVoices;
			while (numActive--)
			{
				SynthGlobalState.CurrentVoice = &(W->Voice[W->ActiveVoices[numActive]]);
				DestroyVoiceNodes(SynthGlobalState.CurrentVoice->VoiceRoot);
				SynthGlobalState.CurrentVoice->VoiceRoot = NULL;
#ifdef COMPILE_VSTI
				SynthNode::numActiveVoices--;
#endif
			}
			W->NumActiveVoices = 0;
			W->NumKeys = 0;
			W->Glider = 0;
			node->out = sample_t::zero();
			node->lastTick = 0;
			node->currentUpdateStep = 0;
			node->e = sample_t::zero();
			memset(node->v, 0, sizeof(sample_t)*NODE_MAX_WORKVARS);
		}
	}
#undef W

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::panic()
{
	DataAccessMutex.lock();
	_massDataUpdate = true;

	// kill active voices spawned by voicemanagers
	killVoices();

	// clean up the synth
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it = _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		SynthNode* node = it->second.Node;
		if (node->id < CONSTANT_ID)
		{
			node->out = sample_t::zero();
			node->lastTick = 0;
			node->currentUpdateStep = 0;
			node->e = sample_t::zero();
			memset(node->v, 0, sizeof(sample_t)*NODE_MAX_WORKVARS);
			if (node->customMem)
			{
				void* backup = 0;
				// voicemanager contains the step sequence in custom mem, so backup
				if (node->id == VOICEMANAGER_ID)
				{
					backup = (void*)SynthMalloc(sizeof(VMWork));
					memcpy(backup, node->customMem, sizeof(VMWork));
				}

				// free the custom memory
				SynthFree(node->customMem);
				node->customMem = 0;

				// reinit node if init exists
				if (node->init)
					node->init(node);

				// voicemanager: restore
				if (node->id == VOICEMANAGER_ID)
				{
					VMWork* b = (VMWork*)backup;
					VMWork* w = (VMWork*)(node->customMem);
					w->ArpSequenceLoopIndex = b->ArpSequenceLoopIndex;
					memcpy(w->ArpSequence, b->ArpSequence, 32*sizeof(WORD));
					SynthFree(backup);
				}
			}
		}
	}

	_massDataUpdate = false;
	DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::AddDeferredFreeNode(void* node)
{
	_deferredFreeNodes.push_back(node);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::DeferredSynthFree()
{
	if (_deferredFreeNodes.size())
	{
		for (size_t i = 0; i < _deferredFreeNodes.size(); i++)
		{
			SynthFree(_deferredFreeNodes[i]);
		}
		_deferredFreeNodes.clear();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mass data access functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::recursiveAddChannel(SynthNode* node, int channel)
{
	NodeGUIInfo* gi = &(_nodeGUIInfo[node->valueOffset]);
	// skip already processed nodes
	if (gi->RecursionFlag == 0)
	{
		gi->RecursionFlag = 1;

		if (((node->id != CONSTANT_ID) && (node->id > NOTECONTROLLER_ID)) ||
			(gi->FixedChannel == channel) ||
			((node->id == CONSTANT_ID) && gi->Visible))
		{
			gi->Channels.insert(channel);
		}

		// recursion not for constants
		if (node->id != CONSTANT_ID)
		{
			for (DWORD i = 0; i < node->numInputs; i++)
			{
				SynthNode* input = (SynthNode*)(node->input[i]);
				recursiveAddChannel(input, channel);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void setAttrDouble6(TiXmlElement& el, const char* name, double val)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%.6g", val);
	el.SetAttribute(name, buf);
}

void SynthController::saveNode(SynthNode* n, TiXmlElement& root, int saveChannel)
{
	if (n)
	{
		// special case for multiadds
		if (n->id == MULTIADD_ID || n->id == NOTECONTROLLER_ID)
		{
			NodeGUIInfo* ni = &(_nodeGUIInfo[n->valueOffset]);
			TiXmlElement node("Node");
			node.SetAttribute("type", n->id);
			node.SetAttribute("global", n->isGlobal);
			node.SetAttribute("channel", ni->FixedChannel);
			node.SetAttribute("x", (int)ni->X);
			node.SetAttribute("y", (int)ni->Y);
			node.SetAttribute("id", n->valueOffset / NODE_SLOTS);
			if (ni->Name != "")
				node.SetAttribute("name", ni->Name);
			for (DWORD i = 0; i < n->numInputs; i++)
			{
				SynthNode* input = ((SynthNode*)(n->input[i]));
				TiXmlElement inode("Input");
				inode.SetAttribute("id", input->valueOffset / NODE_SLOTS);
				node.InsertEndChild(inode);
			}
			root.InsertEndChild(node);
		}
		// normal nodes
		else if (n->id != CONSTANT_ID)
		{
			NodeGUIInfo* ni = &(_nodeGUIInfo[n->valueOffset]);
			// modadders are not saved
			if (ni->IsModAdder)
				return;

			TiXmlElement node("Node");
			node.SetAttribute("type", n->id);
			node.SetAttribute("global", n->isGlobal);
			node.SetAttribute("channel", ni->FixedChannel);
			node.SetAttribute("x", (int)ni->X);
			node.SetAttribute("y", (int)ni->Y);
			node.SetAttribute("id", n->valueOffset / NODE_SLOTS);
			if (ni->Name != "")
				node.SetAttribute("name", ni->Name);
			// required inputs
			for (DWORD i = 0; i < NodeReqGUISignals[n->id]; i++)
			{
				SynthNode* input = ((SynthNode*)(n->input[i]));
				TiXmlElement inode("Input");
				inode.SetAttribute("id", input->valueOffset / NODE_SLOTS);
				node.InsertEndChild(inode);
			}

			// modulation inputs
			DWORD maxguisignals = NodeMaxGUISignals[n->id];
			// scale and voice constants have a non mode constant
			if (n->id == SCALE_ID ||
				n->id == MIDISIGNAL_ID ||
				n->id == OSRAND_ID ||
				n->id > CONSTANT_ID)
				maxguisignals++;
			// parameter/modulation inputs
			for (DWORD i = NodeReqGUISignals[n->id]; i < maxguisignals; i++)
			{
				SynthNode* input = ((SynthNode*)(n->input[i]));
				TiXmlElement inode("Parameter");
				// modadder not used?
				if (input->id == CONSTANT_ID)
				{
					setAttrDouble6(inode, "value1", input->out.d[0]);
					setAttrDouble6(inode, "value2", input->out.d[1]);
				}
				// modadder used?
				else
				{
					SynthNode* modadder = ni->ModAdder[i];
					setAttrDouble6(inode, "value1", modadder->input[0]->d[0]);
					setAttrDouble6(inode, "value2", modadder->input[0]->d[1]);
					inode.SetAttribute("input", ((SynthNode*)(modadder->input[1]))->valueOffset / NODE_SLOTS);
				}
				node.InsertEndChild(inode);
			}

			// modes
			for (DWORD i = maxguisignals; i < n->numInputs; i++)
			{
				SynthNode* input = ((SynthNode*)(n->input[i]));
				TiXmlElement inode("Mode");
				// midi signal needs to check if the current channel is the set channel
				if (n->id == MIDISIGNAL_ID)
				{
					int c = input->out.i[0] >> 7;
					int cc = input->out.i[0] & 127;
					if (c == saveChannel)
						c = 16;
					inode.SetAttribute("value", (c << 7) | cc);
				}
				else
				{
					inode.SetAttribute("value", input->out.i[0]);
				}
				node.InsertEndChild(inode);
			}

			// data for voicemanager
			if (n->id == VOICEMANAGER_ID)
			{
				TiXmlElement dnode("Data");
				TiXmlElement lnode("LoopIndex");
				lnode.SetAttribute("value", ((VMWork*)(n->customMem))->ArpSequenceLoopIndex);
				dnode.InsertEndChild(lnode);
				for (int i = 0; i < 32; i++)
				{
					TiXmlElement snode("StepData");
					snode.SetAttribute("value", ((VMWork*)(n->customMem))->ArpSequence[i]);
					dnode.InsertEndChild(snode);
				}
				node.InsertEndChild(dnode);
			}
			// data for sapi
			if (n->id == SAPI_ID)
			{
				TiXmlElement dnode("Data");
				TiXmlText tnode("TTS");
				tnode.SetCDATA(true);
				tnode.SetValue((char*)(n->specialData));
				dnode.InsertEndChild(tnode);
				node.InsertEndChild(dnode);
			}
			// data for formula
			if (n->id == FORMULA_ID)
			{
				TiXmlElement dnode("Data");
				TiXmlText tnode("Formula");
				tnode.SetCDATA(true);
				tnode.SetValue((char*)(n->specialDataText.c_str()));
				dnode.InsertEndChild(tnode);
				TiXmlText tnode2("RPN");
				tnode2.SetCDATA(true);
				tnode2.SetValue((char*)(n->specialDataText2.c_str()));
				dnode.InsertEndChild(tnode2);
				node.InsertEndChild(dnode);
			}
			root.InsertEndChild(node);
		}
		// only manually created constants
		else if (n->id == CONSTANT_ID)
		{
			NodeGUIInfo* ni = &(_nodeGUIInfo[n->valueOffset]);
			if (!ni->Visible)
				return;

			TiXmlElement node("Constant");
			node.SetAttribute("type", n->id);
			node.SetAttribute("global", n->isGlobal);
			node.SetAttribute("x", (int)ni->X);
			node.SetAttribute("y", (int)ni->Y);
			node.SetAttribute("id", n->valueOffset / NODE_SLOTS);
			if (ni->Name != "")
				node.SetAttribute("name", ni->Name);
			setAttrDouble6(node, "value1", n->out.d[0]);
			setAttrDouble6(node, "value2", n->out.d[1]);
			root.InsertEndChild(node);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::recursiveSaveNode(SynthNode* node, int channel, TiXmlElement& root, bool &interconnected)
{
	NodeGUIInfo* gi = &(_nodeGUIInfo[node->valueOffset]);
	// skip already processed nodes
	if (gi->RecursionFlag == 0)
	{
		gi->RecursionFlag = 1;

		// bail out if the node is used by more than one channel
		if (gi->Channels.size() > 1)
		{
			interconnected = true;
			return;
		}

		// save the node
		saveNode(node, root, channel);

		// recursion not for constants
		if (node->id != CONSTANT_ID)
		{
			for (DWORD i = 0; i < node->numInputs; i++)
			{
				SynthNode* input = (SynthNode*)(node->input[i]);
				// only write until a global node not of our channel is hit
				if (input->id <= NOTECONTROLLER_ID && _nodeGUIInfo[input->valueOffset].FixedChannel != channel)
					continue;
				else
					recursiveSaveNode(input, channel, root, interconnected);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::loadNode(TiXmlElement* child, std::map<int, SynthNode*>& idNodeMap, std::vector<Connection>& connections, int targetChannel, SynthNode* channelRoot)
{
	if (child->ValueStr() == "Node")
	{
		int type;
		child->QueryIntAttribute("type", &type);
		int global;
		child->QueryIntAttribute("global", &global);
		int channel;
		child->QueryIntAttribute("channel", &channel);
		if (channel != -2 && targetChannel != -2)
			channel = targetChannel;
		int x;
		child->QueryIntAttribute("x", &x);
		int y;
		child->QueryIntAttribute("y", &y);
		int id;
		child->QueryIntAttribute("id", &id);
		std::string name;
		child->QueryStringAttribute("name", &name);
		SynthNode* node;
		// create the node
		if (!channelRoot)
		{
			node = createGUINode(type, channel, global, x, y);
		}
		// special case when loading channels
		else
		{
			node = channelRoot;
			_nodeGUIInfo[node->valueOffset].X = x;
			_nodeGUIInfo[node->valueOffset].Y = y;
		}
		_nodeGUIInfo[node->valueOffset].Name = name;
		idNodeMap[id] = node;

		// get connections and set the parameters & modes
		int i = 0;
		TiXmlElement* inputs = 0;
		while( inputs = (TiXmlElement*)(child->IterateChildren( inputs )) )
		{
			if (inputs->ValueStr() == "Input")
			{
				int inputid;
				inputs->QueryIntAttribute("id", &inputid);
				Connection con = { inputid, id, i };
				connections.push_back(con);
			}
			else if (inputs->ValueStr() == "Parameter")
			{
				double v1;
				inputs->QueryDoubleAttribute("value1", &v1);
				double v2;
				inputs->QueryDoubleAttribute("value2", &v2);
				node->input[i]->d[0] = v1;
				node->input[i]->d[1] = v2;
				// modulation input?
				int inputid = 0;
				if (inputs->QueryIntAttribute("input", &inputid) == TIXML_SUCCESS)
				{
					Connection con = { inputid, id, i };
					connections.push_back(con);
				}
			}
			else if (inputs->ValueStr() == "Mode")
			{
				int mode;
				inputs->QueryIntAttribute("value", &mode);
				// midi signal needs to check if the current channel is the set channel
				if (node->id == MIDISIGNAL_ID)
				{
					int c = mode >> 7;
					int cc = mode & 127;
					if (c == 16)
						c = targetChannel;
					mode = (c << 7) | cc;
				}
				int modeIndex = i;

				// old glitch had mode on index 5, GLITCH_SPEED is there now
				if (node->id == GLITCH_ID)
				{
					if (modeIndex == GLITCH_SPEED)
						modeIndex = GLITCH_MODE;
				}
				// old distortion node had mode on index 2, DISTORTION_THRESHOLD is there now
				if (node->id == DISTORTION_ID)
				{
					if (modeIndex == DISTORTION_THRESHOLD)
						modeIndex = DISTORTION_MODE;
				}
				// old oscillator node had mode on index 6, OSCILLATOR_UDETUNE is there now
				if (node->id == OSCILLATOR_ID)
				{
					if (modeIndex == OSCILLATOR_UDETUNE)
						modeIndex = OSCILLATOR_MODE;
				}

				// set the mode
				node->input[modeIndex]->i[0] = mode;
			}

			if (inputs->ValueStr() == "Data")
			{
				// data for voicemanager
				if (node->id == VOICEMANAGER_ID)
				{
					int step = 0;
					TiXmlElement* dataitem = 0;
					while( dataitem = (TiXmlElement*)(inputs->IterateChildren( dataitem )) )
					{
						int value;
						dataitem->QueryIntAttribute("value", &value);
						if (dataitem->ValueStr() == "LoopIndex")
						{
							((VMWork*)(node->customMem))->ArpSequenceLoopIndex = value;
						}
						if (dataitem->ValueStr() == "StepData")
						{
							((VMWork*)(node->customMem))->ArpSequence[step] = value;
							step++;
						}
					}
				}
				// data for sapi
				if (node->id == SAPI_ID)
				{
					TiXmlText* tnode = (TiXmlText*)(inputs->FirstChild());
					std::string tts = tnode->ValueStr();
					setSAPIText(node->valueOffset, tts);
				}
				// data for formula
				if (node->id == FORMULA_ID)
				{
					TiXmlText* tnode = (TiXmlText*)(inputs->FirstChild());
					std::string formula = tnode->ValueStr();
					tnode = (TiXmlText*)(inputs->LastChild());
					std::string rpn = tnode->ValueStr();
					setFormulaText(node->valueOffset, formula, rpn);
				}
			}

			i++;
		}
	}
	else if (child->ValueStr() == "Constant")
	{
		int type;
		child->QueryIntAttribute("type", &type);
		int global;
		child->QueryIntAttribute("global", &global);
		int x;
		child->QueryIntAttribute("x", &x);
		int y;
		child->QueryIntAttribute("y", &y);
		int id;
		child->QueryIntAttribute("id", &id);
		std::string name;
		child->QueryStringAttribute("name", &name);
		double v1;
		child->QueryDoubleAttribute("value1", &v1);
		double v2;
		child->QueryDoubleAttribute("value2", &v2);
		// create the node
		SynthNode* node = createGUINode(type, -1, global, x, y);
		_nodeGUIInfo[node->valueOffset].Name = name;
		idNodeMap[id] = node;
		// set the values
		node->out.d[0] = v1;
		node->out.d[1] = v2;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::loadPatch(const std::string& filename)
{
	TiXmlDocument doc( filename );
	if (!doc.LoadFile())
	{
		return false;
	}

	DataAccessMutex.lock();
	_massDataUpdate = true;

	// kill all active voices
	killVoices();

	// clear patch
	resetPatch(false, false);

	// id to node map
	std::map<int, SynthNode*> idNodeMap;
	// connection list
	std::vector<Connection> connections;

	TiXmlElement* root = doc.RootElement();

	// step 1: create nodes
	TiXmlElement* child = 0;
	while( child = (TiXmlElement*)(root->IterateChildren( child )) )
	{
		// load the node
		loadNode(child, idNodeMap, connections);

		// load data
		if (child->ValueStr() == "Data")
		{
			TiXmlElement* dchild = 0;
			while( dchild = (TiXmlElement*)(child->IterateChildren( dchild )) )
			{
				if (dchild->ValueStr() == "WaveFileReferences")
				{
					TiXmlElement* wfrchild = 0;
					while( wfrchild = (TiXmlElement*)(dchild->IterateChildren( wfrchild )) )
					{
						int index;
						wfrchild->QueryIntAttribute("index", &index);
						int format;
						wfrchild->QueryIntAttribute("format", &format);
						int frequency;
						wfrchild->QueryIntAttribute("frequency", &frequency);
						std::string wfilename;
						wfrchild->QueryStringAttribute("filename", &wfilename);

						if (wfilename != "")
						{
							// get absolute path from patch filename
							int ls = (int)filename.rfind("/");
							int lbs= (int)filename.rfind("\\");
							if (lbs > ls)
								ls = lbs;
							std::string path = filename.substr(0, ls+1);
							std::string wfp = path + wfilename;
							setWaveFileReference(index, format, frequency, wfp);
						}
						else
						{
							setWaveFileReference(index, format, frequency, "");
						}
					}
				}
			}
		}
	}

	// step 2: establish connections
	for (DWORD i = 0; i < connections.size(); i++)
	{
		// show only non default0 connections
		if (connections[i].from != (MAX_NODES-1))
		{
			SynthNode* from = idNodeMap[connections[i].from];
			SynthNode* to = idNodeMap[connections[i].to];
			int index = connections[i].index;
			// establish connection
			connectInput(from->valueOffset, to->valueOffset, index);
		}
	}

	_massDataUpdate = false;
	DataAccessMutex.unlock();

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::savePatch(const std::string& filename)
{
	TiXmlDocument doc( filename);

	TiXmlDeclaration decl("1.0", "utf-8", "yes");
	doc.InsertEndChild(decl);
	TiXmlElement root("Patch");
	root.SetAttribute("version", "1.0");

	// write nodes
	for (DWORD i = 0; i < MAX_NODES; i++)
	{
		SynthNode* n = _nodes[i];
		saveNode(n, root);
	}

	// write data
	TiXmlElement data("Data");
	// referenced wave files for playback
	TiXmlElement wfr("WaveFileReferences");
	for (int i = 0; i < 32; i++)
	{
		TiXmlElement wf("WaveFile");

		wf.SetAttribute("index", i);
		wf.SetAttribute("format", SynthGlobalState.RawWaveFormat[i]);
		wf.SetAttribute("frequency", SynthGlobalState.RawWaveFrequency[i]);
		std::string wfilename = SynthGlobalState.RawWaveFileName[i];
		// check if a local copy of referenced file is needed
		if (wfilename != "")
		{
			std::string sourcepath = wfilename;
			// get filename from sourcepath
			int ls = (int)wfilename.rfind("/");
			int lbs= (int)wfilename.rfind("\\");
			if (lbs > ls)
				ls = lbs;
			wfilename = wfilename.substr(ls+1);

			// get absolute path from patch filename
			ls = (int)filename.rfind("/");
			lbs= (int)filename.rfind("\\");
			if (lbs > ls)
				ls = lbs;
			std::string targetpath = filename.substr(0, ls+1);
			targetpath += wfilename;

			// copy if needed
			if (targetpath != sourcepath)
			{
				std::ifstream  src(sourcepath.c_str(), std::ios::binary);
				std::ofstream  dst(targetpath.c_str(), std::ios::binary);
				dst << src.rdbuf();
			}
		}
		wf.SetAttribute("filename", wfilename);

		wfr.InsertEndChild(wf);
	}
	data.InsertEndChild(wfr);
	root.InsertEndChild(data);

	doc.InsertEndChild(root);
	doc.SaveFile();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SynthController::savePatchToString()
{
	TiXmlDocument doc;

	TiXmlDeclaration decl("1.0", "utf-8", "yes");
	doc.InsertEndChild(decl);
	TiXmlElement root("Patch");
	root.SetAttribute("version", "1.0");

	// write nodes
	for (DWORD i = 0; i < MAX_NODES; i++)
	{
		SynthNode* n = _nodes[i];
		saveNode(n, root);
	}

	// write data (wave file references without file paths — in-memory only)
	TiXmlElement data("Data");
	TiXmlElement wfr("WaveFileReferences");
	for (int i = 0; i < 32; i++)
	{
		TiXmlElement wf("WaveFile");
		wf.SetAttribute("index", i);
		wf.SetAttribute("format", SynthGlobalState.RawWaveFormat[i]);
		wf.SetAttribute("frequency", SynthGlobalState.RawWaveFrequency[i]);
		wf.SetAttribute("filename", SynthGlobalState.RawWaveFileName[i]);
		wfr.InsertEndChild(wf);
	}
	data.InsertEndChild(wfr);
	root.InsertEndChild(data);
	doc.InsertEndChild(root);

	TiXmlPrinter printer;
	doc.Accept(&printer);
	return printer.Str();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::loadPatchFromString(const std::string& xmlData)
{
	TiXmlDocument doc;
	doc.Parse(xmlData.c_str());
	if (doc.Error())
	{
		return false;
	}

	DataAccessMutex.lock();
	_massDataUpdate = true;

	// kill all active voices
	killVoices();

	// clear patch
	resetPatch(false, false);

	// id to node map
	std::map<int, SynthNode*> idNodeMap;
	// connection list
	std::vector<Connection> connections;

	TiXmlElement* root = doc.RootElement();

	// step 1: create nodes
	TiXmlElement* child = 0;
	while( child = (TiXmlElement*)(root->IterateChildren( child )) )
	{
		loadNode(child, idNodeMap, connections);

		if (child->ValueStr() == "Data")
		{
			TiXmlElement* dchild = 0;
			while( dchild = (TiXmlElement*)(child->IterateChildren( dchild )) )
			{
				if (dchild->ValueStr() == "WaveFileReferences")
				{
					TiXmlElement* wfrchild = 0;
					while( wfrchild = (TiXmlElement*)(dchild->IterateChildren( wfrchild )) )
					{
						int index;
						wfrchild->QueryIntAttribute("index", &index);
						int format;
						wfrchild->QueryIntAttribute("format", &format);
						int frequency;
						wfrchild->QueryIntAttribute("frequency", &frequency);
						std::string wfilename;
						wfrchild->QueryStringAttribute("filename", &wfilename);

						if (wfilename != "")
							setWaveFileReference(index, format, frequency, wfilename);
						else
							setWaveFileReference(index, format, frequency, "");
					}
				}
			}
		}
	}

	// step 2: establish connections
	for (DWORD i = 0; i < connections.size(); i++)
	{
		if (connections[i].from != (MAX_NODES-1))
		{
			SynthNode* from = idNodeMap[connections[i].from];
			SynthNode* to = idNodeMap[connections[i].to];
			int index = connections[i].index;
			connectInput(from->valueOffset, to->valueOffset, index);
		}
	}

	_massDataUpdate = false;
	DataAccessMutex.unlock();

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::resetPatch(bool createDefault, bool acquireMutex)
{
	if (acquireMutex)
	{
		_massDataUpdate = true;
		DataAccessMutex.lock();
	}

	// delete nodes (except global 0)
	for (DWORD i = 0; i < MAX_NODES; i++)
	{
		if (_nodes[i])
			deleteNode(_nodes[i]->valueOffset);
	}

	// create default 0 constant
	_nodes[MAX_NODES-1] = createNode(CONSTANT_ID, -1, 1);
	_nodes[MAX_NODES-1]->numInputs = 1; // default0 is a mono float constant

	// create default node hierarchy when needed
	if (createDefault)
	{
		SynthNode* synthroot = createGUINode(SYNTHROOT_ID, -1, 1, 16384 + 3000, 16384);
		SynthNode* multiadd = createGUINode(MULTIADD_ID, -1, 1, 16384 + 2400, 16384);
		connectInput(multiadd->valueOffset, synthroot->valueOffset, SYNTHROOT_IN);

		// create the channels
		for (int c = 0; c < 16; c++)
		{
			int channely = 16384 + (c-8) * 1200 + 300;

			SynthNode* channelroot = createGUINode(CHANNELROOT_ID, c, 1, 16384 + 600, channely);
			connectInput(channelroot->valueOffset, multiadd->valueOffset, 0);

			SynthNode* notecontroller = createGUINode(NOTECONTROLLER_ID, c, 1, 16384 + 400, channely);
			connectInput(notecontroller->valueOffset, channelroot->valueOffset, CHANNELROOT_IN);

			SynthNode* voicemanager = createGUINode(VOICEMANAGER_ID, -2, 1, 16384 + 200, channely);
			connectInput(voicemanager->valueOffset, notecontroller->valueOffset, 0);

			// voice nodes
			SynthNode* intrumentroot = createGUINode(VOICEROOT_ID, -2, 0, 16384-200, channely);
			connectInput(intrumentroot->valueOffset, voicemanager->valueOffset, VOICEMANAGER_VOICEROOT);
			// move way to the left
			for (DWORD i = NodeMaxGUISignals[intrumentroot->id]; i < NodeInputs[intrumentroot->id]; i++)
			{
				_nodeGUIInfo[((SynthNode*)(intrumentroot->input[i]))->valueOffset].X -= 800;
				_nodeGUIInfo[((SynthNode*)(intrumentroot->input[i]))->valueOffset].Y -= 40 - (i-2)*30;
			}

			SynthNode* osc = createGUINode(OSCILLATOR_ID, -2, 0, 16384 - 600, channely - 100);
			connectInput(osc->valueOffset, intrumentroot->valueOffset, VOICEROOT_IN);

			SynthNode* adsr = createGUINode(ADSR_ID, -2, 0, 16384 - 600, 100 + channely);
			SynthNode* node = adsr;
			SETMODE(ADSR_MODE, sample_t((int)(ADSR_VOICETRIGGER | ADSR_VOICEGATE | ADSR_EXP)));

			connectInput(adsr->valueOffset, intrumentroot->valueOffset, VOICEROOT_GAIN);

			SynthNode* gateout = createGUINode(EVENTSIGNAL_ID, -2, 0, 16384 - 400, 150 + channely);
			connectInput(adsr->valueOffset, gateout->valueOffset, EVENTSIGNAL_IN);
			connectInput(gateout->valueOffset, intrumentroot->valueOffset, VOICEROOT_GATE_OUT);
		}
	}

	if (acquireMutex)
	{
		_massDataUpdate = false;
		DataAccessMutex.unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::loadChannel(int channel, const std::string& filename)
{
	TiXmlDocument doc( filename );
	if (!doc.LoadFile())
	{
		return false;
	}

	DataAccessMutex.lock();
	_massDataUpdate = true;

	killVoices();

	// find the channel root and clear the recursion and channels usage info for all nodes
	SynthNode* channelRoots[MAX_CHANNELS];
	for (DWORD i = 0; i < MAX_NODES; i++)
	{
		if (_nodes[i])
		{
			if (_nodes[i]->id == CHANNELROOT_ID)
				channelRoots[_nodeGUIInfo[_nodes[i]->valueOffset].FixedChannel] = _nodes[i];

			_nodeGUIInfo[_nodes[i]->valueOffset].RecursionFlag = 0;
			_nodeGUIInfo[_nodes[i]->valueOffset].Channels.clear();
		}
	}
	// get the reference xy pos from current channel root
	double refX = _nodeGUIInfo[channelRoots[channel]->valueOffset].X;
	double refY = _nodeGUIInfo[channelRoots[channel]->valueOffset].Y;

	// propagate channel usage of nodes to all nodes beeing input to channel roots
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		for (DWORD n = 0; n < MAX_NODES; n++)
		{
			if (_nodes[n])
				_nodeGUIInfo[_nodes[n]->valueOffset].RecursionFlag = 0;
		}
		recursiveAddChannel(channelRoots[i], i);
	}

	// check the channel hierarchy for unique usage
	bool interconnected = false;
	for (DWORD i = 0; i < MAX_NODES; i++)
	{
		if (_nodes[i])
		{
			if ((_nodeGUIInfo[_nodes[i]->valueOffset].Channels.find(channel) != _nodeGUIInfo[_nodes[i]->valueOffset].Channels.end()) && (_nodeGUIInfo[_nodes[i]->valueOffset].Channels.size() > 1))
			{
				interconnected = true;
				break;
			}
		}
	}

	// no interconnections from/to the channel? load the new file
	if (!interconnected)
	{
		// step 0: delete all nodes that have the fixedchannel as the provided channel or are exactly belonging to the provided channel (and no other)
		for (DWORD i = 0; i < MAX_NODES; i++)
		{
			if (_nodes[i])
			{
				NodeGUIInfo* gi = &(_nodeGUIInfo[_nodes[i]->valueOffset]);
				if ((gi->FixedChannel == channel) ||
					(gi->Channels.find(channel) != gi->Channels.end()))
				{
					// dont delete the channel root itself
					if (_nodes[i] != channelRoots[channel])
						deleteNode(_nodes[i]->valueOffset);
				}
			}
		}

		// id to node map
		std::map<int, SynthNode*> idNodeMap;
		// connection list
		std::vector<Connection> connections;

		TiXmlElement* root = doc.RootElement();

		// step 1: create nodes
		TiXmlElement* child = 0;
		// get the channel root directly but dont create the node, just update the values
		child = (TiXmlElement*)(root->IterateChildren( child ));
		loadNode(child, idNodeMap, connections, channel, channelRoots[channel]);
		// load the remaining nodes
		while( child = (TiXmlElement*)(root->IterateChildren( child )) )
		{
			loadNode(child, idNodeMap, connections, channel);
		}

		// step 2: reposition nodes
		double refXNew = _nodeGUIInfo[channelRoots[channel]->valueOffset].X;
		double refYNew = _nodeGUIInfo[channelRoots[channel]->valueOffset].Y;
		for (std::map<int, SynthNode*>::iterator it = idNodeMap.begin(); it != idNodeMap.end(); it++)
		{
			SynthNode* curNode = it->second;
			double dX = _nodeGUIInfo[curNode->valueOffset].X - refXNew;
			double dY = _nodeGUIInfo[curNode->valueOffset].Y - refYNew;
			_nodeGUIInfo[curNode->valueOffset].X = refX + dX;
			_nodeGUIInfo[curNode->valueOffset].Y = refY + dY;
		}

		// step 3: establish connections
		for (DWORD i = 0; i < connections.size(); i++)
		{
			if ((connections[i].from != (MAX_NODES-1)) && (idNodeMap.find(connections[i].from) != idNodeMap.end()))
			{
				SynthNode* from = idNodeMap[connections[i].from];
				SynthNode* to = idNodeMap[connections[i].to];
				int index = connections[i].index;
				connectInput(from->valueOffset, to->valueOffset, index);
			}
		}

		// Set channel name from filename when empty (e.g. old channel files without name attribute)
		if (_nodeGUIInfo[channelRoots[channel]->valueOffset].Name.empty())
		{
			int ls = (int)filename.rfind("/");
			int lbs = (int)filename.rfind("\\");
			if (lbs > ls)
				ls = lbs;
			std::string name = filename.substr(ls + 1);
			int ext = (int)name.find_last_of(".");
			if (ext > 0)
				name = name.substr(0, ext);
			_nodeGUIInfo[channelRoots[channel]->valueOffset].Name = name;
		}
	}

	_massDataUpdate = false;
	DataAccessMutex.unlock();

	return !interconnected;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::saveChannel(int channel, const std::string& filename)
{
	TiXmlDocument doc( filename);

	TiXmlDeclaration decl("1.0", "utf-8", "yes");
	doc.InsertEndChild(decl);
	TiXmlElement root("Channel");
	root.SetAttribute("version", "1.0");

	// find the channel root and clear the recursion and channels usage info for all nodes
	SynthNode* channelRoots[MAX_CHANNELS];
	for (DWORD i = 0; i < MAX_NODES; i++)
	{
		if (_nodes[i])
		{
			if (_nodes[i]->id == CHANNELROOT_ID)
				channelRoots[_nodeGUIInfo[_nodes[i]->valueOffset].FixedChannel] = _nodes[i];

			_nodeGUIInfo[_nodes[i]->valueOffset].RecursionFlag = 0;
			_nodeGUIInfo[_nodes[i]->valueOffset].Channels.clear();
		}
	}

	// propagate channel usage of nodes to all nodes beeing input to channel roots
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		for (DWORD n = 0; n < MAX_NODES; n++)
		{
			if (_nodes[n])
				_nodeGUIInfo[_nodes[n]->valueOffset].RecursionFlag = 0;
		}
		recursiveAddChannel(channelRoots[i], i);
	}

	// Always update channel name from filename (for Save and Save As)
	{
		int ls = (int)filename.rfind("/");
		int lbs = (int)filename.rfind("\\");
		if (lbs > ls)
			ls = lbs;
		std::string name = filename.substr(ls + 1);
		int ext = (int)name.find_last_of(".");
		if (ext > 0)
			name = name.substr(0, ext);
		_nodeGUIInfo[channelRoots[channel]->valueOffset].Name = name;
	}

	// recursively call the channel hierarchy
	bool interconnected = false;
	recursiveSaveNode(channelRoots[channel], channel, root, interconnected);

	if (interconnected)
		return 0;

	doc.InsertEndChild(root);
	doc.SaveFile();
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::resetChannel(int channel, bool createDefault)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::loadSelection(const std::string& filename, int refX, int refY)
{
	TiXmlDocument doc( filename );
	if (!doc.LoadFile())
	{
		return false;
	}

	clearSelection();

	// id to node map
	std::map<int, SynthNode*> idNodeMap;
	// connection list
	std::vector<Connection> connections;

	TiXmlElement* root = doc.RootElement();

	// step 1: create nodes and find minXY
	TiXmlElement* child = 0;
	int minX = 100000000;
	int minY = 100000000;
	while( child = (TiXmlElement*)(root->IterateChildren( child )) )
	{
		loadNode(child, idNodeMap, connections);
		int x;
		child->QueryIntAttribute("x", &x);
		int y;
		child->QueryIntAttribute("y", &y);
		if (x < minX)
			minX = x;
		if (y < minY)
			minY = y;
	}

	// step 2: reposition nodes
	double refXNew = minX;
	double refYNew = minY;
	for (std::map<int, SynthNode*>::iterator it = idNodeMap.begin(); it != idNodeMap.end(); it++)
	{
		SynthNode* curNode = it->second;
		double dX = _nodeGUIInfo[curNode->valueOffset].X - refXNew;
		double dY = _nodeGUIInfo[curNode->valueOffset].Y - refYNew;
		_nodeGUIInfo[curNode->valueOffset].X = refX + dX;
		_nodeGUIInfo[curNode->valueOffset].Y = refY + dY;
		_nodeGUIInfo[curNode->valueOffset].IsSelected = true;
	}

	// step 3: establish connections
	for (DWORD i = 0; i < connections.size(); i++)
	{
		if ((connections[i].from != (MAX_NODES-1)) && (idNodeMap.find(connections[i].from) != idNodeMap.end()))
		{
			SynthNode* from = idNodeMap[connections[i].from];
			SynthNode* to = idNodeMap[connections[i].to];
			int index = connections[i].index;
			connectInput(from->valueOffset, to->valueOffset, index);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::saveSelection(const std::string& filename)
{
	TiXmlDocument doc( filename);

	TiXmlDeclaration decl("1.0", "utf-8", "yes");
	doc.InsertEndChild(decl);
	TiXmlElement root("Selection");
	root.SetAttribute("version", "1.0");

	// save all selected nodes
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it = _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		if (it->second.IsSelected)
			saveNode(it->second.Node, root);
	}

	doc.InsertEndChild(root);
	doc.SaveFile();
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setWaveFileReference(int index, int format, int frequency, const std::string& wfpath)
{
	// block mutex if updates are not a mass data update
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	// clear
	if (SynthGlobalState.RawWaveTable[index])
		SynthFree(SynthGlobalState.RawWaveTable[index]);
	SynthGlobalState.RawWaveTable[index] = 0;
	SynthGlobalState.RawWaveFormat[index] = 0;
	SynthGlobalState.RawWaveFrequency[index] = 0;
	SynthGlobalState.RawWaveFileName[index] = "";
	SynthGlobalState.RawWavePackedSize[index] = 0;
	SynthGlobalState.CompSampleRate[index] = 0;
	SynthGlobalState.CompAvgBytes[index] = 0;
	SynthGlobalState.CompWaveTableSize[index] = 0;
	if (SynthGlobalState.CompWaveTable[index])
		SynthFree(SynthGlobalState.CompWaveTable[index]);
	SynthGlobalState.CompWaveTable[index] = 0;

	if (!_massDataUpdate)
		DataAccessMutex.unlock();

	if (wfpath == "")
		return;

	int compFreqIndex = frequency; // 0 = 11khz, 1 = 22khz, 2 = 44khz

	struct FormatFrequencies
	{
		DWORD	sampleRate;
		DWORD	avgBytes;
	};
	FormatFrequencies fmtFreqs[3] =
	{
		{ 11025, 2239 },
		{ 22050, 4478 },
		{ 44100, 8957 },
	};

	DWORD SampleRate = 0;
	int nBytes = 0;
	int nChannels = 0;
	bool needResampling = false;

	uint32_t srcBufSize = 0;
	uint32_t dstBufSize = 0;
	uint8_t* srcBuf = NULL;
	uint8_t* dstBuf = NULL;

	// read the input wav
	std::ifstream file;
	file.open(wfpath.c_str(), std::ios_base::binary );
	if (file.is_open())
	{
		DWORD riff;
		file.read((char*)&riff, 4);
		if (riff != MAKEFOURCC('R', 'I', 'F', 'F'))
		{
			fprintf(stderr, "64klang: %s\n", "Invalid File Type");
			file.close();
			return;
		}
		DWORD fsize;
		file.read((char*)&fsize, 4);
		DWORD wave;
		file.read((char*)&wave, 4);
		if (wave != MAKEFOURCC('W', 'A', 'V', 'E'))
		{
			fprintf(stderr, "64klang: %s\n", "Unsupported file type");
			file.close();
			return;
		}
		DWORD fmt;
		file.read((char*)&fmt, 4);
		if (fmt != MAKEFOURCC('f', 'm', 't', ' '))
		{
			fprintf(stderr, "64klang: %s\n", "Expected 'fmt ' chunk");
			file.close();
			return;
		}
		DWORD fmtsize;
		file.read((char*)&fmtsize, 4);
		WORD fmtid;
		file.read((char*)&fmtid, 2);
		if (fmtid != WAVE_FORMAT_PCM)
		{
			fprintf(stderr, "64klang: %s\n", "File must be PCM format");
			file.close();
			return;
		}
		WORD channels;
		file.read((char*)&channels, 2);
		nChannels = channels;
		if (nChannels != 1)
		{
			fprintf(stderr, "64klang: %s\n", "File must be mono");
			file.close();
			return;
		}
		file.read((char*)&SampleRate, 4);
		if (SampleRate != fmtFreqs[compFreqIndex].sampleRate)
		{
			needResampling = true;
			if (SampleRate < fmtFreqs[compFreqIndex].sampleRate)
			{
				fprintf(stderr, "64klang: %s\n", "File sample rate is lower than target sample rate. Please perform upsampling in some other editor!");
				return;
			}
		}
		DWORD bytesPerSec;
		file.read((char*)&bytesPerSec, 4);
		WORD blockAlign;
		file.read((char*)&blockAlign, 2);
		WORD bitsPerSample;
		file.read((char*)&bitsPerSample, 2);
		nBytes = bitsPerSample/8;
		DWORD data;
		file.read((char*)&data, 4);
		if (data != MAKEFOURCC('d', 'a', 't', 'a'))
		{
			fprintf(stderr, "64klang: %s\n", "Expected 'data' chunk");
			file.close();
			return;
		}
		DWORD datasize;
		file.read((char*)&datasize, 4);
		// align to 320 (samplesperblock of gsm)
		DWORD effectiveDatasize = datasize;
		DWORD blockRemainder = datasize % (320*nBytes);
		if (blockRemainder != 0)
			effectiveDatasize = ((datasize / (320*nBytes))+1) * (320*nBytes);
		srcBuf = (LPBYTE)SynthMalloc( effectiveDatasize );
		srcBufSize = effectiveDatasize;
		file.read((char*)srcBuf, datasize);
		file.close();
	}
	else
	{
		fprintf(stderr, "64klang: Couldn't open file: %s\n", wfpath.c_str());
		return;
	}

#ifdef _WIN32
	GSM610WAVEFORMAT gsmFormat =
	{
		{
			WAVE_FORMAT_GSM610,
			1,      // WORD        nChannels;
			0,		// DWORD       nSamplesPerSec;
			0,		// DWORD       nAvgBytesPerSec;
			65,		// WORD        nBlockAlign;
			0,      // WORD        wBitsPerSample;
			2		// WORD        cbSize;
		},
		320			// WORD        wSamplesPerBlock
	};
	WAVEFORMATEX pcmFormat =
	{
		WAVE_FORMAT_PCM,
		1,      // WORD        nChannels;
		0,		// DWORD       nSamplesPerSec;
		0,		// DWORD       nAvgBytesPerSec;
		0,		// WORD        nBlockAlign;
		0,      // WORD        wBitsPerSample;
		0,		// WORD        cbSize;
	};

	// set pcm format
	pcmFormat.nSamplesPerSec = fmtFreqs[compFreqIndex].sampleRate;
	pcmFormat.nAvgBytesPerSec = fmtFreqs[compFreqIndex].sampleRate*nChannels*nBytes;
	pcmFormat.nBlockAlign = nChannels*nBytes;
	pcmFormat.wBitsPerSample = 8*nBytes;
	// set gsm format
	gsmFormat.wfx.nSamplesPerSec = fmtFreqs[compFreqIndex].sampleRate;
	gsmFormat.wfx.nAvgBytesPerSec = fmtFreqs[compFreqIndex].avgBytes;
#endif

	// up/downsample srcBuf when input is not matching the target frequency
	if (needResampling)
	{
		if (SampleRate > fmtFreqs[compFreqIndex].sampleRate)
		{
			if (nBytes == 2)
			{
				int loops = SampleRate / fmtFreqs[compFreqIndex].sampleRate;
				srcBufSize /= 2;
				short filter_state = 0;
				downsample<short>((short*)srcBuf, (short*)srcBuf, srcBufSize/2, filter_state);
				if (loops == 4)
				{
					srcBufSize /= 2;
					filter_state = 0;
					downsample<short>((short*)srcBuf, (short*)srcBuf, srcBufSize/2, filter_state);
				}
			}
			else
			{
				int loops = SampleRate / fmtFreqs[compFreqIndex].sampleRate;
				srcBufSize /= 2;
				char filter_state = 0;
				downsample<char>((char*)srcBuf, (char*)srcBuf, srcBufSize, filter_state);
				if (loops == 4)
				{
					srcBufSize /= 2;
					filter_state = 0;
					downsample<char>((char*)srcBuf, (char*)srcBuf, srcBufSize, filter_state);
				}
			}

			// align reduced buffer to 320 sample boundarys for gsm blocksize fit
			DWORD zeroPaddingBytes = 0;
			DWORD blockRemainder = srcBufSize % (320*nBytes);
			if (blockRemainder != 0)
			{
				zeroPaddingBytes = (((srcBufSize / (320*nBytes))+1) * (320*nBytes)) - srcBufSize;
				memset(srcBuf+srcBufSize, 0, zeroPaddingBytes);
				srcBufSize += zeroPaddingBytes;
			}
		}
	}

#ifdef _WIN32
	// convert to gsm
	int acmRes;
	acmRes = _64klang_ACMConvert(&pcmFormat, &gsmFormat, srcBuf, srcBufSize, dstBuf, dstBufSize);
	SynthFree(srcBuf);
	if (acmRes)
	{
		fprintf(stderr, "64klang: %s\n", "Could not convert from PCM to GSM6.10");
		return;
	}

	// store the compressed info in struct for usage when exporting
	SynthGlobalState.CompSampleRate[index] = fmtFreqs[compFreqIndex].sampleRate;
	SynthGlobalState.CompAvgBytes[index] = fmtFreqs[compFreqIndex].avgBytes;
	SynthGlobalState.CompWaveTableSize[index] = dstBufSize;
	SynthGlobalState.CompWaveTable[index] = dstBuf;

	// convert back to pcm
	nBytes = 2; // force 16bit
	pcmFormat.nAvgBytesPerSec = fmtFreqs[compFreqIndex].sampleRate*nChannels*nBytes;
	pcmFormat.nBlockAlign = nChannels*nBytes;
	pcmFormat.wBitsPerSample = 8*nBytes;

	DWORD unpackedsize = srcBufSize;
	DWORD packedsize = dstBufSize;

	srcBuf = dstBuf;
	srcBufSize = dstBufSize;
	acmRes = _64klang_ACMConvert(&gsmFormat, &pcmFormat, srcBuf, srcBufSize, dstBuf, dstBufSize);
	if (acmRes)
	{
		fprintf(stderr, "64klang: %s\n", "Could not convert from GSM6.10 to PCM");
		return;
	}
#else
	DWORD packedsize = srcBufSize;
	dstBuf = srcBuf;
	dstBufSize = srcBufSize;
	srcBuf = NULL;
#endif

	// upsample to 44khz for synth if needed
	if (fmtFreqs[compFreqIndex].sampleRate != 44100)
	{
		int loops = 44100 / fmtFreqs[compFreqIndex].sampleRate;
		dstBufSize *= loops;
		short* sBuf = (short*)dstBuf;
		short* dBuf = (short*)SynthMalloc( dstBufSize );
		dstBuf = (LPBYTE)dBuf;
		for (DWORD i = 0; i < dstBufSize/2/loops; i++)
		{
			dBuf[loops*i+0] = sBuf[i];
			for (int j = 1; j < loops; j++)
			{
				dBuf[loops*i+j] = sBuf[i] + j*(sBuf[i+1] - sBuf[i]) / loops;
			}
		}
		SynthFree(sBuf);
	}

	// set in core wavetable array (first sample is number of samples to follow)
	int numSamples = dstBufSize/2;
	sample_t* coreBuf = (sample_t*)SynthMalloc(sizeof(sample_t)*(1 + numSamples));
	sample_t* writeBuf = coreBuf;
	// number of samples to follow
	sample_t ns;
	ns.i[0] = ns.i[1] = numSamples;
	*writeBuf++ = s_toSample(ns);
	// copy/convert the samples
	short* sourceBuffer = (short*)dstBuf;
	for (int i = 0; i < numSamples; i++)
	{
		sample_t csi;
		csi.i[0] = csi.i[1] = *sourceBuffer++;
		*writeBuf++ = s_toSample(csi)/SC[S_32768_0];
	}
	SynthFree(dstBuf);

	// block mutex if updates are not a mass data update
	if (!_massDataUpdate)
		DataAccessMutex.lock();

	SynthGlobalState.RawWaveTable[index] = coreBuf;
	SynthGlobalState.RawWaveFormat[index] = format;
	SynthGlobalState.RawWaveFrequency[index] = frequency;
	SynthGlobalState.RawWaveFileName[index] = wfpath;
	SynthGlobalState.RawWavePackedSize[index] = packedsize;

	if (!_massDataUpdate)
		DataAccessMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getWaveFileFrequency(int index)
{
	return SynthGlobalState.RawWaveFrequency[index];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SynthController::getWaveFileName(int index)
{
	return SynthGlobalState.RawWaveFileName[index];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SynthController::getWaveFileCompressedSize(int index)
{
	return SynthGlobalState.RawWavePackedSize[index];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64klang recording helper class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <list>

class StreamRecorder
{
public:

	class SingleValue
	{
	public:
		DWORD time;
		BYTE value;
		bool operator < (SingleValue b) const { return (time < b.time) || (time == b.time && value < b.value); }
	};

	class DoubleValue
	{
	public:
		DWORD time;
		BYTE value1, value2;
		bool operator < (DoubleValue b) const { return (time < b.time) || (time == b.time && value1 < b.value1) || (time == b.time && value1 == b.value1 && value2 < b.value2); }
	};

	class SingleValueStream
	{
	public:
		std::list<SingleValue> values;
		std::list<SingleValue> qvalues;
		void Add(int timestamp, int value)
		{
			SingleValue v;
			v.time = timestamp;
			v.value = value;
			values.push_back(v);
		}
		void QuantizeAndSort(int framesize, bool ccStream)
		{
			qvalues.clear();
			// 1. create frametimes
			int lastValue = -1;
			for (std::list<SingleValue>::iterator it = values.begin(); it != values.end(); it++)
			{
				SingleValue v;
				v.time = it->time / framesize;
				v.value = it->value;
				if (ccStream)
				{
					// all cc's are set to 0 per default, so ignore any first events with a 0 value
					if (lastValue == -1 && it->value == 0)
						continue;
					// also ignore events with the same value as before
					if (lastValue == it->value)
						continue;
				}
				qvalues.push_back(v);
				lastValue = it->value;
			}
			// 2. sort
			qvalues.sort();
			if (ccStream)
			{
				std::map<int, SingleValue> frameValue;
				std::map<int, SingleValue> lastFrameValue;
				for (std::list<SingleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
				{
					if (frameValue.find(it->time) != frameValue.end())
						lastFrameValue[it->time] = *it;
					else
						frameValue[it->time] = *it;
				}
				for (std::map<int, SingleValue>::iterator it = lastFrameValue.begin(); it != lastFrameValue.end(); it++)
				{
					if (frameValue.find(it->first + 1) == frameValue.end())
					{
						it->second.time = it->first + 1;
						frameValue[it->first + 1] = it->second;
					}
				}
				qvalues.clear();
				for (std::map<int, SingleValue>::iterator it = frameValue.begin(); it != frameValue.end(); it++)
				{
					qvalues.push_back(it->second);
				}
			}
		}
		std::vector<BYTE> GetTime0DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<SingleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)(it->time & 0xff);
				v -= startValue;
				startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetTime1DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<SingleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)((it->time >> 8) & 0xff);
				v -= startValue;
				startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetTime2DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<SingleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)((it->time >> 16) & 0xff);
				v -= startValue;
				startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetValueDeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<SingleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = it->value;
				v -= startValue;
				startValue += v;
				d.push_back(v);
			}
			return d;
		}
	};

	class DoubleValueStream
	{
	public:
		std::list<DoubleValue> values;
		std::list<DoubleValue> qvalues;
		void Add(int timestamp, int value1, int value2)
		{
			DoubleValue v;
			v.time = timestamp;
			v.value1 = value1;
			v.value2 = value2;
			values.push_back(v);
		}
		void QuantizeAndSort(int framesize)
		{
			qvalues.clear();
			for (std::list<DoubleValue>::iterator it = values.begin(); it != values.end(); it++)
			{
				DoubleValue v;
				v.time = it->time / framesize;
				v.value1 = it->value1;
				v.value2 = it->value2;
				qvalues.push_back(v);
			}
			qvalues.sort();
		}
		std::vector<BYTE> GetTime0DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)(it->time & 0xff);
				v -= startValue; startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetTime1DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)((it->time >> 8) & 0xff);
				v -= startValue; startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetTime2DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)((it->time >> 16) & 0xff);
				v -= startValue; startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetValue1DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = it->value1;
				v -= startValue; startValue += v;
				d.push_back(v);
			}
			return d;
		}
		std::vector<BYTE> GetValue2DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = it->value2;
				v -= startValue; startValue += v;
				d.push_back(v);
			}
			return d;
		}
	};

	class ChannelStream
	{
	public:
		DoubleValueStream				NoteOnStream;
		SingleValueStream				NoteOffStream;
		std::vector<SingleValueStream>	CCStream;
		DoubleValueStream				AftertouchStream;
		ChannelStream()
		{
			for (int i = 0; i < 128; i++)
			{
				SingleValueStream cc;
				CCStream.push_back(cc);
			}
		}
	};

	StreamRecorder()
	{
		Reset();
	}

	void Reset()
	{
		IsActive = false;
		CurrentSample = 0;
		FirstEventSample = true;
		Streams.clear();
		for (int i = 0; i < 16; i++)
		{
			ChannelStream s;
			Streams.push_back(s);
		}
	}

	void Activate()
	{
		Reset();
		IsActive = true;
	}

	void Deactivate()
	{
		IsActive = false;
	}

	void AddSamples(int samples)
	{
		if (!IsActive)
			return;
		CurrentSample += samples;
	}

	void AddNoteOn(DWORD channel, DWORD note, DWORD velocity)
	{
		if (!IsActive)
			return;
		if (FirstEventSample)
		{
			FirstEventSample = false;
			CurrentSample = 0;
		}
		Streams[channel].NoteOnStream.Add(CurrentSample, note, velocity);
	}

	void AddNoteOff(DWORD channel, DWORD note)
	{
		if (!IsActive)
			return;
		if (FirstEventSample)
		{
			FirstEventSample = false;
			CurrentSample = 0;
		}
		Streams[channel].NoteOffStream.Add(CurrentSample, note);
	}

	void AddNoteAftertouch(DWORD channel, DWORD note, DWORD pressure)
	{
		if (!IsActive)
			return;
		if (FirstEventSample)
		{
			FirstEventSample = false;
			CurrentSample = 0;
		}
		Streams[channel].AftertouchStream.Add(CurrentSample, note, pressure);
	}

	void AddCC(DWORD channel, DWORD value, DWORD cc)
	{
		if (!IsActive)
			return;
		if (FirstEventSample)
		{
			FirstEventSample = false;
			CurrentSample = 0;
		}
		Streams[channel].CCStream[cc].Add(CurrentSample, value);
	}

	// SaveRaw and SaveOptimized omitted for brevity — implement as needed from original source
	void SaveRaw(std::string filename) {}
	void SaveOptimized(std::string filename, int timeQuant) {}

	bool IsActive;
	int CurrentSample;
	bool FirstEventSample;
	std::vector<ChannelStream> Streams;
};

static StreamRecorder Recorder;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::startRecording()
{
	Recorder.Activate();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::stopRecording()
{
	Recorder.Deactivate();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::isRecording()
{
	return Recorder.IsActive;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::exportSong(const std::string& filename, int timeQuant)
{
	if (filename != "")
	{
		std::string newfilename = filename.substr(0, filename.size() - 2) + "_rawexport.h";
		Recorder.SaveRaw(newfilename);
		Recorder.SaveOptimized(filename, timeQuant);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::doExportSong(const std::string& filename)
{
	// stub — full implementation requires song export machinery from original
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::recursiveCollectUsedNodes(SynthNode* node, std::map<SynthNode*, bool>& usedNodes)
{
	if (usedNodes[node])
		return;

	usedNodes[node] = true;

	// recursion not for constants
	if (node->id != CONSTANT_ID)
	{
		for (DWORD i = 0; i < node->numInputs; i++)
		{
			SynthNode* input = (SynthNode*)(node->input[i]);
			recursiveCollectUsedNodes(input, usedNodes);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::exportPatch(const std::string& filename)
{
	// exportPatch is a large code-generation function that writes C header data.
	// Full implementation mirrors the original SynthController.cpp exportPatch exactly;
	// see VSTiPluginSourceCode/64klang2Core/SynthController.cpp lines 4690-5882.
	// Stub provided here; copy the full function body from the original when needed.
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::noteOn(int channel, int note, int velocity)
{
	Recorder.AddNoteOn(channel, note, velocity);
	_64klang_NoteOn(channel, note, velocity);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::noteOff(int channel, int note, int velocity)
{
	Recorder.AddNoteOff(channel, note);
	_64klang_NoteOff(channel, note, velocity);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::noteAftertouch(int channel, int note, int pressure)
{
	Recorder.AddNoteAftertouch(channel, note, pressure);
	_64klang_NoteAftertouch(channel, note, pressure);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::midiSignal(int channel, int value, int cc)
{
	Recorder.AddCC(channel, value, cc);
	_64klang_MidiSignal(channel, value, cc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setBPM(float bpm)
{
	_64klang_SetBPM(bpm);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::tick(float* left, float* right, int samples)
{
	Recorder.AddSamples(samples);
	_64klang_Tick(left, right, samples);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::isInitialized()
{
	return _initialized;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::checkCPUSupport()
{
	int CPUInfo[4] = { -1 };
	unsigned int nIds;
	unsigned int nExIds;

	// Calling k64_cpuid with 0x0 as the InfoType argument
	// gets the number of valid IDs.
	k64_cpuid(CPUInfo, 0);
	nIds = CPUInfo[0];

	bool bSSE41 = false;

	for (unsigned int i = 0; i <= nIds; i++)
	{
		k64_cpuid(CPUInfo, i);

		// Interpret CPU feature information.
		if (i == 1)
		{
			// Check SSE4.1 support: bit 19 of ECX
			bSSE41 = (CPUInfo[2] & (1 << 19)) != 0;
		}
	}

	// Calling k64_cpuid with 0x80000000 as the InfoType argument
	// gets the number of valid extended IDs.
	k64_cpuid(CPUInfo, 0x80000000);
	nExIds = CPUInfo[0];

	return bSSE41;
}
