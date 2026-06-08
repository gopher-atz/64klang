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
#ifdef COMPILE_VSTI
#include <windows.h>
#include <math.h>
#define GMDLS_FILEBUFFER_SIZE 1024*1024*10
const char*	lpGMDLSSuffix = "\\drivers\\gm.dls";
char	lpGMDLSBuffer[GMDLS_FILEBUFFER_SIZE]; // 10Mb is enough for reading in

// Convert a DLS Level 1 timecent value (16.16 fixed-point lScale) to seconds.
// timecents = lScale / 65536;  seconds = 2 ^ (timecents / 1200)
// Clamped to ±10 octaves to guard against extreme / garbage values.
static inline float gmdls_tc_to_sec(int32_t lScale)
{
	double tc = (double)lScale / 65536.0;
	if (tc < -12000.0) tc = -12000.0;
	if (tc >  12000.0) tc =  12000.0;
	return (float)pow(2.0, tc / 1200.0);
}

// Map a DLS WLOOP ulType to the internal loopType convention:
//   DLS 0 (forward)          -> 1
//   DLS 1 (release/bidi)     -> 2
// Returns 0 for unsupported types (caller should reject such loops).
static inline uint8_t gmdls_loop_type(uint32_t dlsType)
{
	if (dlsType == 0) return 1; // forward loop
	if (dlsType == 1) return 2; // release / bidirectional loop
	return 0;
}

