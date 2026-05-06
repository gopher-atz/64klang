///////////////////////////////////////////////////////////////////////////////
// 64klang blob runtime-loading API
//
// Link against the 64klang_blob static library (built with USE_BLOBS defined).
// Patch and song data are loaded at runtime from .blob files exported by the
// 64klang3 VST3 plugin — no recompilation required when switching songs.
//
// Typical usage:
//   if (k64_blob_init_from_files("64k2Patch.h.blob", "64k2Song.h.blob") != 0)
//       ... handle error ...
//   float* buf = malloc(k64_blob_song_length() * 2 * sizeof(float));
//   k64_blob_render(buf);  // blocks until done
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

// Load patch and song from in-memory blobs.
// Safe to call multiple times; previous engine state is freed automatically.
// Both blob buffers are copied internally — the caller may free them after
// this function returns.
// Returns 0 on success, non-zero on error.
int k64_blob_init(const void* patchBlob, size_t patchSize,
                  const void* songBlob,  size_t songSize);

// Convenience: load patch and song blobs from files on disk.
// Returns 0 on success, non-zero on error.
int k64_blob_init_from_files(const char* patchBlobPath, const char* songBlobPath);

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

// Render the full song into outBuffer (interleaved L/R floats).
// outBuffer must hold at least k64_blob_song_length() * 2 floats.
// Blocks until rendering is complete.
void k64_blob_render(float* outBuffer);

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

// Returns song length in samples (stereo frames).  0 before a successful init.
int k64_blob_song_length(void);

// Returns the song BPM.  0.0f before a successful init.
float k64_blob_bpm(void);

#ifdef __cplusplus
} // extern "C"
#endif
