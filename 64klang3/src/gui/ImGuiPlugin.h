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

} // namespace K64GUI
