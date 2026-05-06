cmake_minimum_required(VERSION 3.20)
# OBJC and OBJCXX are needed for .mm files on macOS
if(APPLE)
    project(64klang3 VERSION 3.0.0 LANGUAGES CXX C OBJC OBJCXX)
else()
    project(64klang3 VERSION 3.0.0 LANGUAGES CXX C)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable VST3 SDK examples and VSTGUI (we use ImGui). Must match option() names in vst3sdk CMakeLists.
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)

# Don't try to symlink to Program Files (needs admin)
set(SMTG_PLUGIN_TARGET_PATH "" CACHE STRING "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_MODULE_INFO OFF CACHE BOOL "" FORCE)

# Fetch all third-party dependencies
include(cmake/Dependencies.cmake)

# Required so smtg_target_add_library_main can add macmain.cpp (bundleEntry) on macOS
set(public_sdk_SOURCE_DIR ${vst3sdk_SOURCE_DIR}/public.sdk)

###############################################################################
# ImGui static library (built from fetched sources)
###############################################################################

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

# Platform backends (added per-platform)
if(WIN32)
    target_sources(imgui PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC ${IMGUI_DIR}/backends)
    target_link_libraries(imgui PUBLIC opengl32)
elseif(APPLE)
    target_sources(imgui PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_osx.mm
        ${IMGUI_DIR}/backends/imgui_impl_metal.mm
    )
    target_include_directories(imgui PUBLIC ${IMGUI_DIR}/backends)
    target_link_libraries(imgui PUBLIC
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
    )
    set_source_files_properties(
        ${IMGUI_DIR}/backends/imgui_impl_osx.mm
        ${IMGUI_DIR}/backends/imgui_impl_metal.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
else()
    # exports ModuleEntry/ModuleExit expected by VST3 hosts on Linux.
    set(public_sdk_SOURCE_DIR ${vst3sdk_SOURCE_DIR}/public.sdk)

    # Linux / other UNIX — X11 + OpenGL3
    target_sources(imgui PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC ${IMGUI_DIR}/backends)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(imgui PUBLIC OpenGL::GL ${X11_LIBRARIES})
endif()

###############################################################################
# 64klang Core — static library
###############################################################################

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

add_library(64klang_core STATIC
    ${CORE_DIR}/platform.h
    ${CORE_DIR}/sample_t.h
    ${CORE_DIR}/sample_t.cpp
    ${CORE_DIR}/SynthNode.h
    ${CORE_DIR}/SynthNode.cpp
    ${CORE_DIR}/SynthAllocator.h
    ${CORE_DIR}/SynthAllocator.cpp
    ${CORE_DIR}/Synth.h
    ${CORE_DIR}/Synth.cpp
    ${CORE_DIR}/SynthController.h
    ${CORE_DIR}/SynthController.cpp
    ${CORE_DIR}/tinyxml.h
    ${CORE_DIR}/tinyxml.cpp
    ${CORE_DIR}/tinystr.h
    ${CORE_DIR}/tinystr.cpp
    ${CORE_DIR}/tinyxmlerror.cpp
    ${CORE_DIR}/tinyxmlparser.cpp
)

target_include_directories(64klang_core PUBLIC
    ${CORE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)
target_compile_definitions(64klang_core PUBLIC COMPILE_VSTI TIXML_USE_STL)

# Platform-specific sources
if(WIN32)
    target_sources(64klang_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/WindowsPlatform.cpp
    )
else()
    target_sources(64klang_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/stub/StubPlatform.cpp
    )
endif()

# Compiler-specific flags
if(MSVC)
    target_compile_options(64klang_core PRIVATE
        /W3
        /wd4244   # conversion warnings (original code uses these extensively)
        /wd4267   # size_t to int conversion
        /wd4018   # signed/unsigned mismatch
    )
    # SSE4.1 is default on x64 MSVC, only needed for x86
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_options(64klang_core PRIVATE /arch:SSE2)
    endif()
else()
    # SSE4.1 only for x86/x64 (sample_t); ARM uses NEON (detected in platform.h via __aarch64__)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i386|AMD64")
        target_compile_options(64klang_core PRIVATE -msse4.1)
    endif()
    target_compile_options(64klang_core PRIVATE
        -Wno-conversion
        -Wno-sign-conversion
    )
endif()

# Platform-specific link libraries
if(WIN32)
    target_link_libraries(64klang_core PUBLIC msacm32 wmvcore ole32)
endif()

###############################################################################
# VST3 Plugin
###############################################################################

set(VST3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/vst3)
set(GUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gui)

set(VST3_COMMON_SOURCES
    ${VST3_DIR}/Factory.cpp
    ${VST3_DIR}/Plugin.cpp
    ${VST3_DIR}/Plugin.h
    ${VST3_DIR}/Controller.cpp
    ${VST3_DIR}/Controller.h
    ${VST3_DIR}/PluginView.cpp
    ${VST3_DIR}/PluginView.h
    ${GUI_DIR}/ImGuiPlugin.cpp
    ${GUI_DIR}/ImGuiPlugin.h
    ${GUI_DIR}/NodeCanvas.cpp
    ${GUI_DIR}/NodeCanvas.h
    ${GUI_DIR}/NodeConfig.cpp
    ${GUI_DIR}/NodeConfig.h
    ${GUI_DIR}/Widgets.cpp
    ${GUI_DIR}/Widgets.h
    ${GUI_DIR}/ArpEditor.cpp
    ${GUI_DIR}/ArpEditor.h
    ${GUI_DIR}/NodeConfigData.h
)

if(APPLE)
    list(APPEND VST3_COMMON_SOURCES ${VST3_DIR}/PluginView_macOS.mm)
    set_source_files_properties(
        ${VST3_DIR}/PluginView_macOS.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

set(SMTG_PACKAGE_ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/alcatraz.ico CACHE STRING "" FORCE)
smtg_add_vst3plugin(64klang3 ${VST3_COMMON_SOURCES})

target_link_libraries(64klang3 PRIVATE
    64klang_core
    imgui
    sdk
)

target_include_directories(64klang3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GUI_DIR}
)

if(APPLE)
    target_link_libraries(64klang3 PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
    )
    # So Finder shows .vst3 as package icon (like other VST3s), not as folder
    add_custom_command(TARGET 64klang3 POST_BUILD
        COMMAND SetFile -a B "${CMAKE_CURRENT_BINARY_DIR}/VST3/Release/64klang3.vst3"
        COMMENT "Set VST3 bundle attribute for Finder"
    )
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(64klang3 PRIVATE OpenGL::GL ${X11_LIBRARIES} pthread)
endif()

if(MSVC)
    target_compile_options(64klang3 PRIVATE /wd4244 /wd4267)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(64klang3 PRIVATE -Wno-conversion -Wno-sign-conversion)
endif()

###############################################################################
# DebugExe — standalone test host (Windows only)
###############################################################################

if(WIN32)
    set(DEBUG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/DebugExe)
    if(EXISTS ${DEBUG_DIR}/Exe.cpp)
        add_executable(DebugExe WIN32
            ${DEBUG_DIR}/Exe.cpp
            ${GUI_DIR}/ImGuiPlugin.cpp
            ${GUI_DIR}/ImGuiPlugin.h
            ${GUI_DIR}/NodeCanvas.cpp
            ${GUI_DIR}/NodeCanvas.h
            ${GUI_DIR}/NodeConfig.cpp
            ${GUI_DIR}/NodeConfig.h
            ${GUI_DIR}/Widgets.cpp
            ${GUI_DIR}/Widgets.h
            ${GUI_DIR}/ArpEditor.cpp
            ${GUI_DIR}/ArpEditor.h
            ${GUI_DIR}/NodeConfigData.h
        )
        target_link_libraries(DebugExe PRIVATE
            64klang_core
            imgui
            opengl32
            winmm
        )
        target_include_directories(DebugExe PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${GUI_DIR}
            ${DEBUG_DIR}
        )
        if(MSVC)
            target_compile_options(DebugExe PRIVATE /wd4244 /wd4267)
        endif()
    endif()
endif()

###############################################################################
# Player — size-optimised standalone playback executable (Windows only)
###############################################################################

if(WIN32)
    add_subdirectory(player)
endif()

###############################################################################
# BlobPlayer — runtime blob-loading static lib + example host (Windows only)
###############################################################################

if(WIN32)
    add_subdirectory(blobplayer)
endif()
