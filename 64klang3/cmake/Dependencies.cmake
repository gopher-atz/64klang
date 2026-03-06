include(FetchContent)

FetchContent_Declare(vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG        v3.7.11_build_10
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.6
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(vst3sdk imgui)
