#include "ImGuiPlugin.h"
#include "NodeCanvas.h"
#include "NodeConfig.h"
#include "imgui.h"
#include "core/SynthController.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#elif defined(__APPLE__)
// Use Cocoa file panels via Objective-C; we call a C bridge defined below.
// The actual implementation must live in a .mm translation unit.
// Here we forward-declare the C bridge so this .cpp file can call it.
extern "C" bool k64_macOS_openFileDialog(char* outBuf, int bufSz,
                                          const char* ext, bool forSave);
#else
// Linux: invoke zenity or kdialog as a subprocess
#include <cstdio>    // popen / fgets
#include <cstring>   // strncpy
#endif

namespace K64GUI {

static bool        s_initialized    = false;
static NodeCanvas* s_canvas         = nullptr;
static void*       g_windowHandle   = nullptr;
static int         s_exportQuantIdx = 3;  // default = 64 samples (index 3), matches reference
static char        s_searchBuf[128] = {};

// ── public API ───────────────────────────────────────────────────────────────

void setWindowHandle(void* hwnd)
{
    g_windowHandle = hwnd;
}

void* getWindowHandle()
{
    return g_windowHandle;
}

void setViewport(float offsetX, float offsetY, float zoom)
{
    if (s_canvas)
        s_canvas->restoreViewport(offsetX, offsetY, zoom);
}

void getViewport(float& offsetX, float& offsetY, float& zoom)
{
    if (s_canvas)
    {
        s_canvas->getOffset(offsetX, offsetY);
        zoom = s_canvas->getZoom();
    }
    else
    {
        offsetX = offsetY = 0.f;
        zoom = 1.f;
    }
}

void createCanvas()
{
    if (!NodeConfig::instance().load())
        fprintf(stderr, "64klang3: Failed to parse embedded node config\n");

    if (!s_canvas)
        s_canvas = new NodeCanvas();
}

void destroyCanvas()
{
    delete s_canvas;
    s_canvas = nullptr;
}

void init()
{
    // Canvas already exists (created in createCanvas); just arm rendering.
    s_initialized = true;
}

void shutdown()
{
    // Disarm rendering but keep the canvas alive so view state survives
    // the window being closed and reopened by the DAW.
    s_initialized = false;
}

bool isInitialized()
{
    return s_initialized;
}

// ── helpers ──────────────────────────────────────────────────────────────────

// Opens a native file dialog.  Returns true and fills outBuf on success.
// filter is the Win32 double-null-terminated filter string (Windows only).
// defExt is the default file extension (without dot).
bool openFileDialog(char* outBuf, int outBufSz,
                    const char* filter, const char* defExt,
                    bool forSave)
{
#ifdef _WIN32
    OPENFILENAMEA ofn  = {};
    ofn.lStructSize    = sizeof(ofn);
    ofn.hwndOwner      = (HWND)g_windowHandle;
    ofn.lpstrFilter    = filter;
    ofn.lpstrFile      = outBuf;
    ofn.nMaxFile       = (DWORD)outBufSz;
    ofn.lpstrDefExt    = defExt;
    ofn.Flags          = OFN_NOCHANGEDIR
                       | (forSave ? OFN_OVERWRITEPROMPT
                                  : (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST));
    bool ok = forSave ? (GetSaveFileNameA(&ofn) != 0)
                      : (GetOpenFileNameA(&ofn)  != 0);
    // Flush any WM_KEY* messages that accumulated while the dialog pumped the
    // message loop; without this the host receives them as MIDI note triggers.
    { MSG m; while (PeekMessageA(&m, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {} }
    // Tell ImGui the left button was released; the dialog consumed the WM_LBUTTONUP
    // so ImGui never saw it and would otherwise require two clicks to interact again.
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    return ok;

#elif defined(__APPLE__)
    return k64_macOS_openFileDialog(outBuf, outBufSz, defExt, forSave);

#else
    // Linux: try zenity first, fall back to kdialog
    char cmd[512];
    if (forSave)
        snprintf(cmd, sizeof(cmd),
                 "zenity --file-selection --save --confirm-overwrite"
                 " --file-filter='*.%s' 2>/dev/null", defExt);
    else
        snprintf(cmd, sizeof(cmd),
                 "zenity --file-selection"
                 " --file-filter='*.%s' 2>/dev/null", defExt);

    FILE* fp = popen(cmd, "r");
    if (!fp)
    {
        // zenity not found — try kdialog
        if (forSave)
            snprintf(cmd, sizeof(cmd),
                     "kdialog --getsavefilename . '*.%s' 2>/dev/null", defExt);
        else
            snprintf(cmd, sizeof(cmd),
                     "kdialog --getopenfilename . '*.%s' 2>/dev/null", defExt);
        fp = popen(cmd, "r");
    }
    if (!fp)
        return false;

    outBuf[0] = '\0';
    if (fgets(outBuf, outBufSz, fp))
    {
        // Strip trailing newline
        int len = (int)strlen(outBuf);
        while (len > 0 && (outBuf[len-1] == '\n' || outBuf[len-1] == '\r'))
            outBuf[--len] = '\0';
    }
    pclose(fp);
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    return (outBuf[0] != '\0');
#endif
}

// Draw a thin vertical separator line at the current cursor X and advance past it.
static void toolbarSeparator(ImDrawList* dl, const ImVec2& tbPos, float tbH)
{
    ImGui::SameLine(0.f, 8.f);
    ImVec2 sep = ImGui::GetCursorScreenPos();
    dl->AddLine(ImVec2(sep.x, tbPos.y + 3.f),
                ImVec2(sep.x, tbPos.y + tbH - 4.f),
                IM_COL32(100, 100, 105, 200));
    ImGui::Dummy(ImVec2(1.f, 1.f));
    ImGui::SameLine(0.f, 8.f);
}

// ── toolbar ──────────────────────────────────────────────────────────────────

static void renderToolbar()
{
    SynthController* sc  = SynthController::instance();
    const float      tbH = 28.f;

    ImVec2 tbPos = ImGui::GetCursorScreenPos();
    float  tbW   = ImGui::GetContentRegionAvail().x;

    // Background strip + bottom separator line
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(tbPos, ImVec2(tbPos.x + tbW, tbPos.y + tbH),
                      IM_COL32(42, 42, 46, 255));
    dl->AddLine(ImVec2(tbPos.x,        tbPos.y + tbH - 1.f),
                ImVec2(tbPos.x + tbW,  tbPos.y + tbH - 1.f),
                IM_COL32(80, 80, 85, 255));

    // Compact button style + fixed small font
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.f, 3.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(3.f, 0.f));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    // Start 4 px from left, 4 px from top
    ImGui::SetCursorScreenPos(ImVec2(tbPos.x + 4.f, tbPos.y + 4.f));

    // ── Patch file operations ─────────────────────────────────────────────
    if (ImGui::Button("Load Patch"))
    {
        char buf[512] = {};
        if (openFileDialog(buf, 512,
                "64klang2 Patch\0*.64k2Patch\0All Files\0*.*\0",
                "64k2Patch", false) && sc)
        {
            sc->killVoices();
            sc->loadPatch(std::string(buf));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Patch"))
    {
        char buf[512] = {"MyPatch.64k2Patch"};
        if (openFileDialog(buf, 512,
                "64klang2 Patch\0*.64k2Patch\0All Files\0*.*\0",
                "64k2Patch", true) && sc)
            sc->savePatch(std::string(buf));
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Patch"))
    {
        if (sc)
        {
            sc->killVoices();
            sc->resetPatch(true, true);
        }
    }

    toolbarSeparator(dl, tbPos, tbH);


    if (ImGui::Button("Wavetables"))
    {
        if (s_canvas)
            s_canvas->toggleWaveFileDialog();
    }

    toolbarSeparator(dl, tbPos, tbH);

    if (ImGui::Button("Export Patch"))
    {
        char buf[512] = {"64k2Patch.h"};
        if (openFileDialog(buf, 512,
                "64klang2 Patch Header\0*.h\0All Files\0*.*\0",
                "h", true) && sc)
            sc->exportPatch(std::string(buf));
    }
    ImGui::SameLine();
    // Quantization values are raw sample counts (16..256 in steps of 16).
    // Default index 3 = 64 samples.
    static const int        kQuantValues[] = { 16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256 };
    static const char* kQuantLabels[]      = {"16","32","48","64","80","96","112","128","144","160","176","192","208","224","240","256"};
    if (ImGui::Button("Export Song"))
    {
        char buf[512] = {"64k2Song.h"};
        if (openFileDialog(buf, 512,
                "64klang2 Song Header\0*.h\0All Files\0*.*\0",
                "h", true) && sc)
            sc->exportSong(std::string(buf), kQuantValues[s_exportQuantIdx]);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(44.f);
    ImGui::Combo("##quant", &s_exportQuantIdx, kQuantLabels, 16, ImGuiComboFlags_HeightLargest);

    toolbarSeparator(dl, tbPos, tbH);

    // Indices 0-15 = channels 1-16, index 16 = SynthRoot (channel -1).
    ImGui::TextUnformatted("Jump To:");
    ImGui::SameLine(0.f, 3.f);
    {
        // Use BeginCombo so we can show a blank placeholder when nothing is selected.
        // Channel names are built only when the dropdown is actually opened.
        ImGui::SetNextItemWidth(150.f);
        if (ImGui::BeginCombo("##ch", "---", ImGuiComboFlags_HeightLargest))
        {
            // Single pass over the node list to collect ChannelRoot names.
            char        chBufs[16][64];
            const char* chLabels[17];
            std::string chNames[16];
            int nn = sc ? sc->numGUINodes() : 0;
            for (int i = 0; i < nn; i++)
            {
                if (sc->gnType(i) == CHANNELROOT_ID)
                {
                    int ch = sc->gnChannel(i);
                    if (ch >= 0 && ch < 16)
                        chNames[ch] = sc->gnName(i);
                }
            }
            for (int ch = 0; ch < 16; ch++)
            {
                if (chNames[ch].empty())
                    snprintf(chBufs[ch], sizeof(chBufs[ch]), "Channel %d", ch + 1);
                else
                    snprintf(chBufs[ch], sizeof(chBufs[ch]), "%d : %s", ch + 1, chNames[ch].c_str());
                chLabels[ch] = chBufs[ch];
            }
            chLabels[16] = "SynthRoot";

            for (int i = 0; i < 17; i++)
            {
                if (ImGui::Selectable(chLabels[i]))
                {
                    if (s_canvas)
                        s_canvas->jumpToChannel(i == 16 ? -1 : i);
                }
            }
            ImGui::EndCombo();
        }
    }

    toolbarSeparator(dl, tbPos, tbH);


    ImGui::TextUnformatted("Search:");
    ImGui::SameLine(0.f, 3.f);
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::InputText("##search", s_searchBuf, sizeof(s_searchBuf)) && s_canvas)
        s_canvas->setSearchFilter(s_searchBuf);

    {
        int  voices = sc ? sc->getNumActiveVoices() : 0;
        char voiceBuf[32];
        snprintf(voiceBuf, sizeof(voiceBuf), "Voices: %d", voices);

        float framePadX  = ImGui::GetStyle().FramePadding.x;
        float panicW     = ImGui::CalcTextSize("P.A.N.I.C").x + framePadX * 2.f;
        float voiceTextW = ImGui::CalcTextSize(voiceBuf).x;
        float rightEdge  = tbPos.x + tbW - 6.f;

        // P.A.N.I.C button (rightmost)
        ImGui::SetCursorScreenPos(ImVec2(
            rightEdge - panicW,
            tbPos.y + 4.f));
        if (ImGui::Button("P.A.N.I.C") && sc)
            sc->panic();

        // Voices label (left of P.A.N.I.C)
        ImGui::SetCursorScreenPos(ImVec2(
            rightEdge - panicW - 8.f - voiceTextW,
            tbPos.y + (tbH - ImGui::GetTextLineHeight()) * 0.5f));
        ImGui::TextUnformatted(voiceBuf);
    }

    ImGui::PopFont();
    ImGui::PopStyleVar(2);

    // Advance cursor past the toolbar so the canvas starts directly below.
    ImGui::SetCursorScreenPos(ImVec2(tbPos.x, tbPos.y + tbH));
}

void render()
{
    if (!s_initialized || !s_canvas)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##64klang3Canvas",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(2);

    renderToolbar();
    s_canvas->render();

    ImGui::End();
}

} // namespace K64GUI
