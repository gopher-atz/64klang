#include "SynthController.h"
#include "Synth.h"
#include "SynthNode.h"
#include "SynthAllocator.h"
#include <intrin.h>

#include <mmreg.h>
#include <msacm.h>
#include <wmsdk.h> 
#pragma comment(lib, "msacm32.lib")
#pragma comment(lib, "wmvcore.lib")

#include "tinyxml.h"
#include <fstream>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// vsti specific 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define	NODE_SLOTS	(NODE_MAX_INPUTS+1)
#define CONSTANT_0	((MAX_NODES-1)*NODE_SLOTS)

SynthController*	SynthController::_instance = 0;
MY64KLANG2CORE_API	HINSTANCE			SynthController::ModuleInstance = 0;
MY64KLANG2CORE_API	HANDLE				SynthController::DataAccessMutex = 0;

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

// id based number of required connections (gui side) for a node, this is essentially the offset where optional/modulation inputs start
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

// id based number of maximum connections you can add in the gui, essentially keeping out the mode inputs for certain nodes
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
	DataAccessMutex = CreateMutex(NULL, FALSE, L"_64klangDataMutex");
	_64klang_Init(NULL, NULL, 0, 0, MAX_NODES*NODE_SLOTS);	
	_nodes = (SynthNode**)SynthMalloc(MAX_NODES*sizeof(SynthNode*));
	// init free nodes index list
	for (DWORD i = 0; i < MAX_NODES; i++)
		_freeSlots[i] = 1;
	// reset patch to default
	resetPatch(true, false);

	if (!checkCPUSupport())
	{
		MessageBoxA(0, "Your CPU does not support SSE4.1 instructions\n64klang will not work without!", "CRITICAL ERROR", MB_ICONEXCLAMATION);
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
	CloseHandle(DataAccessMutex);
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
	// always put the node in the lobal nodes map (global nodes are right, voice nodes will be replaced there on creation as needed (this is needed for parameter value display)
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
		WaitForSingleObject(DataAccessMutex, INFINITE);

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
	
	// set default values for constants
	switch (id)
	{
		case SYNTHROOT_ID:
			SETCONSTANT(SYNTHROOT_GAIN, sample_t(0.5));
			break;
		case CHANNELROOT_ID:
			SETCONSTANT(CHANNELROOT_GAIN, sample_t(0.5));
			break;
		case NOTECONTROLLER_ID:
			break;
		case VOICEMANAGER_ID:
			SETCONSTANT(VOICEMANAGER_TRANSPOSE, sample_t(0.0));
			SETCONSTANT(VOICEMANAGER_GLIDE,		sample_t(0.0));
			SETCONSTANT(VOICEMANAGER_ARPSPEED,	sample_t(0.140625));
			SETMODE(VOICEMANAGER_MODE,			sample_t((int)(0x00107f00))); // full note range
			break;
		case VOICEROOT_ID:
			break;
		case ADSR_ID:
			SETCONSTANT(ADSR_ATTACK,	sample_t(0.375));
			SETCONSTANT(ADSR_DECAY,		sample_t(0.375));
			SETCONSTANT(ADSR_SUSTAIN,	sample_t(0.5));
			SETCONSTANT(ADSR_RELEASE,	sample_t(0.375));
			SETCONSTANT(ADSR_GAIN,		sample_t(0.75));
			if (!isGlobal)
			{
				SETMODE(ADSR_MODE, sample_t((int)(ADSR_VOICETRIGGER | ADSR_VOICEGATE)));
			}
			else
			{
				SETMODE(ADSR_MODE, sample_t((int)(0)));
			}
			break;
		case LFO_ID:
			SETCONSTANT(LFO_FREQ,	sample_t(0.5));
			SETCONSTANT(LFO_PHASE,	sample_t(0.0));
			SETCONSTANT(LFO_COLOR,	sample_t(1.0));
			SETCONSTANT(LFO_GAIN,	sample_t(1.0));
			SETMODE(LFO_MODE,		sample_t((int)(LFO_SINE)));
			break;
		case OSCILLATOR_ID:
			SETCONSTANT(OSCILLATOR_FREQ,		sample_t(0.0));
			SETCONSTANT(OSCILLATOR_PHASE,		sample_t(0.0));
			SETCONSTANT(OSCILLATOR_COLOR,		sample_t(0.5));
			SETCONSTANT(OSCILLATOR_TRANSPOSE,	sample_t(0.0));
			SETCONSTANT(OSCILLATOR_DETUNE,		sample_t(0.0));
			SETCONSTANT(OSCILLATOR_GAIN,		sample_t(1.0));
			SETCONSTANT(OSCILLATOR_UDETUNE,		sample_t(-0.5));
			if (!isGlobal)
			{
				SETMODE(OSCILLATOR_MODE,			sample_t((int)(OSCILLATOR_SINE | OSCILLATOR_VOICEFREQ)));
			}
			else
			{
				SETMODE(OSCILLATOR_MODE,			sample_t((int)(OSCILLATOR_SINE)));
			}
			break;
		case NOISEGEN_ID:
			SETCONSTANT(NOISEGEN_MIX,	sample_t(0.0));
			SETCONSTANT(NOISEGEN_GAIN,	sample_t(1.0));
			break;
		case BQFILTER_ID:
			SETCONSTANT(BQFILTER_FREQ,	sample_t(1.0));
			SETCONSTANT(BQFILTER_Q,		sample_t(0.0));
			SETCONSTANT(BQFILTER_DBGAIN,sample_t(0.75));
			SETMODE(BQFILTER_MODE,		sample_t((int)(BQFILTER_LOWPASS)));
			break;
		case ONEPOLEFILTER_ID:
			SETCONSTANT(ONEPOLEFILTER_POLE, sample_t(0.0));
			break;
		case ONEZEROFILTER_ID:
			SETCONSTANT(ONEZEROFILTER_ZERO, sample_t(0.0));
			break;
		case SHAPER_ID:
			SETCONSTANT(SHAPER_DRIVE, sample_t(0.0));
			break;
		case PANNING_ID:
			SETCONSTANT(PANNING_PAN, sample_t(0.5));
			break;
		case SCALE_ID:
			SETCONSTANT(SCALE_SCALE, sample_t(0.0));
			break;
		case MUL_ID:
			break;
		case DIV_ID:
			break;
		case ADD_ID:
			break;
		case SUB_ID:
			break;
		case CLIP_ID:
			SETCONSTANT(CLIP_LEVEL, sample_t(1.0));
			break;
		case MIX_ID:
			SETCONSTANT(MIX_MIX, sample_t(0.5));
			break;
		case MULTIADD_ID:
			break;
		case MONO_ID:
			SETMODE(MONO_MODE, sample_t(0.0));
			break;
		case ENVFOLLOWER_ID:
			SETCONSTANT(ENVFOLLOWER_ATTACK,		sample_t(0.125));
			SETCONSTANT(ENVFOLLOWER_RELEASE,	sample_t(0.375));
			break;
		case LOGIC_ID:
			SETMODE(LOGIC_MODE, sample_t((int)(LOGIC_AND)));
			break;
		case COMPARE_ID:
			SETMODE(COMPARE_MODE, sample_t((int)(COMPARE_GT)));
			break;
		case SELECT_ID:
			break;
		case EVENTSIGNAL_ID:
			break;
		case PROCESS_ID:
			break;
		case MIDISIGNAL_ID:
			SETCONSTANT(MIDISIGNAL_SCALE, sample_t(1.0));
			SETMODE(MIDISIGNAL_MODE, sample_t((int)(0)));
			break;
		case DISTORTION_ID:
			SETCONSTANT(DISTORTION_DRIVE,		sample_t(0.0));
			SETCONSTANT(DISTORTION_THRESHOLD,	sample_t(0.75));
			SETMODE(DISTORTION_MODE,			sample_t((int)(DISTORTION_OVERDRIVE)));
			break;
		case CROSSMIX_ID:
			SETCONSTANT(CROSSMIX_MIX, sample_t(0.0));
			break;
		case DELAY_ID:
			SETCONSTANT(DELAY_TIME, sample_t(0.5));
			SETMODE(DELAY_MODE,		sample_t((int)(DELAY_BPMSYNC)));
			break;
		case FBDELAY_ID:
			SETCONSTANT(FBDELAY_TIME,		sample_t(0.3125));
			SETCONSTANT(FBDELAY_FEEDBACK,	sample_t(0.5));
			SETCONSTANT(FBDELAY_DAMP,		sample_t(0.5));
			SETCONSTANT(FBDELAY_MIX,		sample_t(0.5));
			SETMODE(FBDELAY_MODE,			sample_t((int)(DELAY_BPMSYNC)));
			break;
		case DCFILTER_ID:
			SETCONSTANT(DCFILTER_POLE,		SC[NOISEGEN_B0]);
			break;
		case OSRAND_ID:
			SETCONSTANT(OSRAND_SCALE, sample_t(0.0));
			break;
		case REVERB_ID:
			SETCONSTANT(REVERB_GAIN,		sample_t(0.5));
			SETCONSTANT(REVERB_ROOMSIZE,	sample_t(0.75));
			SETCONSTANT(REVERB_DAMP,		sample_t(0.5));
			SETCONSTANT(REVERB_WIDTH,		sample_t(0.5));
			SETCONSTANT(REVERB_MIX,			sample_t(0.5));
			break;
		case EQ_ID:
			SETCONSTANT(EQ_B1,	sample_t(0.75));
			SETCONSTANT(EQ_B2,	sample_t(0.75));
			SETCONSTANT(EQ_B3,	sample_t(0.75));
			SETCONSTANT(EQ_B4,	sample_t(0.75));
			SETCONSTANT(EQ_B5,	sample_t(0.75));
			SETCONSTANT(EQ_B6,	sample_t(0.75));
			SETCONSTANT(EQ_B7,	sample_t(0.75));
			SETCONSTANT(EQ_B8,	sample_t(0.75));	
			SETCONSTANT(EQ_B9,	sample_t(0.75));
			SETCONSTANT(EQ_B10, sample_t(0.75));
			break;
		case COMPEXP_ID:
			SETCONSTANT(COMPEXP_THRESHOLD,	sample_t(0.75));
			SETCONSTANT(COMPEXP_RATIO,		sample_t(0.0));
			SETCONSTANT(COMPEXP_ATTACK,		sample_t(0.0));
			SETCONSTANT(COMPEXP_RELEASE,	sample_t(0.75));
			break;		
		case TRIGGER_ID:
			break;
		case TRIGGERSEQ_ID:
			SETMODE(TRIGGERSEQ_MODE,			sample_t((int)(0x0400 | 0x1)));
			SETMODE(TRIGGERSEQ_PATTERN0_3L,		sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN4_7L,		sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN8_11L,	sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN12_15L,	sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN0_3R,		sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN4_7R,		sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN8_11R,	sample_t((int)(0x11111111)));
			SETMODE(TRIGGERSEQ_PATTERN12_15R,	sample_t((int)(0x11111111)));
			break;
		case SAMPLEREC_ID:
			SETMODE(SAMPLEREC_MODE,				sample_t((int)(0x0ffff000)));
			break;
		case SAMPLER_ID:
			SETCONSTANT(SAMPLER_POSITION,	sample_t(0.0));
			SETCONSTANT(SAMPLER_SPEED,		sample_t(0.0));
			SETCONSTANT(SAMPLER_DIRECTION,	sample_t(0.0));
			SETCONSTANT(SAMPLER_LOOPSTART,	sample_t(0.0));
			SETCONSTANT(SAMPLER_LOOPEND,	sample_t(1.0));
			SETCONSTANT(SAMPLER_CROSSFADE,	sample_t(0.0));
			SETMODE(SAMPLER_MODE,			sample_t(SAMPLER_STORED));
			break;
		case SVFILTER_ID:
			SETCONSTANT(SVFILTER_FREQ,	sample_t(1.0));
			SETCONSTANT(SVFILTER_Q,		sample_t(0.0));
			SETMODE(SVFILTER_MODE,		sample_t((int)(SVFILTER_LOWPASS)));
			break;
		case ABS_ID:
			break;
		case NEG_ID:
			break;
		case SQRT_ID:
			break;
		case MIN_ID:
			break;
		case MAX_ID:
			break;
		case GLITCH_ID:
			SETCONSTANT(GLITCH_ACTIVE,	sample_t(0.0));
			SETCONSTANT(GLITCH_TIME,	sample_t(0.3125));
			SETCONSTANT(GLITCH_P1,		sample_t(0.125));
			SETCONSTANT(GLITCH_P2,		sample_t(0.125));
			SETCONSTANT(GLITCH_SPEED,	sample_t(0.0));
			SETMODE(GLITCH_MODE,		sample_t((int)0));
			break;
		case SAPI_ID:
			break;
		case COMBDELAY_ID:
			SETCONSTANT(COMBDELAY_TIME, sample_t(0.5));
			SETMODE(COMBDELAY_MODE,		sample_t((int)(DELAY_BPMSYNC)));
			break;
		case GMDLS_ID:
			SETMODE(GMDLS_MODE, sample_t((int)0));
			break;
		case BOWED_ID:
			SETCONSTANT(BOWED_POSITION,		sample_t(0.5));
			SETCONSTANT(BOWED_PRESSURE,		sample_t(0.5));
			SETCONSTANT(BOWED_VELOCITY,		sample_t(0.75));
			SETCONSTANT(BOWED_VIBRATO,		sample_t(0.0));
			SETCONSTANT(BOWED_FRICTIONSYM,	sample_t(0.0));
			break;
		case FORMULA_ID:
			break;
		case SNH_ID:
			SETCONSTANT(SNH_SNH, sample_t(1.0));
			SETCONSTANT(SNH_SNHSMOOTH, sample_t(0.0));
			SETMODE(SNH_MODE, sample_t((int)(SNH_QUADRATIC)));
			break;
		case WTFOSC_ID:
			SETCONSTANT(WTFOSC_POSITION, sample_t(0.0));
			SETCONSTANT(WTFOSC_SPEED, sample_t(0.0));
			SETCONSTANT(WTFOSC_TARGETSEARCHLEN, sample_t(0.125, 0.0625));
			SETCONSTANT(WTFOSC_COUNTSKIP, sample_t(0.125, 0.0));
			SETMODE(WTFOSC_MODE, sample_t(SAMPLER_STORED));
			break;
		case FORMANT_ID:
			SETCONSTANT(FORMANT_GAIN,	sample_t(1.0));
			SETMODE(FORMANT_MODE,		sample_t((int)0));
			break;
		case EQ3_ID:
			SETCONSTANT(EQ3_LGAIN, sample_t(0.75));
			SETCONSTANT(EQ3_MGAIN, sample_t(0.75));
			SETCONSTANT(EQ3_HGAIN, sample_t(0.75));
			break;
		case OSCSYNC_ID:
			break;

		case CONSTANT_ID:
			node->out = sample_t(0.0);
			break;

		case VOICE_FREQUENCY_ID:
			SETCONSTANT(VOICEPARAM_SCALE, sample_t(0.0));
			break;
		case VOICE_NOTE_ID:
			SETCONSTANT(VOICEPARAM_SCALE, sample_t(0.0));
			break;
		case VOICE_ATTACKVELOCITY_ID:
			SETCONSTANT(VOICEPARAM_SCALE, sample_t(0.0));
			break;		
		case VOICE_TRIGGER_ID:
			SETCONSTANT(VOICEPARAM_SCALE, sample_t(0.0));
			break;		
		case VOICE_GATE_ID:
			SETCONSTANT(VOICEPARAM_SCALE, sample_t(0.0));
			break;
		case VOICE_AFTERTOUCH_ID:
			SETCONSTANT(VOICEPARAM_SCALE, sample_t(0.0));
			break;

		default:
			node->out = sample_t(0.0);
			break;
	}
	
	if (!_massDataUpdate)		
		ReleaseMutex(DataAccessMutex);

	return node;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::deleteNode(DWORD node)
{
	if (!_massDataUpdate)
		WaitForSingleObject(DataAccessMutex, INFINITE);

	// parameters and modadders are destroyed when their owner nodes are destroyed
	if (_nodeGUIInfo[node].IsParameter || _nodeGUIInfo[node].IsModAdder)
		return;

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
		// special case for multiadd based nodes with variable inputs, not needed here
		if (dnode->id != MULTIADD_ID && dnode->id != NOTECONTROLLER_ID)
		{
			// remove all constants and modadders and mode constants created by this node
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

	if (!_massDataUpdate)		
		ReleaseMutex(DataAccessMutex);
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
		WaitForSingleObject(DataAccessMutex, INFINITE);

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
		// something connected from the gui as a modulator, so set modadder as input and connect incoming input to that
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
		ReleaseMutex(DataAccessMutex);
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
		WaitForSingleObject(DataAccessMutex, INFINITE);

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

			// core disconnect (set to default constant with value 0, so that at least a value can be returned)
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
		ReleaseMutex(DataAccessMutex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setInputValue(DWORD nodeid, DWORD index, double value1, double value2)
{
	if (!_massDataUpdate)
		WaitForSingleObject(DataAccessMutex, INFINITE);

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
		ReleaseMutex(DataAccessMutex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::setInputMode(DWORD nodeid, DWORD index, DWORD mode, DWORD modemask)
{
	if (!_massDataUpdate)
		WaitForSingleObject(DataAccessMutex, INFINITE);

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
		ReleaseMutex(DataAccessMutex);
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

int	SynthController::getInputMode(DWORD nodeid, DWORD inputIndex)
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
		WaitForSingleObject(DataAccessMutex, INFINITE);

	SynthNode* node = _nodeGUIInfo[nodeid].Node;
	node->e = sample_t::zero();

	if (!_massDataUpdate)		
		ReleaseMutex(DataAccessMutex);
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

		return node->v[19].i[0];
	}
	return 0;
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
					if (name == "+")
						comseq.push_back(FORMULA_CMD_ADD);					
					if (name == "*")
						comseq.push_back(FORMULA_CMD_MUL);														
					if (name == "==")
						comseq.push_back(FORMULA_CMD_E);
					if (name == "!=")
						comseq.push_back(FORMULA_CMD_NE);
					if (name == ">")
						comseq.push_back(FORMULA_CMD_GT);					
					if (name == ">=")
						comseq.push_back(FORMULA_CMD_GTE);															
					if (name == "&&")
						comseq.push_back(FORMULA_CMD_AND);
					if (name == "||")
						comseq.push_back(FORMULA_CMD_OR);
					if (name == "<")
						comseq.push_back(FORMULA_CMD_LT);
					if (name == "<=")
						comseq.push_back(FORMULA_CMD_LTE);
					if (name == "-")
						comseq.push_back(FORMULA_CMD_SUB);
					if (name == "/")
						comseq.push_back(FORMULA_CMD_DIV);
					if (name == "[mM]in")
						comseq.push_back(FORMULA_CMD_MIN);
					if (name == "[mM]ax")
						comseq.push_back(FORMULA_CMD_MAX);
					if (name == "%")
						comseq.push_back(FORMULA_CMD_MOD);
					if (name == "[lL]erp")
						comseq.push_back(FORMULA_CMD_LERP);
					if (name == "[iI]fthen")
						comseq.push_back(FORMULA_CMD_IFTHEN);
					if (name == "[mM]axtime")
						comseq.push_back(FORMULA_CMD_MAXTIME);					
					if (name == "Neg")
						comseq.push_back(FORMULA_CMD_NEG);
					if (name == "[aA]bs")
						comseq.push_back(FORMULA_CMD_ABS);
					if (name == "[sS]qrt")
						comseq.push_back(FORMULA_CMD_SQRT);
					if (name == "[cC]eil")
						comseq.push_back(FORMULA_CMD_CEIL);
					if (name == "[fF]loor")
						comseq.push_back(FORMULA_CMD_FLOOR);
					if (name == "[sS]qr")
						comseq.push_back(FORMULA_CMD_SQR);
					if (name == "[cC]os")
						comseq.push_back(FORMULA_CMD_COS);
					if (name == "[sS]in")
						comseq.push_back(FORMULA_CMD_SIN);
					if (name == "[eE]xp2")
						comseq.push_back(FORMULA_CMD_EXP2);
					if (name == "[lL]og2")
						comseq.push_back(FORMULA_CMD_LOG2);
					if (name == "[rR]and")
						comseq.push_back(FORMULA_CMD_RAND);
					if (name == "[pP]i")
						comseq.push_back(FORMULA_CMD_PI);										
					if (name == "[tT]au")
						comseq.push_back(FORMULA_CMD_TAU);					
					if (name == "[tT]autime")
						comseq.push_back(FORMULA_CMD_TAUTIME);				
					if (name == "[iI]n0")
						comseq.push_back(FORMULA_CMD_IN0);
					if (name == "[iI]n1")
						comseq.push_back(FORMULA_CMD_IN1);				
					if (name == "[vV]frequency")
						comseq.push_back(FORMULA_CMD_VFREQUENCY);
					if (name == "[vV]note")
						comseq.push_back(FORMULA_CMD_VNOTE);
					if (name == "[vV]velocity")
						comseq.push_back(FORMULA_CMD_VVELOCITY);
					if (name == "[vV]trigger")
						comseq.push_back(FORMULA_CMD_VTRIGGER);
					if (name == "[vV]gate")
						comseq.push_back(FORMULA_CMD_VGATE);
					if (name == "[vV]aftertouch")
						comseq.push_back(FORMULA_CMD_VAFTERTOUCH);
					if (name == "[tT]risaw")
						comseq.push_back(FORMULA_CMD_TRISAW);
					if (name == "[pP]ulse")
						comseq.push_back(FORMULA_CMD_PULSE);
					// get variables
					if (name == "out")
						comseq.push_back(FORMULA_CMD_GETVAR + 0);
					if (name == "time")
						comseq.push_back(FORMULA_CMD_GETVAR + 1);
					if (name == "a")
						comseq.push_back(FORMULA_CMD_GETVAR + 2);
					if (name == "b")
						comseq.push_back(FORMULA_CMD_GETVAR + 3);
					if (name == "c")
						comseq.push_back(FORMULA_CMD_GETVAR + 4);
					if (name == "d")
						comseq.push_back(FORMULA_CMD_GETVAR + 5);
					if (name == "e")
						comseq.push_back(FORMULA_CMD_GETVAR + 6);
					if (name == "f")
						comseq.push_back(FORMULA_CMD_GETVAR + 7);
					if (name == "g")
						comseq.push_back(FORMULA_CMD_GETVAR + 8);
					if (name == "h")
						comseq.push_back(FORMULA_CMD_GETVAR + 9);
					if (name == "i")
						comseq.push_back(FORMULA_CMD_GETVAR + 10);
					if (name == "j")
						comseq.push_back(FORMULA_CMD_GETVAR + 11);
					if (name == "k")
						comseq.push_back(FORMULA_CMD_GETVAR + 12);
					if (name == "l")
						comseq.push_back(FORMULA_CMD_GETVAR + 13);
					if (name == "m")
						comseq.push_back(FORMULA_CMD_GETVAR + 14);
					if (name == "n")
						comseq.push_back(FORMULA_CMD_GETVAR + 15);
					if (name == "o")
						comseq.push_back(FORMULA_CMD_GETVAR + 16);
					if (name == "p")
						comseq.push_back(FORMULA_CMD_GETVAR + 17);
					if (name == "q")
						comseq.push_back(FORMULA_CMD_GETVAR + 18);
					if (name == "r")
						comseq.push_back(FORMULA_CMD_GETVAR + 19);
					if (name == "s")
						comseq.push_back(FORMULA_CMD_GETVAR + 20);
					if (name == "t")
						comseq.push_back(FORMULA_CMD_GETVAR + 21);
					if (name == "u")
						comseq.push_back(FORMULA_CMD_GETVAR + 22);
					if (name == "v")
						comseq.push_back(FORMULA_CMD_GETVAR + 23);
					// set variables
					if (name == "out=")
						comseq.push_back(FORMULA_CMD_SETVAR + 0);
					if (name == "time=")
						comseq.push_back(FORMULA_CMD_SETVAR + 1);
					if (name == "a=")
						comseq.push_back(FORMULA_CMD_SETVAR + 2);
					if (name == "b=")
						comseq.push_back(FORMULA_CMD_SETVAR + 3);
					if (name == "c=")
						comseq.push_back(FORMULA_CMD_SETVAR + 4);
					if (name == "d=")
						comseq.push_back(FORMULA_CMD_SETVAR + 5);
					if (name == "e=")
						comseq.push_back(FORMULA_CMD_SETVAR + 6);
					if (name == "f=")
						comseq.push_back(FORMULA_CMD_SETVAR + 7);
					if (name == "g=")
						comseq.push_back(FORMULA_CMD_SETVAR + 8);
					if (name == "h=")
						comseq.push_back(FORMULA_CMD_SETVAR + 9);
					if (name == "i=")
						comseq.push_back(FORMULA_CMD_SETVAR + 10);
					if (name == "j=")
						comseq.push_back(FORMULA_CMD_SETVAR + 11);
					if (name == "k=")
						comseq.push_back(FORMULA_CMD_SETVAR + 12);
					if (name == "l=")
						comseq.push_back(FORMULA_CMD_SETVAR + 13);
					if (name == "m=")
						comseq.push_back(FORMULA_CMD_SETVAR + 14);
					if (name == "n=")
						comseq.push_back(FORMULA_CMD_SETVAR + 15);
					if (name == "o=")
						comseq.push_back(FORMULA_CMD_SETVAR + 16);
					if (name == "p=")
						comseq.push_back(FORMULA_CMD_SETVAR + 17);
					if (name == "q=")
						comseq.push_back(FORMULA_CMD_SETVAR + 18);
					if (name == "r=")
						comseq.push_back(FORMULA_CMD_SETVAR + 19);
					if (name == "s=")
						comseq.push_back(FORMULA_CMD_SETVAR + 20);
					if (name == "t=")
						comseq.push_back(FORMULA_CMD_SETVAR + 21);
					if (name == "u=")
						comseq.push_back(FORMULA_CMD_SETVAR + 22);
					if (name == "v=")
						comseq.push_back(FORMULA_CMD_SETVAR + 23);
				}
				ct = token + 1;
			}
			token++;
		}
		// always push done byte (0xff)
		comseq.push_back(FORMULA_DONE);
			
		// first assignment? (mem stays constant an is reused)
		if (!SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset])
			SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset] = (char*)malloc(65536);
		BYTE* buf = (BYTE*)SynthGlobalState.SpecialDataPointer[it->second.Node->valueOffset];
		
		// fill the buffer
		for (int i = 0; i < comseq.size(); i++)
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
	for (it	= _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
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

int		SynthController::numGUINodes()
{
	// rebuild accessor list
	_nodesGUIAccessor.clear();
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it	= _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		_nodesGUIAccessor.push_back(&(it->second));
	}
	return _nodesGUIAccessor.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::numSelectedGUINodes()
{
	// rebuild accessor list
	_nodesGUIAccessor.clear();
	std::map<DWORD, NodeGUIInfo>::iterator it;
	for (it	= _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
	{
		if (it->second.IsSelected)
			_nodesGUIAccessor.push_back(&(it->second));
	}
	return _nodesGUIAccessor.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthNode* SynthController::gnNode(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return NULL;
	return _nodesGUIAccessor[index]->Node;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool	SynthController::gnIsVisible(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return false;
	return _nodesGUIAccessor[index]->Visible;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::gnType(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->id;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::gnID(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->valueOffset;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::gnNodeInputs(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->numInputs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double	SynthController::gnX(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->X;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double	SynthController::gnY(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string	SynthController::gnName(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Name;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::gnChannel(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->FixedChannel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::gnInput(DWORD index, DWORD inputIndex)
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

double		SynthController::gnInputValue(DWORD index, DWORD inputIndex, DWORD channel)
{
	if (index >= _nodesGUIAccessor.size())
		return 0.0;
	SynthNode* node = _nodesGUIAccessor[index]->Node;
	return getInputValue(node->valueOffset, inputIndex, channel);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::gnInputMode(DWORD index, DWORD inputIndex) 
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	SynthNode* node = _nodesGUIAccessor[index]->Node;
	return getInputMode(node->valueOffset, inputIndex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool	SynthController::gnIsGlobal(DWORD index)
{
	if (index >= _nodesGUIAccessor.size())
		return 0;
	return _nodesGUIAccessor[index]->Node->isGlobal;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::getNumActiveVoices()
{
	return SynthNode::numActiveVoices;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int		SynthController::getNumActiveVoices(DWORD node)
{
	std::map<DWORD, NodeGUIInfo>::iterator it = _nodeGUIInfo.find(node);
	if (it->second.Node->id == VOICEMANAGER_ID)
		return ((VMWork*)(it->second.Node->customMem))->NumActiveVoices;
	else
		return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double	SynthController::getNodeSignal(DWORD nodeid, int left, int inp)
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

void SynthController::queryCoreProcessingMutex(bool acquire)
{
	if (acquire)
	{
		DWORD res = WaitForSingleObject(DataAccessMutex, INFINITE);
	}
	else
	{
		BOOL res = ReleaseMutex(DataAccessMutex);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::killVoices()
{
	if (!_massDataUpdate)
		WaitForSingleObject(DataAccessMutex, INFINITE);

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
		ReleaseMutex(DataAccessMutex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void	SynthController::panic()
{	
	WaitForSingleObject(DataAccessMutex, INFINITE);
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
					CopyMemory(backup, node->customMem, sizeof(VMWork));
				}

				// free the custom memory
				SynthFree(node->customMem);
				node->customMem = 0;

				// reinit node if init exists
				if (node->init)
					node->init(node);

				// voicemanager contains the step sequence in custom mem, so restore
				if (node->id == VOICEMANAGER_ID)
				{
					VMWork* b = (VMWork*)backup;
					VMWork* w = (VMWork*)(node->customMem);
					w->ArpSequenceLoopIndex = b->ArpSequenceLoopIndex;
					CopyMemory(w->ArpSequence, b->ArpSequence, 32*sizeof(WORD));
					SynthFree(backup);
				}
			}
		}
	}
	
	ReleaseMutex(DataAccessMutex);
	_massDataUpdate = false;
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
			GlobalFree(_deferredFreeNodes[i]);
		}
		_deferredFreeNodes.clear();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mass data access funtions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::recursiveAddChannel(SynthNode* node, int channel)
{
	NodeGUIInfo* gi = &(_nodeGUIInfo[node->valueOffset]);
	// skip already processed nodes
	if (gi->RecursionFlag == 0)
	{
		gi->RecursionFlag = 1;

		// distribute channel when hitting either
		if (((node->id != CONSTANT_ID) && (node->id > NOTECONTROLLER_ID))	||	// non constant and non global (synthroot, channelroot, notecontroller)
			(gi->FixedChannel == channel)					||	// default global node of our channel (channelroot, notecontroller)
			((node->id == CONSTANT_ID) && gi->Visible))			// visible constant (user constant)
		{	
			gi->Channels.insert(channel);
		}

		// recursion not for constants
		if (node->id != CONSTANT_ID )
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
			// scale and voice constants have an non mode constant
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
					inode.SetDoubleAttribute("value1", input->out.d[0]);
					inode.SetDoubleAttribute("value2", input->out.d[1]);						
				}
				// modadder used?
				else
				{
					SynthNode* modadder = ni->ModAdder[i];
					inode.SetDoubleAttribute("value1", modadder->input[0]->d[0]);
					inode.SetDoubleAttribute("value2", modadder->input[0]->d[1]);
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
			node.SetDoubleAttribute("value1", n->out.d[0]);
			node.SetDoubleAttribute("value2", n->out.d[1]);
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
		if (node->id != CONSTANT_ID )
		{		
			for (DWORD i = 0; i < node->numInputs; i++)
			{				
				SynthNode* input = (SynthNode*)(node->input[i]);
				// only write until a global node (synthroot, channelroot, notecontroller) not of our channel is hit
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
		if (channel != -2 && targetChannel != -2) // no specific channel assigned, use xml value
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
		// special case when loading channels. we keep the channel root 
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

				///////////////////////////////////////////////////////////////////////////////////////////////////
				// some checks for old data formats need to be added here
				///////////////////////////////////////////////////////////////////////////////////////////////////
				
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

	WaitForSingleObject(DataAccessMutex, INFINITE);
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
							int ls = filename.rfind("/");
							int lbs= filename.rfind("\\");
							if (lbs > ls)
								ls = lbs;
							std::string path = filename.substr(0, ls+1);
							// absolute wave file path
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
		// show only non default0 connectinons
		if (connections[i].from != (MAX_NODES-1))
		{
			SynthNode* from = idNodeMap[connections[i].from];
			SynthNode* to = idNodeMap[connections[i].to];
			int index = connections[i].index;
			// establish connection
			connectInput(from->valueOffset, to->valueOffset, index);
		}
	}

	ReleaseMutex(DataAccessMutex);
	_massDataUpdate = false;

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
			int ls = wfilename.rfind("/");
			int lbs= wfilename.rfind("\\");
			if (lbs > ls)
				ls = lbs;			
			wfilename = wfilename.substr(ls+1);

			// get absolute path from patch filename
			ls = filename.rfind("/");
			lbs= filename.rfind("\\");
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

void SynthController::resetPatch(bool createDefault, bool acquireMutex)
{
	if (acquireMutex)
	{
		_massDataUpdate = true;
		WaitForSingleObject(DataAccessMutex, INFINITE);
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
		ReleaseMutex(DataAccessMutex);
		_massDataUpdate = false;
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
	
	WaitForSingleObject(DataAccessMutex, INFINITE);
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
					// dont delete the channel root itself, so its outputs stay connected, see below
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
		// get loaded channel roots position and calculate delta pos to apply for all new nodes
		double refXNew = _nodeGUIInfo[channelRoots[channel]->valueOffset].X;
		double refYNew = _nodeGUIInfo[channelRoots[channel]->valueOffset].Y;
		for (std::map<int, SynthNode*>::iterator it = idNodeMap.begin(); it != idNodeMap.end(); it++)
		{
			SynthNode* curNode = it->second;
			// delta pos in loaded coords
			double dX = _nodeGUIInfo[curNode->valueOffset].X - refXNew;
			double dY = _nodeGUIInfo[curNode->valueOffset].Y - refYNew;
			// add delta pos to original coords
			_nodeGUIInfo[curNode->valueOffset].X = refX + dX;
			_nodeGUIInfo[curNode->valueOffset].Y = refY + dY;
		}

		// step 3: establish connections
		for (DWORD i = 0; i < connections.size(); i++)
		{
			// show only non default0 connectinons and skip connections which are from nodes we didnt load
			if ((connections[i].from != (MAX_NODES-1)) && (idNodeMap.find(connections[i].from) != idNodeMap.end()))
			{
				SynthNode* from = idNodeMap[connections[i].from];
				SynthNode* to = idNodeMap[connections[i].to];
				int index = connections[i].index;
				// establish connection
				connectInput(from->valueOffset, to->valueOffset, index);
			}
		}
	}

	ReleaseMutex(DataAccessMutex);
	_massDataUpdate = false;

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

	// check if no name was given, use filename then
	if (_nodeGUIInfo[channelRoots[channel]->valueOffset].Name == "")
	{
		// get filename
		int ls = filename.rfind("/");
		int lbs= filename.rfind("\\");
		if (lbs > ls)
			ls = lbs;			
		std::string name = filename.substr(ls+1);
		// remove extension
		int ext = name.find_last_of(".");
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
	
	//WaitForSingleObject(DataAccessMutex, INFINITE);
	//_massDataUpdate = true;
	//killVoices();

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
	// calculate delta pos to apply for all new nodes
	double refXNew = minX;
	double refYNew = minY;
	for (std::map<int, SynthNode*>::iterator it = idNodeMap.begin(); it != idNodeMap.end(); it++)
	{
		SynthNode* curNode = it->second;
		// delta pos in loaded coords
		double dX = _nodeGUIInfo[curNode->valueOffset].X - refXNew;
		double dY = _nodeGUIInfo[curNode->valueOffset].Y - refYNew;
		// add delta pos to original coords
		_nodeGUIInfo[curNode->valueOffset].X = refX + dX;
		_nodeGUIInfo[curNode->valueOffset].Y = refY + dY;
		// set selection
		_nodeGUIInfo[curNode->valueOffset].IsSelected = true;
	}

	// step 3: establish connections
	for (DWORD i = 0; i < connections.size(); i++)
	{
		// show only non default0 connectinons and skip connections which are from nodes we didnt load
		if ((connections[i].from != (MAX_NODES-1)) && (idNodeMap.find(connections[i].from) != idNodeMap.end()))
		{
			SynthNode* from = idNodeMap[connections[i].from];
			SynthNode* to = idNodeMap[connections[i].to];
			int index = connections[i].index;
			// establish connection
			connectInput(from->valueOffset, to->valueOffset, index);
		}
	}

	//ReleaseMutex(DataAccessMutex);
	//_massDataUpdate = false;

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
	for (it	= _nodeGUIInfo.begin(); it != _nodeGUIInfo.end(); it++)
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
		WaitForSingleObject(DataAccessMutex, INFINITE);

	// clear
	if (SynthGlobalState.RawWaveTable[index])
		SynthFree(SynthGlobalState.RawWaveTable[index]);
	SynthGlobalState.RawWaveTable[index] = 0;
	SynthGlobalState.RawWaveFormat[index] = 0;
	SynthGlobalState.RawWaveFrequency[index] = 0;
	SynthGlobalState.RawWaveFileName[index] = "";	
	SynthGlobalState.CompSampleRate[index] = 0;
	SynthGlobalState.CompAvgBytes[index] = 0;
	SynthGlobalState.CompWaveTableSize[index] = 0;
	if (SynthGlobalState.CompWaveTable[index])
		SynthFree(SynthGlobalState.CompWaveTable[index]);
	SynthGlobalState.CompWaveTable[index] = 0;

	if (!_massDataUpdate)		
		ReleaseMutex(DataAccessMutex);

	if (wfpath == "")
		return;

	int compFreqIndex = frequency; // 0 = 11khz, 1 = 22khz, 2 = 44khz
	//if (frequency == 44100)
	//	compFreqIndex = 2;		 
	//else if (frequency == 22050)
	//	compFreqIndex = 1;		 
	//else if (frequency == 11025)
	//	compFreqIndex = 0;		 
	//else
	//{
	//	MessageBoxA(0, "Only 11khz, 22khz and 44khz target frequencies are valid", "Error", MB_ICONERROR | MB_TOPMOST);
	//	return;
	//}

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

	DWORD srcBufSize = 0;
	DWORD dstBufSize = 0;
	LPBYTE srcBuf = NULL;
	LPBYTE dstBuf = NULL;

	// read the input wav
	std::ifstream file;
	file.open(wfpath.c_str(), std::ios_base::binary );
	if (file.is_open())
	{
		DWORD riff;
		file.read((char*)&riff, 4);
		if (riff != MAKEFOURCC('R', 'I', 'F', 'F'))
		{
			MessageBoxA(0, "Invalid File Type", "Error", MB_ICONERROR | MB_TOPMOST);
			file.close();
			return;
		}
		DWORD fsize;
		file.read((char*)&fsize, 4);
		DWORD wave;
		file.read((char*)&wave, 4);
		if (wave != MAKEFOURCC('W', 'A', 'V', 'E'))
		{
			MessageBoxA(0, "Unsupported file type", "Error", MB_ICONERROR | MB_TOPMOST);
			file.close();
			return;
		}
		DWORD fmt;
		file.read((char*)&fmt, 4);
		if (fmt != MAKEFOURCC('f', 'm', 't', ' '))
		{
			MessageBoxA(0, "Expected 'fmt ' chunk", "Error", MB_ICONERROR | MB_TOPMOST);
			file.close();
			return;
		}
		DWORD fmtsize;
		file.read((char*)&fmtsize, 4);
		WORD fmtid;
		file.read((char*)&fmtid, 2);
		if (fmtid != WAVE_FORMAT_PCM)
		{
			MessageBoxA(0, "File must be PCM format", "Error", MB_ICONERROR | MB_TOPMOST);
			file.close();
			return;
		}
		WORD channels;
		file.read((char*)&channels, 2);
		nChannels = channels;
		if (nChannels != 1)
		{
			MessageBoxA(0, "File must be mono", "Error", MB_ICONERROR | MB_TOPMOST);
			file.close();
			return;
		}
		file.read((char*)&SampleRate, 4);
		if (SampleRate != fmtFreqs[compFreqIndex].sampleRate)
		{
			needResampling = true;
			// downlsample with a triangular filter
			if (SampleRate < fmtFreqs[compFreqIndex].sampleRate)
			{
				MessageBoxA(0, "File sample rate is lower than target sample rate.\nPlease perform upsampling in some other editor!", "Error", MB_ICONERROR | MB_TOPMOST);
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
			MessageBoxA(0, "Expected 'data' chunk", "Error", MB_ICONERROR | MB_TOPMOST);
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
		MessageBoxA(0, wfpath.c_str(), "Couldnt open file", MB_ICONERROR | MB_TOPMOST);
		return;
	}

	GSM610WAVEFORMAT gsmFormat =
	{
		{
			WAVE_FORMAT_GSM610,
			1,      // WORD        nChannels;          
			0,		// DWORD       nSamplesPerSec;     
			0,		// DWORD       nAvgBytesPerSec;    
			65,		// WORD        nBlockAlign;        
			0,      // WORD        wBitsPerSample;     
			2		// WORD        cbSize;       // extra bytes after wfx struct
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
		0,		// WORD        cbSize;       // extra bytes after wfx struct, UNUSED
	};	

	// set pcm format
	pcmFormat.nSamplesPerSec = fmtFreqs[compFreqIndex].sampleRate;
	pcmFormat.nAvgBytesPerSec = fmtFreqs[compFreqIndex].sampleRate*nChannels*nBytes;    
	pcmFormat.nBlockAlign = nChannels*nBytes;        
	pcmFormat.wBitsPerSample = 8*nBytes;
	// set gsm format
	gsmFormat.wfx.nSamplesPerSec = fmtFreqs[compFreqIndex].sampleRate;
	gsmFormat.wfx.nAvgBytesPerSec = fmtFreqs[compFreqIndex].avgBytes;	

	// up/downsample srcBuf when input is not matching the target frequency
	if (needResampling)
	{
		// downlsample with a triangular filter
		if (SampleRate > fmtFreqs[compFreqIndex].sampleRate)
		{
			if (nBytes == 2)
			{
				int loops = SampleRate / fmtFreqs[compFreqIndex].sampleRate;
				srcBufSize /= 2;
				short filter_state = 0;
				downsample<short>((short*)srcBuf, (short*)srcBuf, srcBufSize/2, filter_state);
				// downsample again
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
				// downsample again
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

	// convert to gsm
	int acmRes;
	acmRes = _64klang_ACMConvert(&pcmFormat, &gsmFormat, srcBuf, srcBufSize, dstBuf, dstBufSize);
	SynthFree(srcBuf);
	if (acmRes)		
	{
		MessageBoxA(0, "Could not convert from PCM to GSM6.10", "Error", MB_ICONERROR | MB_TOPMOST);
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
		MessageBoxA(0, "Could not convert from GSM6.10 to PCM", "Error", MB_ICONERROR | MB_TOPMOST);
		return;
	}

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
//				dBuf[loops*i+j] = sBuf[i];
				dBuf[loops*i+j] = sBuf[i] + j*(sBuf[i+1] - sBuf[i]) / loops;
			}
		}
		// free old dst buffer which was source for the upsampling
		SynthFree(sBuf);		
	}

	// set in core wavetable array (first sample is number of samples to follow)
	int numSamples = dstBufSize/2;
	sample_t* coreBuf = (sample_t*)SynthMalloc(sizeof(sample_t)*(1 + numSamples));
	sample_t* writeBuf = coreBuf;
	// number of samples to follow
	sample_t ns;
	ns.i[0] = ns.i[1] = numSamples;
	*writeBuf++ = s_toSample(ns.pi);
	// copy/convert the samples
	short* sourceBuffer = (short*)dstBuf;
	for (int i = 0; i < numSamples; i++)
	{
		sample_t csi;
		csi.i[0] = csi.i[1] = *sourceBuffer++;
		*writeBuf++ = s_toSample(csi.pi)/SC[S_32768_0];
	}
	SynthFree(dstBuf);
	
	// block mutex if updates are not a mass data update
	if (!_massDataUpdate)
		WaitForSingleObject(DataAccessMutex, INFINITE);

	SynthGlobalState.RawWaveTable[index] = coreBuf;
	SynthGlobalState.RawWaveFormat[index] = format;
	SynthGlobalState.RawWaveFrequency[index] = frequency;
	SynthGlobalState.RawWaveFileName[index] = wfpath;
	SynthGlobalState.RawWavePackedSize[index] = packedsize;

	if (!_massDataUpdate)		
		ReleaseMutex(DataAccessMutex);
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
#include <vector>
#include <map>

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
				// take care of quantizing effects. allow only 1 value per frame (others would be swallowed anyway), propagate to the next frame if possible
				std::map<int, SingleValue> frameValue;
				std::map<int, SingleValue> lastFrameValue;
				for (std::list<SingleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
				{
					// if a frame value already exists, add/update the lastFrameValue for that frame
					if (frameValue.find(it->time) != frameValue.end())
						lastFrameValue[it->time] = *it;
					// no frame value exists yet, so insert
					else
						frameValue[it->time] = *it;
				}
				// loop the frame duplicates (containing the last values)
				for (std::map<int, SingleValue>::iterator it = lastFrameValue.begin(); it != lastFrameValue.end(); it++)
				{
					// check if the next frame is unassigned, if so put last value there
					if (frameValue.find(it->first + 1) == frameValue.end())
					{
						it->second.time = it->first + 1;
						frameValue[it->first + 1] = it->second;
					}
				}
				// cleanup and rebuild uniqe value list
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
			// 1. create frametimes
			for (std::list<DoubleValue>::iterator it = values.begin(); it != values.end(); it++)
			{
				DoubleValue v;
				v.time = it->time / framesize;
				v.value1 = it->value1;
				v.value2 = it->value2;
				qvalues.push_back(v);
			}
			// 2. sort
			qvalues.sort();
		}
		std::vector<BYTE> GetTime0DeltaStream(BYTE& startValue)
		{
			std::vector<BYTE> d;
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
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
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
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
			for (std::list<DoubleValue>::iterator it = qvalues.begin(); it != qvalues.end(); it++)
			{
				BYTE v = (BYTE)((it->time >> 16) & 0xff);
				v -= startValue;
				startValue += v;
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
				v -= startValue;
				startValue += v;
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
				v -= startValue;
				startValue += v;
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
		// global first event resets timestamp (current sample)
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
		// global first event resets timestamp (current sample)
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
		// global first event resets timestamp (current sample)
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
		// global first event resets timestamp (current sample)
		if (FirstEventSample)
		{
			FirstEventSample = false;
			CurrentSample = 0;
		}
		Streams[channel].CCStream[cc].Add(CurrentSample, value);
	}
	
	// for debug, load from existing raw stream
	void LoadRaw(BYTE SongStream[])
	{
		Reset();

		// read song bpm(1 DWORD)
		_64klang_SetBPM(*((float*)SongStream));

		// read song length (1 DWORD)
		CurrentSample = *((DWORD*)(SongStream + 4));
		
		BYTE* RawStream = SongStream + 8;
		// for all channels load data
		for (int c = 0; c < 16; c++)
		{
			DWORD numValues;
			BYTE* streamValue;
			BYTE* streamValue2;
			DWORD timestamp;
			BYTE value,value2;

			// NOTEOFF
			// read number of values (1 DWORD)
			numValues = *((DWORD*)RawStream);
			RawStream += 4;
			streamValue = RawStream + numValues * 4;
			for (int i = 0; i < numValues; i++)
			{
				timestamp = *((DWORD*)(&(RawStream[i * 4])));
				value = streamValue[i];
				Streams[c].NoteOffStream.Add(timestamp, value);
			}
			RawStream += numValues * 4 + numValues*sizeof(BYTE);

			// NOTEON
			// read number of values (1 DWORD)
			numValues = *((DWORD*)RawStream);
			RawStream += 4;
			streamValue = RawStream + numValues * 4;
			streamValue2 = streamValue + numValues*sizeof(BYTE);
			for (int i = 0; i < numValues; i++)
			{
				timestamp = *((DWORD*)(&(RawStream[i * 4])));
				value = streamValue[i];
				value2 = streamValue2[i];
				Streams[c].NoteOnStream.Add(timestamp, value, value2);
			}
			RawStream += numValues * 4 + numValues*sizeof(BYTE) * 2;

			// ATs
			// read number ov values (1 DWORD)
			numValues = *((DWORD*)RawStream);
			RawStream += 4;
			streamValue = RawStream + numValues * 4;
			streamValue2 = streamValue + numValues*sizeof(BYTE);
			for (int i = 0; i < numValues; i++)
			{
				timestamp = *((DWORD*)(&(RawStream[i * 4])));
				value = streamValue[i];
				value2 = streamValue2[i];
				Streams[c].AftertouchStream.Add(timestamp, value, value2);
			}
			RawStream += numValues * 4 + numValues*sizeof(BYTE) * 2;

			// CCs
			// read number of ccs (1 BYTE)
			BYTE numCCs = *RawStream++;
			for (int cc = 0; cc < numCCs; cc++)
			{
				// read CC number (1 BYTE)
				BYTE ccNumber = *RawStream++;
				// read number of values (1 DWORD)
				numValues = *((DWORD*)RawStream);
				RawStream += 4;
				streamValue = RawStream + numValues * 4;
				for (int i = 0; i < numValues; i++)
				{
					timestamp = *((DWORD*)(&(RawStream[i * 4])));
					value = streamValue[i];
					Streams[c].CCStream[ccNumber].Add(timestamp, value);
				}
				RawStream += numValues * 4 + numValues*sizeof(BYTE);
			}			
		}
	}

	// for debug, save a raw stream (not optimized)
	void SaveRaw(std::string filename)
	{		
		// write export song header file
		FILE* expFile = fopen(filename.c_str(), "w");
		if (expFile)
		{
			float bpm = SynthGlobalState.CurrentBPM.d[0];
			fprintf(expFile, "#define SAMPLE_RATE 44100\n");
			fprintf(expFile, "#define BPM %f\n", bpm);
			fprintf(expFile, "#define SAMPLES_PER_TICK %d\n", (int)(44100.0f*4.0f*60.0f/(bpm*16)));
			fprintf(expFile, "#define MAX_SAMPLES %d\n\n", CurrentSample);
			int songSize = 0;

			fprintf(expFile, "static BYTE SynthRawStream[] = \n");
			fprintf(expFile, "{");

			// write song bpm (1 DWORD)		
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n// BPM %f", bpm);
			fprintf(expFile, "\n/////////////////////////////////////////////");
			DWORD* ibpm = (DWORD*)(&bpm);
			fprintf(expFile, "\n0x%02x,0x%02x,0x%02x,0x%02x, ", (*ibpm >> 0) & 0xff, (*ibpm >> 8) & 0xff, (*ibpm >> 16) & 0xff, (*ibpm >> 24) & 0xff);
			songSize += 4;

			// write song length (1 DWORD)
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n// SongLength (Samples) %d", CurrentSample);
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n0x%02x,0x%02x,0x%02x,0x%02x, ", (CurrentSample >> 0) & 0xff, (CurrentSample >> 8) & 0xff, (CurrentSample >> 16) & 0xff, (CurrentSample >> 24) & 0xff);
			songSize += 4;

			// process recorded stream and output to file
			for (int c = 0; c < 16; c++)
			{
				fprintf(expFile, "\n///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n// Channel %d", c + 1);
				fprintf(expFile, "\n///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////");

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// NoteOffs
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				int numOffValues = Streams[c].NoteOffStream.values.size();
				// write number of items (1 DWORD)
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t// NoteOffs %d", numOffValues);
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t0x%02x,0x%02x,0x%02x,0x%02x, ", (numOffValues >> 0) & 0xff, (numOffValues >> 8) & 0xff, (numOffValues >> 16) & 0xff, (numOffValues >> 24) & 0xff);
				songSize += 4;
				// 1. write timestamp stream (DWORDs)
				fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
				for (std::list<SingleValue>::iterator it = Streams[c].NoteOffStream.values.begin(); it != Streams[c].NoteOffStream.values.end(); it++)
				{
					int dtime = it->time;
					fprintf(expFile, "0x%02x,0x%02x,0x%02x,0x%02x, ", (dtime >> 0) & 0xff, (dtime >> 8) & 0xff, (dtime >> 16) & 0xff, (dtime >> 24) & 0xff);
					songSize += 4;
				}
				// 2. write value stream (BYTEs)
				fprintf(expFile, "\n\t\t// Notes\n\t\t");
				for (std::list<SingleValue>::iterator it = Streams[c].NoteOffStream.values.begin(); it != Streams[c].NoteOffStream.values.end(); it++)
				{
					BYTE value = it->value;
					fprintf(expFile, "0x%02x,", value);
					songSize += 1;
				}

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// NoteOns
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				int numOnValues = Streams[c].NoteOnStream.values.size();
				// write number of items (1 DWORD)
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t// NoteOns %d", numOnValues);
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t0x%02x,0x%02x,0x%02x,0x%02x, ", (numOnValues >> 0) & 0xff, (numOnValues >> 8) & 0xff, (numOnValues >> 16) & 0xff, (numOnValues >> 24) & 0xff);
				songSize += 4;
				// 1. write timestamp stream (DWORDs)
				fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
				for (std::list<DoubleValue>::iterator it = Streams[c].NoteOnStream.values.begin(); it != Streams[c].NoteOnStream.values.end(); it++)
				{
					int dtime = it->time;
					fprintf(expFile, "0x%02x,0x%02x,0x%02x,0x%02x, ", (dtime >> 0) & 0xff, (dtime >> 8) & 0xff, (dtime >> 16) & 0xff, (dtime >> 24) & 0xff);
					songSize += 4;
				}
				// 2. write value1 stream (BYTEs)
				fprintf(expFile, "\n\t\t// Notes\n\t\t");
				for (std::list<DoubleValue>::iterator it = Streams[c].NoteOnStream.values.begin(); it != Streams[c].NoteOnStream.values.end(); it++)
				{
					BYTE value = it->value1;
					fprintf(expFile, "0x%02x,", value);
					songSize += 1;
				}
				// 3. write value2 stream (BYTEs)
				fprintf(expFile, "\n\t\t// Velocities\n\t\t");
				for (std::list<DoubleValue>::iterator it = Streams[c].NoteOnStream.values.begin(); it != Streams[c].NoteOnStream.values.end(); it++)
				{
					BYTE value = it->value2;
					fprintf(expFile, "0x%02x,", value);
					songSize += 1;
				}

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// ATs
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				int numATValues = Streams[c].AftertouchStream.values.size();
				// write number of items (1 DWORD)
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t// Aftertouches %d", numATValues);
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t0x%02x,0x%02x,0x%02x,0x%02x, ", (numATValues >> 0) & 0xff, (numATValues >> 8) & 0xff, (numATValues >> 16) & 0xff, (numATValues >> 24) & 0xff);
				songSize += 4;
				if (numATValues > 0)
				{
					// 1. write timestamp stream (DWORDs)
					fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
					for (std::list<DoubleValue>::iterator it = Streams[c].AftertouchStream.values.begin(); it != Streams[c].AftertouchStream.values.end(); it++)
					{
						int dtime = it->time;
						fprintf(expFile, "0x%02x,0x%02x,0x%02x,0x%02x, ", (dtime >> 0) & 0xff, (dtime >> 8) & 0xff, (dtime >> 16) & 0xff, (dtime >> 24) & 0xff);
						songSize += 4;
					}
					// 2. write value1 stream (BYTEs)
					fprintf(expFile, "\n\t\t// Notes\n\t\t");
					for (std::list<DoubleValue>::iterator it = Streams[c].AftertouchStream.values.begin(); it != Streams[c].AftertouchStream.values.end(); it++)
					{
						BYTE value = it->value1;
						fprintf(expFile, "0x%02x,", value);
						songSize += 1;
					}
					// 3. write value2 stream (BYTEs)
					fprintf(expFile, "\n\t\t// Pressures\n\t\t");
					for (std::list<DoubleValue>::iterator it = Streams[c].AftertouchStream.values.begin(); it != Streams[c].AftertouchStream.values.end(); it++)
					{
						BYTE value = it->value2;
						fprintf(expFile, "0x%02x,", value);
						songSize += 1;
					}
				}

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// CCs
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				// first count filled ccs in channel
				BYTE numCCs = 0;
				for (int cc = 0; cc < 128; cc++)
					if (Streams[c].CCStream[cc].values.size() != 0)
						numCCs++;
				// write number of cc streams to follow (1 BYTE)
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t// Number of CC streams %d", numCCs);
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t0x%02x,", numCCs);
				songSize += 1;

				// write ccs
				for (int cc = 0; cc < 128; cc++)
				{
					int numCCValues = Streams[c].CCStream[cc].values.size();
					if (numCCValues != 0)
					{
						// write cc number (1 BYTE) and number of items (1 DWORD)
						fprintf(expFile, "\n\t\t////////////////////////////////////////");
						fprintf(expFile, "\n\t\t// CC %d, Number of Values %d", cc, numCCValues);
						fprintf(expFile, "\n\t\t////////////////////////////////////////");
						fprintf(expFile, "\n\t\t0x%02x, 0x%02x,0x%02x,0x%02x,0x%02x,", cc, (numCCValues >> 0) & 0xff, (numCCValues >> 8) & 0xff, (numCCValues >> 16) & 0xff, (numCCValues >> 24) & 0xff);
						songSize += 5;

						// 1. write timestamp stream (DWORDs)
						fprintf(expFile, "\n\t\t\t// Timestamps\n\t\t\t");
						for (std::list<SingleValue>::iterator it = Streams[c].CCStream[cc].values.begin(); it != Streams[c].CCStream[cc].values.end(); it++)
						{
							int dtime = it->time;
							fprintf(expFile, "0x%02x,0x%02x,0x%02x,0x%02x, ", (dtime >> 0) & 0xff, (dtime >> 8) & 0xff, (dtime >> 16) & 0xff, (dtime >> 24) & 0xff);
							songSize += 4;
						}
						// 2. write value stream (BYTEs)
						fprintf(expFile, "\n\t\t\t// Values\n\t\t\t");
						for (std::list<SingleValue>::iterator it = Streams[c].CCStream[cc].values.begin(); it != Streams[c].CCStream[cc].values.end(); it++)
						{
							BYTE value = it->value;
							fprintf(expFile, "0x%02x,", value);
							songSize += 1;
						}
					}
				}				
			}

			fprintf(expFile, "\n};\n");
			fprintf(expFile, "// total song data size: %d Bytes\n", songSize);
			fclose(expFile);
		}
	}

	// save an quantized and deltaencoded stream
	void SaveOptimized(std::string filename, int framesize)
	{
		// write export song header file
		FILE* expFile = fopen(filename.c_str(), "w");
		std::string blobFile = filename + ".blob";
		FILE* expBlob = fopen(blobFile.c_str(), "wb");
		if (expFile && expBlob)
		{
			float bpm = SynthGlobalState.CurrentBPM.d[0];

			fprintf(expFile, "#define SAMPLE_RATE 44100\n");
			fprintf(expFile, "#define BPM %f\n", bpm);
			fprintf(expFile, "#define SAMPLES_PER_TICK %d\n", (int)(44100.0f*4.0f*60.0f/(bpm*16)));
			fprintf(expFile, "#define MAX_SAMPLES %d // 0x%02x,0x%02x,0x%02x,0x%02x\n\n", CurrentSample, (CurrentSample >> 0) & 0xff, (CurrentSample >> 8) & 0xff, (CurrentSample >> 16) & 0xff, (CurrentSample >> 24) & 0xff);
			int songSize = 0;

			fprintf(expFile, "static BYTE SynthStream[] = \n");
			fprintf(expFile, "{");
						
			// write song bpm (1 DWORD)		
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n// BPM %f", bpm);
			fprintf(expFile, "\n/////////////////////////////////////////////");
			DWORD* ibpm = (DWORD*)(&bpm);
			fprintf(expFile, "\n0x%02x,0x%02x,0x%02x,0x%02x, ", (*ibpm >> 0) & 0xff, (*ibpm >> 8) & 0xff, (*ibpm >> 16) & 0xff, (*ibpm >> 24) & 0xff);
			fwrite(&ibpm, 4, 1, expBlob);
			songSize += 4;

			// write song length (1 DWORD)
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n// SongLength (Samples) %d", CurrentSample);
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n0x%02x,0x%02x,0x%02x,0x%02x, ", (CurrentSample >> 0) & 0xff, (CurrentSample >> 8) & 0xff, (CurrentSample >> 16) & 0xff, (CurrentSample >> 24) & 0xff);
			fwrite(&CurrentSample, 4, 1, expBlob);
			songSize += 4;

			// write quantization frame size (1 DWORD)
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n// Quantization Frame Size (Samples) %d", framesize);
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n0x%02x,0x%02x,0x%02x,0x%02x, ", (framesize >> 0) & 0xff, (framesize >> 8) & 0xff, (framesize >> 16) & 0xff, (framesize >> 24) & 0xff);
			fwrite(&framesize, 4, 1, expBlob);
			songSize += 4;

			// quantize, sort and calculate total bytes to come for deltadecoding
			int totalDeltaBytes = 0;
			for (int c = 0; c < 16; c++)
			{
				// seperate noteoff stream
#define SEPERATE_NOTEOFF_STREAM
#ifdef SEPERATE_NOTEOFF_STREAM
				Streams[c].NoteOffStream.QuantizeAndSort(framesize, false);
				totalDeltaBytes += 4 + Streams[c].NoteOffStream.qvalues.size() * 4; // single values
#else
				// merge noteoffs with noteons
				for (std::list<SingleValue>::iterator it = Streams[c].NoteOffStream.values.begin(); it != Streams[c].NoteOffStream.values.end(); it++)
				{
					Streams[c].NoteOnStream.Add(it->time, it->value, 0);
				}
#endif				
				Streams[c].NoteOnStream.QuantizeAndSort(framesize);
				totalDeltaBytes += 4 + Streams[c].NoteOnStream.qvalues.size() * 5; // double values
				Streams[c].AftertouchStream.QuantizeAndSort(framesize);
				if (Streams[c].AftertouchStream.qvalues.size() > 0)
					totalDeltaBytes += 4 + Streams[c].AftertouchStream.qvalues.size() * 5; // double values
				for (int cc = 0; cc < 128; cc++)
				{
					Streams[c].CCStream[cc].QuantizeAndSort(framesize, true);
					if (Streams[c].CCStream[cc].qvalues.size())
						totalDeltaBytes += 4 + Streams[c].CCStream[cc].qvalues.size() * 4; // single values
				}
			}

//			UpdateUniqueDeltaFrames();

			// write total bytes to come for deltadecoding (1 DWORD)
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n// Deltaencoded Song Bytes %d", totalDeltaBytes);
			fprintf(expFile, "\n/////////////////////////////////////////////");
			fprintf(expFile, "\n0x%02x,0x%02x,0x%02x,0x%02x, ", (totalDeltaBytes >> 0) & 0xff, (totalDeltaBytes >> 8) & 0xff, (totalDeltaBytes >> 16) & 0xff, (totalDeltaBytes >> 24) & 0xff);
			fwrite(&totalDeltaBytes, 4, 1, expBlob);
			songSize += 4;

			BYTE lastDelta = 0;
			BYTE codedByte = 0;
#define DELTA_ENCODE_VALUE codedByte -= lastDelta; lastDelta += codedByte;
			std::vector<BYTE> deltaStream;
			// process recorded stream and output
			for (int c = 0; c < 16; c++)
			{
				fprintf(expFile, "\n///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n// Channel %d", c + 1);
				fprintf(expFile, "\n///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////");
#ifdef SEPERATE_NOTEOFF_STREAM
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// NoteOffs
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				int numOffValues = Streams[c].NoteOffStream.qvalues.size();				
				// write number of items (1 DWORD)
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t// NoteOffs %d", numOffValues);
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////\n\t");
				codedByte = (numOffValues >> 0) & 0xff; DELTA_ENCODE_VALUE;
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				codedByte = (numOffValues >> 8) & 0xff; DELTA_ENCODE_VALUE;
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				codedByte = (numOffValues >> 16) & 0xff; DELTA_ENCODE_VALUE;
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				codedByte = (c << 4) + 0; DELTA_ENCODE_VALUE; // last byte: stream type + channel
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				songSize += 4;
				// 1. write timestamp stream (3xBYTEs)
				fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
				deltaStream = Streams[c].NoteOffStream.GetTime0DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}

				fprintf(expFile, "\n\t\t");
				deltaStream = Streams[c].NoteOffStream.GetTime1DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}

				fprintf(expFile, "\n\t\t");
				deltaStream = Streams[c].NoteOffStream.GetTime2DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}
				// 2. write value stream (BYTEs)
				fprintf(expFile, "\n\t\t// Notes\n\t\t");
				deltaStream = Streams[c].NoteOffStream.GetValueDeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}

				songSize += numOffValues*4;
#endif
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// NoteOns
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				int numOnValues = Streams[c].NoteOnStream.qvalues.size();				
				// write number of items (1 DWORD)
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
				fprintf(expFile, "\n\t// NoteOns %d", numOnValues);
				fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////\n\t");
				codedByte = (numOnValues >> 0) & 0xff; DELTA_ENCODE_VALUE;
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				codedByte = (numOnValues >> 8) & 0xff; DELTA_ENCODE_VALUE;
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				codedByte = (numOnValues >> 16) & 0xff; DELTA_ENCODE_VALUE;
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				codedByte = (c << 4) + 1; DELTA_ENCODE_VALUE; // last byte: stream type + channel
				fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
				songSize += 4;
				// 1. write timestamp stream (3xBYTEs)
				fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
				deltaStream = Streams[c].NoteOnStream.GetTime0DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}

				fprintf(expFile, "\n\t\t");
				deltaStream = Streams[c].NoteOnStream.GetTime1DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}

				fprintf(expFile, "\n\t\t");
				deltaStream = Streams[c].NoteOnStream.GetTime2DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}
				// 2. write value stream (BYTEs)
				fprintf(expFile, "\n\t\t// Notes\n\t\t");
				deltaStream = Streams[c].NoteOnStream.GetValue1DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}
				// 3. write value stream (BYTEs)
				fprintf(expFile, "\n\t\t// Velocities\n\t\t");
				deltaStream = Streams[c].NoteOnStream.GetValue2DeltaStream(lastDelta);
				for (int i = 0; i < deltaStream.size(); i++)
				{
					fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
				}

				songSize += numOnValues * 5;

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// ATs
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								
				int numATValues = Streams[c].AftertouchStream.qvalues.size();
				if (numATValues > 0)
				{
					// write number of items (1 DWORD)
					fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////");
					fprintf(expFile, "\n\t// Aftertouches %d", numATValues);
					fprintf(expFile, "\n\t///////////////////////////////////////////////////////////////////////////////////\n\t");
					codedByte = (numATValues >> 0) & 0xff; DELTA_ENCODE_VALUE;
					fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
					codedByte = (numATValues >> 8) & 0xff; DELTA_ENCODE_VALUE;
					fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
					codedByte = (numATValues >> 16) & 0xff; DELTA_ENCODE_VALUE;
					fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
					codedByte = (c << 4) + 2; DELTA_ENCODE_VALUE; // last byte: stream type + channel
					fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
					songSize += 4;

				
					// 1. write timestamp stream (3xBYTEs)
					fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
					deltaStream = Streams[c].AftertouchStream.GetTime0DeltaStream(lastDelta);
					for (int i = 0; i < deltaStream.size(); i++)
					{
						fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
					}

					fprintf(expFile, "\n\t\t");
					deltaStream = Streams[c].AftertouchStream.GetTime1DeltaStream(lastDelta);
					for (int i = 0; i < deltaStream.size(); i++)
					{
						fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
					}

					fprintf(expFile, "\n\t\t");
					deltaStream = Streams[c].AftertouchStream.GetTime2DeltaStream(lastDelta);
					for (int i = 0; i < deltaStream.size(); i++)
					{
						fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
					}
					// 2. write value stream (BYTEs)
					fprintf(expFile, "\n\t\t// Notes\n\t\t");
					deltaStream = Streams[c].AftertouchStream.GetValue1DeltaStream(lastDelta);
					for (int i = 0; i < deltaStream.size(); i++)
					{
						fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
					}
					// 3. write value stream (BYTEs)
					fprintf(expFile, "\n\t\t// Pressures\n\t\t");
					deltaStream = Streams[c].AftertouchStream.GetValue2DeltaStream(lastDelta);
					for (int i = 0; i < deltaStream.size(); i++)
					{
						fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
					}

					songSize += numATValues * 5;
				}

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// CCs
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				// write ccs
				for (int cc = 0; cc < 128; cc++)
				{
					int numCCValues = Streams[c].CCStream[cc].qvalues.size();
					if (numCCValues > 0)
					{
						// write number of items (1 DWORD)
						fprintf(expFile, "\n\t////////////////////////////////////////");
						fprintf(expFile, "\n\t// CC %d, Number of Values %d", cc, numCCValues);
						fprintf(expFile, "\n\t////////////////////////////////////////\n\t");
						codedByte = (numCCValues >> 0) & 0xff; DELTA_ENCODE_VALUE;
						fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
						codedByte = (numCCValues >> 8) & 0xff; DELTA_ENCODE_VALUE;
						fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
						codedByte = cc; DELTA_ENCODE_VALUE;
						fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
						codedByte = (c << 4) + 3; DELTA_ENCODE_VALUE; // last bytes: cc id , stream type + channel
						fprintf(expFile, "0x%02x,", codedByte); fwrite(&codedByte, 1, 1, expBlob);
						songSize += 4;
						// 1. write timestamp stream (3xBYTEs)
						fprintf(expFile, "\n\t\t// Timestamps\n\t\t");
						deltaStream = Streams[c].CCStream[cc].GetTime0DeltaStream(lastDelta);
						for (int i = 0; i < deltaStream.size(); i++)
						{
							fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
						}

						fprintf(expFile, "\n\t\t");
						deltaStream = Streams[c].CCStream[cc].GetTime1DeltaStream(lastDelta);
						for (int i = 0; i < deltaStream.size(); i++)
						{
							fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
						}

						fprintf(expFile, "\n\t\t");
						deltaStream = Streams[c].CCStream[cc].GetTime2DeltaStream(lastDelta);
						for (int i = 0; i < deltaStream.size(); i++)
						{
							fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
						}
						// 2. write value stream (BYTEs)
						fprintf(expFile, "\n\t\t// Values\n\t\t");
						deltaStream = Streams[c].CCStream[cc].GetValueDeltaStream(lastDelta);
						for (int i = 0; i < deltaStream.size(); i++)
						{
							fprintf(expFile, "0x%02x,", deltaStream[i]); fwrite(&deltaStream[i], 1, 1, expBlob);
						}

						songSize += numCCValues * 4;
					}
				}				
			}

			if (totalDeltaBytes != songSize-16)
				fprintf(expFile, "\n// !!! Song size does not match!!!\n");

			fprintf(expFile, "\n// End of Streams\n");
			fprintf(expFile, "0x%02x,0x%02x,0x%02x,0x%02x, ", 0xff, 0xff, 0xff, 0xff);
			DWORD eos = 0xffffffff;
			fwrite(&eos, 4, 1, expBlob);
			songSize += 4;

			fprintf(expFile, "\n};\n");
			fprintf(expFile, "// total song data size: %d Bytes\n", songSize);
			fclose(expFile);
			fclose(expBlob);
		}
	}

	void UpdateUniqueDeltaFrames()
	{
		uniqueDeltaFrames.clear();
		for (int c = 0; c < 16; c++)
		{
			DWORD lastFrame;
			// seperate noteoff stream
#define SEPERATE_NOTEOFF_STREAM
#ifdef SEPERATE_NOTEOFF_STREAM
			lastFrame = 0;
			for (std::list<SingleValue>::iterator it = Streams[c].NoteOffStream.qvalues.begin(); it != Streams[c].NoteOffStream.qvalues.end(); it++)
			{
				uniqueDeltaFrames[it->time - lastFrame] = 1;
				lastFrame = it->time;
			}
#endif				
			lastFrame = 0;
			for (std::list<DoubleValue>::iterator it = Streams[c].NoteOnStream.qvalues.begin(); it != Streams[c].NoteOnStream.qvalues.end(); it++)
			{
				uniqueDeltaFrames[it->time - lastFrame] = 1;
				lastFrame = it->time;
			}
			lastFrame = 0;
			for (std::list<DoubleValue>::iterator it = Streams[c].AftertouchStream.qvalues.begin(); it != Streams[c].AftertouchStream.qvalues.end(); it++)
			{
				uniqueDeltaFrames[it->time - lastFrame] = 1;
				lastFrame = it->time;
			}			
			for (int cc = 0; cc < 128; cc++)
			{
				lastFrame = 0;
				for (std::list<SingleValue>::iterator it = Streams[c].CCStream[cc].qvalues.begin(); it != Streams[c].CCStream[cc].qvalues.end(); it++)
				{
					uniqueDeltaFrames[it->time - lastFrame] = 1;
					lastFrame = it->time;
				}
			}
		}
	}

	// status
	bool IsActive;
	DWORD CurrentSample;
	bool FirstEventSample;
	// channel streams
	std::vector<ChannelStream> Streams;
	std::map<int, int> uniqueDeltaFrames;
};

// the recorder class
StreamRecorder Recorder;

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

#define LOAD_FROM_RAWEXPORT_H_
#ifdef LOAD_FROM_RAWEXPORT_H
#include "../ExeMusic/Debug64k2Song.h" // raw export file
#endif
void SynthController::exportSong(const std::string& filename, int timeQuant)
{

#ifdef LOAD_FROM_RAWEXPORT_H
	// load existing export to recorder
	Recorder.LoadRaw(SynthRawStream);
	timeQuant = 128;
#endif
	if (filename != "")
	{		
		// save raw export to a parallel file
		std::string newfilename = filename.substr(0, filename.size() - 2) + "_rawexport.h";		
		Recorder.SaveRaw(newfilename);

		// save optimized export data
		Recorder.SaveOptimized(filename, timeQuant);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::recursiveCollectUsedNodes(SynthNode* node, std::map<SynthNode*, bool>& usedNodes)
{
	if (usedNodes[node])
		return;

	usedNodes[node] = true;

	// recursion not for constants
	if (node->id != CONSTANT_ID )
	{		
		for (DWORD i = 0; i < node->numInputs; i++)
		{				
			SynthNode* input = (SynthNode*)(node->input[i]);
			recursiveCollectUsedNodes(input, usedNodes);
		}
	}
}

void GetConstantValueString(SynthNode* n, std::string& ret, int& type, SynthNode* pn, std::vector<WORD>& blobVec)
{	
	char tmp[256];
	blobVec.clear();
	// mode constant?
	if (n->numInputs == 0)
	{
		DWORD iv = n->out.i[0];		
		// voicemanager and samplerecorder are using 4 bytes for mode
		if (pn->id == VOICEMANAGER_ID || pn->id == SAMPLEREC_ID)
		{
			sprintf(tmp, "0x%04x,0x%04x", (iv >> 0) & 0xffff, (iv >> 16) & 0xffff);
			blobVec.push_back((iv >> 0) & 0xffff); blobVec.push_back((iv >> 16) & 0xffff);
		}
		// all other modes are using only 2 bytes
		else
		{
			sprintf(tmp, "0x%04x", (iv >> 0) & 0xffff);
			blobVec.push_back((iv >> 0) & 0xffff);
		}
			
		type = 0;
	}
	else
	{
		// mono constant?
		if (n->out.d[0] == n->out.d[1])
		{
			float fv = n->out.d[0];
			DWORD iv = *((DWORD*)(&fv)) & 0xffffff00; // always zero out the least significant byte
			sprintf(tmp, "0x%04x,0x%04x", (iv >> 16) & 0xffff, iv & 0xffff);
			blobVec.push_back(iv & 0xffff); blobVec.push_back((iv >> 16) & 0xffff);
			type = 1;
		}
		// stereo constant
		else
		{
			float fv1 = n->out.d[0];
			float fv2 = n->out.d[1];
			DWORD iv1 = *((DWORD*)(&fv1)) & 0xffffff00; // always zero out the least significant byte
			DWORD iv2 = *((DWORD*)(&fv2)) & 0xffffff00; // always zero out the least significant byte
			// a real stereo constant?
			if (iv1 != iv2)
			{
				sprintf(tmp, "0x%04x,0x%04x,0x%04x,0x%04x", (iv1 >> 16) & 0xffff, iv1 & 0xffff, (iv2 >> 16) & 0xffff, iv2 & 0xffff);
				blobVec.push_back(iv1 & 0xffff); blobVec.push_back((iv1 >> 16) & 0xffff);
				blobVec.push_back(iv2 & 0xffff); blobVec.push_back((iv2 >> 16) & 0xffff);
				type = 2;
			}
			// actually a mono constant
			else
			{
				sprintf(tmp, "0x%04x,0x%04x", (iv1 >> 16) & 0xffff, iv1 & 0xffff);
				blobVec.push_back(iv1 & 0xffff); blobVec.push_back((iv1 >> 16) & 0xffff);
				type = 1;
			}
		}
	}
	ret = tmp;
}

void GetArpSequenceValueString(SynthNode* n, std::string& ret, std::vector<WORD>& blobVec)
{	
	char tmp[256];
	blobVec.clear();
	ret = "";
	sprintf(tmp, "0x%04x,", ((VMWork*)(n->customMem))->ArpSequenceLoopIndex);
	blobVec.push_back(((VMWork*)(n->customMem))->ArpSequenceLoopIndex);
	ret += tmp;
	for (int i = 0; i < 32; i++)
	{
		sprintf(tmp, "0x%04x,", ((VMWork*)(n->customMem))->ArpSequencePtr[i]);
		blobVec.push_back(((VMWork*)(n->customMem))->ArpSequencePtr[i]);
		ret += tmp;
	}
}

void GetTriggerSeqValueString(SynthNode* n, std::string& ret, std::vector<WORD>& blobVec)
{
	char tmp[256];
	blobVec.clear();
	ret = "";
	for (int i = TRIGGERSEQ_PATTERN0_3L; i <= TRIGGERSEQ_PATTERN12_15R; i++)
	{
		int pattern = (*(n->input[i])).i[0];
		sprintf(tmp, "0x%04x,0x%04x,", (pattern >> 0) & 0xffff, (pattern >> 16) & 0xffff);
		blobVec.push_back((pattern >> 0) & 0xffff); blobVec.push_back((pattern >> 16) & 0xffff);
		ret += tmp;
	}
}

int GetFormulaLength(SynthNode* n)
{
	BYTE* cmdbuf = (BYTE*)(SynthGlobalState.SpecialDataPointer[n->valueOffset]);
	BYTE* command = cmdbuf;
	while (*command != FORMULA_DONE)
	{
		if (*command == FORMULA_CMD_CONSTANT)
			command += 4;
		else
			command++;
	}
	int datalength = 1 + command - cmdbuf;
	return datalength;
}

void GetFormulaValueString(SynthNode* n, std::string& ret, std::vector<WORD>& blobVec)
{
	char tmp[256];
	blobVec.clear();
	ret = "";

	int datalength = GetFormulaLength(n);
	// convert to hex word string and write
	WORD* buf = (WORD*)(SynthGlobalState.SpecialDataPointer[n->valueOffset]);
	for (int j = 0; j < datalength / 2; j++)
	{
		sprintf(tmp, "0x%04x,", *buf);
		blobVec.push_back(*buf);
		ret += tmp;
		buf++;
	}
	// write an addtional word if datalength is odd, so lowbyte contains the last data byte
	if (datalength & 1)
	{
		WORD padding = 0;
		padding += *((BYTE*)(buf));
		sprintf(tmp, "0x%04x,", padding);
		blobVec.push_back(padding);
		ret += tmp;
	}
}

std::string GetVoiceParamValueString(SynthNode* n, std::string& ret)
{
	char tmp[256];

	SynthNode* vps = (SynthNode*)(n->input[0]);
	float fv1 = vps->out.d[0];
	float fv2 = vps->out.d[1];
	DWORD iv1 = *((DWORD*)(&fv1)) & 0xffffff00; // always zero out the least significant byte
	DWORD iv2 = *((DWORD*)(&fv2)) & 0xffffff00; // always zero out the least significant byte
	// a real stereo constant?
	sprintf(tmp, "%0x02x,0x%04x,0x%04x,0x%04x,0x%04x", n->id, (iv1 >> 16) & 0xffff, iv1 & 0xffff, (iv2 >> 16) & 0xffff, iv2 & 0xffff);
	ret = tmp;
	return ret;
}

struct SynthFeatures
{
	// global
	bool useSpecialData;
	bool useStoredSamples;		
	bool useAftertouch;
	// voicemanager
	bool useARP;
	bool useGlide;	
	// adsr
	bool useADSRTriggerGate;
	bool useADSRDBGain;
	bool useADSRExp;
	// oscillator/lfo
	bool useWaveSINE;
	bool useWaveSAW;
	bool useWavePULSE;
	bool useWaveAATRISAW;
	bool useWaveAAPULSE;
	bool useWaveAABITRISAW;
	bool useWaveAABIPULSE;
	bool useWaveRPSINE;
	bool useWavePNOISE;
	bool useWaveAATRISAWSEQ;
	bool useWaveSINESEQ;
	bool useLFOPolarity;
	bool useOSCFrequency;
	// noisegen
	bool useNGBrown;
	// bqfilter/svfilter
	bool useFrequencyMap;
	bool useBQFLP;
	bool useBQFHP;
	bool useBQFBP;
	bool useBQFBPBW;
	bool useBQFNOTCH;
	bool useBQFAP;
	bool useBQFPEAK;
	bool useBQFLSHELF;
	bool useBQFHSHELF;
	bool useBQFRES;
	bool useBQFRESN;
	// distortion
	bool useDSTOD;
	bool useDSTDAMP;
	bool useDSTQUANT;
	bool useDSTLIMIT;
	bool useDSTASYM;
	bool useDSTSIN;
	// delays
	bool useDLAP;
	bool useDLBPM;
	bool useDLS;
	bool useDLM;
	bool useDLL;
	bool useDLNM;
	bool useDLNM2;
	// sampler
	bool useSMPLoop;
	// wtf
	bool useWTFReduceClick;
	// glitch
	bool useGLTS;
	bool useGLRT;
	bool useGLSH;
	bool useGLREV;
	// snh
	bool useSNHSM;
	
	std::map<int, std::map<void*, SynthNode*> > specialDataMap;
	std::vector<std::string> uniqueArpSequenceList;
	std::vector<WORD> uniqueArpSequenceBlobList;

	SynthFeatures()
	{		
		useSpecialData = false;
		useStoredSamples = false;
		useAftertouch = false;
		useARP = false;
		useGlide = false;		
		useADSRTriggerGate = false;
		useADSRDBGain = false;
		useADSRExp = false;
		useWaveSINE = false;
		useWaveSAW = false;
		useWavePULSE = false;
		useWaveAATRISAW = false;
		useWaveAAPULSE = false;
		useWaveAABITRISAW = false;
		useWaveAABIPULSE = false;
		useWaveRPSINE = false;
		useWavePNOISE = false;
		useWaveAATRISAWSEQ = false;
		useWaveSINESEQ = false;
		useLFOPolarity = false;
		useOSCFrequency = false;
		useNGBrown = false;
		useFrequencyMap = false;
		useBQFLP = false;
		useBQFHP = false;
		useBQFBP = false;
		useBQFBPBW = false;
		useBQFNOTCH = false;
		useBQFAP = false;
		useBQFPEAK = false;
		useBQFLSHELF = false;
		useBQFHSHELF = false;
		useBQFRES = false;
		useBQFRESN = false;
		useDSTOD = false;
		useDSTDAMP = false;
		useDSTQUANT = false;
		useDSTLIMIT = false;
		useDSTASYM = false;
		useDSTSIN = false;
		useDLAP = false;
		useDLBPM = false;
		useDLS = false;
		useDLM = false;
		useDLL = false;
		useDLNM = false;
		useDLNM2 = false;
		useSMPLoop = false;
		useWTFReduceClick = false;
		useGLTS = false;
		useGLRT = false;
		useGLSH = false;
		useGLREV = false;
		useSNHSM = false;

		specialDataMap.clear();
		uniqueArpSequenceList.clear();
		uniqueArpSequenceBlobList.clear();
	}
};

void SynthController::exportPatch(const std::string& filename)
{
	SynthFeatures features;
	std::vector<WORD> blobVec;

	// 1. recursively collect relevant/used nodes from synth root on	
	// clear used node list
	std::map<SynthNode*, bool> usedNodes;
	for (DWORD i = 0; i < MAX_NODES; i++)
	{	
		if (_nodes[i])
			usedNodes[_nodes[i]] = false;
	}
	// collect used nodes
	recursiveCollectUsedNodes(SynthGlobalState.GlobalNodes[0], usedNodes);	

	// scan for special uses of features
	for (DWORD i = 0; i < MAX_NODES; i++)
	{	
		if (_nodes[i])
		{
			// if we have a non used node (e.g. due to disconnection) check if it is a required one for the export and use it anyway
			if (!usedNodes[_nodes[i]])
			{
				switch (_nodes[i]->id)
				{			
					// always export all channelroots and notecontrollers so the offsets are right for the replayer when indexing channels
					case CHANNELROOT_ID: 
					case NOTECONTROLLER_ID:
					{
						usedNodes[_nodes[i]] = true;
						break;
					}
				}
			}
			else
			{
				switch (_nodes[i]->id)
				{			
					case VOICEMANAGER_ID:
					{
						// check for ARP usage and store sequence
						if (_nodes[i]->input[VOICEMANAGER_MODE]->i[0] & VOICEMANAGER_ARP_RUNMASK)
						{
							std::string arpstr;
							GetArpSequenceValueString(_nodes[i], arpstr, blobVec);
							// get sequence index
							int arpSeqIndex = features.uniqueArpSequenceList.size();
							for (int as = 0; as < features.uniqueArpSequenceList.size(); as++)
							{
								if (features.uniqueArpSequenceList[as] == arpstr)
								{
									arpSeqIndex = as;
									break;
								}
							}
							// not found, so add new sequence
							if (arpSeqIndex == features.uniqueArpSequenceList.size())
							{
								features.uniqueArpSequenceList.push_back(arpstr);
								features.uniqueArpSequenceBlobList.insert(features.uniqueArpSequenceBlobList.end(), blobVec.begin(), blobVec.end());
							}					

							// set the sequence index in the mode (doesnt hurt in plugin as those bits are unused, but we need this before unifying constants)
							_nodes[i]->input[VOICEMANAGER_MODE]->i[0] &= ~VOICEMANAGER_ARP_INDEXMASK;
							_nodes[i]->input[VOICEMANAGER_MODE]->i[0] |= arpSeqIndex << VOICEMANAGER_ARP_INDEXSHIFT;

							features.useARP = true;
						}
						// check for glide usage (used when input is a non constant or the constants value is not zero)
						SynthNode* glide = (SynthNode*)(_nodes[i]->input[VOICEMANAGER_GLIDE]);
						if (glide->id != CONSTANT_ID || !_mm_testz_si128(glide->out.pi, SC[S_ALLBITS].pi))
						{
							features.useGlide = true;
						}
						break;
					}	
					case ADSR_ID:
					{
						// check if trigger and gate are used at all?
						SynthNode* trigger = (SynthNode*)(_nodes[i]->input[ADSR_TRIGGER]);
						SynthNode* gate = (SynthNode*)(_nodes[i]->input[ADSR_GATE]);
						if (trigger->id != CONSTANT_ID || !_mm_testz_si128(trigger->out.pi, SC[S_ALLBITS].pi) ||
							gate->id != CONSTANT_ID || !_mm_testz_si128(gate->out.pi, SC[S_ALLBITS].pi))
						{
							features.useADSRTriggerGate = true;
						}

						// check if dbgain and exp options are used
						if (_nodes[i]->input[ADSR_MODE]->i[0] & ADSR_EXP)
							features.useADSRExp = true;
						if (_nodes[i]->input[ADSR_MODE]->i[0] & ADSR_DBGAIN)
							features.useADSRDBGain = true;
						break;
					}
					case LFO_ID:
					case OSCILLATOR_ID:
					{
						int waveMask;
						if (_nodes[i]->id == LFO_ID)
						{
							// check polarity
							int mode = _nodes[i]->input[LFO_MODE]->i[0];
							if (mode & LFO_BIPOLAR)
								features.useLFOPolarity = true;

							waveMask = mode & LFO_WAVEMASK;
						}
						if (_nodes[i]->id == OSCILLATOR_ID)
						{
							// check if frequency input exists
							SynthNode* freq = (SynthNode*)(_nodes[i]->input[OSCILLATOR_FREQ]);
							if (freq->id != CONSTANT_ID || !_mm_testz_si128(freq->out.pi, SC[S_ALLBITS].pi))
								features.useOSCFrequency = true;

							waveMask = _nodes[i]->input[OSCILLATOR_MODE]->i[0] & OSCILLATOR_WAVEMASK;
						}	
						// waves
						if (waveMask == OSCILLATOR_SINE)
							features.useWaveSINE = true;
						if (waveMask == OSCILLATOR_SAW)
							features.useWaveSAW = true;
						if (waveMask == OSCILLATOR_PULSE)
							features.useWavePULSE = true;
						if (waveMask == OSCILLATOR_AATRISAW)
							features.useWaveAATRISAW = true;
						if (waveMask == OSCILLATOR_AAPULSE)
							features.useWaveAAPULSE = true;
						if (waveMask == OSCILLATOR_AABITRISAW)
							features.useWaveAABITRISAW = true;
						if (waveMask == OSCILLATOR_AABIPULSE)
							features.useWaveAABIPULSE = true;
						if (waveMask == OSCILLATOR_RPSINE)
							features.useWaveRPSINE = true;
						if (waveMask == OSCILLATOR_PNOISE)
							features.useWavePNOISE = true;
						if (waveMask == OSCILLATOR_AATRISAWSEQ)
							features.useWaveAATRISAWSEQ = true;
						if (waveMask == OSCILLATOR_SINESEQ)
							features.useWaveSINESEQ = true;						
						
						break;
					}
					case NOISEGEN_ID:
					{
						// check for constant mix if brown noise calc can be skipped
						SynthNode* mix = (SynthNode*)(_nodes[i]->input[NOISEGEN_MIX]);
						if (mix->id == CONSTANT_ID && mix->out.d[0] > 0.5 && mix->out.d[1] > 0.5)
							features.useNGBrown = true;
						break;
					}
					case BQFILTER_ID:
					case SVFILTER_ID:
					{
						int mode;
						if (_nodes[i]->id == BQFILTER_ID)
						{	
							mode = _nodes[i]->input[BQFILTER_MODE]->i[0];

							int type = mode & BQFILTER_MODEMASK;
							if (type == BQFILTER_LOWPASS)
								features.useBQFLP = true;
							if (type == BQFILTER_HIGHPASS)
								features.useBQFHP = true;
							if (type == BQFILTER_BANDPASSQ)
								features.useBQFBP = true;
							if (type == BQFILTER_BANDPASSBW)
								features.useBQFBPBW = true;
							if (type == BQFILTER_NOTCH)
								features.useBQFNOTCH = true;
							if (type == BQFILTER_ALLPASS)
								features.useBQFAP = true;
							if (type == BQFILTER_PEAK)
								features.useBQFPEAK = true;
							if (type == BQFILTER_LOWSHELF)
								features.useBQFLSHELF = true;
							if (type == BQFILTER_HIGHSHELF)
								features.useBQFHSHELF = true;
							if (type == BQFILTER_RESONANCE)
								features.useBQFRES = true;
							if (type == BQFILTER_RESONANCENORM)
								features.useBQFRESN = true;
						}
						if (_nodes[i]->id == SVFILTER_ID)
						{
							mode = _nodes[i]->input[SVFILTER_MODE]->i[0];
						}

						if (!(mode & BQFILTER_QUADRATIC))
							features.useFrequencyMap = true;				

						break;
					}
					case DISTORTION_ID:
					{
						int type = _nodes[i]->input[DISTORTION_MODE]->i[0];
						if (type == DISTORTION_OVERDRIVE)
							features.useDSTOD = true;
						if (type == DISTORTION_DAMP)
							features.useDSTDAMP = true;
						if (type == DISTORTION_QUANT)
							features.useDSTQUANT = true;
						if (type == DISTORTION_LIMIT)
							features.useDSTLIMIT = true;
						if (type == DISTORTION_ASYM)
							features.useDSTASYM = true;
						if (type == DISTORTION_SIN)
							features.useDSTSIN = true;
						break;
					}
					case COMBDELAY_ID:
					case DELAY_ID:
					case FBDELAY_ID:
					{
						// allpass interpolation
						if (_nodes[i]->id == DELAY_ID || _nodes[i]->id == FBDELAY_ID)
							features.useDLAP = true;

						// timing modes
						int mode;
						if (_nodes[i]->id == COMBDELAY_ID)
							mode = _nodes[i]->input[COMBDELAY_MODE]->i[0];
						if (_nodes[i]->id == DELAY_ID)
							mode = _nodes[i]->input[DELAY_MODE]->i[0];
						if (_nodes[i]->id == FBDELAY_ID)
							mode = _nodes[i]->input[FBDELAY_MODE]->i[0];
						mode &= DELAY_TIMEMASK;

						if (mode == DELAY_BPMSYNC)
							features.useDLBPM = true;
						if (mode == DELAY_SHORT)
							features.useDLS = true;
						if (mode == DELAY_MIDDLE)
							features.useDLM = true;
						if (mode == DELAY_LONG)
							features.useDLL = true;
						if (mode == DELAY_NOTEMAP)
							features.useDLNM = true;
						if (mode == DELAY_NOTEMAP2)
							features.useDLNM2 = true;

						break;
					}
					case SAMPLER_ID:
					{
						int mode = _nodes[i]->input[SAMPLER_MODE]->i[0];
						// check for sample playback
						if ((mode & SAMPLER_MODEMASK) == SAMPLER_STORED)
							features.useStoredSamples = true;
						// check for loop
						if (mode & SAMPLER_LOOP)
							features.useSMPLoop = true;
						break;
					}
					case WTFOSC_ID:
					{
						int mode = _nodes[i]->input[WTFOSC_MODE]->i[0];
						// check for sample playback
						if ((mode & WTFOSC_MODEMASK) == WTFOSC_STORED)
							features.useStoredSamples = true;
						// check for click reduction
						if (mode & WTFOSC_REDUCECLICK)
							features.useWTFReduceClick = true;
						break;
					}
					case GLITCH_ID:
					{
						int type = _nodes[i]->input[GLITCH_MODE]->i[0];
						if (type == GLITCH_TAPESTOP)
							features.useGLTS = true;
						if (type == GLITCH_RETRIGGER)
							features.useGLRT = true;
						if (type == GLITCH_SHUFFLE)
							features.useGLSH = true;
						if (type == GLITCH_REVERSE)
							features.useGLREV = true;
						break;				
					}
					case SAPI_ID:
					{
						// add data blob to specialdatamap
						features.useSpecialData = true;

						if (features.specialDataMap.find(_nodes[i]->id) == features.specialDataMap.end())
						{
							std::map<void*, SynthNode*> dataMap;
							features.specialDataMap[_nodes[i]->id] = dataMap;
						}
						features.specialDataMap[_nodes[i]->id][_nodes[i]->specialData] = _nodes[i];
						break;
					}
					case SNH_ID:
					{
						// check for snh smooth usage (used when input is a non constant or the constants value is not zero)
						SynthNode* snh = (SynthNode*)(_nodes[i]->input[SNH_SNHSMOOTH]);
						if (snh->id != CONSTANT_ID || !_mm_testz_si128(snh->out.pi, SC[S_ALLBITS].pi))
						{
							features.useSNHSM = true;
						}
						break;
					}
					case VOICE_AFTERTOUCH_ID:
					{
						features.useAftertouch = true;
						break;
					}
					default:
						break;
				}
			}
		}
	}

	// 2. group nodes by node type
	std::vector<std::map<std::string, SynthNode*> > uniqueConstantMap;
	uniqueConstantMap.push_back(std::map<std::string, SynthNode*>()); // mode   constants
	uniqueConstantMap.push_back(std::map<std::string, SynthNode*>()); // mono   constants
	uniqueConstantMap.push_back(std::map<std::string, SynthNode*>()); // stereo constants
	std::map<int, std::pair<std::vector<SynthNode*>, std::vector<SynthNode*> > > groupedNodes; // global, local for each id
	std::map<SynthNode*, SynthNode*> noteRemap;
	std::map<std::string, SynthNode*> uniqueVoiceParamMap;
	for (DWORD i = 0; i < MAX_NODES; i++)
	{	
		SynthNode* n = _nodes[i];
		if (n && usedNodes[n])
		{
			// special case for constants, not adding to grouped list but unifying values in maps for the 3 constant types
			if (n->id == CONSTANT_ID)
			{
				int consttype;
				std::string conststr;
				GetConstantValueString(n, conststr, consttype, n, blobVec);
				if (consttype > 0)
				{
					std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[consttype].find(conststr);
					if (it == uniqueConstantMap[consttype].end())
					{
						uniqueConstantMap[consttype][conststr] = n;
					}
				}
			}
			else
			{
				int groupid = n->id;
				// a scale is a mul
				if (groupid == SCALE_ID)
					groupid = MUL_ID;
				// first encounter of a node type
				if (groupedNodes.find(groupid) == groupedNodes.end())
				{
					std::vector<SynthNode*> nodeListGlobal;
					std::vector<SynthNode*> nodeListLocal;
					groupedNodes[groupid] = std::make_pair(nodeListGlobal, nodeListLocal);
				}
				// add nodes can be skipped if they add the 0 constant (happens a lot for modulations)
				if (n->id == ADD_ID)
				{
					if (((SynthNode*)(n->input[0]))->id == CONSTANT_ID)
					{
						int consttype;
						std::string conststr;
						GetConstantValueString(((SynthNode*)(n->input[0])), conststr, consttype, n, blobVec);
						if (consttype > 0)
						{
							if (conststr == "0x0000,0x0000")
							{
								noteRemap[n] = (SynthNode*)(n->input[1]);
								continue;
							}
						}
					}
				}
				// voice constants can be optimized so that voice constants of same type with same scale values are reused as well.
				if (n->id > CONSTANT_ID)
				{
					std::string vpstr;
					GetVoiceParamValueString(n, vpstr);
					std::map<std::string, SynthNode*>::iterator it = uniqueVoiceParamMap.find(vpstr);
					if (it == uniqueVoiceParamMap.end())
					{
						uniqueVoiceParamMap[vpstr] = n;
					}
					else
					{
						noteRemap[n] = it->second;
						continue;
					}
				}
				if (n->isGlobal)
					groupedNodes[groupid].first.push_back(n);
				else
					groupedNodes[groupid].second.push_back(n);				
			}
			// all other nodes just refer to themselves
			noteRemap[n] = n;
		}
	}

	// 3. calculate packed node group offsets for output	
	std::map<SynthNode*, int> nodeOffsets;
	std::map<SynthNode*, int> nodeOffsetsBlob;
	int MaxOffset = 0;
	int MaxOffsetBlob = 0;
	for (int i = 0; i < MAXIMUM_ID; i++)
	{
		if (groupedNodes.find(i) != groupedNodes.end())
		{
			for (int gl = 0; gl < 2; gl++)
			{
				std::vector<SynthNode*>* nlist;
				if (gl == 0)
					nlist = &(groupedNodes[i].first);
				else
					nlist = &(groupedNodes[i].second);

				for (int j = 0; j < (*nlist).size(); j++)
				{
					SynthNode* curNode = (*nlist)[j];
					nodeOffsets[curNode] = MaxOffset;
					nodeOffsetsBlob[curNode] = MaxOffsetBlob;

					// delta is 1 for the nodeinfo plus the number of inputs
					int deltaOffset = 1 + curNode->numInputs;

					// mode constants if they exist, they are the last input
					// for 2 byte modes we simply inline the value at that index directly instead of referring to some mode constant offset (see below)
					// for voicemanager and triggersequencer we do the same but need a 4 byte mode, so one additional index needs to exist for inlining for that node type
					if (curNode->id == VOICEMANAGER_ID || curNode->id == SAMPLEREC_ID)
						deltaOffset = 1 + curNode->numInputs + 1;

					// special case for triggerseq where the patterns are given as input after the mode
					if (curNode->id == TRIGGERSEQ_ID)
						deltaOffset = 1 + NodeMaxGUISignals[curNode->id] + 1 + 16;

					// reduce adsr inputs when useADSRTriggerGate not used
					if (curNode->id == ADSR_ID && features.useADSRTriggerGate == false)
					{
						deltaOffset -= 2;
						// dont do it for blob export
						MaxOffsetBlob += 2;
					}

					// calc offset for formula nodes (formula is stored in place as "mode")
					if (curNode->id == FORMULA_ID)
					{
						int comsize = GetFormulaLength(curNode);
						if (comsize & 1)
							comsize++;
						deltaOffset += comsize /= 2;
					}

					MaxOffset += deltaOffset;
					MaxOffsetBlob += deltaOffset;
				}
			}
		}
	}

#define ALIGN_CONSTANTS
#define GROUP_BASE_CONSTANTS

#ifdef ALIGN_CONSTANTS
	// align constants to a 256byte boundary
	int ConstAlignPadding = 0;
	int ConstAlignPaddingBlob = 0;
	while ((MaxOffset & 0x0f) != 0)
	{
		MaxOffset++;
		ConstAlignPadding++;
	}
	while ((MaxOffsetBlob & 0x0f) != 0)
	{
		MaxOffsetBlob++;
		ConstAlignPaddingBlob++;
	}
#endif
	int MonoConstantOffset = MaxOffset;
	int MonoConstantOffsetBlob = MaxOffsetBlob;
#ifdef GROUP_BASE_CONSTANTS
	// always put 0.0, 0.25, 0.5, 0.75, 1.0 right in front
	MaxOffset += 10;
	MaxOffsetBlob += 10;
#endif
	for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[1].begin(); it != uniqueConstantMap[1].end(); it++)
	{
#ifdef GROUP_BASE_CONSTANTS
		// 0.0?
		if (it->first == "0x0000,0x0000") { nodeOffsets[it->second] = MonoConstantOffset; nodeOffsetsBlob[it->second] = MonoConstantOffsetBlob; }
		// 0.25?
		else if (it->first == "0x3e80,0x0000") { nodeOffsets[it->second] = MonoConstantOffset + 2; nodeOffsetsBlob[it->second] = MonoConstantOffsetBlob + 2; }
		// 0.5?
		else if (it->first == "0x3f00,0x0000") { nodeOffsets[it->second] = MonoConstantOffset + 4; nodeOffsetsBlob[it->second] = MonoConstantOffsetBlob + 4; }
		// 0.75?
		else if (it->first == "0x3f40,0x0000") { nodeOffsets[it->second] = MonoConstantOffset + 6; nodeOffsetsBlob[it->second] = MonoConstantOffsetBlob + 6; }
		// 1.0?
		else if (it->first == "0x3f80,0x0000") { nodeOffsets[it->second] = MonoConstantOffset + 8; nodeOffsetsBlob[it->second] = MonoConstantOffsetBlob + 8; }
		// rest
		else
#endif
		{
			nodeOffsets[it->second] = MaxOffset;
			MaxOffset += 2;
			nodeOffsetsBlob[it->second] = MaxOffsetBlob;			
			MaxOffsetBlob += 2;
		}
	}
	int StereoConstantOffset = MaxOffset;
	int StereoConstantOffsetBlob = MaxOffsetBlob;
	for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	{
		nodeOffsets[it->second] = MaxOffset;
		MaxOffset += 4;
		nodeOffsetsBlob[it->second] = MaxOffsetBlob;
		MaxOffsetBlob += 4;
	}

	// write export patch header file
	FILE* expFile = fopen(filename.c_str(), "w");
	std::string blobFile = filename + ".blob";
	FILE* expBlob = fopen(blobFile.c_str(), "wb");	
	
	fprintf(expFile, "#ifndef _64K2PATCH_H_\n");
	fprintf(expFile, "#define _64K2PATCH_H_\n");

	fprintf(expFile, "#ifdef INCLUDE_NODES\n\n");
	#define NID(__u, __n, __g) ((((WORD)__g) << 15) | (((WORD)__n) << 8) | ((WORD)__u))
	fprintf(expFile, "#define NID(__u, __n, __g) ((((WORD)__g) << 15) | (((WORD)__n) << 8) | ((WORD)__u))\n");
	fprintf(expFile, "#define CONST1(__h1, __l1) __l1, __h1\n");
	fprintf(expFile, "#define CONST2(__h1, __l1, __h2, __l2) __l1, __h1, __l2, __h2\n\n");

	fprintf(expFile, "enum NODE_NAMES\n");
	fprintf(expFile, "{\n");
	for (int i = 0; i < MAXIMUM_ID; i++)
	{
		int ncount = 0;
		if (groupedNodes.find(i) != groupedNodes.end())
		{
			ncount = groupedNodes[i].first.size() + groupedNodes[i].second.size(); // global + local
		}
		fprintf(expFile, "\t%s, // %d\n", NodeNames[i], ncount);
	}
	fprintf(expFile, "};\n\n");

	fprintf(expFile, "static DWORD SynthMonoConstantOffset = %d;\n", MonoConstantOffset);
	fprintf(expFile, "static DWORD SynthStereoConstantOffset = %d;\n", StereoConstantOffset);
	fprintf(expFile, "static DWORD SynthMaxOffset = %d;\n\n", MaxOffset);

	fwrite(&MonoConstantOffsetBlob, 4, 1, expBlob);
	fwrite(&StereoConstantOffsetBlob, 4, 1, expBlob);
	fwrite(&MaxOffsetBlob, 4, 1, expBlob);

	fprintf(expFile, "static WORD SynthNodes[] = \n");
	fprintf(expFile, "{\n");

	// write nodes	
	for (int i = 0; i < MAXIMUM_ID; i++)
	{
		if (groupedNodes.find(i) != groupedNodes.end())
		{
			for (int gl = 0; gl < 2; gl++)
			{
				std::vector<SynthNode*>* nlist;
				if (gl == 0)
					nlist = &(groupedNodes[i].first);
				else
					nlist = &(groupedNodes[i].second);

				for (int j = 0; j < (*nlist).size(); j++)
				{
					SynthNode* node = (*nlist)[j];
					int nid = node->id;
					// a scale is a mul
					if (nid == SCALE_ID)
						nid = MUL_ID;

					// the number of inputs depends on the node type
					// in intro mode we inline the mode constants of nodes instead of referring to mode constants
					// exception there is VOICEMANAGER because it needs 4 bytes for the mode
					// therefore we need to adjust the number of inputs for nodes with modes (via NodeMaxGUISignals)
					int numInputs;
					int preSkip = 0;
					if (nid == MULTIADD_ID || nid == NOTECONTROLLER_ID)
					{
						numInputs = node->numInputs;
					}
					else
					{
						numInputs = NodeMaxGUISignals[nid];
						// some special cases of course ...
						if (nid > CONSTANT_ID || nid == OSRAND_ID || nid == MIDISIGNAL_ID)
							numInputs = 1;
						if (nid == GMDLS_ID)
							numInputs = 0;

						// reduce adsr inputs when useADSRTriggerGate not used
						if (nid == ADSR_ID && features.useADSRTriggerGate == false)
							preSkip = 2;						
					}
									
					// remove preskip inputs from numeber of inputs
					fprintf(expFile, "\tNID(%s,%d,%d),", NodeNames[i], numInputs - preSkip, node->isGlobal);
					// no preskip for blob (always full feature)
					int blob4 = NID(i, numInputs, node->isGlobal);
					fwrite(&blob4, 2, 1, expBlob);

					for (int k = 0; k < numInputs; k++)
					{
						// get referenced node and write actual offset
						SynthNode* refNode = (SynthNode*)(node->input[k]);
						if (refNode->id != CONSTANT_ID)
						{
							// resolve remapped nodes
							while (refNode != noteRemap[refNode])
								refNode = noteRemap[refNode];
							// no export for preskipped inputs for .h export
							if (k >= preSkip)
								fprintf(expFile, "0x%04x,", nodeOffsets[refNode]);
							// export all inputs for blob
							blob4 = nodeOffsetsBlob[refNode];
							fwrite(&blob4, 2, 1, expBlob);
						}
						// search constant by value in the available constant maps
						else
						{
							int refOffset = 0;
							int refOffsetBlob = 0;

							int consttype;
							std::string conststr;							
							GetConstantValueString(refNode, conststr, consttype, NULL, blobVec);

							std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[consttype].find(conststr);
							if (it != uniqueConstantMap[consttype].end())
							{
								refOffset = nodeOffsets[it->second];
								refOffsetBlob = nodeOffsetsBlob[it->second];
							}

							if (refOffset && k >= preSkip) // no export for preskipped inputs for .h export
								fprintf(expFile, "0x%04x,", refOffset);
							else
								bool error = true;

							if (refOffsetBlob)
								fwrite(&refOffsetBlob, 2, 1, expBlob);
							else
								bool error = true;
						}
					}

					// add inline mode values if needed (see comment above)
					if (node->numInputs > numInputs)
					{
						// get referenced node
						SynthNode* refNode = (SynthNode*)(node->input[numInputs]);

						int consttype;
						std::string conststr;
						GetConstantValueString(refNode, conststr, consttype, node, blobVec);

						// write mode value
						fprintf(expFile, "%s,", conststr.c_str());
						fwrite(&blobVec[0], 2, blobVec.size(), expBlob);

						// add triggerseq pattern words inline after the mode word
						if (nid == TRIGGERSEQ_ID)
						{
							std::string tsstr;
							GetTriggerSeqValueString(node, tsstr, blobVec);
							fprintf(expFile, "%s", tsstr.c_str());
							fwrite(&blobVec[0], 2, blobVec.size(), expBlob);
						}
					}

					// add formula values in place
					if (nid == FORMULA_ID)
					{
						std::string tsstr;
						GetFormulaValueString(node, tsstr, blobVec);
						fprintf(expFile, "%s", tsstr.c_str());
						fwrite(&blobVec[0], 2, blobVec.size(), expBlob);
					}

					fprintf(expFile, " // 0x%04x\n", nodeOffsets[node]);
				}
			}
		}
	}
	fprintf(expFile, "// total node data size: %d Bytes\n", MonoConstantOffset*2);

	int constsize = 0;
#ifdef ALIGN_CONSTANTS
	fprintf(expFile, "\t");
	// add padding words
	for (int i = 0; i < ConstAlignPadding; i++)
	{
		fprintf(expFile, "0x0000,");
		constsize+=2;
	}
	fprintf(expFile, "\n// total constant padding bytes: %d\n", ConstAlignPadding*2);
	// add padding words
	for (int i = 0; i < ConstAlignPaddingBlob; i++)
	{
		WORD padzero = 0;
		fwrite(&padzero, 2, 1, expBlob);
	}
#endif
	// write mono constants
#ifdef GROUP_BASE_CONSTANTS
	DWORD bconst;
	// always put 0.0, 0.25, 0.5, 0.75, 1.0 right in front
	fprintf(expFile, "\tCONST1(0x0000,0x0000), // 0x%04x , %f\n", MonoConstantOffset, (float)(0.0f));
	bconst = 0; fwrite(&bconst, 4, 1, expBlob);
	constsize += 4;
	fprintf(expFile, "\tCONST1(0x3e80,0x0000), // 0x%04x , %f\n", MonoConstantOffset + 2, (float)(0.25f));
	bconst = 0x3e800000; fwrite(&bconst, 4, 1, expBlob);
	constsize += 4;
	fprintf(expFile, "\tCONST1(0x3f00,0x0000), // 0x%04x , %f\n", MonoConstantOffset + 4, (float)(0.5f));
	bconst = 0x3f000000; fwrite(&bconst, 4, 1, expBlob);
	constsize += 4;
	fprintf(expFile, "\tCONST1(0x3f40,0x0000), // 0x%04x , %f\n", MonoConstantOffset + 6, (float)(0.75f));
	bconst = 0x3f400000; fwrite(&bconst, 4, 1, expBlob);
	constsize += 4;
	fprintf(expFile, "\tCONST1(0x3f80,0x0000), // 0x%04x , %f\n", MonoConstantOffset + 8, (float)(1.0f));
	bconst = 0x3f800000; fwrite(&bconst, 4, 1, expBlob);
	constsize += 4;
#endif
	for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[1].begin(); it != uniqueConstantMap[1].end(); it++)
	{
#ifdef GROUP_BASE_CONSTANTS
		// skip the numbers already written above
		if ((it->first == "0x0000,0x0000") || (it->first == "0x3e80,0x0000") || (it->first == "0x3f00,0x0000") || (it->first == "0x3f40,0x0000") || (it->first == "0x3f80,0x0000"))
			continue;
#endif
		int consttype;
		std::string conststr;
		GetConstantValueString(it->second, conststr, consttype, NULL, blobVec);
		fprintf(expFile, "\tCONST1(%s), // 0x%04x , %f\n", conststr.c_str(), nodeOffsets[it->second], (float)(it->second->out.d[0]));
		fwrite(&blobVec[0], 2, blobVec.size(), expBlob);
		constsize += 4;
	}
	// write stereo constants
	for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	{
		int consttype;
		std::string conststr;
		GetConstantValueString(it->second, conststr, consttype, NULL, blobVec);
		fprintf(expFile, "\tCONST2(%s), // 0x%04x , %f , %f\n", conststr.c_str(), nodeOffsets[it->second], (float)(it->second->out.d[0]), (float)(it->second->out.d[1]));
		fwrite(&blobVec[0], 2, blobVec.size(), expBlob);
		constsize += 8;
	}


	//// write mono constants (3xBYTEs)
	//fprintf(expFile, "// mono constants\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[1].begin(); it != uniqueConstantMap[1].end(); it++)
	//{
	//	float fv = it->second->out.d[0];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 0) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[1].begin(); it != uniqueConstantMap[1].end(); it++)
	//{
	//	float fv = it->second->out.d[0];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 8) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[1].begin(); it != uniqueConstantMap[1].end(); it++)
	//{
	//	float fv = it->second->out.d[0];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 16) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n");

	//// write stereo constants (2x 3xBYTEs)
	//fprintf(expFile, "// stereo constants\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	//{
	//	float fv = it->second->out.d[0];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 0) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	//{
	//	float fv = it->second->out.d[0];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 8) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	//{
	//	float fv = it->second->out.d[0];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 16) & 0xff);
	//	constsize++;
	//}
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	//{
	//	float fv = it->second->out.d[1];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 0) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	//{
	//	float fv = it->second->out.d[1];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 8) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n\t");
	//for (std::map<std::string, SynthNode*>::iterator it = uniqueConstantMap[2].begin(); it != uniqueConstantMap[2].end(); it++)
	//{
	//	float fv = it->second->out.d[1];
	//	DWORD iv = *((DWORD*)(&fv));
	//	fprintf(expFile, "0x%02x,", (iv >> 16) & 0xff);
	//	constsize++;
	//}
	//fprintf(expFile, "\n");

	fprintf(expFile, "// total constants data size: %d Bytes\n", constsize);
	
	// store arpeggiator sequences if available
	int arpTotalSize = 0;
	if (features.useARP)
	{
		fprintf(expFile, "\t// arpggiator sequences\n");
		fprintf(expFile, "\t0x%04x,\n", features.uniqueArpSequenceList.size());
		WORD arpblob = features.uniqueArpSequenceList.size();
		fwrite(&arpblob, 2, 1, expBlob);
		arpTotalSize += 2;
		for (int i = 0; i < features.uniqueArpSequenceList.size(); i++)
		{
			fprintf(expFile, "\t%s\n", features.uniqueArpSequenceList[i].c_str());
			arpTotalSize += 33 * 2;
		}
		fwrite(&features.uniqueArpSequenceBlobList[0], 2, features.uniqueArpSequenceBlobList.size(), expBlob);
		fprintf(expFile, "// total arp sequence data size: %d Bytes\n", arpTotalSize);
	}
	else
	{
		// only binary blob needs to write count, header export will ifdef this out in player
		WORD arpblob = 0;
		fwrite(&arpblob, 2, 1, expBlob);
	}

	// store special data for global nodes (e.g. arpeggiator steps, sapi text) if available
	int specialTotalSize = 0;
	if (features.useSpecialData)
	{	
		std::list<std::pair<void*, SynthNode*> > specialDataList;
		int specialDataCount = 0;
		// loop all special data types
		for (std::map<int, std::map<void*, SynthNode*> >::iterator itg = features.specialDataMap.begin(); itg != features.specialDataMap.end(); itg++)
		{
			// sort each type by offset of referenced node
			std::map<int, std::pair<void*, SynthNode*> > sortedSpecialData;
			for (std::map<void*, SynthNode*>::iterator it = itg->second.begin(); it != itg->second.end(); it++)
			{
				sortedSpecialData[nodeOffsets[it->second]] = std::make_pair(it->first, it->second);
				specialDataCount++;
			}
			// insert into the one list
			for (std::map<int, std::pair<void*, SynthNode*> >::iterator it = sortedSpecialData.begin(); it != sortedSpecialData.end(); it++)
			{
				specialDataList.push_back(it->second);
			}
		}
		fprintf(expFile, "\t// special data\n");
		fprintf(expFile, "\t0x%04x,\n", specialDataCount);
		fwrite(&specialDataCount, 2, 1, expBlob);
		specialTotalSize += 2;		
		for (std::list<std::pair<void*, SynthNode*> >::iterator it = specialDataList.begin(); it != specialDataList.end(); it++)
		{
			if (it->second->id == SAPI_ID)
				fprintf(expFile, "\t// SAPI text\n");
			
			// write the node which this special data belongs to
			fprintf(expFile, "\t0x%04x, ", nodeOffsets[it->second]);
			WORD snoffs = nodeOffsetsBlob[it->second];
			fwrite(&snoffs, 2, 1, expBlob);
			specialTotalSize += 2;

			// get the data to store
			char* storeData = "";
			if (it->first)
				storeData = (char*)(it->first);
				
			DWORD datalength;
			// for sapi its the string length plus terminating 0, will be binarized and stored below
			if (it->second->id == SAPI_ID)
				datalength = strlen(storeData) + 1;

			// write length of data to come in bytes
			DWORD actlenght = datalength;
			if (actlenght & 1)
				actlenght++;
			fprintf(expFile, "0x%04x, ", actlenght);
			fwrite(&actlenght, 2, 1, expBlob);
			specialTotalSize += 2;

			// convert to hex word string and write
			WORD* buf = (WORD*)storeData;
			for (int j = 0; j < datalength / 2; j++)
			{
				fprintf(expFile, "0x%04x,", *buf);
				fwrite(buf, 2, 1, expBlob);
				specialTotalSize += 2;
				buf++;
			}
			// write an addtional word if datalength is odd, so lowbyte contains the last data byte
			if (datalength & 1)
			{
				WORD padding = 0;
				padding += *((BYTE*)(buf));
				fprintf(expFile, "0x%04x,", padding);
				fwrite(&padding, 2, 1, expBlob);
				specialTotalSize += 2;
			}
			fprintf(expFile, "\n");
			
		}
		fprintf(expFile, "// total special data size: %d Bytes\n", specialTotalSize);
	}
	else
	{
		// only binary blob needs to write count, header export will ifdef this out in player
		WORD sdblob = 0;
		fwrite(&sdblob, 2, 1, expBlob);
	}

	// store compressed samples if available
	int compTotalSize = 0;
	if (features.useStoredSamples)
	{		
		fprintf(expFile, "\t// compressed samples\n");
		std::map<int, int> writtenTables;

		for (int nt = 0; nt < 2; nt++)
		{
			int type = SAMPLER_ID;
			if (nt == 1)
				type = WTFOSC_ID;
			if (groupedNodes.find(type) == groupedNodes.end()) // skip if neither SAMPLER or WTFOSC are used
				continue;
			for (int gl = 0; gl < 2; gl++)
			{
				std::vector<SynthNode*>* nlist;
				if (gl == 0)
					nlist = &(groupedNodes[type].first);
				else
					nlist = &(groupedNodes[type].second);

				for (int i = 0; i < (*nlist).size(); i++)
				{
					int table;
					bool stored = false;
					if (type == SAMPLER_ID)
					{
						stored = ((*nlist)[i]->input[SAMPLER_MODE]->i[0] & SAMPLER_MODEMASK) == SAMPLER_STORED;
						table = ((*nlist)[i]->input[SAMPLER_MODE]->i[0] & SAMPLER_SRCMASK) >> SAMPLER_SRCSHIFT;						
					}
					if (type == WTFOSC_ID)
					{
						stored = ((*nlist)[i]->input[WTFOSC_MODE]->i[0] & WTFOSC_MODEMASK) == WTFOSC_STORED;
						table = ((*nlist)[i]->input[WTFOSC_MODE]->i[0] & WTFOSC_SRCMASK) >> WTFOSC_SRCSHIFT;						
					}

					// write table only once, and only if the mode is actually set to stored samples
					if (stored && (writtenTables.find(table) == writtenTables.end()))
					{
						fprintf(expFile, "\t// %s\n", SynthGlobalState.RawWaveFileName[table].c_str());
						fprintf(expFile, "\t0x%04x, ", table);
						fwrite(&table, 2, 1, expBlob);
						compTotalSize += 2;
						int csr = SynthGlobalState.CompSampleRate[table];
						fprintf(expFile, "0x%04x,0x%04x, ", (csr >> 0) & 0xffff, (csr >> 16) & 0xffff);
						fwrite(&csr, 4, 1, expBlob);
						compTotalSize += 4;
						int avg = SynthGlobalState.CompAvgBytes[table];
						fprintf(expFile, "0x%04x,0x%04x, ", (avg >> 0) & 0xffff, (avg >> 16) & 0xffff);
						fwrite(&avg, 4, 1, expBlob);
						compTotalSize += 4;
						int wts = SynthGlobalState.CompWaveTableSize[table];
						fprintf(expFile, "0x%04x,0x%04x,\n\t", (wts >> 0) & 0xffff, (wts >> 16) & 0xffff);
						fwrite(&wts, 4, 1, expBlob);
						compTotalSize += 4;
						WORD* buf = (WORD*)SynthGlobalState.CompWaveTable[table];
						for (int j = 0; j < wts / 2; j++)
						{
							fprintf(expFile, "0x%04x,", *buf);
							fwrite(buf, 2, 1, expBlob);
							compTotalSize += 2;
							buf++;
						}
						// write an addtional word if wts is odd, so lowbyte contains the last data byte
						if (wts & 1)
						{
							WORD padding = 0;
							padding += *((BYTE*)(buf));
							fprintf(expFile, "0x%04x,", padding);
							fwrite(&padding, 2, 1, expBlob);
							compTotalSize += 2;
						}

						writtenTables[table] = 1;
						fprintf(expFile, "\n");
					}
				}
			}
		}
		fprintf(expFile, "// total compressed waves data size: %d Bytes\n", compTotalSize);
	}

	// mark end of stream
	fprintf(expFile, "\t0x%04x\n", 0xdead);
	WORD eos = 0xdead;
	fwrite(&eos, 2, 1, expBlob);

	fprintf(expFile, "};\n");
	fprintf(expFile, "// total patch data size: %d Bytes\n\n", MaxOffset * 2 + arpTotalSize + specialTotalSize + compTotalSize + 2);

	fprintf(expFile, "#endif // INCLUDE_NODES\n\n");

	// write the skip defines for the unused nodes
	fprintf(expFile, "// unused nodes\n");
	for (int i = 0; i < MAXIMUM_ID; i++)
	{
		if ((i != CONSTANT_ID) && (groupedNodes.find(i) == groupedNodes.end()))
		{
			fprintf(expFile, "#define %s_SKIP\n", NodeNames[i]);
		}		
	}
	// additional defines from features
	if (!features.useSpecialData)
		fprintf(expFile, "#define SPECIALDATA_SKIP\n");
	if (!features.useStoredSamples)
		fprintf(expFile, "#define STOREDSAMPLES_SKIP\n");
	if (!features.useAftertouch)
		fprintf(expFile, "#define SONG_AFTERTOUCH_SKIP\n");
	if (!features.useARP)
		fprintf(expFile, "#define VOICEMANAGER_ARP_SKIP\n");
	if (!features.useGlide)
		fprintf(expFile, "#define VOICEMANAGER_GLIDE_SKIP\n");	
	if (!features.useADSRTriggerGate)
		fprintf(expFile, "#define ADSR_SKIP_TRIGGER_GATE\n");
	if (!features.useADSRDBGain)
		fprintf(expFile, "#define ADSR_SKIP_DBGAIN\n");
	if (!features.useADSRExp)
		fprintf(expFile, "#define ADSR_SKIP_EXP\n");
	if (!features.useLFOPolarity)
		fprintf(expFile, "#define LFO_SKIP_POLARITY\n");
	if (!features.useOSCFrequency)
		fprintf(expFile, "#define OSCILLATOR_SKIP_FREQUENCY\n");
	if (!features.useWaveSINE)
		fprintf(expFile, "#define WAVE_SKIP_SINE\n");
	if (!features.useWaveSAW)
		fprintf(expFile, "#define WAVE_SKIP_SAW\n");
	if (!features.useWavePULSE)
		fprintf(expFile, "#define WAVE_SKIP_PULSE\n");
	if (!features.useWaveAATRISAW)
		fprintf(expFile, "#define WAVE_SKIP_AATRISAW\n");
	if (!features.useWaveAAPULSE)
		fprintf(expFile, "#define WAVE_SKIP_AAPULSE\n");
	if (!features.useWaveAABITRISAW)
		fprintf(expFile, "#define WAVE_SKIP_AABITRISAW\n");
	if (!features.useWaveAABIPULSE)
		fprintf(expFile, "#define WAVE_SKIP_AABIPULSE\n");
	if (!features.useWaveRPSINE)
		fprintf(expFile, "#define WAVE_SKIP_RPSINE\n");
	if (!features.useWavePNOISE)
		fprintf(expFile, "#define WAVE_SKIP_PNOISE\n");
	if (!features.useWaveAATRISAWSEQ)
		fprintf(expFile, "#define WAVE_SKIP_AATRISAWSEQ\n");
	if (!features.useWaveSINESEQ)
		fprintf(expFile, "#define WAVE_SKIP_SINESEQ\n");
	if (!features.useNGBrown)
		fprintf(expFile, "#define NOISEGEN_SKIP_BROWN\n");	
	if (!features.useFrequencyMap)
		fprintf(expFile, "#define FREQUENCY_MAP_SKIP\n");
	if (!features.useBQFLP)
		fprintf(expFile, "#define BQFILTER_SKIP_LOWPASS\n");
	if (!features.useBQFHP)
		fprintf(expFile, "#define BQFILTER_SKIP_HIGHPASS\n");
	if (!features.useBQFBP)
		fprintf(expFile, "#define BQFILTER_SKIP_BANDPASS\n");
	if (!features.useBQFBPBW)
		fprintf(expFile, "#define BQFILTER_SKIP_BANDPASSBW\n");
	if (!features.useBQFNOTCH)
		fprintf(expFile, "#define BQFILTER_SKIP_NOTCH\n");
	if (!features.useBQFAP)
		fprintf(expFile, "#define BQFILTER_SKIP_ALLPASS\n");
	if (!features.useBQFPEAK)
		fprintf(expFile, "#define BQFILTER_SKIP_PEAK\n");
	if (!features.useBQFLSHELF)
		fprintf(expFile, "#define BQFILTER_SKIP_LOWSHELF\n");
	if (!features.useBQFHSHELF)
		fprintf(expFile, "#define BQFILTER_SKIP_HIGHSHELF\n");
	if (!features.useBQFRES)
		fprintf(expFile, "#define BQFILTER_SKIP_RESONANCE\n");
	if (!features.useBQFRESN)
		fprintf(expFile, "#define BQFILTER_SKIP_RESONANCENORM\n");
	if (!features.useDSTOD)
		fprintf(expFile, "#define DISTORTION_SKIP_OVERDRIVE\n");
	if (!features.useDSTDAMP)
		fprintf(expFile, "#define DISTORTION_SKIP_DAMP\n");
	if (!features.useDSTQUANT)
		fprintf(expFile, "#define DISTORTION_SKIP_QUANT\n");
	if (!features.useDSTLIMIT)
		fprintf(expFile, "#define DISTORTION_SKIP_LIMIT\n");
	if (!features.useDSTASYM)
		fprintf(expFile, "#define DISTORTION_SKIP_ASYM\n");	
	if (!features.useDSTSIN)
		fprintf(expFile, "#define DISTORTION_SKIP_SIN\n");	
	if (!features.useDLAP)
		fprintf(expFile, "#define DELAY_SKIP_ALLPASS\n");
	if (!features.useDLBPM)
		fprintf(expFile, "#define DELAY_SKIP_BPMSYNC\n");
	if (!features.useDLS)
		fprintf(expFile, "#define DELAY_SKIP_SHORT\n");
	if (!features.useDLM)
		fprintf(expFile, "#define DELAY_SKIP_MIDDLE\n");
	if (!features.useDLL)
		fprintf(expFile, "#define DELAY_SKIP_LONG\n");
	if (!features.useDLNM)
		fprintf(expFile, "#define DELAY_SKIP_NOTEMAP\n");
	if (!features.useDLNM2)
		fprintf(expFile, "#define DELAY_SKIP_NOTEMAP2\n");
	if (!features.useSMPLoop)
		fprintf(expFile, "#define SAMPLER_SKIP_LOOP\n");
	if (!features.useWTFReduceClick)
		fprintf(expFile, "#define WTFOSC_SKIP_REDUCECLICK\n");	
	if (!features.useGLTS)
		fprintf(expFile, "#define GLITCH_SKIP_TAPESTOP\n");
	if (!features.useGLRT)
		fprintf(expFile, "#define GLITCH_SKIP_RETRIGGER\n");
	if (!features.useGLSH)
		fprintf(expFile, "#define GLITCH_SKIP_SHUFFLE\n");
	if (!features.useGLREV)
		fprintf(expFile, "#define GLITCH_SKIP_REVERSE\n");
	if (!features.useSNHSM)
		fprintf(expFile, "#define SNH_SKIP_SNHSMOOTH\n");

	// for formula just write out the used op ids in a comment
	if (groupedNodes.find(FORMULA_ID) != groupedNodes.end())
	{
		std::map<int, int> ops;
		for (int gl = 0; gl < 2; gl++)
		{
			std::vector<SynthNode*>* nlist;
			if (gl == 0)
				nlist = &(groupedNodes[FORMULA_ID].first);
			else
				nlist = &(groupedNodes[FORMULA_ID].second);

			for (int i = 0; i < (*nlist).size(); i++)
			{
				BYTE* cmdbuf = (BYTE*)(SynthGlobalState.SpecialDataPointer[(*nlist)[i]->valueOffset]);
				BYTE* command = cmdbuf;
				while (*command != FORMULA_DONE)
				{
					ops[*command] = 1;
					if (*command == FORMULA_CMD_CONSTANT)
						command += 4;
					else
						command++;
				}
			}
		}	
		fprintf(expFile, "// Used Formula OPs: ");
		for (std::map<int, int>::iterator it = ops.begin(); it != ops.end(); it++)
		{
			fprintf(expFile, "%2d, ", it->first);
		}
		fprintf(expFile, "\n");
	}
		
	fprintf(expFile, "#endif // _64K2PATCH_H_\n");

	fclose(expFile);
	fclose(expBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// command forwarding to core
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::noteOn(int channel, int note, int velocity)		
{ 
	// no synth processing in recording mode, just record incoming events
	if (isRecording())
		Recorder.AddNoteOn(channel, note, velocity);
	else
		_64klang_NoteOn(channel, note, velocity); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::noteOff(int channel, int note, int velocity)		
{
	// no synth processing in recording mode, just record incoming events
	if (isRecording())
		Recorder.AddNoteOff(channel, note);
	else
		_64klang_NoteOff(channel, note, velocity);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::noteAftertouch(int channel, int note, int pressure)		
{
	// no synth processing in recording mode, just record incoming events
	if (isRecording())
		Recorder.AddNoteAftertouch(channel, note, pressure);
	else
		_64klang_NoteAftertouch(channel, note, pressure);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthController::midiSignal(int channel, int value, int cc)			
{
	// no synth processing in recording mode, just record incoming events
	if (isRecording())
		Recorder.AddCC(channel, value, cc);
	else
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
	// no synth processing in recording mode, just record incoming events
	if (isRecording())
	{
		Recorder.AddSamples(samples);
		// send a stayalive signal to the host (some sawtooth sound)
		for (int i = 0; i < samples; i++)
		{
			float signal = 0.03125*((float)(i & 255) / 128.0f - 1.0f);
			*left++ = signal;
			*right++ = signal;
		}
	}
	else
	{
		_64klang_Tick(left, right, samples);
		DeferredSynthFree();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPU check for support of SSE4.1
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthController::isInitialized()
{
	return _initialized;
}

const char* szFeatures[] =
{
	"x87 FPU On Chip",
	"Virtual-8086 Mode Enhancement",
	"Debugging Extensions",
	"Page Size Extensions",
	"Time Stamp Counter",
	"RDMSR and WRMSR Support",
	"Physical Address Extensions",
	"Machine Check Exception",
	"CMPXCHG8B Instruction",
	"APIC On Chip",
	"Unknown1",
	"SYSENTER and SYSEXIT",
	"Memory Type Range Registers",
	"PTE Global Bit",
	"Machine Check Architecture",
	"Conditional Move/Compare Instruction",
	"Page Attribute Table",
	"36-bit Page Size Extension",
	"Processor Serial Number",
	"CFLUSH Extension",
	"Unknown2",
	"Debug Store",
	"Thermal Monitor and Clock Ctrl",
	"MMX Technology",
	"FXSAVE/FXRSTOR",
	"SSE Extensions",
	"SSE2 Extensions",
	"Self Snoop",
	"Multithreading Technology",
	"Thermal Monitor",
	"Unknown4",
	"Pending Break Enable"
};

bool SynthController::checkCPUSupport()
{
	char CPUString[0x20];
	char CPUBrandString[0x40];
	int CPUInfo[4] = {-1};
	int nSteppingID = 0;
	int nModel = 0;
	int nFamily = 0;
	int nProcessorType = 0;
	int nExtendedmodel = 0;
	int nExtendedfamily = 0;
	int nBrandIndex = 0;
	int nCLFLUSHcachelinesize = 0;
	int nLogicalProcessors = 0;
	int nAPICPhysicalID = 0;
	int nFeatureInfo = 0;
	int nCacheLineSize = 0;
	int nL2Associativity = 0;
	int nCacheSizeK = 0;
	int nPhysicalAddress = 0;
	int nVirtualAddress = 0;
	int nRet = 0;

	int nCores = 0;
	int nCacheType = 0;
	int nCacheLevel = 0;
	int nMaxThread = 0;
	int nSysLineSize = 0;
	int nPhysicalLinePartitions = 0;
	int nWaysAssociativity = 0;
	int nNumberSets = 0;

	unsigned    nIds, nExIds, i;

	bool    bSSE3Instructions = false;
	bool    bMONITOR_MWAIT = false;
	bool    bCPLQualifiedDebugStore = false;
	bool    bVirtualMachineExtensions = false;
	bool    bEnhancedIntelSpeedStepTechnology = false;
	bool    bThermalMonitor2 = false;
	bool    bSupplementalSSE3 = false;
	bool    bL1ContextID = false;
	bool    bCMPXCHG16B = false;
	bool    bxTPRUpdateControl = false;
	bool    bPerfDebugCapabilityMSR = false;
	bool    bSSE41Extensions = false;
	bool    bSSE42Extensions = false;
	bool    bPOPCNT = false;

	bool    bMultithreading = false;

	bool    bLAHF_SAHFAvailable = false;
	bool    bCmpLegacy = false;
	bool    bSVM = false;
	bool    bExtApicSpace = false;
	bool    bAltMovCr8 = false;
	bool    bLZCNT = false;
	bool    bSSE4A = false;
	bool    bMisalignedSSE = false;
	bool    bPREFETCH = false;
	bool    bSKINITandDEV = false;
	bool    bSYSCALL_SYSRETAvailable = false;
	bool    bExecuteDisableBitAvailable = false;
	bool    bMMXExtensions = false;
	bool    bFFXSR = false;
	bool    b1GBSupport = false;
	bool    bRDTSCP = false;
	bool    b64Available = false;
	bool    b3DNowExt = false;
	bool    b3DNow = false;
	bool    bNestedPaging = false;
	bool    bLBRVisualization = false;
	bool    bFP128 = false;
	bool    bMOVOptimization = false;

	bool    bSelfInit = false;
	bool    bFullyAssociative = false;

	// __cpuid with an InfoType argument of 0 returns the number of
	// valid Ids in CPUInfo[0] and the CPU identification string in
	// the other three array elements. The CPU identification string is
	// not in linear order. The code below arranges the information 
	// in a human readable form.
	__cpuid(CPUInfo, 0);
	nIds = CPUInfo[0];
	memset(CPUString, 0, sizeof(CPUString));
	*((int*)CPUString) = CPUInfo[1];
	*((int*)(CPUString+4)) = CPUInfo[3];
	*((int*)(CPUString+8)) = CPUInfo[2];

	// Get the information associated with each valid Id
	for (i=0; i<=nIds; ++i)
	{
		__cpuid(CPUInfo, i);
		// Interpret CPU feature information.
		if  (i == 1)
		{
			nSteppingID = CPUInfo[0] & 0xf;
			nModel = (CPUInfo[0] >> 4) & 0xf;
			nFamily = (CPUInfo[0] >> 8) & 0xf;
			nProcessorType = (CPUInfo[0] >> 12) & 0x3;
			nExtendedmodel = (CPUInfo[0] >> 16) & 0xf;
			nExtendedfamily = (CPUInfo[0] >> 20) & 0xff;
			nBrandIndex = CPUInfo[1] & 0xff;
			nCLFLUSHcachelinesize = ((CPUInfo[1] >> 8) & 0xff) * 8;
			nLogicalProcessors = ((CPUInfo[1] >> 16) & 0xff);
			nAPICPhysicalID = (CPUInfo[1] >> 24) & 0xff;
			bSSE3Instructions = (CPUInfo[2] & 0x1) || false;
			bMONITOR_MWAIT = (CPUInfo[2] & 0x8) || false;
			bCPLQualifiedDebugStore = (CPUInfo[2] & 0x10) || false;
			bVirtualMachineExtensions = (CPUInfo[2] & 0x20) || false;
			bEnhancedIntelSpeedStepTechnology = (CPUInfo[2] & 0x80) || false;
			bThermalMonitor2 = (CPUInfo[2] & 0x100) || false;
			bSupplementalSSE3 = (CPUInfo[2] & 0x200) || false;
			bL1ContextID = (CPUInfo[2] & 0x300) || false;
			bCMPXCHG16B= (CPUInfo[2] & 0x2000) || false;
			bxTPRUpdateControl = (CPUInfo[2] & 0x4000) || false;
			bPerfDebugCapabilityMSR = (CPUInfo[2] & 0x8000) || false;
			bSSE41Extensions = (CPUInfo[2] & 0x80000) || false;
			bSSE42Extensions = (CPUInfo[2] & 0x100000) || false;
			bPOPCNT= (CPUInfo[2] & 0x800000) || false;
			nFeatureInfo = CPUInfo[3];
			bMultithreading = (nFeatureInfo & (1 << 28)) || false;
		}
	}

	// Calling __cpuid with 0x80000000 as the InfoType argument
	// gets the number of valid extended IDs.
	__cpuid(CPUInfo, 0x80000000);
	nExIds = CPUInfo[0];
	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	// Get the information associated with each extended ID.
	for (i=0x80000000; i<=nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		if  (i == 0x80000001)
		{
			bLAHF_SAHFAvailable = (CPUInfo[2] & 0x1) || false;
			bCmpLegacy = (CPUInfo[2] & 0x2) || false;
			bSVM = (CPUInfo[2] & 0x4) || false;
			bExtApicSpace = (CPUInfo[2] & 0x8) || false;
			bAltMovCr8 = (CPUInfo[2] & 0x10) || false;
			bLZCNT = (CPUInfo[2] & 0x20) || false;
			bSSE4A = (CPUInfo[2] & 0x40) || false;
			bMisalignedSSE = (CPUInfo[2] & 0x80) || false;
			bPREFETCH = (CPUInfo[2] & 0x100) || false;
			bSKINITandDEV = (CPUInfo[2] & 0x1000) || false;
			bSYSCALL_SYSRETAvailable = (CPUInfo[3] & 0x800) || false;
			bExecuteDisableBitAvailable = (CPUInfo[3] & 0x10000) || false;
			bMMXExtensions = (CPUInfo[3] & 0x40000) || false;
			bFFXSR = (CPUInfo[3] & 0x200000) || false;
			b1GBSupport = (CPUInfo[3] & 0x400000) || false;
			bRDTSCP = (CPUInfo[3] & 0x8000000) || false;
			b64Available = (CPUInfo[3] & 0x20000000) || false;
			b3DNowExt = (CPUInfo[3] & 0x40000000) || false;
			b3DNow = (CPUInfo[3] & 0x80000000) || false;
		}

		// Interpret CPU brand string and cache information.
		if  (i == 0x80000002)
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000003)
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000004)
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000006)
		{
			nCacheLineSize = CPUInfo[2] & 0xff;
			nL2Associativity = (CPUInfo[2] >> 12) & 0xf;
			nCacheSizeK = (CPUInfo[2] >> 16) & 0xffff;
		}
		else if  (i == 0x80000008)
		{
		   nPhysicalAddress = CPUInfo[0] & 0xff;
		   nVirtualAddress = (CPUInfo[0] >> 8) & 0xff;
		}
		else if  (i == 0x8000000A)
		{
			bNestedPaging = (CPUInfo[3] & 0x1) || false;
			bLBRVisualization = (CPUInfo[3] & 0x2) || false;
		}
		else if  (i == 0x8000001A)
		{
			bFP128 = (CPUInfo[0] & 0x1) || false;
			bMOVOptimization = (CPUInfo[0] & 0x2) || false;
		}
	}

	// Display all the information in user-friendly format.
#if 0
	printf_s("\n\nCPU String: %s\n", CPUString);

	if  (nIds >= 1)
	{
		if  (nSteppingID)
			printf_s("Stepping ID = %d\n", nSteppingID);
		if  (nModel)
			printf_s("Model = %d\n", nModel);
		if  (nFamily)
			printf_s("Family = %d\n", nFamily);
		if  (nProcessorType)
			printf_s("Processor Type = %d\n", nProcessorType);
		if  (nExtendedmodel)
			printf_s("Extended model = %d\n", nExtendedmodel);
		if  (nExtendedfamily)
			printf_s("Extended family = %d\n", nExtendedfamily);
		if  (nBrandIndex)
			printf_s("Brand Index = %d\n", nBrandIndex);
		if  (nCLFLUSHcachelinesize)
			printf_s("CLFLUSH cache line size = %d\n",
					 nCLFLUSHcachelinesize);
		if (bMultithreading && (nLogicalProcessors > 0))
		   printf_s("Logical Processor Count = %d\n", nLogicalProcessors);
		if  (nAPICPhysicalID)
			printf_s("APIC Physical ID = %d\n", nAPICPhysicalID);

		if  (nFeatureInfo || bSSE3Instructions ||
			 bMONITOR_MWAIT || bCPLQualifiedDebugStore ||
			 bVirtualMachineExtensions || bEnhancedIntelSpeedStepTechnology ||
			 bThermalMonitor2 || bSupplementalSSE3 || bL1ContextID || 
			 bCMPXCHG16B || bxTPRUpdateControl || bPerfDebugCapabilityMSR || 
			 bSSE41Extensions || bSSE42Extensions || bPOPCNT || 
			 bLAHF_SAHFAvailable || bCmpLegacy || bSVM ||
			 bExtApicSpace || bAltMovCr8 ||
			 bLZCNT || bSSE4A || bMisalignedSSE ||
			 bPREFETCH || bSKINITandDEV || bSYSCALL_SYSRETAvailable || 
			 bExecuteDisableBitAvailable || bMMXExtensions || bFFXSR || b1GBSupport ||
			 bRDTSCP || b64Available || b3DNowExt || b3DNow || bNestedPaging || 
			 bLBRVisualization || bFP128 || bMOVOptimization )
		{
			printf_s("\nThe following features are supported:\n");

			if  (bSSE3Instructions)
				printf_s("\tSSE3\n");
			if  (bMONITOR_MWAIT)
				printf_s("\tMONITOR/MWAIT\n");
			if  (bCPLQualifiedDebugStore)
				printf_s("\tCPL Qualified Debug Store\n");
			if  (bVirtualMachineExtensions)
				printf_s("\tVirtual Machine Extensions\n");
			if  (bEnhancedIntelSpeedStepTechnology)
				printf_s("\tEnhanced Intel SpeedStep Technology\n");
			if  (bThermalMonitor2)
				printf_s("\tThermal Monitor 2\n");
			if  (bSupplementalSSE3)
				printf_s("\tSupplemental Streaming SIMD Extensions 3\n");
			if  (bL1ContextID)
				printf_s("\tL1 Context ID\n");
			if  (bCMPXCHG16B)
				printf_s("\tCMPXCHG16B Instruction\n");
			if  (bxTPRUpdateControl)
				printf_s("\txTPR Update Control\n");
			if  (bPerfDebugCapabilityMSR)
				printf_s("\tPerf\\Debug Capability MSR\n");
			if  (bSSE41Extensions)
				printf_s("\tSSE4.1 Extensions\n");
			if  (bSSE42Extensions)
				printf_s("\tSSE4.2 Extensions\n");
			if  (bPOPCNT)
				printf_s("\tPPOPCNT Instruction\n");

			i = 0;
			nIds = 1;
			while (i < (sizeof(szFeatures)/sizeof(const char*)))
			{
				if  (nFeatureInfo & nIds)
				{
					printf_s("\t");
					printf_s(szFeatures[i]);
					printf_s("\n");
				}

				nIds <<= 1;
				++i;
			}
			if (bLAHF_SAHFAvailable)
				printf_s("\tLAHF/SAHF in 64-bit mode\n");
			if (bCmpLegacy)
				printf_s("\tCore multi-processing legacy mode\n");
			if (bSVM)
				printf_s("\tSecure Virtual Machine\n");
			if (bExtApicSpace)
				printf_s("\tExtended APIC Register Space\n");
			if (bAltMovCr8)
				printf_s("\tAltMovCr8\n");
			if (bLZCNT)
				printf_s("\tLZCNT instruction\n");
			if (bSSE4A)
				printf_s("\tSSE4A (EXTRQ, INSERTQ, MOVNTSD, MOVNTSS)\n");
			if (bMisalignedSSE)
				printf_s("\tMisaligned SSE mode\n");
			if (bPREFETCH)
				printf_s("\tPREFETCH and PREFETCHW Instructions\n");
			if (bSKINITandDEV)
				printf_s("\tSKINIT and DEV support\n");
			if (bSYSCALL_SYSRETAvailable)
				printf_s("\tSYSCALL/SYSRET in 64-bit mode\n");
			if (bExecuteDisableBitAvailable)
				printf_s("\tExecute Disable Bit\n");
			if (bMMXExtensions)
				printf_s("\tExtensions to MMX Instructions\n");
			if (bFFXSR)
				printf_s("\tFFXSR\n");
			if (b1GBSupport)
				printf_s("\t1GB page support\n");
			if (bRDTSCP)
				printf_s("\tRDTSCP instruction\n");
			if (b64Available)
				printf_s("\t64 bit Technology\n");
			if (b3DNowExt)
				printf_s("\t3Dnow Ext\n");
			if (b3DNow)
				printf_s("\t3Dnow! instructions\n");
			if (bNestedPaging)
				printf_s("\tNested Paging\n");
			if (bLBRVisualization)
				printf_s("\tLBR Visualization\n");
			if (bFP128)
				printf_s("\tFP128 optimization\n");
			if (bMOVOptimization)
				printf_s("\tMOVU Optimization\n");
		}
	}

	if  (nExIds >= 0x80000004)
		printf_s("\nCPU Brand String: %s\n", CPUBrandString);

	if  (nExIds >= 0x80000006)
	{
		printf_s("Cache Line Size = %d\n", nCacheLineSize);
		printf_s("L2 Associativity = %d\n", nL2Associativity);
		printf_s("Cache Size = %dK\n", nCacheSizeK);
	}


	for (i=0;;i++)
	{
		__cpuidex(CPUInfo, 0x4, i);
		if(!(CPUInfo[0] & 0xf0)) break;

		if(i == 0)
		{
			nCores = CPUInfo[0] >> 26;
			printf_s("\n\nNumber of Cores = %d\n", nCores + 1);
		}

		nCacheType = (CPUInfo[0] & 0x1f);
		nCacheLevel = (CPUInfo[0] & 0xe0) >> 5;
		bSelfInit = (CPUInfo[0] & 0x100) >> 8;
		bFullyAssociative = (CPUInfo[0] & 0x200) >> 9;
		nMaxThread = (CPUInfo[0] & 0x03ffc000) >> 14;
		nSysLineSize = (CPUInfo[1] & 0x0fff);
		nPhysicalLinePartitions = (CPUInfo[1] & 0x03ff000) >> 12;
		nWaysAssociativity = (CPUInfo[1]) >> 22;
		nNumberSets = CPUInfo[2];

		printf_s("\n");

		printf_s("ECX Index %d\n", i);
		switch (nCacheType)
		{
			case 0:
				printf_s("   Type: Null\n");
				break;
			case 1:
				printf_s("   Type: Data Cache\n");
				break;
			case 2:
				printf_s("   Type: Instruction Cache\n");
				break;
			case 3:
				printf_s("   Type: Unified Cache\n");
				break;
			default:
				 printf_s("   Type: Unknown\n");
		}

		printf_s("   Level = %d\n", nCacheLevel + 1); 
		if (bSelfInit)
		{
			printf_s("   Self Initializing\n");
		}
		else
		{
			printf_s("   Not Self Initializing\n");
		}
		if (bFullyAssociative)
		{
			printf_s("   Is Fully Associatve\n");
		}
		else
		{
			printf_s("   Is Not Fully Associatve\n");
		}
		printf_s("   Max Threads = %d\n", 
			nMaxThread+1);
		printf_s("   System Line Size = %d\n", 
			nSysLineSize+1);
		printf_s("   Physical Line Partions = %d\n", 
			nPhysicalLinePartitions+1);
		printf_s("   Ways of Associativity = %d\n", 
			nWaysAssociativity+1);
		printf_s("   Number of Sets = %d\n", 
			nNumberSets+1);
	}
#endif

	return bSSE41Extensions;
}