#include "vstxsynth.h"
#include <windows.h>
#include "../64klang2Wrapper/64klang2Wrapper.h"
#include "../64klang2Core/SynthController.h"

//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new VstXSynth (audioMaster);
}

//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------
VstXSynth::VstXSynth (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, 128, 1)
{
	if (audioMaster)
	{
		setNumInputs (0);				// no inputs
		setNumOutputs (2);	// 2 outputs, 1 for each oscillator
		canProcessReplacing ();
		isSynth ();
		setUniqueID ('64k2');			// <<<! *must* change this!!!!
	}
	programsAreChunks(true);		
	initProcess ();
	suspend ();	

	// set the appartment model to STA before initializing the .NET wrapper. seems to fix the STA exception when called by 64bit bridges like cubase 8/jbridger
	CoInitialize(NULL);

	// init the .net wrapper
	SynthWrapper::instance();
}

//-----------------------------------------------------------------------------------------
VstXSynth::~VstXSynth ()
{
//	MessageBox(NULL, "VstXSynth::~VstXSynth", "VstXSynth", MB_OK);
}

void VstXSynth::open()
{
	//MessageBoxA(NULL, "VstXSynth::open", "VstXSynth", MB_OK);
	// notify wrapper the gui needs to rebuild the data model
	SynthWrapper::instance()->openWindow();
}

void VstXSynth::close()
{
	//MessageBoxA(NULL, "VstXSynth::close", "VstXSynth", MB_OK);
	SynthWrapper::instance()->closeWindow();
}

void VstXSynth::suspend()
{
//	MessageBoxA(NULL, "VstXSynth::suspend", "VstXSynth", MB_OK);
}

void VstXSynth::resume()
{
//	MessageBoxA(NULL, "VstXSynth::resume", "VstXSynth", MB_OK);
}

///< Host stores plug-in state. Returns the size in bytes of the chunk (plug-in allocates the data array)
VstInt32 VstXSynth::getChunk (void** data, bool isPreset)
{
	if (!SynthController::instance()->isInitialized())
		return 0;
	
	// save to temp file
	std::string tmpPath = getenv("TEMP");
	tmpPath += "\\64klang2_chunk_store.tmp";
	SynthController::instance()->savePatch(tmpPath);
	
	// read tmp file
	FILE* file = fopen( tmpPath.c_str (), "rb" );	
	long length = 0;
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	fseek( file, 0, SEEK_SET );

	// read in patch to data block
	*data = malloc(length);
	fread(*data, 1, length, file);
	fclose(file);
	return length;
}

///< Host restores plug-in state
VstInt32 VstXSynth::setChunk(void* data, VstInt32 byteSize, bool isPreset)
{
	if (!SynthController::instance()->isInitialized())
		return 0;

	std::string tmpPath = getenv("TEMP");
	tmpPath += "\\64klang2_chunk_load.tmp";

	// write tmp file from data block
	FILE* file = fopen( tmpPath.c_str (), "wb" );
	fwrite(data, 1, byteSize, file);
	fclose(file);

	// load the patch
	SynthWrapper::instance()->loadPatchAndUpdateGUI(tmpPath);
	return 1;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgram (VstInt32 program)
{
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgramName (char *name)
{
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getProgramName (char *name)
{
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterLabel (VstInt32 index, char *label)
{
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterDisplay (VstInt32 index, char *text)
{
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterName (VstInt32 index, char *label)
{
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setParameter (VstInt32 index, float value)
{
}

//-----------------------------------------------------------------------------------------
float VstXSynth::getParameter (VstInt32 index)
{
	return 0;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{
	if (index < 2)
	{
		vst_strncpy (properties->label, "Vstx ", 63);
		char temp[11] = {0};
		int2string (index + 1, temp, 10);
		vst_strncat (properties->label, temp, 63);

		properties->flags = kVstPinIsActive;
		if (index < 2)
			properties->flags |= kVstPinIsStereo;	// make channel 1+2 stereo
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	return false;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getEffectName (char* name)
{
	vst_strncpy (name, "64klang2", kVstMaxEffectNameLen);
	return true;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getVendorString (char* text)
{
	vst_strncpy (text, "Alcatraz", kVstMaxVendorStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getProductString (char* text)
{
	vst_strncpy (text, "64klang Version 2.0", kVstMaxProductStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getVendorVersion ()
{ 
	return 2000; 
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::canDo (char* text)
{
	if (!strcmp (text, "receiveVstEvents"))
		return 1;
	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;
	if (!strcmp (text, "midiProgramNames"))
		return 1;
	return -1;	// explicitly can't do; 0 => don't know
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getNumMidiInputChannels ()
{
	return 16;
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getNumMidiOutputChannels ()
{
	return 0; // no MIDI output back to Host app
}

