#ifdef _WIN32

#include "../PlatformInterfaces.h"
#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#pragma comment(lib, "msacm32.lib")

#include "SynthAllocator.h"

namespace K64Platform {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows ACM codec — GSM 6.10 conversion
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class WindowsCodec : public ICodec
{
public:
    int convertGSMtoPCM(
        uint32_t sampleRate,
        const uint8_t* srcBuffer, uint32_t srcBufferSize,
        uint8_t*& dstBuffer, uint32_t& dstBufferSize) override
    {
        GSM610WAVEFORMAT gsmFmt = {};
        gsmFmt.wfx.wFormatTag = WAVE_FORMAT_GSM610;
        gsmFmt.wfx.nChannels = 1;
        gsmFmt.wfx.nSamplesPerSec = sampleRate;
        gsmFmt.wfx.nBlockAlign = 65;
        gsmFmt.wfx.wBitsPerSample = 0;
        gsmFmt.wfx.cbSize = 2;
        gsmFmt.wSamplesPerBlock = 320;

        if (sampleRate == 44100) gsmFmt.wfx.nAvgBytesPerSec = 8957;
        else if (sampleRate == 22050) gsmFmt.wfx.nAvgBytesPerSec = 4478;
        else if (sampleRate == 11025) gsmFmt.wfx.nAvgBytesPerSec = 2239;

        WAVEFORMATEX pcmFmt = {};
        pcmFmt.wFormatTag = WAVE_FORMAT_PCM;
        pcmFmt.nChannels = 1;
        pcmFmt.nSamplesPerSec = sampleRate;
        pcmFmt.nAvgBytesPerSec = sampleRate * 2;
        pcmFmt.nBlockAlign = 2;
        pcmFmt.wBitsPerSample = 16;

        HACMSTREAM acmStream = NULL;
        if (acmStreamOpen(&acmStream, NULL, (LPWAVEFORMATEX)&gsmFmt,
                          &pcmFmt, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME))
            return -1;

        if (acmStreamSize(acmStream, srcBufferSize, (DWORD*)&dstBufferSize, ACM_STREAMSIZEF_SOURCE))
            return -1;

        dstBuffer = (uint8_t*)SynthMalloc(dstBufferSize);

        ACMSTREAMHEADER hdr = {};
        hdr.cbStruct = sizeof(ACMSTREAMHEADER);
        hdr.pbSrc = (LPBYTE)srcBuffer;
        hdr.cbSrcLength = srcBufferSize;
        hdr.pbDst = dstBuffer;
        hdr.cbDstLength = dstBufferSize;

        if (acmStreamPrepareHeader(acmStream, &hdr, 0))
        {
            SynthFree(dstBuffer);
            dstBuffer = nullptr;
            return -1;
        }

        if (acmStreamConvert(acmStream, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN))
        {
            SynthFree(dstBuffer);
            dstBuffer = nullptr;
            return -1;
        }

        acmStreamUnprepareHeader(acmStream, &hdr, 0);
        acmStreamClose(acmStream, 0);
        return 0;
    }

    int convertPCMtoGSM(
        uint32_t sampleRate,
        const uint8_t* srcBuffer, uint32_t srcBufferSize,
        uint8_t*& dstBuffer, uint32_t& dstBufferSize) override
    {
        // TODO: Implement PCM → GSM conversion if needed for export
        dstBuffer = nullptr;
        dstBufferSize = 0;
        return -1;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows GM.DLS loader
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class WindowsGMDLSLoader : public IGMDLSLoader
{
public:
    bool load() override
    {
        // GM.DLS loading is currently done inline in Synth.cpp
        // This will be refactored to use this interface in a future phase
        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows SAPI TTS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class WindowsTTSEngine : public ITTSEngine
{
public:
    uint8_t* synthesize(
        const std::string& text,
        uint32_t sampleRate,
        uint32_t& outSize) override
    {
        // SAPI TTS is currently done inline in SynthNode.cpp SAPI_tick
        // This will be refactored to use this interface in a future phase
        outSize = 0;
        return nullptr;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ICodec* createCodec()
{
    return new WindowsCodec();
}

IGMDLSLoader* createGMDLSLoader()
{
    return new WindowsGMDLSLoader();
}

ITTSEngine* createTTSEngine()
{
    return new WindowsTTSEngine();
}

} // namespace K64Platform

#endif // _WIN32
