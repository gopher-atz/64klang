#include "Synth.h"
#include "SynthNode.h"
#include "SynthAllocator.h"
#include "sample_t.h"

// enable the define if midi cc note triggers should be used
#define MIDICC_NOTE_TRIGGER

// get skip node definitions in exe mode
#if !defined(COMPILE_VSTI) && !defined(USE_BLOBS)
	#include "64k2Patch.h"
#endif

#ifndef STOREDSAMPLES_SKIP
#ifdef _WIN32
	#include <windows.h>
	#include <mmreg.h>
	#include <msacm.h>
	#pragma comment(lib, "msacm32.lib")
	#pragma comment(lib, "wmvcore.lib")

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
	2,		// WORD        nBlockAlign;
	16,     // WORD        wBitsPerSample;
	0,		// WORD        cbSize;       // extra bytes after wfx struct, UNUSED
};
#endif // _WIN32
#endif // STOREDSAMPLES_SKIP

#ifndef GMDLS_SKIP
#ifdef _WIN32
#include <windows.h>
#define GMDLS_FILEBUFFER_SIZE 1024*1024*10
const char*	lpGMDLSSuffix = "\\drivers\\gm.dls";
char	lpGMDLSBuffer[GMDLS_FILEBUFFER_SIZE]; // 10Mb is enough for reading in
#endif // _WIN32
#endif // GMDLS_SKIP

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64klang core interface functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_BLOBS
void _64klang_Init(uint8_t* songStream, void* patchData)
#else
void _64klang_Init(uint8_t* songStream, void* patchData, uint32_t const1Offset, uint32_t const2Offset, uint32_t maxOffset)
#endif
{
	// init sample_t class constants
	sample_t::init();

#ifdef USE_BLOBS
	uint32_t* offsets = (uint32_t*)patchData;
	uint32_t const1Offset = *offsets++;
	uint32_t const2Offset = *offsets++;
	uint32_t maxOffset = *offsets++;
	patchData = (void*)offsets;
	// bss section is zeroed, we only need this if an exe calls this function several times with different songs and if we actually care about memory leaks
	if (SynthGlobalState.GlobalNodes != NULL && SynthGlobalState.NodeValues != NULL && SynthGlobalState.SongStream != NULL && SynthGlobalState.MaxOffset > 0)
	{
		for (uint32_t i = 0; i < SynthGlobalState.MaxOffset; i++)
		{
			SynthNode* sn = SynthGlobalState.GlobalNodes[i];
			if (sn != NULL)
			{
				if (sn->isGlobal)
				{
					if (sn->customMem != NULL)
						SynthFree(sn->customMem);
					SynthFree(sn);
				}
			}
		}
	}
#endif

#if defined(COMPILE_VSTI) || defined(USE_BLOBS)
	SynthMemSet(&SynthGlobalState, sizeof(SynthGlobalStateStruct), 0);
#endif
	SynthGlobalState.GlobalNodes = (SynthNode**)SynthMalloc(maxOffset*sizeof(SynthNode*));
	SynthGlobalState.CreatedNodeTicks = (DWORD*)SynthMalloc(maxOffset*sizeof(DWORD));
#ifdef COMPILE_VSTI
	SynthGlobalState.NodeValues = (SYNTH_WORD*)SynthMalloc(maxOffset*sizeof(SYNTH_WORD));
	SynthGlobalState.SpecialDataPointer = (void**)SynthMalloc(maxOffset*sizeof(void*));
#else
	SynthGlobalState.CurrentTick++;
	SynthGlobalState.ConstantOffset = const1Offset;
	SynthGlobalState.MaxOffset = maxOffset;
	SynthGlobalState.NodeValues = (SYNTH_WORD*)patchData;
	SynthGlobalState.SongStream = songStream;
	uint32_t deltaSize = *((uint32_t*)(songStream+12));
	uint8_t* curSongByte = songStream + 16; // skipping: bpm, songlength, quantizationframes, and deltaencoded size (= 4 DWORDS)
	// deltadecode all song streams
	uint8_t lastValue = 0;
	while (deltaSize--)
	{
		*curSongByte += lastValue;
		lastValue = *curSongByte++;
	}
#endif

#ifndef COMPILE_VSTI
	// create constants
	uint32_t offset = const1Offset;
	while (offset < const2Offset)
	{
		CreateNode(CONSTANT_ID, 1, offset, 1);
		offset+=2;
	}
	while (offset < maxOffset)
	{
		CreateNode(CONSTANT_ID, 2, offset, 1);
		offset+=4;
	}
	// create global nodes from patch data
	CreateNodes(0, true);

	// jump over arpeggiator step sequences (they are pointed to directly at first use of an voicemanager)
#ifndef VOICEMANAGER_ARP_SKIP
	offset += 1 + SynthGlobalState.NodeValues[offset] * 33;
#endif

	// process special data for global nodes (e.g. arpeggiator step sequences, text to speech text)
#ifndef SPECIALDATA_SKIP
	WORD* specialBuf = &(SynthGlobalState.NodeValues[offset++]);
	// get the number of special data blocks to follow
	WORD numSpecialNodes = *specialBuf++;
	while (numSpecialNodes--)
	{
		// get the referenced sapi node
		SynthNode* node = CreateNodes(*specialBuf++, true);
		offset++;
		// get the size of data to follow (in bytes)
		uint32_t dsize = *specialBuf++;
		offset ++;
		// set the special data pointer
		node->specialData = (void*)specialBuf;
		// go to next block, consider odd buffer sizes as well since the storage is WORD sized
		if (dsize & 1)
			dsize++;
		specialBuf += dsize/2;
		offset += dsize/2;
	}
#endif

	// process compressed wavetables
#ifndef STOREDSAMPLES_SKIP
#ifdef _WIN32
	WORD* compBuf = &(SynthGlobalState.NodeValues[offset]);
	while (true)
	{
		WORD table = *compBuf++;
		if (table == 0xdead)
			break;
		uint32_t csr = *((uint32_t*)compBuf);
		compBuf += 2;
		uint32_t avg = *((uint32_t*)compBuf);
		compBuf += 2;
		uint32_t srcBufSize = *((uint32_t*)compBuf);
		compBuf += 2;

		// set pcm format
		pcmFormat.nSamplesPerSec = csr;
		pcmFormat.nAvgBytesPerSec = csr*1*2;    // 1 channel a 2 bytes (16bit) per sample
		// set gsm format
		gsmFormat.wfx.nSamplesPerSec = csr;
		if (csr == 44100)
			gsmFormat.wfx.nAvgBytesPerSec = 8957;
		if (csr == 22050)
			gsmFormat.wfx.nAvgBytesPerSec = 4478;
		if (csr == 11025)
			gsmFormat.wfx.nAvgBytesPerSec = 2239;

		// convert gsm to pcm
		uint32_t dstBufSize = 0;
		uint8_t* dstBuf = NULL;
		_64klang_ACMConvert(&gsmFormat, &pcmFormat, (uint8_t*)compBuf, srcBufSize, dstBuf, dstBufSize);
		// consider odd buffer sizes as well since the storage is WORD sized
		if (srcBufSize & 1)
			srcBufSize++;
		compBuf += srcBufSize/2;

		// set in core wavetable array (first sample is number of samples to follow)
		int loops = 44100 / csr;
		dstBufSize *= loops;
		int numSamples = dstBufSize/2;
		sample_t* coreBuf = (sample_t*)SynthMalloc(sizeof(sample_t)*(1 + numSamples));
		SynthGlobalState.RawWaveTable[table] = coreBuf;
		// number of samples to follow
		sample_t ns(numSamples);
		*coreBuf++ = s_toSample(ns.pi);
		// copy/convert the samples, including upsampling if needed
		int i = 0;
		while (i < numSamples/loops)
		{
			int cur = ((short*)dstBuf)[i];
			int nex = cur;
			if (i < numSamples/loops-1)
				nex = ((short*)dstBuf)[i+1];
			sample_t frac = sample_t::zero();
			int j = 0;
			while (j < loops)
			{
				coreBuf[loops*i+j] = s_lerp(s_toSample(sample_t(cur).pi), s_toSample(sample_t(nex).pi), frac)/SC[S_32768_0];
				frac += SC[S_1_0]/s_toSample(sample_t(loops).pi);
				j++;
			}
			i++;
		}
		SynthFree(dstBuf);
	}
#endif // _WIN32
#endif // STOREDSAMPLES_SKIP

#endif // !COMPILE_VSTI

#ifndef GMDLS_SKIP
#ifdef _WIN32
	char lpGMDLSName[1024];
	int len = GetSystemDirectoryA(lpGMDLSName, 1024);
	SynthMemCopy(lpGMDLSName+len, (void*)lpGMDLSSuffix, 30);
	*(lpGMDLSName+len+15) = 0;

	HANDLE hFile = CreateFileA(lpGMDLSName,               // file to open
					   GENERIC_READ,          // open for reading
					   FILE_SHARE_READ,       // share for reading
					   NULL,                  // default security
					   OPEN_EXISTING,         // existing file only
					   FILE_ATTRIBUTE_NORMAL, // normal file
					   NULL);                 // no attr. template
	DWORD numbytes = 0;
	ReadFile(hFile, lpGMDLSBuffer, GMDLS_FILEBUFFER_SIZE-1, &numbytes, NULL);
	CloseHandle(hFile);

	uint32_t numWaves = 0;
	for (uint32_t i = 0; i < numbytes; i++)
	{
		uint32_t* data = (uint32_t*)(lpGMDLSBuffer+i);
		if (*data == MAKEFOURCC('d','a','t','a'))
		{
			i+=4;
			// number of data bytes, so samples = numBytes/2 but we need to upsample from 22050 to 44100
			uint32_t numSamples = *((uint32_t*)(lpGMDLSBuffer+i));
			i+=4;
			GMDLS_NumSamples[numWaves] = numSamples;
			sample_t* buf = (sample_t*)SynthMalloc(numSamples*sizeof(sample_t));
			GMDLS_SampleBuffer[numWaves] = buf;
			for (uint32_t s = 0; s < numSamples/2; s++)
			{
				int si = *((short*)(lpGMDLSBuffer+i));
				sample_t ss = s_toSample(sample_t(si).pi)/SC[S_32768_0];
				*buf++ = ss;
				*buf++ = ss; // upsampling by doubling the samples
				i+=2;
			}
			numWaves++;
		}
	}
#endif // _WIN32
#endif // GMDLS_SKIP
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef STOREDSAMPLES_SKIP
#ifdef _WIN32

int _64klang_ACMConvert(void* srcFormat, void* dstFormat, uint8_t* srcBuffer, uint32_t srcBufferSize, uint8_t*& dstBuffer, uint32_t& dstBufferSize)
{
	// open the conversion stream
	HACMSTREAM acmStream = NULL;
	if (acmStreamOpen(&acmStream, NULL, (LPWAVEFORMATEX)srcFormat, (LPWAVEFORMATEX)dstFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME))
		return E_FAIL;

	// determine gsm output buffer size
	if (acmStreamSize( acmStream, srcBufferSize, (DWORD*)&dstBufferSize, ACM_STREAMSIZEF_SOURCE ))
		return E_FAIL;

	dstBuffer = (uint8_t*)SynthMalloc( dstBufferSize );

	// prepare header
	ACMSTREAMHEADER acmStreamHeader;
	SynthMemSet( &acmStreamHeader, sizeof(ACMSTREAMHEADER ), 0);
	acmStreamHeader.cbStruct = sizeof(ACMSTREAMHEADER );
	acmStreamHeader.pbSrc = srcBuffer;
	acmStreamHeader.cbSrcLength = srcBufferSize;
	acmStreamHeader.pbDst = dstBuffer;
	acmStreamHeader.cbDstLength = dstBufferSize;
	if (acmStreamPrepareHeader( acmStream, &acmStreamHeader, 0))
	{
		SynthFree(dstBuffer);
		dstBuffer = 0;
		return E_FAIL;
	}

	// convert stream
	if (acmStreamConvert( acmStream, &acmStreamHeader, ACM_STREAMCONVERTF_BLOCKALIGN ))
	{
		SynthFree(dstBuffer);
		dstBuffer = 0;
		return E_FAIL;
	}

	// unprepare header
	acmStreamUnprepareHeader( acmStream, &acmStreamHeader, 0 );

	// close stream handle
	acmStreamClose( acmStream, 0 );

	return 0;
}

#else // !_WIN32

int _64klang_ACMConvert(void* srcFormat, void* dstFormat, uint8_t* srcBuffer, uint32_t srcBufferSize, uint8_t*& dstBuffer, uint32_t& dstBufferSize)
{
	// Stub: ACM conversion not available on non-Windows platforms
	dstBuffer = nullptr;
	dstBufferSize = 0;
	return -1;
}

#endif // _WIN32
#endif // STOREDSAMPLES_SKIP

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// note controller offsets can vary since it has a variable number of inputs
#define W ((VMWork*)(n->customMem))
void AddToNoteController(uint32_t channel, uint32_t note, uint32_t velocity, bool on)
{
	// get channelroot (channel). index 0 of GlobalNodes is the synth root (which is a MUL), directly afterwards the channelroots
#ifdef COMPILE_VSTI
	SynthNode* cr = SynthGlobalState.GlobalNodes[(1+channel)*(NODE_MAX_INPUTS+1)];
#else
	SynthNode* cr = SynthGlobalState.GlobalNodes[(SYNTHROOT_MAX+1)+channel*(CHANNELROOT_MAX+1)];
#endif
	// set envelope follower state active to ensure the channel will be ticked an processes the note changes
	cr->v[0] = SC[S_ALLBITS];
	// need to set channel root envelope level to some small level so theres time for the output to arrive at the channel root
	cr->v[1] = SC[S_0_125];

	// get notecontroller
	SynthNode* nc;
#ifdef COMPILE_VSTI
	nc = SynthGlobalState.GlobalNodes[(1+16+channel)*(NODE_MAX_INPUTS+1)];
#else
	// start offset is synth root + all channel roots
	uint32_t offset = (SYNTHROOT_MAX+1)+MAX_CHANNELS*(CHANNELROOT_MAX+1);
	uint32_t ccount = channel;
	while (ccount-- != 0)
	{
		uint32_t refnodes 	= (SynthGlobalState.NodeValues[offset] >> 8) & 0x7f;
		offset += refnodes+1;
	}
	nc = SynthGlobalState.GlobalNodes[offset];
#endif

	// combine note and velocity data
	uint32_t data = note | (velocity << 8);
	// forward note event to responsible voice manager (depending on note range)
	int v = nc->numInputs;
	while (v--)
	{
		SynthNode* n = (SynthNode*)(nc->input[v]);
		// set the voicemanagers channel
		n->v[23].i[0] = channel;
		// get the note range from the voicemanager
#ifdef COMPILE_VSTI
		uint32_t mode = n->input[VOICEMANAGER_MODE]->i[0];
#else
		uint32_t mode = *(n->modePointer);
#endif
		uint32_t note_min = mode & VOICEMANAGER_MINNOTEMASK;
		uint32_t note_max = (mode & VOICEMANAGER_MAXNOTEMASK) >> 0x8;
		// found a matching voicemanager to insert?
		if (note >= note_min && note <= note_max)
		{
#ifdef COMPILE_VSTI
			// reset voicemanager updatestep for immediate processing
			n->currentUpdateStep = 0;
#endif
			W->Rescan = 1;
			// keyed on is stored via counter 1 and offset 32
			if (on)
			{
#ifdef COMPILE_VSTI
				// prevent overflow when synth is not triggered (e.g. due to disconnection)
				if (n->v[20].i[1] == 32)
					break;
#endif
				W->Keys[note] = ++SynthGlobalState.CurrentNoteTick;
#ifndef VOICEMANAGER_ARP_SKIP
				// if arp is active dont insert note into voicemanager, arp will handle the voices
				if (mode & VOICEMANAGER_ARP_RUNMASK)
				{
					if (mode & VOICEMANAGER_ARP_RETRIGGER)
						W->ArpCurrentKey = -1;
				}
				else
#endif
				{
					*(((uint32_t*)(n->v))+32+n->v[20].i[1]) = data;
					n->v[20].i[1]++;
				}
				break;
			}
			// keyed off is stored via counter 0 and offset 0
			else
			{
#ifdef COMPILE_VSTI
				// prevent overflow when synth is not triggered (e.g. due to disconnection)
				if (n->v[20].i[0] == 32)
					break;
#endif

				W->Keys[note] = 0;
#ifndef VOICEMANAGER_ARP_SKIP
				// if arp is active dont remove note from voicemanager, arp will handle the voices
				if (mode & VOICEMANAGER_ARP_RUNMASK)
				{
				}
				else
#endif
				{
					*(((uint32_t*)(n->v))+0+n->v[20].i[0]) = data;
					n->v[20].i[0]++;
				}
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SONG_AFTERTOUCH_SKIP
void _64klang_NoteAftertouch(uint32_t channel, uint32_t note, uint32_t value)
{
	SynthGlobalState.NoteAftertouch[channel*128+note] = sample_t((double)value)/SC[S_128_0];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef MIDICC_NOTE_TRIGGER
// note state for cc trigger notes
int ccTriggerNotes[16*16];
#endif

void _64klang_MidiSignal(uint32_t channel, int value, uint32_t cc)
{
	// special case for pitch bend. can be negative and also uses 2x the precision (normaly it would be 64x the precision but to kee streams uniform as bytes we sacrifice)
	if (cc == 0)
		value -= 128;
#ifdef MIDICC_NOTE_TRIGGER
	// when enabled midi cc's 112 to 127 offer 16 trigger voices
	// voice base note will be equivalent to midi note 60 to 75 (C4 to D#5), velocity is equivalent to CC value
	// cc commands should be discreet (no continuous cc change, but rather 0 -> value -> 0)
	if (cc >= 112)
	{
		int ccnote = cc - 112;
		// old note playing? kill it
		if (ccTriggerNotes[channel*16+ccnote] != 0)
			AddToNoteController(channel, 60 + ccnote, value, false);
		// new note wanted? create it
		if (value != 0)
			AddToNoteController(channel, 60 + ccnote, value, true);
		// store note state
		ccTriggerNotes[channel*16+ccnote] = value;
	}
#endif
	SynthGlobalState.MidiSignals[channel*128+cc] = sample_t((double)value)/SC[S_128_0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef COMPILE_VSTI

void _64klang_NoteOn(uint32_t channel, uint32_t note, uint32_t velocity)
{
	AddToNoteController(channel, note, velocity, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void _64klang_NoteOff(uint32_t channel, uint32_t note, uint32_t velocity)
{
	AddToNoteController(channel, note, velocity, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void _64klang_SetBPM(float bpm)
{
	SynthGlobalState.CurrentBPM = sample_t((double)bpm);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void _64klang_Tick(float* left, float* right, uint32_t samples)
{
	// save state with cleared flags
	unsigned int sse_control_store = _mm_getcsr() & 0xffc0;
	// bits: 15 = flush to zero | 6 = denormals are zero
	// rounding 14:13, 00 = nearest, 01 = neg, 10 = pos, 11 = to zero
	// bits 12-7 exception masks
	_mm_setcsr(0x8040 | 0x1f80 | ((unsigned int)3 << 13));

	while (samples--)
	{
		SynthGlobalState.GlobalNodes[0]->tick(SynthGlobalState.GlobalNodes[0]);
		*left++ = (float)SynthGlobalState.GlobalNodes[0]->out.d[0];
		*right++ = (float)SynthGlobalState.GlobalNodes[0]->out.d[1];
		SynthGlobalState.CurrentTick++;
	}

	// restore previous state
	_mm_setcsr(sse_control_store);
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// for time skipping and data checks in a a song
//#define DEBUG_DECODE_VALUES_
//#define DEBUG_DECODE_SKIP 15895296

#define NOTEOFF_STREAM	0
#define NOTEON_STREAM	1
#define AT_STREAM		2
#define CC_STREAM		3

uint8_t* DeltaStream;
int FrameSize;
uint32_t StreamIndex[65536];
uint32_t TimeStamp[65536];

void CombineTimeBytes(int streamIndex, uint8_t* value, uint32_t stride)
{
	uint32_t b0 = ((uint32_t)(*value));
	value += stride;
	uint32_t b1 = ((uint32_t)(*value));
	value += stride;
	uint32_t b2 = ((uint32_t)(*value));
	TimeStamp[streamIndex] = FrameSize * ((b2 << 16) + (b1 << 8) + b0);
}

bool ProcessStream(uint32_t currentTimeStamp, uint32_t& streamIndex)
{
	// read info (1 DWORD)
	uint32_t streamInfo = *((uint32_t*)DeltaStream);
	DeltaStream += 4;
	if (streamInfo == 0xffffffff)
		return false;
	// decode info
	uint32_t numValues		= streamInfo & 0x0000ffff;
	uint32_t channel		= (streamInfo >> 28);
	uint32_t streamType	= (streamInfo >> 24) & 0x0f;
	uint32_t specialVal	= (streamInfo >> 16) & 0xff;

	// get timestamp and value stream positions
	uint32_t curIndex = StreamIndex[streamIndex];
	if (curIndex < numValues)
	{
		uint8_t* StreamData = DeltaStream + curIndex;
		// need to init first timestamp?
		if (TimeStamp[streamIndex] == 0xffffffff)
		{
			CombineTimeBytes(streamIndex, StreamData, numValues);
		}
		// need to apply events? (could be several simultaneously)
		while (TimeStamp[streamIndex] <= currentTimeStamp)
		{
			uint8_t* streamValue2 = StreamData + numValues * 4;
			uint8_t* streamValue = streamValue2 - numValues;
#ifdef DEBUG_DECODE_VALUES
			// debug skip events
			if (currentTimeStamp >= DEBUG_DECODE_SKIP)
#endif
			{
				// apply event for respective type
				if (streamType == CC_STREAM)
					_64klang_MidiSignal(channel, *streamValue, specialVal);
#ifndef SONG_AFTERTOUCH_SKIP
				else if (streamType == AT_STREAM)
					_64klang_NoteAftertouch(channel, *streamValue, *streamValue2);
#endif
				else
					AddToNoteController(channel, *streamValue, *streamValue2, streamType == NOTEON_STREAM);
			}
			// increase index
			StreamIndex[streamIndex] = ++curIndex;
			// leave when no more events
			if (curIndex >= numValues)
				break;
			// next value
			StreamData++;
			// set timestamp for next value
			CombineTimeBytes(streamIndex, StreamData, numValues);
		}
	}

	// go to next stream
	DeltaStream += numValues * 4;
	if (streamType == NOTEON_STREAM
#ifndef SONG_AFTERTOUCH_SKIP
		|| streamType == AT_STREAM
#endif
		)
		DeltaStream += numValues;

	// next stream index
	streamIndex++;
	return true;
}

#ifndef COMPILE_VSTI
bool renderDone;
#ifdef AUTHORING
int CurrentBufferSample;
#endif
void _64klang_Render(float* dstbuffer)
{
	renderDone = false;
	// save state with cleared flags
	unsigned int sse_control_store = _mm_getcsr() & 0xffc0;
	_mm_setcsr(0x8040 | 0x1f80 | ((unsigned int)3 << 13));

	// read song bpm (1 DWORD)
	SynthGlobalState.CurrentBPM = sample_t((double)(*((float*)SynthGlobalState.SongStream)));
	// read song length (1 DWORD)
	uint32_t maxTimeStamp = *((uint32_t*)(SynthGlobalState.SongStream+4));
	// read frame size (1 DWORD)
	FrameSize = *((uint32_t*)(SynthGlobalState.SongStream + 8));
	// position index list for each stream (initialized with 0)
	SynthMemSet(StreamIndex, sizeof(uint32_t)*65536, 0);
	// timestamp for each stream (initialized with -1)
	SynthMemSet(TimeStamp, sizeof(int) * 65536, 0xff);

	// process song streams
	uint32_t currentTimeStamp = 0;
	while (currentTimeStamp < maxTimeStamp)
	{
		DeltaStream = SynthGlobalState.SongStream+16; // skipping: bpm, songlength, quantizationframes, and deltaencoded size (= 4 DWORDS)
		uint32_t currentStreamIndex = 0;
		// loop all channels
		while (ProcessStream(currentTimeStamp, currentStreamIndex));
		// tick samples until next frame
		int renderSamples = FrameSize;
		while (renderSamples--)
		{
			SynthGlobalState.GlobalNodes[0]->tick(SynthGlobalState.GlobalNodes[0]);
			*dstbuffer++ = (float)(SynthGlobalState.GlobalNodes[0]->out.d[0]);
			*dstbuffer++ = (float)(SynthGlobalState.GlobalNodes[0]->out.d[1]);
			SynthGlobalState.CurrentTick++;
			currentTimeStamp++;
#ifdef AUTHORING
			CurrentBufferSample = currentTimeStamp;
#endif
		}
		// deferred free of deleted nodes
		SynthDeferredFree();
	}
	renderDone = true;

	// restore previous state
	_mm_setcsr(sse_control_store);
}

bool _64klang_RenderDone()
{
	return renderDone;
}

#ifdef AUTHORING
int _64klang_CurrentBufferSample()
{
	return CurrentBufferSample;
}
#endif
#endif
