# Windows-only: example host uses Win32 waveOut API.
# The static library itself is cross-platform; only the example host is gated.
set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/core)
set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform)

###############################################################################
# 64klang_blob — static library compiled with USE_BLOBS
#
# Full-feature build (no *_SKIP defines).  Patch + song data are loaded at
# runtime from .blob files, so no recompilation is required when switching
# songs.  No SynthController / TinyXML / ImGui dependency — lean embedding
# target suitable for dropping into editor tools and other host codebases.
###############################################################################

set(BLOB_PLATFORM_SRC "")
if(WIN32)
    set(BLOB_PLATFORM_SRC ${PLATFORM_DIR}/windows/WindowsPlatform.cpp)
elseif(APPLE OR UNIX)
    set(BLOB_PLATFORM_SRC ${PLATFORM_DIR}/stub/StubPlatform.cpp)
endif()

add_library(64klang_blob STATIC
    64klang_blob_api.cpp
    ${CORE_DIR}/sample_t.cpp
    ${CORE_DIR}/SynthNode.cpp
    ${CORE_DIR}/SynthAllocator.cpp
    ${CORE_DIR}/Synth.cpp
    ${BLOB_PLATFORM_SRC}
)

target_include_directories(64klang_blob PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}   # exposes 64klang_blob_api.h to consumers
    ${CORE_DIR}
    ${PLATFORM_DIR}
)

target_compile_definitions(64klang_blob PUBLIC USE_BLOBS)

if(WIN32)
    target_link_libraries(64klang_blob PUBLIC msacm32 wmvcore ole32 winmm)
endif()

if(MSVC)
    target_compile_options(64klang_blob PRIVATE
        /W3 /wd4244 /wd4267 /wd4018 /wd4996
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_options(64klang_blob PRIVATE /arch:SSE2)
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i386|AMD64")
        target_compile_options(64klang_blob PRIVATE -msse4.1)
    endif()
    target_compile_options(64klang_blob PRIVATE
        -Wno-conversion
        -Wno-sign-conversion
    )
endif()

###############################################################################
# BlobHostExample — minimal Win32 waveOut host demonstrating runtime blob loading
# (Windows only — waveOut is a Win32 API)
###############################################################################

if(WIN32)
    add_executable(BlobHostExample
        example_host.cpp
    )

    target_link_libraries(BlobHostExample PRIVATE 64klang_blob)

    if(MSVC)
        target_compile_options(BlobHostExample PRIVATE
            /W3 /wd4244 /wd4267 /wd4018
            $<$<CONFIG:Release>:/O1 /Os /Zi>
        )
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_compile_options(BlobHostExample PRIVATE /arch:SSE2)
        endif()
        target_link_options(BlobHostExample PRIVATE
            $<$<CONFIG:Release>:/SUBSYSTEM:CONSOLE /DEBUG>
        )
    endif()
endif()
