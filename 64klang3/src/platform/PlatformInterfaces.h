#pragma once

#include <cstdint>
#include <string>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform abstraction interfaces for 64klang3
// Win32 implementations in platform/windows/
// Cross-platform stubs in platform/stub/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace K64Platform {

// Audio codec interface — GSM 6.10 conversion
class ICodec
{
public:
    virtual ~ICodec() = default;

    // Convert GSM 6.10 to PCM 16-bit mono
    // Returns 0 on success, non-zero on failure
    virtual int convertGSMtoPCM(
        uint32_t sampleRate,
        const uint8_t* srcBuffer, uint32_t srcBufferSize,
        uint8_t*& dstBuffer, uint32_t& dstBufferSize) = 0;

    // Convert PCM 16-bit mono to GSM 6.10
    virtual int convertPCMtoGSM(
        uint32_t sampleRate,
        const uint8_t* srcBuffer, uint32_t srcBufferSize,
        uint8_t*& dstBuffer, uint32_t& dstBufferSize) = 0;
};

// GM.DLS sample loader interface
class IGMDLSLoader
{
public:
    virtual ~IGMDLSLoader() = default;

    // Load all wave samples from GM.DLS (or equivalent SF2)
    // Populates the GMDLS_NumSamples and GMDLS_SampleBuffer arrays
    virtual bool load() = 0;
};

// Text-to-speech engine interface
class ITTSEngine
{
public:
    virtual ~ITTSEngine() = default;

    // Synthesize text to 16-bit PCM audio
    // Returns nullptr and sets outSize=0 if TTS unavailable
    virtual uint8_t* synthesize(
        const std::string& text,
        uint32_t sampleRate,
        uint32_t& outSize) = 0;
};

// Factory function — returns platform-specific implementations
ICodec*       createCodec();
IGMDLSLoader* createGMDLSLoader();
ITTSEngine*   createTTSEngine();

} // namespace K64Platform
