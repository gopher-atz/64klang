#include "64klang_blob_api.h"
#include "Synth.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
// Internal state
///////////////////////////////////////////////////////////////////////////////

static void*  s_patchBuf    = NULL;
static void*  s_songBuf     = NULL;
static int    s_songSamples = 0;
static float  s_bpm         = 0.f;

///////////////////////////////////////////////////////////////////////////////
// k64_blob_init
///////////////////////////////////////////////////////////////////////////////

int k64_blob_init(const void* patchBlob, size_t patchSize,
                  const void* songBlob,  size_t songSize)
{
    // Minimum sanity: patch needs 3 × uint32 header, song needs 4 × uint32 header
    if (!patchBlob || patchSize < 12 || !songBlob || songSize < 16)
        return -1;

    // Release previous internal copies
    free(s_patchBuf);
    free(s_songBuf);
    s_patchBuf    = NULL;
    s_songBuf     = NULL;
    s_songSamples = 0;
    s_bpm         = 0.f;

    // Copy blobs — _64klang_Init delta-decodes the song stream in-place, so
    // we keep our own copies to allow the caller to reuse the original buffers
    // and to allow future re-inits with fresh data.
    s_patchBuf = malloc(patchSize);
    s_songBuf  = malloc(songSize);
    if (!s_patchBuf || !s_songBuf)
    {
        free(s_patchBuf); s_patchBuf = NULL;
        free(s_songBuf);  s_songBuf  = NULL;
        return -2;
    }
    memcpy(s_patchBuf, patchBlob, patchSize);
    memcpy(s_songBuf,  songBlob,  songSize);

    // Cache song header fields before _64klang_Init performs in-place
    // delta-decoding (bytes 16+ are modified; the 16-byte header stays intact).
    // Song blob layout: [float bpm][uint32 samples][uint32 framesize][uint32 deltasize][...]
    s_bpm         = *(const float*)   s_songBuf;
    s_songSamples = (int)(*(const uint32_t*)((const uint8_t*)s_songBuf + 4));

    // Hand the copies to the synth engine.
    // USE_BLOBS signature: _64klang_Init(songStream, patchData)
    // The engine reads the 3 uint32 offsets from the front of patchData
    // and advances the pointer past them internally.
    _64klang_Init((uint8_t*)s_songBuf, s_patchBuf);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// k64_blob_init_from_files
///////////////////////////////////////////////////////////////////////////////

int k64_blob_init_from_files(const char* patchBlobPath, const char* songBlobPath)
{
    if (!patchBlobPath || !songBlobPath)
        return -1;

    // Read patch blob
    FILE* fp = fopen(patchBlobPath, "rb");
    if (!fp)
        return -1;
    fseek(fp, 0, SEEK_END);
    long patchSize = ftell(fp);
    rewind(fp);
    void* patchData = malloc((size_t)patchSize);
    if (!patchData)
    {
        fclose(fp);
        return -2;
    }
    fread(patchData, 1, (size_t)patchSize, fp);
    fclose(fp);

    // Read song blob
    FILE* fs = fopen(songBlobPath, "rb");
    if (!fs)
    {
        free(patchData);
        return -1;
    }
    fseek(fs, 0, SEEK_END);
    long songSize = ftell(fs);
    rewind(fs);
    void* songData = malloc((size_t)songSize);
    if (!songData)
    {
        fclose(fs);
        free(patchData);
        return -2;
    }
    fread(songData, 1, (size_t)songSize, fs);
    fclose(fs);

    int result = k64_blob_init(patchData, (size_t)patchSize,
                               songData,  (size_t)songSize);
    free(patchData);
    free(songData);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// k64_blob_render
///////////////////////////////////////////////////////////////////////////////

void k64_blob_render(float* outBuffer)
{
    _64klang_Render(outBuffer);
}

///////////////////////////////////////////////////////////////////////////////
// k64_blob_song_length / k64_blob_bpm
///////////////////////////////////////////////////////////////////////////////

int k64_blob_song_length(void)
{
    return s_songSamples;
}

float k64_blob_bpm(void)
{
    return s_bpm;
}