#endif // COMPILE_VSTI
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
#ifdef COMPILE_VSTI
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
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD numbytes = 0;
		ReadFile(hFile, lpGMDLSBuffer, GMDLS_FILEBUFFER_SIZE-1, &numbytes, NULL);
		CloseHandle(hFile);

		// ----------------------------------------------------------------
		// Validate outer RIFF 'DLS ' container
		// ----------------------------------------------------------------
		const uint8_t* buf = (const uint8_t*)lpGMDLSBuffer;
		do
		{
			if (numbytes < 12) break;
			if (*(uint32_t*)(buf+0) != MAKEFOURCC('R','I','F','F')) break;
			if (*(uint32_t*)(buf+8) != MAKEFOURCC('D','L','S',' ')) break;
			uint32_t riffSize = *(uint32_t*)(buf+4);
			// Clamp riffSize to what was actually read (handles truncated files)
			if ((uint32_t)8 + riffSize > numbytes)
				riffSize = numbytes - 8;

			uint32_t riffDataEnd   = 8 + riffSize; // absolute end of RIFF data
			uint32_t riffDataStart = 12;            // first sub-chunk (skips 'RIFF'+size+'DLS ')

			uint32_t numWaves = 0;

			// ============================================================
			// Pass 1 – LIST('wvpl'): load wave samples + wave-level loops
			// Each LIST('wave') in the pool is one instrument sample slot.
			// Upsampling: input is 22050 Hz 16-bit mono; each raw sample is
			// duplicated to produce 44100 Hz output (2x factor).
			// All loop point indices stored in GMDLS_LoopData are already
			// scaled by this 2x factor.
			// ============================================================
			for (uint32_t p = riffDataStart; p + 8 <= riffDataEnd; )
			{
				uint32_t chId   = *(uint32_t*)(buf + p);
				uint32_t chSize = *(uint32_t*)(buf + p + 4);
				uint32_t chData = p + 8;
				// Clamp malformed chunk size
				if (chData + chSize > riffDataEnd)
					chSize = riffDataEnd - chData;
				uint32_t chNext = chData + ((chSize + 1u) & ~1u); // word-aligned next

				if (chId == MAKEFOURCC('L','I','S','T') && chSize >= 4 &&
				    *(uint32_t*)(buf + chData) == MAKEFOURCC('w','v','p','l'))
				{
					uint32_t wvplEnd = chData + chSize;
					// Walk LIST('wave') children
					for (uint32_t wp = chData + 4; wp + 8 <= wvplEnd && numWaves < 512; )
					{
						uint32_t wId   = *(uint32_t*)(buf + wp);
						uint32_t wSize = *(uint32_t*)(buf + wp + 4);
						uint32_t wData = wp + 8;
						if (wData + wSize > wvplEnd) wSize = wvplEnd - wData;
						uint32_t wNext = wData + ((wSize + 1u) & ~1u);

						if (wId == MAKEFOURCC('L','I','S','T') && wSize >= 4 &&
						    *(uint32_t*)(buf + wData) == MAKEFOURCC('w','a','v','e'))
						{
							// Initialise metadata for this slot
							GMDLS_NumSamples[numWaves]              = 0;
							GMDLS_SampleBuffer[numWaves]            = NULL;
							GMDLS_LoopData[numWaves].loopStart      = 0;
							GMDLS_LoopData[numWaves].loopEnd        = 0;
							GMDLS_LoopData[numWaves].loopType       = 0;
							GMDLS_LoopData[numWaves].sourcePriority = 0;
							GMDLS_EnvData[numWaves].attack          = 0.f;
							GMDLS_EnvData[numWaves].decay           = 0.f;
							GMDLS_EnvData[numWaves].sustain         = 1.f;
							GMDLS_EnvData[numWaves].release         = 0.f;
							GMDLS_EnvData[numWaves].validMask       = 0;

							bool     wavePCM16    = false;
							bool     waveLoaded   = false;
							uint32_t waveEnd      = wData + wSize;

							for (uint32_t cp = wData + 4; cp + 8 <= waveEnd; )
							{
								uint32_t cId   = *(uint32_t*)(buf + cp);
								uint32_t cSize = *(uint32_t*)(buf + cp + 4);
								uint32_t cData = cp + 8;
								if (cData + cSize > waveEnd) cSize = waveEnd - cData;
								uint32_t cNext = cData + ((cSize + 1u) & ~1u);

								if (cId == MAKEFOURCC('f','m','t',' ') && cSize >= 16)
								{
									// WAVEFORMATEX: wFormatTag(2) nChannels(2) nSamplesPerSec(4)
									//               nAvgBytesPerSec(4) nBlockAlign(2) wBitsPerSample(2)
									uint16_t fmtTag  = *(uint16_t*)(buf + cData + 0);
									uint16_t nChan   = *(uint16_t*)(buf + cData + 2);
									uint16_t nBits   = *(uint16_t*)(buf + cData + 14);
									wavePCM16 = (fmtTag == 1 && nChan == 1 && nBits == 16);
								}
								else if (cId == MAKEFOURCC('w','s','m','p') && cSize >= 20)
								{
									// WSMPL: cbSize(4) usUnityNote(2) sFineTune(2) lAttenuation(4)
									//        fulOptions(4) cSampleLoops(4) [WLOOP...]
									// WLOOPs start at cbSize bytes from cData.
									uint32_t wsmpCbSize = *(uint32_t*)(buf + cData + 0);
									uint32_t cLoops     = *(uint32_t*)(buf + cData + 16);
									if (cLoops > 0 && wsmpCbSize >= 20 &&
									    cData + wsmpCbSize + 16 <= cData + cSize)
									{
										// First WLOOP: cbSize(4) ulType(4) ulStart(4) ulLength(4)
										uint32_t lb    = cData + wsmpCbSize;
										uint32_t lType = *(uint32_t*)(buf + lb + 4);
										uint32_t lSt   = *(uint32_t*)(buf + lb + 8);
										uint32_t lLen  = *(uint32_t*)(buf + lb + 12);
										// DLS loop type 0 = forward, 1 = release/bidirectional
										if (lType == 0 || lType == 1)
										{
											// Scale raw sample indices by 2x upsampling factor
											GMDLS_LoopData[numWaves].loopStart      = lSt * 2;
											GMDLS_LoopData[numWaves].loopEnd        = (lSt + lLen) * 2;
											GMDLS_LoopData[numWaves].loopType       = gmdls_loop_type(lType);
											GMDLS_LoopData[numWaves].sourcePriority = 1; // wave-level
										}
									}
								}
								else if (cId == MAKEFOURCC('d','a','t','a') && wavePCM16)
								{
									// Load sample data.  Each raw 16-bit sample is duplicated to
									// produce 44100 Hz output from a 22050 Hz source (legacy gm.dls).
									uint32_t numRaw      = cSize / 2;
									uint32_t numStored   = numRaw * 2; // upsampled
									GMDLS_NumSamples[numWaves]   = numStored;
									sample_t* sbuf = (sample_t*)SynthMalloc(numStored * sizeof(sample_t));
									GMDLS_SampleBuffer[numWaves] = sbuf;
									for (uint32_t s = 0; s < numRaw; s++)
									{
										int si = *(const short*)(buf + cData + s * 2);
										sample_t ss = s_toSample(sample_t(si).pi) / SC[S_32768_0];
										*sbuf++ = ss;
										*sbuf++ = ss; // upsampling by duplicating
									}

									// Clamp and validate wave-level loop bounds now that we know the size
									if (GMDLS_LoopData[numWaves].sourcePriority > 0)
									{
										if (GMDLS_LoopData[numWaves].loopEnd > numStored)
											GMDLS_LoopData[numWaves].loopEnd = numStored;
										if (GMDLS_LoopData[numWaves].loopStart >= GMDLS_LoopData[numWaves].loopEnd)
										{
											GMDLS_LoopData[numWaves].loopType       = 0;
											GMDLS_LoopData[numWaves].sourcePriority = 0;
										}
									}
									waveLoaded = true;
								}

								cp = cNext;
							}

							if (waveLoaded)
								numWaves++;
						}

						wp = wNext;
					}
					break; // only one wvpl expected
				}

				p = chNext;
			}

			// ============================================================
			// Pass 2 – LIST('lins'): region-level loops (higher priority)
			//          and EG1 volume-envelope data from art1/art2 blocks.
			//
			// DLS art1 connection blocks use:
			//   Time values in timecents  (seconds = 2^(lScale/65536/1200))
			//   EG1 sustain in hundredths of a percent (0 = silence,
			//   1000 = 100%; linear = (lScale/65536)/1000)
			// ============================================================
			for (uint32_t p = riffDataStart; p + 8 <= riffDataEnd; )
			{
				uint32_t chId   = *(uint32_t*)(buf + p);
				uint32_t chSize = *(uint32_t*)(buf + p + 4);
				uint32_t chData = p + 8;
				if (chData + chSize > riffDataEnd)
					chSize = riffDataEnd - chData;
				uint32_t chNext = chData + ((chSize + 1u) & ~1u);

				if (chId == MAKEFOURCC('L','I','S','T') && chSize >= 4 &&
				    *(uint32_t*)(buf + chData) == MAKEFOURCC('l','i','n','s'))
				{
					uint32_t linsEnd = chData + chSize;

					// Walk LIST('ins ')
					for (uint32_t ip = chData + 4; ip + 8 <= linsEnd; )
					{
						uint32_t iId   = *(uint32_t*)(buf + ip);
						uint32_t iSize = *(uint32_t*)(buf + ip + 4);
						uint32_t iData = ip + 8;
						if (iData + iSize > linsEnd) iSize = linsEnd - iData;
						uint32_t iNext = iData + ((iSize + 1u) & ~1u);

						if (iId == MAKEFOURCC('L','I','S','T') && iSize >= 4 &&
						    *(uint32_t*)(buf + iData) == MAKEFOURCC('i','n','s',' '))
						{
							uint32_t insEnd = iData + iSize;

							// Find LIST('lrgn') inside this instrument
							for (uint32_t lp = iData + 4; lp + 8 <= insEnd; )
							{
								uint32_t lId   = *(uint32_t*)(buf + lp);
								uint32_t lSize = *(uint32_t*)(buf + lp + 4);
								uint32_t lData = lp + 8;
								if (lData + lSize > insEnd) lSize = insEnd - lData;
								uint32_t lNext = lData + ((lSize + 1u) & ~1u);

								if (lId == MAKEFOURCC('L','I','S','T') && lSize >= 4 &&
								    *(uint32_t*)(buf + lData) == MAKEFOURCC('l','r','g','n'))
								{
									uint32_t lrgnEnd = lData + lSize;

									// Walk LIST('rgn ') or LIST('rgn2')
									for (uint32_t rp = lData + 4; rp + 8 <= lrgnEnd; )
									{
										uint32_t rId   = *(uint32_t*)(buf + rp);
										uint32_t rSize = *(uint32_t*)(buf + rp + 4);
										uint32_t rData = rp + 8;
										if (rData + rSize > lrgnEnd) rSize = lrgnEnd - rData;
										uint32_t rNext = rData + ((rSize + 1u) & ~1u);

										if (rId == MAKEFOURCC('L','I','S','T') && rSize >= 4 &&
										    (*(uint32_t*)(buf + rData) == MAKEFOURCC('r','g','n',' ') ||
										     *(uint32_t*)(buf + rData) == MAKEFOURCC('r','g','n','2')))
										{
											uint32_t rgnEnd = rData + rSize;

											uint32_t waveIdx   = 0xFFFFFFFFu;
											bool     hasWlnk   = false;

											// Collected region-level loop
											uint32_t rLoopStart = 0, rLoopEnd = 0;
											uint8_t  rLoopType  = 0;
											bool     rHasLoop   = false;

											// Collected EG1 envelope
											float    envAttack  = 0.f, envDecay  = 0.f;
											float    envSustain = 1.f, envRelease = 0.f;
											uint8_t  envValid   = 0;

											for (uint32_t cp = rData + 4; cp + 8 <= rgnEnd; )
											{
												uint32_t cId   = *(uint32_t*)(buf + cp);
												uint32_t cSize = *(uint32_t*)(buf + cp + 4);
												uint32_t cData = cp + 8;
												if (cData + cSize > rgnEnd) cSize = rgnEnd - cData;
												uint32_t cNext = cData + ((cSize + 1u) & ~1u);

												if (cId == MAKEFOURCC('w','l','n','k') && cSize >= 12)
												{
													// WAVELINK: fusOptions(2) usPhaseGroup(2) ulChannel(4) ulTableIndex(4)
													uint32_t idx = *(uint32_t*)(buf + cData + 8);
													if (idx < numWaves)
													{
														waveIdx = idx;
														hasWlnk = true;
													}
												}
												else if (cId == MAKEFOURCC('w','s','m','p') && cSize >= 20)
												{
													uint32_t wsmpCbSize = *(uint32_t*)(buf + cData + 0);
													uint32_t cLoops     = *(uint32_t*)(buf + cData + 16);
													if (cLoops > 0 && wsmpCbSize >= 20 &&
													    cData + wsmpCbSize + 16 <= cData + cSize)
													{
														uint32_t lb    = cData + wsmpCbSize;
														uint32_t lType = *(uint32_t*)(buf + lb + 4);
														uint32_t lSt   = *(uint32_t*)(buf + lb + 8);
														uint32_t lLen  = *(uint32_t*)(buf + lb + 12);
														if (lType == 0 || lType == 1)
														{
															rLoopStart = lSt * 2;
															rLoopEnd   = (lSt + lLen) * 2;
															rLoopType  = gmdls_loop_type(lType); // 1=forward, 2=release/bidi
															rHasLoop   = true;
														}
													}
												}
												else if (cId == MAKEFOURCC('L','I','S','T') && cSize >= 4 &&
												         (*(uint32_t*)(buf + cData) == MAKEFOURCC('l','a','r','t') ||
												          *(uint32_t*)(buf + cData) == MAKEFOURCC('l','a','r','2')))
												{
													// Walk art1/art2 connection blocks
													uint32_t lartEnd = cData + cSize;
													for (uint32_t ap = cData + 4; ap + 8 <= lartEnd; )
													{
														uint32_t aId   = *(uint32_t*)(buf + ap);
														uint32_t aSize = *(uint32_t*)(buf + ap + 4);
														uint32_t aData = ap + 8;
														if (aData + aSize > lartEnd) aSize = lartEnd - aData;
														uint32_t aNext = aData + ((aSize + 1u) & ~1u);

														if ((aId == MAKEFOURCC('a','r','t','1') ||
														     aId == MAKEFOURCC('a','r','t','2')) && aSize >= 8)
														{
															// ART1 header: cbSize(4) cConnectionBlocks(4)
															// Connection blocks follow at cbSize offset from aData
															uint32_t artCbSize = *(uint32_t*)(buf + aData + 0);
															uint32_t nConns    = *(uint32_t*)(buf + aData + 4);
															uint32_t connBase  = aData + artCbSize;

															for (uint32_t ci = 0;
															     ci < nConns && connBase + 12 <= aData + aSize;
															     ci++, connBase += 12)
															{
																// CONNECTION: usSource(2) usControl(2) usDestination(2) usTransform(2) lScale(4)
																uint16_t src   = *(uint16_t*)(buf + connBase + 0);
																uint16_t ctrl  = *(uint16_t*)(buf + connBase + 2);
																uint16_t dst   = *(uint16_t*)(buf + connBase + 4);
																int32_t  scale = *(int32_t*) (buf + connBase + 8);

																// Only plain (unmodulated) connections
																if (src != 0 || ctrl != 0)
																	continue;

																switch (dst)
																{
																case 0x0104: // EG1_ATTACK_TIME
																	envAttack  = gmdls_tc_to_sec(scale);
																	envValid  |= 1;
																	break;
																case 0x0105: // EG1_DECAY_TIME
																	envDecay   = gmdls_tc_to_sec(scale);
																	envValid  |= 2;
																	break;
																case 0x010A: // EG1_SUSTAIN_LEVEL
																	// DLS Level 1: hundredths of a percent
																	// (0 = silence, 1000 * 65536 = 100% = full)
																	{
																		double lin = (double)scale / 65536.0 / 1000.0;
																		if (lin < 0.0) lin = 0.0;
																		if (lin > 1.0) lin = 1.0;
																		envSustain = (float)lin;
																	}
																	envValid  |= 4;
																	break;
																case 0x0107: // EG1_RELEASE_TIME
																	envRelease = gmdls_tc_to_sec(scale);
																	envValid  |= 8;
																	break;
																}
															}
														}

														ap = aNext;
													}
												}

												cp = cNext;
											}

											// Apply collected data to the wave slot
											if (hasWlnk)
											{
												// Region-level wsmp overrides wave-level wsmp
												if (rHasLoop)
												{
													uint32_t numSamp = GMDLS_NumSamples[waveIdx];
													if (rLoopEnd > numSamp)
														rLoopEnd = numSamp;
													if (rLoopStart < rLoopEnd)
													{
														GMDLS_LoopData[waveIdx].loopStart      = rLoopStart;
														GMDLS_LoopData[waveIdx].loopEnd        = rLoopEnd;
														GMDLS_LoopData[waveIdx].loopType       = rLoopType;
														GMDLS_LoopData[waveIdx].sourcePriority = 2; // region-level
													}
												}
												// Envelope: last region's valid fields win
												if (envValid & 1) { GMDLS_EnvData[waveIdx].attack   = envAttack;  GMDLS_EnvData[waveIdx].validMask |= 1; }
												if (envValid & 2) { GMDLS_EnvData[waveIdx].decay    = envDecay;   GMDLS_EnvData[waveIdx].validMask |= 2; }
												if (envValid & 4) { GMDLS_EnvData[waveIdx].sustain  = envSustain; GMDLS_EnvData[waveIdx].validMask |= 4; }
												if (envValid & 8) { GMDLS_EnvData[waveIdx].release  = envRelease; GMDLS_EnvData[waveIdx].validMask |= 8; }
											}
										}

										rp = rNext;
									}
								}

								lp = lNext;
							}
						}

						ip = iNext;
					}
					break; // only one lins expected
				}

				p = chNext;
			}
		} while (0); // single-pass scope guard (allows early break on malformed file)
	}
#endif // COMPILE_VSTI
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
	unsigned int sse_control_store = k64_getcsr() & 0xffc0;
	// bits: 15 = flush to zero | 6 = denormals are zero
	// rounding 14:13, 00 = nearest, 01 = neg, 10 = pos, 11 = to zero
	// bits 12-7 exception masks
	k64_setcsr(0x8040 | 0x1f80 | ((unsigned int)3 << 13));

	while (samples--)
	{
		SynthGlobalState.GlobalNodes[0]->tick(SynthGlobalState.GlobalNodes[0]);
		*left++ = (float)SynthGlobalState.GlobalNodes[0]->out.d[0];
		*right++ = (float)SynthGlobalState.GlobalNodes[0]->out.d[1];
		SynthGlobalState.CurrentTick++;
	}

	// restore previous state
	k64_setcsr(sse_control_store);
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
	unsigned int sse_control_store = k64_getcsr() & 0xffc0;
	k64_setcsr(0x8040 | 0x1f80 | ((unsigned int)3 << 13));

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
	k64_setcsr(sse_control_store);
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
