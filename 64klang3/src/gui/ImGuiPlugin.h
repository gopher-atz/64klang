#pragma once

// ImGui-based GUI for 64klang3
// This will be the main render loop for the plugin's editor.

namespace K64GUI {

// Initialize the ImGui GUI context
void init();

// Render one frame of the GUI
void render();

// Shutdown the ImGui GUI context
void shutdown();

// Check if GUI is initialized
bool isInitialized();

// Pass / retrieve the native window handle (HWND on Windows).
void  setWindowHandle(void* hwnd);
void* getWindowHandle();

} // namespace K64GUI
