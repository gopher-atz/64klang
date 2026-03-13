#pragma once

// ImGui-based GUI for 64klang3
// This will be the main render loop for the plugin's editor.

namespace K64GUI {

// Create / destroy the canvas (call once from plugin initialize / terminate).
// The canvas persists across window open/close cycles so view state is preserved.
void createCanvas();
void destroyCanvas();

// Called on window attach / detach — arms and disarms rendering, does not touch the canvas.
void init();
void shutdown();

// Render one frame of the GUI
void render();

// Check if GUI is initialized
bool isInitialized();

// Pass / retrieve the native window handle (HWND on Windows).
void  setWindowHandle(void* hwnd);
void* getWindowHandle();

// Viewport state (pan + zoom) — saved/restored with the DAW project per instance.
void setViewport(float offsetX, float offsetY, float zoom);
void getViewport(float& offsetX, float& offsetY, float& zoom);

// Native file dialog — cross-platform (Win32 common dialog / macOS NSPanel / Linux zenity).
// filter   : Win32-style double-null-terminated filter string (only used on Windows).
// defExt   : default extension without dot (e.g. "64k2Channel").
// forSave  : true → save dialog, false → open dialog.
// Returns true and writes the chosen path into outBuf[outBufSz] on success.
bool openFileDialog(char* outBuf, int outBufSz,
                    const char* filter, const char* defExt,
                    bool forSave);

} // namespace K64GUI
