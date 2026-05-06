# Windows-only: Player uses Win32 waveOut API
if(NOT WIN32)
    message(STATUS "64klang3 Player: skipping (Windows only)")
    return()
endif()

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/core)
set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform)

###############################################################################
# Player executable
#
# Song data headers (64k2Patch.h and 64k2Song.h) are exported from the plugin
# and expected in PLAYER_SONG_DIR.  Default is this directory so you can drop
# them alongside main.cpp.  Override via:
#   cmake -DPLAYER_SONG_DIR=<path_to_headers>
###############################################################################

set(PLAYER_SONG_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH
    "Directory containing 64k2Patch.h and 64k2Song.h exported from the 64klang3 plugin")

# All synth sources compiled directly into the executable — one target, one
# set of flags, no CRT mismatch between a static lib and the exe.
# No COMPILE_VSTI: exposes _64klang_Render/_64klang_RenderDone instead of VSTi API.
# No TinyXML / SynthController: not needed for player builds.
add_executable(Player
    main.cpp
    ${CORE_DIR}/sample_t.cpp
    ${CORE_DIR}/SynthNode.cpp
    ${CORE_DIR}/SynthAllocator.cpp
    ${CORE_DIR}/Synth.cpp
    ${PLATFORM_DIR}/windows/WindowsPlatform.cpp
)

target_include_directories(Player PRIVATE
    ${CORE_DIR}
    ${PLATFORM_DIR}
    ${PLAYER_SONG_DIR}
)

target_link_libraries(Player PRIVATE msacm32 wmvcore ole32 winmm)

if(MSVC)
    target_compile_options(Player PRIVATE
        /W3 /wd4244 /wd4267 /wd4018
        $<$<CONFIG:Release>:/O1 /Os /GS- /GR- /EHs-c- /Gs65536 /Zi>
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32-bit: no-CRT build — custom entry point, no default libs, SSE2
        target_compile_options(Player PRIVATE /arch:SSE2)
        target_link_options(Player PRIVATE
            $<$<CONFIG:Release>:/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /NODEFAULTLIB /DEBUG>
        )
    else()
        # 64-bit: link CRT normally (sin/cos and other CRT calls are used)
        target_link_options(Player PRIVATE
            $<$<CONFIG:Release>:/SUBSYSTEM:CONSOLE /DEBUG>
        )
    endif()
endif()
