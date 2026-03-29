#include "Widgets.h"
#include "imgui.h"
#include "core/SynthNode.h"
#include "core/SynthController.h"

#ifndef __APPLE__
#  include <GL/gl.h>
#  ifndef GL_CLAMP_TO_EDGE
#    define GL_CLAMP_TO_EDGE 0x812F
#  endif
#  ifndef GL_RGBA8
#    define GL_RGBA8 0x8058
#  endif
#  ifndef GL_LINEAR_MIPMAP_LINEAR
#    define GL_LINEAR_MIPMAP_LINEAR 0x2703
#  endif
// glGenerateMipmap is GL 3.0 — not in the GL 1.1 header; load lazily.
#  ifdef _WIN32
#    include <wingdi.h>
typedef void (APIENTRY* PFNGLGENERATEMIPMAPPROC)(unsigned int target);
static PFNGLGENERATEMIPMAPPROC s_glGenerateMipmap = nullptr;
static inline void glGenerateMipmap_lazy(unsigned int target) {
    if (!s_glGenerateMipmap)
        s_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
    if (s_glGenerateMipmap) s_glGenerateMipmap(target);
}
#    define glGenerateMipmap glGenerateMipmap_lazy
#  else
// Linux: use the function directly (linked via OpenGL::GL which includes GL 3+)
extern "C" void glGenerateMipmap(unsigned int target);
#  endif
#endif

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
#include <unordered_map>

namespace K64GUI {
namespace Widgets {

// ── Display format helpers ───────────────────────────────────────────────

static KnobLabel defaultFormat(double value)
{
    // value is already in display units (e.g. 0..128 or -64..64).
    int intPart = (int)value;
    double fracPart = std::abs(value - (double)intPart);
    int fracInt = (int)(fracPart * 100.0);
    if (fracInt == 0)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", intPart);
        return { buf, "" };
    }
    char l1[32], l2[32];
    snprintf(l1, sizeof(l1), "%d.", intPart);
    snprintf(l2, sizeof(l2), "%02d", fracInt);
    return { l1, l2 };
}

static KnobLabel truncatedMs(double ms)
{
    double tailscale = 10000.0;
    if (ms >= 10.0)  tailscale = 1000.0;
    if (ms >= 100.0) tailscale = 100.0;
    if (ms >= 1000.0) tailscale = 10.0;
    ms = std::floor(ms * tailscale) / tailscale;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.4g", ms);
    return { buf, "ms" };
}

static KnobLabel truncatedHz(double freq)
{
    double tailscale = 1000.0;
    if (freq >= 100.0)   tailscale = 100.0;
    if (freq >= 1000.0)  tailscale = 100.0;
    if (freq >= 10000.0) tailscale = 10.0;
    freq = std::floor(freq * tailscale) / tailscale;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.4g", freq);
    return { buf, "hz" };
}

static const char* BPM_TIME_NAMES[] = {
    "1/128", "1/64T", "1/128D", "1/64",
    "1/32T", "1/64D", "1/32",   "1/16T",
    "1/32D", "1/16",  "1/8T",   "1/16D",
    "1/8",   "1/4T",  "1/8D",   "1/4",
    "1/2T",  "1/4D",  "1/2",    "1T",
    "1/2D",  "1",     "1D",     "3/8",
    "5/8",   "7/8",   "9/8",    "11/8",
    "13/8",  "15/8",  "3/4",    "5/4",
    "7/4"
    };

static const char* GLIDE_TIME_NAMES[] = {
    "OFF",   "1/64T", "1/128D", "1/64",
    "1/32T", "1/64D", "1/32",   "1/16T",
    "1/32D", "1/16",  "1/8T",   "1/16D",
    "1/8",   "1/4T",  "1/8D",   "1/4",
    "1/2T",  "1/4D",  "1/2",    "1T",
    "1/2D",  "1",     "1D",     "3/8",
    "5/8",   "7/8",   "9/8",    "11/8",
    "13/8",  "15/8",  "3/4",    "5/4",
    "Instant"
};

KnobLabel formatKnobValue(double normValue, double range, int mapping, int currentMode, int nodeTypeID)
{
    double norm = normValue / range;

    // Constants/voice inputs: just show norm
    if (nodeTypeID >= 64)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.6g", norm);
        return { buf, "" };
    }

    // Special ADSR Gain with dB mode (bit 128)
    if (nodeTypeID == 5 && mapping == 0 && (currentMode & 128) != 0)
    {
        double db = (norm - 0.75) * 128.0;
        db = std::floor(db * 100.0) / 100.0;
        char buf[64];
        snprintf(buf, sizeof(buf), "%.2f", db);
        return { buf, "dB" };
    }

    switch (mapping)
    {
    case 0: // Default
        return defaultFormat(normValue);

    case 1: // ADSR_Rate
    {
        double samples = norm * norm * norm * 418381.824 + 1.0;
        double ms = samples / 44.1;
        return truncatedMs(ms);
    }

    case 2: // LFO_Frequency
    {
        if ((currentMode & 2048) != 0)
        {
            double scale = std::pow(2.0, (norm - 0.5) * 128.0 / 12.0);
            char buf[64];
            if (scale >= 1.0)
            {
                scale = std::floor(scale * 100.0) / 100.0;
                snprintf(buf, sizeof(buf), "%.2f", scale);
                return { "BPM *", buf };
            }
            else
            {
                scale = 1.0 / scale;
                scale = std::floor(scale * 100.0) / 100.0;
                snprintf(buf, sizeof(buf), "%.2f", scale);
                return { "BPM /", buf };
            }
        }
        else
        {
            double freq = 44100.0 * norm * norm * norm * norm * norm * 0.001;
            return truncatedHz(freq);
        }
    }

    case 3: // OSC_Frequency
    {
        if ((currentMode & 2048) == 0)
        {
            double freq = 44100.0 * 0.5 * norm;
            return truncatedHz(freq);
        }
        else
        {
            return defaultFormat(normValue);
        }
    }

    case 4: // BQF_Frequency
    {
        double scale = 1.0;
        if (nodeTypeID == 43) // SVFilter
            scale = 0.63;

        if ((currentMode & 128) == 0)
        {
            double freq;
            if ((currentMode & 64) != 0)
                freq = 44100.0 * std::min(norm * norm, 0.99765) * 0.5 * scale;
            else
                freq = 44100.0 * std::min(1.0 / std::pow(2.0, (1.0 - norm) * 10.0), 0.99765) * 0.5 * scale;
            return truncatedHz(freq);
        }
        else
        {
            return defaultFormat(normValue);
        }
    }

    case 5: // Attack
    {
        double ms = std::max(norm * 128.0, 0.00128);
        return truncatedMs(ms);
    }

    case 6: // Release
    {
        double ms = std::max(norm * 128.0 * 10.0, 0.00128);
        return truncatedMs(ms);
    }

    case 7: // Delay_Time
    {
        int delaymode;
        if (nodeTypeID == 3 || nodeTypeID == 49) // VoiceManager or Glitch
            delaymode = 0;
        else
            delaymode = currentMode & 0xf;

        switch (delaymode)
        {
        case 0: // BPM sync
        {
            int i = std::min((int)(norm * 128.0) / 4, 32);
            return { BPM_TIME_NAMES[i], "" };
        }
        case 1: // Short
        {
            double ms = norm * 1640.0 / 44.1;
            return truncatedMs(ms);
        }
        case 2: // Middle
        {
            double ms = norm * 5644.8 / 44.1;
            return truncatedMs(ms);
        }
        case 3: // Long
        {
            double ms = norm * 44100.0 * 2.0 / 44.1;
            return truncatedMs(ms);
        }
        case 4: // Notemap
            return defaultFormat(-normValue + range / 2.0);
        case 5: // Notemap2
            return defaultFormat(normValue);
        default:
            return defaultFormat(normValue);
        }
    }

    case 8: // Decibel
    {
        double db = (norm - 0.75) * 128.0;
        db = std::floor(db * 100.0) / 100.0;
        char buf[64];
        snprintf(buf, sizeof(buf), "%.2f", db);
        return { buf, "dB" };
    }

    case 9: // Ratio
    {
        double ratio = std::pow(2.0, norm * 6.0);
        char buf[64];
        if (ratio <= 1.0)
        {
            ratio = 1.0 / ratio;
            ratio = std::floor(ratio * 100.0) / 100.0;
            snprintf(buf, sizeof(buf), "%.2f : 1", ratio);
        }
        else
        {
            ratio = std::floor(ratio * 100.0) / 100.0;
            snprintf(buf, sizeof(buf), "1 : %.2f", ratio);
        }
        return { buf, "" };
    }

    case 10: // Speed
    {
        double speed = std::pow(2.0, norm * 128.0 / 12.0);
        speed = std::floor(speed * 100.0) / 100.0;
        char buf[64];
        snprintf(buf, sizeof(buf), "%.2f", speed);
        return { buf, "" };
    }

    case 11: // RecordTime
    {
        double samples = norm * 1024.0 * 512.0;
        double ms = samples / 44.1;
        ms = std::floor(ms * 10.0) / 10.0;
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f", ms);
        return { buf, "ms" };
    }

    case 12: // GlideTime
    {
        int i = std::min((int)(norm * 128.0) / 4, 32);
        return { GLIDE_TIME_NAMES[i], "" };
    }

    case 13: // Normalized
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.6g", norm);
        return { buf, "" };
    }

    case 14: // SNH_Frequency
    {
        if ((currentMode & 16) != 0) // Trigger
        {
            return { (norm < 0.5) ? "0" : "1", "" };
        }
        else if ((currentMode & 128) == 0) // Free frequency
        {
            double freq;
            if ((currentMode & 64) != 0)
                freq = 44100.0 * std::min(norm * norm, 0.99765) * 0.5;
            else
                freq = 44100.0 * std::min(1.0 / std::pow(2.0, (1.0 - norm) * 10.0), 0.99765) * 0.5;
            return truncatedHz(freq);
        }
        else // Note transpose
        {
            double val = (-0.25 + norm) * 128.0;
            val = std::floor(val * 100.0) / 100.0;
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", val);
            return { buf, "" };
        }
    }

    default:
        return defaultFormat(normValue);
    }
}

static constexpr ImU32 kColKnobBody   = IM_COL32( 40,  40,  45, 255);
static constexpr ImU32 kColKnobRim    = IM_COL32( 80,  80,  85, 255);
static constexpr ImU32 kColKnobCenter = IM_COL32(100, 100, 105, 255);
static constexpr ImU32 kColKnobNeedle = IM_COL32(255, 255, 255, 230);
static constexpr ImU32 kColKnobMod    = IM_COL32(255,  60,  60, 200);
static constexpr ImU32 kColKnobTick   = IM_COL32( 20,  20,  20, 220);

void drawKnob(ImDrawList* dl, ImVec2 center, float bodyR, float knobR,
              float needleTipR, float normVal, float normMod, bool showMod,
              unsigned int alpha, float z)
{
    static constexpr float PI         = 3.14159265359f;
    static constexpr float sweepDeg   = 280.f;
    static constexpr float startAngle = 180.f + (360.f - sweepDeg) * 0.5f;

    static const struct { float a; int t; } kKnobTicks[] = {
        {   0.f, 2 },
        {  17.5f, 0 }, {  35.f, 1 }, {  52.5f, 0 }, {  70.f, 2 },
        {  87.5f, 0 }, { 105.f, 1 }, { 122.5f, 0 }, { 140.f, 2 },
        { -17.5f, 0 }, { -35.f, 1 }, { -52.5f, 0 }, { -70.f, 2 },
        { -87.5f, 0 }, {-105.f, 1 }, {-122.5f, 0 }, {-140.f, 2 },
    };

    const float tickOffX = -0.2f * z;
    unsigned int tickA = (unsigned int)((kColKnobTick >> 24) * alpha / 255);
    ImU32 tickCol = (kColKnobTick & 0x00FFFFFF) | (tickA << 24);
    for (const auto& tk : kKnobTicks)
    {
        float rad = tk.a * PI / 180.f;
        float rO = (tk.t == 2) ? knobR : (tk.t == 1) ? knobR * (23.f/25.f) : knobR * (22.5f/25.f);
        float rI = knobR * (19.f / 25.f);
        float th = (tk.t == 2) ? 2.f * z : (tk.t == 1) ? 1.2f * z : 0.75f * z;
        dl->AddLine(ImVec2(center.x + tickOffX + sinf(rad) * rO, center.y - cosf(rad) * rO),
                    ImVec2(center.x + tickOffX + sinf(rad) * rI, center.y - cosf(rad) * rI),
                    tickCol, th);
    }

    dl->AddCircleFilled(center, bodyR, (kColKnobBody & 0x00FFFFFF) | (alpha << 24));
    dl->AddCircle(center, bodyR, (kColKnobRim  & 0x00FFFFFF) | (alpha << 24), 0, 1.5f);

    float angleRad = (startAngle + normVal * sweepDeg) * PI / 180.f;
    float sa = sinf(angleRad), ca = cosf(angleRad);
    float hw = 2.5f * z;
    unsigned int needleA = (unsigned int)(230 * alpha / 255);
    dl->AddTriangleFilled(
        ImVec2(center.x - ca * hw, center.y - sa * hw),
        ImVec2(center.x + ca * hw, center.y + sa * hw),
        ImVec2(center.x + sa * needleTipR, center.y - ca * needleTipR),
        (kColKnobNeedle & 0x00FFFFFF) | (needleA << 24));
    dl->AddCircleFilled(center, hw, (kColKnobCenter & 0x00FFFFFF) | (alpha << 24));

    if (showMod)
    {
        float mr = (startAngle + normMod * sweepDeg) * PI / 180.f;
        float sm = sinf(mr), cm = cosf(mr), mhw = 1.5f * z;
        dl->AddTriangleFilled(
            ImVec2(center.x - cm * mhw, center.y - sm * mhw),
            ImVec2(center.x + cm * mhw, center.y + sm * mhw),
            ImVec2(center.x + sm * needleTipR, center.y - cm * needleTipR),
            kColKnobMod);
    }
}

} // namespace Widgets

// ── Specialized edit-panel section drawers ───────────────────────────────────

void Widgets::drawTriggerSeqPanel(const EditPanelCtx& ctx, float& curY, SynthController* sc)
{
    ImDrawList* dl       = ctx.dl;
    int         nodeID   = ctx.nodeID;
    float       px       = ctx.px;
    float       pw       = ctx.pw;
    float       z        = ctx.z;
    float       fontSize = ctx.fontSize;
    ImVec2      mousePos = ctx.mousePos;
    bool        canClick = ctx.canClick;

    int modeWord    = sc->getInputMode((DWORD)nodeID, TRIGGERSEQ_MODE);
    int maxPatterns = modeWord & (int)TRIGGERSEQ_COUNTMASK;
    if (maxPatterns < 1 || maxPatterns > 16) maxPatterns = 16;
    int bpmIdx      = (modeWord & (int)TRIGGERSEQ_BPMMASK) >> 8;
    if (bpmIdx < 0 || bpmIdx > 32) bpmIdx = 0;

    float ctrlH  = 20.f * z;
    float cellH  = 14.f * z;
    float labelW  = 18.f * z;
    float cellGap = 4.f * z;
    float cellW   = (pw - 8.f*z - labelW - cellGap) / 16.f;

    curY += 4.f * z;

    // ── Max Patterns row ──
    if (fontSize >= 6.f)
        dl->AddText(pickFont(fontSize), fontSize,
                    ImVec2(px + 4.f*z, curY + (ctrlH - fontSize) * 0.5f),
                    kColPanelText, "Max Patterns:");

    float btnSz = 16.f * z, btnGap = 2.f * z;
    char mpVal[8]; snprintf(mpVal, sizeof(mpVal), "%d", maxPatterns);
    ImVec2 plusMin (px + pw - btnSz - 4.f*z,           curY + (ctrlH - btnSz)*0.5f);
    ImVec2 plusMax (plusMin.x + btnSz,                  plusMin.y + btnSz);
    ImVec2 minusMin(plusMin.x - btnGap - btnSz,         plusMin.y);
    ImVec2 minusMax(minusMin.x + btnSz,                 plusMin.y + btnSz);
    ImVec2 valMin  (minusMin.x - btnGap - btnSz*1.2f,  plusMin.y);
    ImVec2 valMax  (valMin.x + btnSz*1.2f,              plusMin.y + btnSz);
    dl->AddRectFilled(minusMin, minusMax, IM_COL32(70, 70, 80, 255));
    dl->AddRectFilled(plusMin,  plusMax,  IM_COL32(70, 70, 80, 255));
    dl->AddRectFilled(valMin,   valMax,   IM_COL32(50, 50, 58, 255));
    if (fontSize >= 6.f)
    {
        float fw;
        fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, "-").x;
        dl->AddText(pickFont(fontSize), fontSize,
                    ImVec2(minusMin.x + (btnSz-fw)*0.5f, minusMin.y + (btnSz-fontSize)*0.5f),
                    IM_COL32(255,255,255,255), "-");
        fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, "+").x;
        dl->AddText(pickFont(fontSize), fontSize,
                    ImVec2(plusMin.x + (btnSz-fw)*0.5f, plusMin.y + (btnSz-fontSize)*0.5f),
                    IM_COL32(255,255,255,255), "+");
        fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, mpVal).x;
        dl->AddText(pickFont(fontSize), fontSize,
                    ImVec2(valMin.x + (valMax.x-valMin.x-fw)*0.5f, valMin.y + (btnSz-fontSize)*0.5f),
                    IM_COL32(220,220,220,255), mpVal);
    }
    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (mousePos.x >= minusMin.x && mousePos.x < minusMax.x &&
            mousePos.y >= minusMin.y && mousePos.y < minusMax.y && maxPatterns > 1)
            sc->setInputMode((DWORD)nodeID, TRIGGERSEQ_MODE,
                             (DWORD)(maxPatterns - 1), (DWORD)TRIGGERSEQ_COUNTMASK);
        if (mousePos.x >= plusMin.x && mousePos.x < plusMax.x &&
            mousePos.y >= plusMin.y && mousePos.y < plusMax.y && maxPatterns < 16)
            sc->setInputMode((DWORD)nodeID, TRIGGERSEQ_MODE,
                             (DWORD)(maxPatterns + 1), (DWORD)TRIGGERSEQ_COUNTMASK);
    }
    curY += ctrlH;

    // ── BPM sync row ──
    {
        float btnX = px + 6.f * z;
        float btnW = pw - 12.f * z;
        float btnH = (ctrlH - 2.f * z);
        float btnY = curY + 1.f * z;
        ImVec2 btnMin(btnX, btnY);
        ImVec2 btnMax(btnX + btnW, btnY + btnH);
        bool hov = mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                   mousePos.y >= btnMin.y && mousePos.y <= btnMax.y;
        dl->AddRectFilled(btnMin, btnMax, hov ? IM_COL32(60,60,70,255) : IM_COL32(40,40,48,255));
        dl->AddRect(btnMin, btnMax, IM_COL32(120,120,130,255), 0.f, 0, 1.f);

        if (fontSize >= 6.f)
        {
            char label[64];
            snprintf(label, sizeof(label), "BPM Sync: %s", BPM_TIME_NAMES[bpmIdx]);
            float lfsz = fontSize * 0.9f;
            float labelY = btnY + (btnH - lfsz) * 0.5f;
            dl->AddText(pickFont(lfsz), lfsz, ImVec2(btnX + 4.f*z, labelY),
                        IM_COL32(220,220,230,255), label);
        }

        float arrMidX = btnMax.x - 10.f*z;
        float arrMidY = btnY + btnH * 0.5f;
        float arrHalf = 4.f * z;
        dl->AddTriangleFilled(
            ImVec2(arrMidX - arrHalf, arrMidY - arrHalf * 0.5f),
            ImVec2(arrMidX + arrHalf, arrMidY - arrHalf * 0.5f),
            ImVec2(arrMidX, arrMidY + arrHalf * 0.5f),
            IM_COL32(200, 200, 210, 255));

        char popupId[64];
        snprintf(popupId, sizeof(popupId), "##tsbpm%d", nodeID);

        if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hov)
            ImGui::OpenPopup(popupId);

        {
            float itemH = fontSize * 1.35f;
            float padY  = ImGui::GetStyle().WindowPadding.y * 2.f;
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, itemH * 8.f + padY));
        }
        ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y));
        ImGui::PushFont(pickFont(fontSize));
        if (ImGui::BeginPopup(popupId))
        {
            ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
            for (int i = 0; i < 33; i++)
            {
                bool sel = (i == bpmIdx);
                if (ImGui::Selectable(BPM_TIME_NAMES[i], sel, 0, ImVec2(btnW, 0)))
                    sc->setInputMode((DWORD)nodeID, TRIGGERSEQ_MODE,
                                     (DWORD)(i << 8), (DWORD)TRIGGERSEQ_BPMMASK);
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndPopup();
        }
        ImGui::PopFont();
    }
    curY += ctrlH;

    // ── Column header ──
    {
        float gridX = px + 4.f*z + labelW;
        float hdrH  = 12.f * z;
        float smallFsz = fontSize * 0.85f;
        for (int t = 0; t < 8; t++)
        {
            char tn[4]; snprintf(tn, sizeof(tn), "%d", t+1);
            float tx = gridX + t * cellW + cellW * 0.5f;
            float tw = pickFont(smallFsz)->CalcTextSizeA(smallFsz, FLT_MAX, 0.f, tn).x;
            if (smallFsz >= 5.f)
                dl->AddText(pickFont(smallFsz), smallFsz,
                            ImVec2(tx - tw*0.5f, curY + (hdrH - smallFsz)*0.5f),
                            kColPanelDimText, tn);
            float rx = gridX + 8.f*cellW + cellGap + t*cellW + cellW*0.5f;
            if (smallFsz >= 5.f)
                dl->AddText(pickFont(smallFsz), smallFsz,
                            ImVec2(rx - tw*0.5f, curY + (hdrH - smallFsz)*0.5f),
                            kColPanelDimText, tn);
        }
        curY += hdrH;
    }

    // ── Live playback cursor ──
    int playPos     = sc->getTriggerSeqPlayPos((DWORD)nodeID);
    int liveTick    = (playPos >= 0) ? (playPos & 0xFF)        : -1;
    int livePattern = (playPos >= 0) ? ((playPos >> 8) & 0xFF) : -1;

    // ── Pattern rows ──
    for (int p = 0; p < maxPatterns; p++)
    {
        char rowLbl[8]; snprintf(rowLbl, sizeof(rowLbl), "%d", p + 1);
        if (fontSize >= 6.f)
        {
            float lw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, rowLbl).x;
            dl->AddText(pickFont(fontSize), fontSize,
                        ImVec2(px + 4.f*z + (labelW - 4.f*z - lw)*0.5f,
                               curY + (cellH - fontSize)*0.5f),
                        kColPanelDimText, rowLbl);
        }

        if (p == livePattern)
            dl->AddRectFilled(ImVec2(px + 4.f*z, curY),
                              ImVec2(px + 4.f*z + labelW, curY + cellH),
                              IM_COL32(255, 220, 50, 30));

        int wordBase = p / 4;
        int byteOfs  = (p % 4) * 8;
        int lWordIdx = TRIGGERSEQ_PATTERN0_3L + wordBase;
        int rWordIdx = TRIGGERSEQ_PATTERN0_3R + wordBase;
        int lWord    = sc->getInputMode((DWORD)nodeID, (DWORD)lWordIdx);
        int rWord    = sc->getInputMode((DWORD)nodeID, (DWORD)rWordIdx);

        float gridX = px + 4.f*z + labelW;

        for (int t = 0; t < 8; t++)
        {
            bool lActive = (lWord >> (byteOfs + t)) & 1;
            float lcx = gridX + t * cellW;
            ImVec2 lcMin(lcx + 1.f, curY + 1.f);
            ImVec2 lcMax(lcx + cellW - 1.f, curY + cellH - 1.f);
            dl->AddRectFilled(lcMin, lcMax,
                lActive ? IM_COL32(100, 200, 100, 255) : IM_COL32(45, 50, 45, 255));
            dl->AddRect(lcMin, lcMax, IM_COL32(30, 30, 30, 160));
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                mousePos.x >= lcMin.x && mousePos.x < lcMax.x &&
                mousePos.y >= lcMin.y && mousePos.y < lcMax.y)
            {
                int bit     = byteOfs + t;
                int newWord = lWord ^ (1 << bit);
                sc->setInputMode((DWORD)nodeID, (DWORD)lWordIdx,
                                 (DWORD)newWord, (DWORD)(1 << bit));
            }

            bool rActive = (rWord >> (byteOfs + t)) & 1;
            float rcx = gridX + 8.f*cellW + cellGap + t * cellW;
            ImVec2 rcMin(rcx + 1.f, curY + 1.f);
            ImVec2 rcMax(rcx + cellW - 1.f, curY + cellH - 1.f);
            dl->AddRectFilled(rcMin, rcMax,
                rActive ? IM_COL32(100, 160, 220, 255) : IM_COL32(45, 45, 55, 255));
            dl->AddRect(rcMin, rcMax, IM_COL32(30, 30, 30, 160));
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                mousePos.x >= rcMin.x && mousePos.x < rcMax.x &&
                mousePos.y >= rcMin.y && mousePos.y < rcMax.y)
            {
                int bit     = byteOfs + t;
                int newWord = rWord ^ (1 << bit);
                sc->setInputMode((DWORD)nodeID, (DWORD)rWordIdx,
                                 (DWORD)newWord, (DWORD)(1 << bit));
            }
        }

        if (p == livePattern && liveTick >= 0 && liveTick < 8)
        {
            float lcx = gridX + liveTick * cellW;
            float rcx = gridX + 8.f*cellW + cellGap + liveTick * cellW;
            dl->AddRect(ImVec2(lcx, curY), ImVec2(lcx + cellW, curY + cellH),
                        IM_COL32(255, 220, 50, 220), 0.f, 0, 1.5f);
            dl->AddRect(ImVec2(rcx, curY), ImVec2(rcx + cellW, curY + cellH),
                        IM_COL32(255, 220, 50, 220), 0.f, 0, 1.5f);
        }

        curY += cellH;
    }
    curY += 4.f * z;
}

void Widgets::drawSAPIPanel(const EditPanelCtx& ctx, float& curY, SynthController* sc,
                             std::unordered_map<int, std::array<char, 4096>>& textEditBuffers)
{
    ImDrawList* dl       = ctx.dl;
    int         nodeID   = ctx.nodeID;
    float       px       = ctx.px;
    float       pw       = ctx.pw;
    float       z        = ctx.z;
    float       fontSize = ctx.fontSize;
    ImVec2      mousePos = ctx.mousePos;
    bool        canClick = ctx.canClick;

    if (textEditBuffers.find(nodeID) == textEditBuffers.end())
    {
        std::string txt = sc->getSAPIText((DWORD)nodeID);
        auto& arr = textEditBuffers[nodeID];
        arr.fill(0);
        txt.copy(arr.data(), std::min(txt.size(), arr.size() - 1));
    }
    auto& buf = textEditBuffers[nodeID];

    float textAreaH = 72.f * z;
    float btnH      = 20.f * z;
    float btnW      = 70.f * z;

    dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), kColPanelBorder, 0.5f);
    curY += 4.f * z;

    ImGui::SetCursorScreenPos(ImVec2(px + 4.f*z, curY));
    ImGui::PushFont(pickFont(fontSize));
    ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
    std::string textId = "##sapitext" + std::to_string(nodeID);
    ImGui::InputTextMultiline(textId.c_str(), buf.data(), buf.size(),
                              ImVec2(pw - 8.f*z, textAreaH));
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopFont();
    curY += textAreaH + 4.f * z;

    float btnX = px + pw - btnW - 4.f*z;
    ImVec2 btnMin(btnX, curY);
    ImVec2 btnMax(btnX + btnW, curY + btnH);
    dl->AddRectFilled(btnMin, btnMax, IM_COL32(50, 140, 50, 255));
    if (fontSize >= 6.f)
    {
        const char* lbl = "Update";
        float fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, lbl).x;
        dl->AddText(pickFont(fontSize), fontSize,
                    ImVec2(btnMin.x + (btnW - fw)*0.5f, btnMin.y + (btnH - fontSize)*0.5f),
                    IM_COL32(255, 255, 255, 255), lbl);
    }
    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
        mousePos.y >= btnMin.y && mousePos.y <= btnMax.y)
    {
        sc->setSAPIText((DWORD)nodeID, std::string(buf.data()));
    }

    float pasteBtnW = 80.f * z;
    ImVec2 pasteMin(px + 4.f * z, curY);
    ImVec2 pasteMax(px + 4.f * z + pasteBtnW, curY + btnH);
    dl->AddRectFilled(pasteMin, pasteMax, IM_COL32(60, 80, 140, 255));
    if (fontSize >= 6.f)
    {
        const char* lbl = "Paste Text";
        float fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, lbl).x;
        dl->AddText(pickFont(fontSize), fontSize,
                    ImVec2(pasteMin.x + (pasteBtnW - fw) * 0.5f, pasteMin.y + (btnH - fontSize) * 0.5f),
                    IM_COL32(255, 255, 255, 255), lbl);
    }
    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        mousePos.x >= pasteMin.x && mousePos.x <= pasteMax.x &&
        mousePos.y >= pasteMin.y && mousePos.y <= pasteMax.y)
    {
        const char* clip = ImGui::GetClipboardText();
        if (clip)
        {
            size_t len = std::min(strlen(clip), buf.size() - 1);
            memcpy(buf.data(), clip, len);
            buf[len] = '\0';
            sc->setSAPIText((DWORD)nodeID, std::string(buf.data()));
        }
    }

    curY += btnH + 4.f * z;
}

void Widgets::drawSignalVisualizerPanel(const EditPanelCtx& ctx, float& curY, SynthController* sc)
{
    ImDrawList* dl       = ctx.dl;
    int         nodeID   = ctx.nodeID;
    float       px       = ctx.px;
    float       pw       = ctx.pw;
    float       z        = ctx.z;
    float       fontSize = ctx.fontSize;
    ImVec2      mousePos = ctx.mousePos;
    bool        canClick = ctx.canClick;

    dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), kColPanelBorder, 0.5f);
    curY += 4.f * z;
    float vizW = pw - 8.f * z;

    int vizMode = sc->getInputMode((DWORD)nodeID, SIGNAL_VISUALIZER_MODE);
    int vizDisp  = vizMode & SIGNAL_VISUALIZER_DISPLAYMASK;
    SynthNode* tmplNode = sc->getNode((DWORD)nodeID);

    // For voice-level nodes: copy live voice data into the template node's customMem.
    // Spectrum mode: copy only the tiny pre-computed results section (~8 KB) — the FFT
    // is now done on the audio thread so there is no need for the 512 KB ring buffer copy.
    // This eliminates the mutex hold time that was causing audio-thread try_lock(0ms) failures.
    // Other modes still need the full ring buffer history.
    {
        bool locked = SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(1));
        if (locked)
        {
            SynthNode* liveNode = sc->getLiveNode((DWORD)nodeID);
            if (liveNode && tmplNode && liveNode != tmplNode &&
                liveNode->customMem && tmplNode->customMem)
            {
                if (vizDisp == (int)SIGNAL_VISUALIZER_SPECTRUM_TIMELINE ||
                    vizDisp == (int)SIGNAL_VISUALIZER_SPECTRUM)
                {
                    // ~8 KB: just the spectrum header + magnitude bins
                    const DWORD kSpecBytes = (SIGVIZ_SPEC_HDR_DW + SIGVIZ_SPEC_BINS) * (DWORD)sizeof(float);
                    memcpy(tmplNode->customMem + SIGVIZ_SPEC_BASE,
                           liveNode->customMem + SIGVIZ_SPEC_BASE,
                           kSpecBytes);
                }
                else
                {
                    // Other modes need the full ring buffer history (512 KB)
                    const DWORD kTotal = (SIGVIZ_HEADER_DW + SIGVIZ_BUF_SIZE * 2u) * (DWORD)sizeof(float);
                    memcpy(tmplNode->customMem, liveNode->customMem, kTotal);
                }
            }
            SynthController::DataAccessMutex.unlock();
        }
    }
    // All display sections use tmplNode exclusively — never a potentially-freed voice pointer.
    SynthNode* vizNode = tmplNode;

    // ── Spectrum Timeline mode: scrolling spectrogram (X=frequency, Y=time, color=magnitude) ──
    if (vizDisp == (int)SIGNAL_VISUALIZER_SPECTRUM_TIMELINE)
    {
        // Mode bits needed for layout/combos and bin mapping (not for FFT — core handles that)
        int fftSel = (vizMode & SIGNAL_VISUALIZER_FFTMASK) >> SIGNAL_VISUALIZER_FFTSHIFT;
        static const int kFFTSizes[] = { 256, 512, 1024, 2048, 4096 };
        if (fftSel < 0 || fftSel >= 5) fftSel = 3;
        int fftHalf = kFFTSizes[fftSel] / 2;

        // ---------- read core pre-computed spectrum section ----------
        // The audio-thread tick now runs the FFT every 'step' samples and stores one
        // magnitude frame in customMem[SIGVIZ_SPEC_BASE..].  The GUI only needs to map
        // those magnitudes to pixels — no FFT, no ring-buffer access, no large memcpy.
        DWORD* specBase  = (vizNode && vizNode->customMem) ? (vizNode->customMem + SIGVIZ_SPEC_BASE) : nullptr;
        DWORD  colCtr    = specBase ? specBase[0] : 0;
        int    fftHalfV  = specBase ? (int)specBase[3] : 0;
        float* specMags  = specBase ? (float*)(specBase + SIGVIZ_SPEC_HDR_DW) : nullptr;
        // fftHalf from mode bits (for log mapping); fall back to core-reported value if 0
        if (fftHalfV <= 0) fftHalfV = fftHalf;

        // ---------- persistent per-node spectrogram cache ----------
        struct SpecCache {
            std::vector<uint8_t> pixels;   // RGBA, numCols * (numRows*2) * 4 — row ring buffer
            int   numCols     = 0;
            int   numRows     = 0;
            int   writePos    = 0;   // next row slot to write into ring (0..numRows*2-1)
            DWORD lastColCtr  = 0;   // last colCtr consumed from core
            int   lastNumCols = -1;
            int   lastFftHalf = -1;
#ifndef __APPLE__
            unsigned int texID = 0;
            int          texW  = 0;
            int          texH  = 0;
#endif
        };
        static std::unordered_map<int, SpecCache> s_specCache;
        SpecCache& cache = s_specCache[nodeID];

        // ---------- layout ----------
        // X axis = frequency (log scale, left=low, right=high)
        // Y axis = time     (new frames at bottom, scroll upward; row 0=top=oldest)
        float specH = 120.f * z;
        // numCols = buffer frequency resolution — fixed to fftHalfV so zoom changes
        // never invalidate the history. The GL texture / rect-rendering stretches
        // these columns to fill vizW, so zooming in simply magnifies existing data.
        int numCols = std::max(8, fftHalfV > 0 ? fftHalfV : fftHalf); // frequency columns
        int numRows = std::max(8, (int)(specH / z + 0.5f));            // time rows
        const int bufRows = numRows * 2;                               // ring-buffer height

        // ---------- Plasma colormap ----------
        static const float kPlasma[][3] = {
            {0.050383f, 0.029803f, 0.152797f},
            {0.341500f, 0.009905f, 0.646365f},
            {0.572067f, 0.143868f, 0.567643f},
            {0.748751f, 0.306346f, 0.444733f},
            {0.876168f, 0.486385f, 0.301656f},
            {0.945636f, 0.671269f, 0.166163f},
            {0.991365f, 0.848964f, 0.882468f},
        };
        static const int kPlasmaN = (int)(sizeof(kPlasma) / sizeof(kPlasma[0])) - 1;

        auto plasmaColor = [&](float t) -> ImU32 {
            t = std::max(0.f, std::min(1.f, t));
            float c = t * kPlasmaN;
            int ci = std::min((int)c, kPlasmaN - 1);
            float f = c - (float)ci;
            float r = kPlasma[ci][0] + (kPlasma[ci+1][0] - kPlasma[ci][0]) * f;
            float g = kPlasma[ci][1] + (kPlasma[ci+1][1] - kPlasma[ci][1]) * f;
            float b = kPlasma[ci][2] + (kPlasma[ci+1][2] - kPlasma[ci][2]) * f;
            return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
        };

        // ---------- check if full reset needed ----------
        bool fullRecomp = (!vizNode || !vizNode->customMem ||
                           cache.numCols != numCols || cache.numRows != numRows ||
                           cache.lastNumCols != numCols || cache.lastFftHalf != fftHalf);

        if (fullRecomp) {
            cache.numCols     = numCols;
            cache.numRows     = numRows;
            cache.lastNumCols = numCols;
            cache.lastFftHalf = fftHalf;
            cache.lastColCtr  = colCtr;   // skip history; populate from new rows only
            cache.writePos    = 0;
            cache.pixels.assign(numCols * bufRows * 4, 0);
#ifndef __APPLE__
            // Delete stale GL texture so it will be recreated at new dimensions
            if (cache.texID != 0) {
                glDeleteTextures(1, &cache.texID);
                cache.texID = 0;
                cache.texW  = 0;
                cache.texH  = 0;
            }
#endif
        }

        // ---------- map new spectrum frames to pixel rows ----------
        // Each FFT frame → one pixel row. Ring buffer on rows; newest = bottom.
        DWORD rawNew = colCtr - cache.lastColCtr;
        int newRows = (rawNew > (DWORD)numRows) ? numRows : (int)rawNew;
        int firstWrittenSlot = -1;  // ring row index of first new write; -1 = nothing new

        if (newRows > 0 && specMags && fftHalfV > 0) {
            firstWrittenSlot = cache.writePos;  // capture before advancing

            // Log-frequency bin ranges for each column (X axis = frequency).
            // Range-max over each column's bin span avoids gaps at high frequencies.
            float logmin = logf(2.f), logmax = logf((float)fftHalfV);
            std::vector<int> binLo(numCols + 1);
            for (int x = 0; x <= numCols; x++) {
                float ld = ((float)x / numCols * (logmax - logmin)) + logmin;
                binLo[x] = std::min((int)expf(ld), fftHalfV - 1);
            }

            // Write new rows into the ring buffer — no memmove needed.
            // writePos advances mod bufRows; oldest visible row = writePos - numRows.
            for (int r = 0; r < newRows; r++) {
                int rowSlot = (cache.writePos + r) % bufRows;
                for (int x = 0; x < numCols; x++) {
                    int lo = binLo[x];
                    int hi = binLo[x + 1];
                    if (hi <= lo) hi = lo + 1;  // always sample at least one bin
                    hi = std::min(hi, fftHalfV);
                    float power = 0.f;
                    for (int b = lo; b < hi; b++)
                        if (specMags[b] > power) power = specMags[b];
                    float db = log2f(power * 2.f + 1e-9f) * 6.f;
                    float tn = 1.f - std::max(std::min(db, 0.f), -100.f) / -100.f;
                    float t  = tn * tn;  // gamma-2
                    int pixIdx = (rowSlot * numCols + x) * 4;
                    ImU32 c = plasmaColor(t);
                    cache.pixels[pixIdx + 0] = (c >> 0) & 0xFF;
                    cache.pixels[pixIdx + 1] = (c >> 8) & 0xFF;
                    cache.pixels[pixIdx + 2] = (c >> 16) & 0xFF;
                    cache.pixels[pixIdx + 3] = 255;
                }
            }
            cache.writePos   = (cache.writePos + newRows) % bufRows;
            cache.lastColCtr = colCtr;
        }

        // ---------- render spectrogram ----------
        ImVec2 specMin(px + 4.f * z, curY);
        ImVec2 specMax(specMin.x + vizW, curY + specH);
        dl->AddRectFilled(specMin, specMax, IM_COL32(0, 0, 0, 255));

        if (cache.numCols > 0 && cache.numRows > 0 && !cache.pixels.empty()) {
#ifndef __APPLE__
            // ---- GL texture path: partial row upload + ring-UV split draw ----
            // Validate handle (catches stale ID if GL context was recreated)
            if (cache.texID != 0 && !glIsTexture(cache.texID)) {
                cache.texID = 0;  cache.texW = 0;  cache.texH = 0;
            }
            if (cache.texID == 0 || cache.texW != cache.numCols || cache.texH != bufRows) {
                // Create/resize — full initial upload
                if (cache.texID != 0)
                    glDeleteTextures(1, &cache.texID);
                glGenTextures(1, &cache.texID);
                glBindTexture(GL_TEXTURE_2D, cache.texID);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                             cache.numCols, bufRows, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, cache.pixels.data());
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
                cache.texW = cache.numCols;
                cache.texH = bufRows;
            } else if (firstWrittenSlot >= 0) {
                // Partial upload: only the new rows written this frame
                glBindTexture(GL_TEXTURE_2D, cache.texID);
                if (firstWrittenSlot + newRows <= bufRows) {
                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                                   0, firstWrittenSlot, cache.numCols, newRows,
                                   GL_RGBA, GL_UNSIGNED_BYTE,
                                   cache.pixels.data() + firstWrittenSlot * cache.numCols * 4);
                } else {
                    // Rows wrap around the ring buffer; two sub-uploads
                    int part1 = bufRows - firstWrittenSlot;
                    int part2 = newRows - part1;
                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                                   0, firstWrittenSlot, cache.numCols, part1,
                                   GL_RGBA, GL_UNSIGNED_BYTE,
                                   cache.pixels.data() + firstWrittenSlot * cache.numCols * 4);
                    if (part2 > 0)
                        glTexSubImage2D(GL_TEXTURE_2D, 0,
                                       0, 0, cache.numCols, part2,
                                       GL_RGBA, GL_UNSIGNED_BYTE,
                                       cache.pixels.data());
                }
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            if (cache.texID != 0) {
                // Render with ring-buffer UV offset using GL_REPEAT on T.
                // The visible range is always exactly 0.5 of the texture height
                // (bufRows = numRows*2). GL_REPEAT wraps seamlessly at the ring
                // boundary, so a single AddImage suffices regardless of writePos.
                ImTextureID tid = (ImTextureID)(uintptr_t)cache.texID;
                int   startRow  = (cache.writePos - cache.numRows + bufRows) % bufRows;
                float invH      = 1.f / (float)bufRows;
                float uvY0      = (float)startRow * invH;
                float uvY1      = uvY0 + (float)cache.numRows * invH;  // = uvY0 + 0.5
                dl->AddImage(tid, specMin, specMax, ImVec2(0.f, uvY0), ImVec2(1.f, uvY1));
            }
#else
            // macOS/Metal fallback: rect-by-rect
            float colW = vizW / (float)cache.numCols;
            float rowH = specH / (float)cache.numRows;
            for (int row = 0; row < cache.numRows; row++) {
                int bufRow = (cache.writePos - cache.numRows + row + bufRows) % bufRows;
                for (int col = 0; col < cache.numCols; col++) {
                    int pixIdx = (bufRow * cache.numCols + col) * 4;
                    ImU32 c = IM_COL32(cache.pixels[pixIdx + 0],
                                       cache.pixels[pixIdx + 1],
                                       cache.pixels[pixIdx + 2], 255);
                    if (c == IM_COL32(0,0,0,255)) continue;
                    float x0 = specMin.x + col * colW;
                    float y0 = specMin.y + row * rowH;
                    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + colW + 0.5f, y0 + rowH + 0.5f), c);
                }
            }
#endif
        }

        // ---------- frequency legend (top-edge overlay, log scale) ----------
        if (fftHalfV > 0 && fontSize >= 6.f) {
            static const float kFreqTicks[] = { 50.f, 100.f, 200.f, 500.f,
                                                 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
            static const int   kFreqTickN   = 9;
            const float sr     = 44100.f;
            const float logmin = logf(2.f);
            const float logmax = logf((float)fftHalfV);
            float lfSz  = fontSize * 0.72f;
            for (int i = 0; i < kFreqTickN; i++) {
                float freq = kFreqTicks[i];
                float bin  = freq * (float)(fftHalfV * 2) / sr;
                if (bin < 2.f || bin >= (float)fftHalfV) continue;
                float normX   = (logf(bin) - logmin) / (logmax - logmin);
                // Shift right by half a column so the tick sits at the column centre
                // (the log mapping left-aligns columns; bin k is centered at x+0.5)
                float screenX = specMin.x + (normX + 0.5f / (float)numCols) * vizW;
                if (screenX < specMin.x + 2.f || screenX > specMax.x - 2.f) continue;

                // tick line from top going down
                dl->AddLine(ImVec2(screenX, specMin.y),
                            ImVec2(screenX, specMin.y + 5.f * z),
                            IM_COL32(255, 255, 255, 140), 1.f);

                char freqLabel[12];
                if (freq >= 1000.f)
                    snprintf(freqLabel, sizeof(freqLabel), "%.0fk", freq / 1000.f);
                else
                    snprintf(freqLabel, sizeof(freqLabel), "%.0f", freq);

                ImVec2 ts = ImGui::CalcTextSize(freqLabel);
                float  tw = ts.x * lfSz / ImGui::GetFont()->FontSize;
                float  th = lfSz;
                // centre label on tick, place just below the tick line
                float  lx = screenX - tw * 0.5f;
                float  ly = specMin.y + 7.f * z;
                lx = std::max(specMin.x + 1.f, std::min(lx, specMax.x - tw - 1.f));
                dl->AddRectFilled(ImVec2(lx - 1.f, ly - 1.f),
                                  ImVec2(lx + tw + 2.f, ly + th + 1.f),
                                  IM_COL32(0, 0, 0, 140));
                dl->AddText(pickFont(lfSz), lfSz,
                            ImVec2(lx, ly),
                            IM_COL32(255, 255, 200, 230), freqLabel);
            }
        }

        dl->AddRect(specMin, specMax, kColPanelBorder, 0.f, 0, 1.f);

        // ---------- hover frequency label (X axis) ----------
        if (fftHalfV > 0 && fontSize >= 6.f &&
            mousePos.x >= specMin.x && mousePos.x <= specMax.x &&
            mousePos.y >= specMin.y && mousePos.y <= specMax.y)
        {
            float normX = (mousePos.x - specMin.x) / vizW;
            normX = std::max(0.f, std::min(1.f, normX));
            // Shift left by half a column to report the centre of the hovered column
            float colCentreNorm = (floorf(normX * (float)numCols) + 0.5f) / (float)numCols;
            colCentreNorm = std::max(0.f, std::min(1.f, colCentreNorm));
            float logmin    = logf(2.f);
            float logmax    = logf((float)fftHalfV);
            float hoverBin  = expf(colCentreNorm * (logmax - logmin) + logmin);
            float hoverFreq = hoverBin * 44100.f / (float)(fftHalfV * 2);

            char tooltip[32];
            if (hoverFreq >= 1000.f)
                snprintf(tooltip, sizeof(tooltip), "%.2f kHz", hoverFreq * 0.001f);
            else
                snprintf(tooltip, sizeof(tooltip), "%.1f Hz", hoverFreq);

            float lfsz = fontSize * 0.72f;
            ImVec2 ts  = ImGui::CalcTextSize(tooltip);
            float  tw  = ts.x * lfsz / ImGui::GetFont()->FontSize;
            float  th  = lfsz;
            // centre tooltip above the mouse, clamp to panel
            float  tx  = mousePos.x - tw * 0.5f;
            float  ty  = mousePos.y - th - 6.f * z;
            tx = std::max(specMin.x + 1.f, std::min(tx, specMax.x - tw - 1.f));
            if (ty < specMin.y + 2.f) ty = mousePos.y + 6.f * z;
            dl->AddRectFilled(ImVec2(tx - 3.f, ty - 2.f),
                              ImVec2(tx + tw + 4.f, ty + th + 2.f),
                              IM_COL32(0, 0, 0, 190));
            dl->AddRect(ImVec2(tx - 3.f, ty - 2.f),
                        ImVec2(tx + tw + 4.f, ty + th + 2.f),
                        IM_COL32(200, 200, 180, 120), 0.f, 0, 1.f);
            dl->AddText(pickFont(lfsz), lfsz, ImVec2(tx, ty),
                        IM_COL32(255, 255, 200, 230), tooltip);
        }

        curY += specH + 4.f * z;

        // ---------- spectrum-specific mode combos (FFT Window, Channel, FFT Size) ----------
        // Read the conditional (showFor==SPECTRUM) groups directly from the config and render
        // them in the same combo-box style used for the generic mode groups in NodeCanvas.
        {
            const NodeTypeDef* svTypeDef = NodeConfig::instance().getNodeType((int)SIGNAL_VISUALIZER_ID);
            if (svTypeDef && SIGNAL_VISUALIZER_MODE < (int)svTypeDef->inputs.size())
            {
                const InputDef& modeDef = svTypeDef->inputs[SIGNAL_VISUALIZER_MODE];
                int currentBits = vizMode;
                int groupIdx = 0;
                for (const auto& mg : modeDef.modeGroups)
                {
                    if (mg.showFor != (int)SIGNAL_VISUALIZER_SPECTRUM_TIMELINE) { groupIdx++; continue; }

                    int groupVal = (currentBits & (int)mg.mask) >> mg.shift;
                    const char* activeName = "???";
                    int activeIdx = 0;
                    for (int j = 0; j < (int)mg.items.size(); j++)
                    {
                        if (mg.items[j].value == groupVal)
                        {
                            activeName = mg.items[j].name.c_str();
                            activeIdx = j;
                        }
                    }

                    float btnH2 = 18.f * z;
                    float btnX  = px + 4.f * z;
                    float btnW  = pw - 8.f * z;
                    ImVec2 btnMin(btnX, curY);
                    ImVec2 btnMax(btnX + btnW, curY + btnH2);
                    bool hov = mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                               mousePos.y >= btnMin.y && mousePos.y <= btnMax.y;
                    dl->AddRectFilled(btnMin, btnMax, hov ? IM_COL32(60,60,70,255) : IM_COL32(40,40,48,255));
                    dl->AddRect(btnMin, btnMax, IM_COL32(120,120,130,255), 0.f, 0, 1.f);
                    if (fontSize >= 6.f)
                    {
                        char label[128];
                        snprintf(label, sizeof(label), "%s: %s", mg.name.c_str(), activeName);
                        float lfsz = fontSize * 0.9f;
                        dl->AddText(pickFont(lfsz), lfsz,
                                    ImVec2(btnX + 4.f*z, curY + (btnH2 - lfsz) * 0.5f),
                                    IM_COL32(220,220,230,255), label);
                    }
                    float arMidX = btnMax.x - 10.f*z, arMidY = curY + btnH2 * 0.5f, arH = 4.f*z;
                    dl->AddTriangleFilled(
                        ImVec2(arMidX - arH, arMidY - arH*0.5f),
                        ImVec2(arMidX + arH, arMidY - arH*0.5f),
                        ImVec2(arMidX,       arMidY + arH*0.5f),
                        IM_COL32(200,200,210,255));

                    char popupId[64];
                    snprintf(popupId, sizeof(popupId), "##specmg%d_%d", nodeID, groupIdx);
                    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hov)
                        ImGui::OpenPopup(popupId);
                    {
                        float itemH = fontSize * 1.35f;
                        float padY  = ImGui::GetStyle().WindowPadding.y * 2.f;
                        ImGui::SetNextWindowSizeConstraints(ImVec2(0,0), ImVec2(FLT_MAX, itemH * 8.f + padY));
                    }
                    ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y));
                    ImGui::PushFont(pickFont(fontSize));
                    if (ImGui::BeginPopup(popupId))
                    {
                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
                        for (int j = 0; j < (int)mg.items.size(); j++)
                        {
                            bool sel = (j == activeIdx);
                            if (ImGui::Selectable(mg.items[j].name.c_str(), sel, 0, ImVec2(btnW, 0)))
                            {
                                int newBits = (currentBits & ~(int)mg.mask) | (mg.items[j].value << mg.shift);
                                sc->setInputMode((DWORD)nodeID, SIGNAL_VISUALIZER_MODE,
                                                 (DWORD)newBits, (DWORD)mg.mask);
                                currentBits = newBits;
                            }
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopFont();
                    curY += btnH2 + 4.f * z;
                    groupIdx++;
                }
            }
        }
        return;
    }

    // ── Spectrum Bars mode: current FFT frame as frequency bars (X=freq log, Y=dB level) ──
    if (vizDisp == (int)SIGNAL_VISUALIZER_SPECTRUM)
    {
        int fftSel = (vizMode & SIGNAL_VISUALIZER_FFTMASK) >> SIGNAL_VISUALIZER_FFTSHIFT;
        static const int kFFTSizes[] = { 256, 512, 1024, 2048, 4096 };
        if (fftSel < 0 || fftSel >= 5) fftSel = 3;
        int fftHalf = kFFTSizes[fftSel] / 2;

        // ---------- read core pre-computed spectrum section ----------
        DWORD* specBase = (vizNode && vizNode->customMem) ? (vizNode->customMem + SIGVIZ_SPEC_BASE) : nullptr;
        int    fftHalfV = specBase ? (int)specBase[3] : 0;
        float* specMags = specBase ? (float*)(specBase + SIGVIZ_SPEC_HDR_DW) : nullptr;
        if (fftHalfV <= 0) fftHalfV = fftHalf;

        // ---------- Plasma colormap ----------
        static const float kPlasmaBars[][3] = {
            {0.050383f, 0.029803f, 0.152797f},
            {0.341500f, 0.009905f, 0.646365f},
            {0.572067f, 0.143868f, 0.567643f},
            {0.748751f, 0.306346f, 0.444733f},
            {0.876168f, 0.486385f, 0.301656f},
            {0.945636f, 0.671269f, 0.166163f},
            {0.991365f, 0.848964f, 0.882468f},
        };
        static const int kPlasmaBarsN = (int)(sizeof(kPlasmaBars) / sizeof(kPlasmaBars[0])) - 1;
        auto plasmaColorBars = [&](float t) -> ImU32 {
            t = std::max(0.f, std::min(1.f, t));
            float c = t * kPlasmaBarsN;
            int ci = std::min((int)c, kPlasmaBarsN - 1);
            float f = c - (float)ci;
            float r = kPlasmaBars[ci][0] + (kPlasmaBars[ci+1][0] - kPlasmaBars[ci][0]) * f;
            float g = kPlasmaBars[ci][1] + (kPlasmaBars[ci+1][1] - kPlasmaBars[ci][1]) * f;
            float b = kPlasmaBars[ci][2] + (kPlasmaBars[ci+1][2] - kPlasmaBars[ci][2]) * f;
            return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
        };

        // ---------- layout ----------
        float specH    = 120.f * z;
        float barAreaW = vizW;   // bars fill the full panel width; dB labels overlay on top

        // Zoom-responsive bin resolution: target ~3 screen pixels per column so zooming
        // in shows more individual bins and zooming out merges bins via power combination.
        // barAreaW is already in screen pixels (scales with z), so dividing by a fixed
        // pixel target gives the correct zoom-dependent column count.
        const float kTargetColPx = 3.f;
        int numCols = std::max(8, (int)(barAreaW / kTargetColPx + 0.5f));
        if (fftHalfV > 0) numCols = std::min(numCols, fftHalfV);

        ImVec2 specMin(px + 4.f * z, curY);
        ImVec2 specMax(specMin.x + vizW, curY + specH);

        dl->AddRectFilled(specMin, specMax, IM_COL32(0, 0, 0, 255));

        // ---------- guide lines (drawn before bars so bars render on top) ----------
        // Horizontal: one per dB tick
        {
            static const int kDbTicks[] = { 0, -20, -40, -60, -80, -100 };
            for (int i = 0; i < 6; i++) {
                float tn = ((float)kDbTicks[i] + 100.f) / 100.f;
                float sy = specMax.y - tn * specH;
                dl->AddLine(ImVec2(specMin.x, sy), ImVec2(specMax.x, sy),
                            IM_COL32(255, 255, 255, 28), 1.f);
            }
        }
        // Vertical: one per freq tick
        if (fftHalfV > 0) {
            static const float kFreqTicksB[] = { 50.f, 100.f, 200.f, 500.f,
                                                  1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
            const float logmin = logf(2.f), logmax = logf((float)fftHalfV);
            for (int i = 0; i < 9; i++) {
                float bin = kFreqTicksB[i] * (float)(fftHalfV * 2) / 44100.f;
                if (bin < 2.f || bin >= (float)fftHalfV) continue;
                float normX   = (logf(bin) - logmin) / (logmax - logmin);
                float screenX = specMin.x + (normX + 0.5f / (float)numCols) * barAreaW;
                if (screenX < specMin.x + 1.f || screenX > specMax.x - 1.f) continue;
                dl->AddLine(ImVec2(screenX, specMin.y), ImVec2(screenX, specMax.y),
                            IM_COL32(255, 255, 255, 28), 1.f);
            }
        }

        // ---------- per-node peak tracker (2s falloff) ----------
        struct SpecBarPeaks {
            std::vector<float> setVal;   // column value at last peak hit
            std::vector<float> setTime;  // ImGui time when peak was last hit
            int lastNumCols = -1;
            int lastFftHalf = -1;
        };
        static std::unordered_map<int, SpecBarPeaks> s_sbPeaks;
        SpecBarPeaks& pk = s_sbPeaks[nodeID];
        if (pk.lastNumCols != numCols || pk.lastFftHalf != fftHalfV) {
            pk.setVal.assign(numCols, 0.f);
            pk.setTime.assign(numCols, 0.f);
            pk.lastNumCols = numCols;
            pk.lastFftHalf = fftHalfV;
        }
        const float kHoldSec      = 2.f;    // hold peak steady for 2s
        const float kFalloffPerSec = 0.5f;   // then full-scale decay over further 2s
        float now = (float)ImGui::GetTime();

        // ---------- compute column values + update peaks ----------
        std::vector<float> colTn(numCols, 0.f);
        std::vector<float> colPk(numCols, 0.f);
        if (specMags && fftHalfV > 0) {
            float logmin = logf(2.f), logmax = logf((float)fftHalfV);
            std::vector<int> binLo(numCols + 1);
            for (int x = 0; x <= numCols; x++) {
                float ld = ((float)x / numCols * (logmax - logmin)) + logmin;
                binLo[x] = std::min((int)expf(ld), fftHalfV - 1);
            }
            for (int x = 0; x < numCols; x++) {
                int lo = binLo[x], hi = binLo[x + 1];
                if (hi <= lo) hi = lo + 1;
                hi = std::min(hi, fftHalfV);
                // Power-sum (RMS) when multiple bins map to one column; single bin → direct
                float power;
                int count = hi - lo;
                if (count <= 1) {
                    power = specMags[lo];
                } else {
                    float sumSq = 0.f;
                    for (int b = lo; b < hi; b++) sumSq += specMags[b] * specMags[b];
                    power = sqrtf(sumSq / (float)count);
                }
                float db  = log2f(power * 2.f + 1e-9f) * 6.f;
                float tn  = 1.f - std::max(std::min(db, 0.f), -100.f) / -100.f;
                colTn[x]  = tn;
                // Hold for kHoldSec, then decay; update if current exceeds held/decayed peak
                float age     = now - pk.setTime[x];
                float peakNow = (age < kHoldSec)
                    ? pk.setVal[x]
                    : std::max(0.f, pk.setVal[x] - (age - kHoldSec) * kFalloffPerSec);
                if (tn >= peakNow) {
                    pk.setVal[x]  = tn;
                    pk.setTime[x] = now;
                    peakNow       = tn;
                }
                colPk[x] = peakNow;
            }
        }

        // ---------- draw spectrum: filled area + instantaneous outline + peak line ----------
        if (numCols > 0) {
            // Center-X of column x (bars rendered between adjacent centers for smooth interpolation)
            auto colCX = [&](int x) -> float {
                return specMin.x + ((float)x + 0.5f) / (float)numCols * barAreaW;
            };

            // Filled area under the interpolated curve.
            // Each segment is a convex trapezoid between adjacent column centres; left/right
            // half-columns are capped with axis-aligned rects so no gap at the edges.
            {
                float cx0 = colCX(0);
                ImU32 col = (plasmaColorBars(colTn[0] * colTn[0]) & 0x00FFFFFFu) | (ImU32(0xA0) << 24u);
                dl->AddRectFilled(ImVec2(specMin.x, specMax.y - colTn[0] * specH),
                                  ImVec2(cx0,       specMax.y), col);
            }
            for (int x = 0; x < numCols - 1; x++) {
                float cx0 = colCX(x),     cy0 = specMax.y - colTn[x]   * specH;
                float cx1 = colCX(x + 1), cy1 = specMax.y - colTn[x+1] * specH;
                ImU32 col = (plasmaColorBars(colTn[x] * colTn[x]) & 0x00FFFFFFu) | (ImU32(0xA0) << 24u);
                dl->AddQuadFilled(ImVec2(cx0, specMax.y), ImVec2(cx0, cy0),
                                  ImVec2(cx1, cy1),       ImVec2(cx1, specMax.y), col);
            }
            {
                int   lc  = numCols - 1;
                float cxL = colCX(lc);
                ImU32 col = (plasmaColorBars(colTn[lc] * colTn[lc]) & 0x00FFFFFFu) | (ImU32(0xA0) << 24u);
                dl->AddRectFilled(ImVec2(cxL,       specMax.y - colTn[lc] * specH),
                                  ImVec2(specMax.x, specMax.y), col);
            }

            // Instantaneous outline polyline — plasma-colored full alpha, 1.5px
            for (int x = 0; x < numCols - 1; x++) {
                float cx0 = colCX(x),     cy0 = specMax.y - colTn[x]   * specH;
                float cx1 = colCX(x + 1), cy1 = specMax.y - colTn[x+1] * specH;
                dl->AddLine(ImVec2(cx0, cy0), ImVec2(cx1, cy1),
                            plasmaColorBars(colTn[x] * colTn[x]), 1.5f);
            }

            // Peak polyline — warm white, semi-transparent, 1px
            for (int x = 0; x < numCols - 1; x++) {
                if (colPk[x] < 0.005f && colPk[x+1] < 0.005f) continue;
                float cx0 = colCX(x),     py0 = specMax.y - colPk[x]   * specH;
                float cx1 = colCX(x + 1), py1 = specMax.y - colPk[x+1] * specH;
                dl->AddLine(ImVec2(cx0, py0), ImVec2(cx1, py1),
                            IM_COL32(255, 255, 200, 170), 1.f);
            }
        }

        // ---------- dB legend (overlaid on left edge) ----------
        if (fontSize >= 6.f) {
            static const int kDbTicks[] = { 0, -20, -40, -60, -80, -100 };
            float lfSz = fontSize * 0.72f;
            for (int i = 0; i < 6; i++) {
                float tn  = ((float)kDbTicks[i] + 100.f) / 100.f;
                float sy  = specMax.y - tn * specH;

                char dbLabel[8];
                snprintf(dbLabel, sizeof(dbLabel), "%d", kDbTicks[i]);
                ImVec2 ts = ImGui::CalcTextSize(dbLabel);
                float  tw = ts.x * lfSz / ImGui::GetFont()->FontSize;
                float  th = lfSz;
                float  lx = specMin.x + 3.f * z;
                float  ly = sy - th * 0.5f;
                ly = std::max(specMin.y + 1.f, std::min(ly, specMax.y - th - 1.f));
                dl->AddRectFilled(ImVec2(lx - 1.f, ly - 1.f),
                                  ImVec2(lx + tw + 2.f, ly + th + 1.f),
                                  IM_COL32(0, 0, 0, 150));
                dl->AddText(pickFont(lfSz), lfSz, ImVec2(lx, ly),
                            IM_COL32(255, 255, 200, 230), dbLabel);
            }
        }

        // ---------- frequency legend (top-edge overlay, log scale) ----------
        if (fftHalfV > 0 && fontSize >= 6.f) {
            static const float kFreqTicksB[] = { 50.f, 100.f, 200.f, 500.f,
                                                  1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
            static const int   kFreqTickBN   = 9;
            const float sr     = 44100.f;
            const float logmin = logf(2.f);
            const float logmax = logf((float)fftHalfV);
            float lfSz  = fontSize * 0.72f;
            for (int i = 0; i < kFreqTickBN; i++) {
                float freq = kFreqTicksB[i];
                float bin  = freq * (float)(fftHalfV * 2) / sr;
                if (bin < 2.f || bin >= (float)fftHalfV) continue;
                float normX   = (logf(bin) - logmin) / (logmax - logmin);
                float screenX = specMin.x + (normX + 0.5f / (float)numCols) * barAreaW;
                if (screenX < specMin.x + 2.f || screenX > specMax.x - 2.f) continue;

                dl->AddLine(ImVec2(screenX, specMin.y),
                            ImVec2(screenX, specMin.y + 5.f * z),
                            IM_COL32(255, 255, 255, 140), 1.f);

                char freqLabel[12];
                if (freq >= 1000.f)
                    snprintf(freqLabel, sizeof(freqLabel), "%.0fk", freq / 1000.f);
                else
                    snprintf(freqLabel, sizeof(freqLabel), "%.0f", freq);

                ImVec2 ts = ImGui::CalcTextSize(freqLabel);
                float  tw = ts.x * lfSz / ImGui::GetFont()->FontSize;
                float  th = lfSz;
                float  lx = screenX - tw * 0.5f;
                float  ly = specMin.y + 7.f * z;
                lx = std::max(specMin.x + 1.f, std::min(lx, specMax.x - tw - 1.f));
                dl->AddRectFilled(ImVec2(lx - 1.f, ly - 1.f),
                                  ImVec2(lx + tw + 2.f, ly + th + 1.f),
                                  IM_COL32(0, 0, 0, 140));
                dl->AddText(pickFont(lfSz), lfSz,
                            ImVec2(lx, ly),
                            IM_COL32(255, 255, 200, 230), freqLabel);
            }
        }

        dl->AddRect(specMin, specMax, kColPanelBorder, 0.f, 0, 1.f);

        // ---------- hover frequency label (X axis) ----------
        if (fftHalfV > 0 && fontSize >= 6.f &&
            mousePos.x >= specMin.x && mousePos.x <= specMax.x &&
            mousePos.y >= specMin.y && mousePos.y <= specMax.y)
        {
            float normX = (mousePos.x - specMin.x) / barAreaW;
            normX = std::max(0.f, std::min(1.f, normX));
            int   hoverCol      = std::min((int)(normX * (float)numCols), numCols - 1);
            float colCentreNorm = ((float)hoverCol + 0.5f) / (float)numCols;
            colCentreNorm = std::max(0.f, std::min(1.f, colCentreNorm));
            float logmin    = logf(2.f);
            float logmax    = logf((float)fftHalfV);
            float hoverBin  = expf(colCentreNorm * (logmax - logmin) + logmin);
            float hoverFreq = hoverBin * 44100.f / (float)(fftHalfV * 2);

            // dB values from the column
            float hoverTn  = (hoverCol >= 0 && hoverCol < (int)colTn.size()) ? colTn[hoverCol] : 0.f;
            float hoverDb  = hoverTn * 100.f - 100.f;   // 0..1 → -100..0 dB
            float hoverPkTn = (hoverCol >= 0 && hoverCol < (int)colPk.size()) ? colPk[hoverCol] : 0.f;
            float hoverPkDb = hoverPkTn * 100.f - 100.f;

            char freqStr[24], dbStr[16], pkStr[16];
            if (hoverFreq >= 1000.f)
                snprintf(freqStr, sizeof(freqStr), "%.2f kHz", hoverFreq * 0.001f);
            else
                snprintf(freqStr, sizeof(freqStr), "%.1f Hz", hoverFreq);
            snprintf(dbStr,  sizeof(dbStr),  "%.1f dB", hoverDb);
            snprintf(pkStr,  sizeof(pkStr),  "%.1f dB pk", hoverPkDb);

            float lfsz  = fontSize * 0.72f;
            float scale = lfsz / ImGui::GetFont()->FontSize;
            float tw    = std::max({ ImGui::CalcTextSize(freqStr).x * scale,
                                    ImGui::CalcTextSize(dbStr).x   * scale,
                                    ImGui::CalcTextSize(pkStr).x   * scale });
            float th    = lfsz * 3.f + 4.f * z;   // three lines + two gaps
            float tx    = mousePos.x - tw * 0.5f;
            float ty    = mousePos.y - th - 6.f * z;
            tx = std::max(specMin.x + 1.f, std::min(tx, specMax.x - tw - 1.f));
            if (ty < specMin.y + 2.f) ty = mousePos.y + 6.f * z;
            dl->AddRectFilled(ImVec2(tx - 3.f, ty - 2.f),
                              ImVec2(tx + tw + 4.f, ty + th + 2.f),
                              IM_COL32(0, 0, 0, 190));
            dl->AddRect(ImVec2(tx - 3.f, ty - 2.f),
                        ImVec2(tx + tw + 4.f, ty + th + 2.f),
                        IM_COL32(200, 200, 180, 120), 0.f, 0, 1.f);
            dl->AddText(pickFont(lfsz), lfsz, ImVec2(tx, ty),
                        IM_COL32(255, 255, 200, 230), freqStr);
            dl->AddText(pickFont(lfsz), lfsz, ImVec2(tx, ty + lfsz + 2.f * z),
                        IM_COL32(180, 220, 255, 220), dbStr);
            dl->AddText(pickFont(lfsz), lfsz, ImVec2(tx, ty + lfsz * 2.f + 4.f * z),
                        IM_COL32(255, 180, 100, 220), pkStr);
        }

        curY += specH + 4.f * z;

        // ---------- spectrum-specific mode combos (FFT Window, Channel, FFT Size) ----------
        {
            const NodeTypeDef* svTypeDef = NodeConfig::instance().getNodeType((int)SIGNAL_VISUALIZER_ID);
            if (svTypeDef && SIGNAL_VISUALIZER_MODE < (int)svTypeDef->inputs.size())
            {
                const InputDef& modeDef = svTypeDef->inputs[SIGNAL_VISUALIZER_MODE];
                int currentBits = vizMode;
                int groupIdx = 0;
                for (const auto& mg : modeDef.modeGroups)
                {
                    if (mg.showFor != (int)SIGNAL_VISUALIZER_SPECTRUM) { groupIdx++; continue; }

                    int groupVal = (currentBits & (int)mg.mask) >> mg.shift;
                    const char* activeName = "???";
                    int activeIdx = 0;
                    for (int j = 0; j < (int)mg.items.size(); j++)
                    {
                        if (mg.items[j].value == groupVal)
                        {
                            activeName = mg.items[j].name.c_str();
                            activeIdx = j;
                        }
                    }

                    float btnH2 = 18.f * z;
                    float btnX  = px + 4.f * z;
                    float btnW  = pw - 8.f * z;
                    ImVec2 btnMin(btnX, curY);
                    ImVec2 btnMax(btnX + btnW, curY + btnH2);
                    bool hov = mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                               mousePos.y >= btnMin.y && mousePos.y <= btnMax.y;
                    dl->AddRectFilled(btnMin, btnMax, hov ? IM_COL32(60,60,70,255) : IM_COL32(40,40,48,255));
                    dl->AddRect(btnMin, btnMax, IM_COL32(120,120,130,255), 0.f, 0, 1.f);
                    if (fontSize >= 6.f)
                    {
                        char label[128];
                        snprintf(label, sizeof(label), "%s: %s", mg.name.c_str(), activeName);
                        float lfsz = fontSize * 0.9f;
                        dl->AddText(pickFont(lfsz), lfsz,
                                    ImVec2(btnX + 4.f*z, curY + (btnH2 - lfsz) * 0.5f),
                                    IM_COL32(220,220,230,255), label);
                    }
                    float arMidX = btnMax.x - 10.f*z, arMidY = curY + btnH2 * 0.5f, arH = 4.f*z;
                    dl->AddTriangleFilled(
                        ImVec2(arMidX - arH, arMidY - arH*0.5f),
                        ImVec2(arMidX + arH, arMidY - arH*0.5f),
                        ImVec2(arMidX,       arMidY + arH*0.5f),
                        IM_COL32(200,200,210,255));

                    char popupId[64];
                    snprintf(popupId, sizeof(popupId), "##specbmg%d_%d", nodeID, groupIdx);
                    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hov)
                        ImGui::OpenPopup(popupId);
                    {
                        float itemH = fontSize * 1.35f;
                        float padY  = ImGui::GetStyle().WindowPadding.y * 2.f;
                        ImGui::SetNextWindowSizeConstraints(ImVec2(0,0), ImVec2(FLT_MAX, itemH * 8.f + padY));
                    }
                    ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y));
                    ImGui::PushFont(pickFont(fontSize));
                    if (ImGui::BeginPopup(popupId))
                    {
                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
                        for (int j = 0; j < (int)mg.items.size(); j++)
                        {
                            bool sel = (j == activeIdx);
                            if (ImGui::Selectable(mg.items[j].name.c_str(), sel, 0, ImVec2(btnW, 0)))
                            {
                                int newBits = (currentBits & ~(int)mg.mask) | (mg.items[j].value << mg.shift);
                                sc->setInputMode((DWORD)nodeID, SIGNAL_VISUALIZER_MODE,
                                                 (DWORD)newBits, (DWORD)mg.mask);
                                currentBits = newBits;
                            }
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopFont();
                    curY += btnH2 + 4.f * z;
                    groupIdx++;
                }
            }
        }
        return;
    }

    // Raw Signal mode: full-height bipolar bars
    if (vizDisp == (int)SIGNAL_VISUALIZER_RAW)
    {
        if (!vizNode || !vizNode->customMem)
            vizNode = nullptr;

        float barH = 120.f * z;
        ImVec2 barMin(px + 4.f * z, curY);
        ImVec2 barMax(barMin.x + vizW, curY + barH);
        dl->AddRectFilled(barMin, barMax, IM_COL32(0, 0, 0, 255));
        dl->AddRect(barMin, barMax, kColPanelBorder, 0.f, 0, 1.f);

        float sampleL = 0.f, sampleR = 0.f;
        if (vizNode && vizNode->customMem)
        {
            DWORD* dw  = vizNode->customMem;
            float* ring = (float*)(dw + SIGVIZ_HEADER_DW);
            DWORD last  = (dw[0] - 1u) & (DWORD)(SIGVIZ_BUF_SIZE - 1);
            sampleL = ring[last * 2];
            sampleR = ring[last * 2 + 1];
        }

        float midY  = barMin.y + barH * 0.5f;
        float bpad  = 4.f * z;
        float bw2   = vizW * 0.5f - bpad * 1.5f;
        float halfH = barH * 0.5f;
        dl->AddLine(ImVec2(barMin.x, midY), ImVec2(barMax.x, midY), IM_COL32(80, 80, 80, 255), 0.5f);

        auto drawBipolarBar = [&](float bx, float val, ImU32 col)
        {
            float v    = val < -1.f ? -1.f : (val > 1.f ? 1.f : val);
            float fill = v * halfH * 0.95f;
            float y0   = midY, y1 = midY - fill;
            if (y0 > y1) { float t = y0; y0 = y1; y1 = t; }
            if (y1 > y0)
                dl->AddRectFilled(ImVec2(bx, y0), ImVec2(bx + bw2, y1), col);
            float tickY = midY - v * halfH * 0.95f;
            dl->AddLine(ImVec2(bx, tickY), ImVec2(bx + bw2, tickY), IM_COL32(255,255,255,200), 1.5f);
            dl->AddRect(ImVec2(bx, barMin.y), ImVec2(bx + bw2, barMax.y), IM_COL32(60,60,60,255), 0.f, 0, 0.5f);
        };
        drawBipolarBar(barMin.x + bpad,                  sampleL, IM_COL32(0,200,200,220));
        drawBipolarBar(barMin.x + vizW*0.5f + bpad*0.5f, sampleR, IM_COL32(200,200,0,  220));
        if (fontSize >= 6.f)
        {
            float lfsz = fontSize * 0.8f;
            dl->AddText(pickFont(lfsz), lfsz,
                ImVec2(barMin.x + bpad + bw2*0.5f - 3.f*z, barMax.y - lfsz - 2.f*z),
                IM_COL32(180,180,180,255), "L");
            dl->AddText(pickFont(lfsz), lfsz,
                ImVec2(barMin.x + vizW*0.5f + bpad*0.5f + bw2*0.5f - 3.f*z, barMax.y - lfsz - 2.f*z),
                IM_COL32(180,180,180,255), "R");
        }
        curY += barH + 4.f * z;
        return;
    }

    // Signal Timeline mode: scrollable history waveform + scrollbar + history-length combo
    if (vizDisp == (int)SIGNAL_VISUALIZER_RAW_TIMELINE)
    {
        // Persistent per-node GUI state (UI-only, never stored in core mode word)
        // scrollFrac: 0=oldest at left, 1=newest at right (default=1, live view)
        static std::unordered_map<int, float> s_timelineScroll;
        static std::unordered_map<int, float> s_sbDragStartX;
        static std::unordered_map<int, float> s_sbDragStartFrac;

        if (!vizNode || !vizNode->customMem)
            vizNode = nullptr;

        static const DWORD kHistLengths[] = { 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256 };
        static const char* kHistLabels[]  = { "65536 (~1.5s)", "32768 (~0.74s)", "16384 (~0.37s)",
                                              "8192 (~0.19s)",  "4096 (~93ms)",  "2048 (~46ms)",
                                              "1024 (~23ms)",   "512 (~12ms)",   "256 (~6ms)" };
        static const int   kHistCount     = 9;

        int histIdx = (vizMode & SIGNAL_VISUALIZER_HISTMASK) >> SIGNAL_VISUALIZER_HISTSHIFT;
        if (histIdx < 0 || histIdx >= kHistCount) histIdx = 0;

        DWORD windowSize = kHistLengths[histIdx];
        DWORD maxScroll  = (histIdx == 0) ? 0u : (DWORD)(SIGVIZ_BUF_SIZE - windowSize);
        bool  sbActive   = (histIdx != 0);

        // Insert with 1.0f default (thumb at right = newest) only on first encounter
        auto [it, inserted] = s_timelineScroll.emplace(nodeID, 1.0f);
        float& scrollFrac = it->second;
        if (!sbActive) scrollFrac = 1.f; // full buffer: no scroll, keep thumb at right
        // Invert: scrollFrac=1 → newest (scrollOff=0), scrollFrac=0 → oldest (scrollOff=max)
        DWORD scrollOff = (maxScroll > 0u)
            ? std::min((DWORD)((1.f - scrollFrac) * (float)maxScroll + 0.5f), maxScroll)
            : 0u;

        // --- scrolling history waveform (120 px, same height as VU/Scope/Raw) ---
        float histH = 120.f * z;
        ImVec2 histMin(px + 4.f * z, curY);
        ImVec2 histMax(histMin.x + vizW, curY + histH);
        dl->AddRectFilled(histMin, histMax, IM_COL32(0, 0, 0, 255));
        dl->AddRect(histMin, histMax, kColPanelBorder, 0.f, 0, 1.f);
        float histMidY = histMin.y + histH * 0.5f;
        dl->AddLine(ImVec2(histMin.x, histMidY), ImVec2(histMax.x, histMidY),
                    IM_COL32(50, 50, 50, 255), 0.5f);

        if (vizNode && vizNode->customMem)
        {
            DWORD* dw    = vizNode->customMem;
            float* ring  = (float*)(dw + SIGVIZ_HEADER_DW);
            DWORD  nextWp = dw[0];
            int numCols   = (int)(vizW / z + 0.5f);
            if (numCols < 2) numCols = 2;
            float px0L = 0.f, py0L = 0.f, px0R = 0.f, py0R = 0.f;
            for (int xi = 0; xi <= numCols; xi++)
            {
                // xi=0 → oldest visible sample, xi=numCols → newest visible sample
                DWORD base = (windowSize - 1u) - (DWORD)((float)xi / numCols * (float)(windowSize - 1u) + 0.5f);
                DWORD ago  = base + scrollOff;
                DWORD pos  = (nextWp - 1u - ago) & (DWORD)(SIGVIZ_BUF_SIZE - 1);
                float L    = ring[pos * 2];
                float R    = ring[pos * 2 + 1];
                float sx   = histMin.x + (float)xi / numCols * vizW;
                float syL  = histMidY - std::max(-1.f, std::min(1.f, L)) * histH * 0.47f;
                float syR  = histMidY - std::max(-1.f, std::min(1.f, R)) * histH * 0.47f;
                if (xi > 0)
                {
                    dl->AddLine(ImVec2(px0L, py0L), ImVec2(sx, syL), IM_COL32(0,200,200,200), 1.f);
                    dl->AddLine(ImVec2(px0R, py0R), ImVec2(sx, syR), IM_COL32(200,200,0, 160), 1.f);
                }
                px0L = sx; py0L = syL;
                px0R = sx; py0R = syR;
            }
        }
        curY += histH + 4.f * z;

        // --- horizontal scrollbar (14 px, always visible; inactive when histIdx==0) ---
        float sbH = 14.f * z;
        ImVec2 sbMin(px + 4.f * z, curY);
        ImVec2 sbMax(sbMin.x + vizW, curY + sbH);
        float thumbFrac  = (float)windowSize / (float)SIGVIZ_BUF_SIZE;
        float thumbW     = std::max(8.f * z, vizW * thumbFrac);
        float trackRange = vizW - thumbW;
        float thumbX     = sbMin.x + (trackRange > 0.f ? scrollFrac * trackRange : 0.f);
        ImU32 trackCol   = sbActive ? IM_COL32( 30, 30, 35, 255) : IM_COL32(20, 20, 25, 180);
        ImU32 rimCol     = sbActive ? IM_COL32( 80, 80, 90, 255) : IM_COL32(50, 50, 55, 180);
        ImU32 thumbCol   = sbActive ? IM_COL32(100,100,110, 255) : IM_COL32(50, 50, 55, 180);
        dl->AddRectFilled(sbMin, sbMax, trackCol);
        dl->AddRect(sbMin, sbMax, rimCol, 0.f, 0, 1.f);
        dl->AddRectFilled(ImVec2(thumbX, sbMin.y + 2.f), ImVec2(thumbX + thumbW, sbMax.y - 2.f), thumbCol);

        if (sbActive && trackRange > 0.f)
        {
            bool inSb = mousePos.x >= sbMin.x && mousePos.x <= sbMax.x &&
                        mousePos.y >= sbMin.y && mousePos.y <= sbMax.y;
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inSb)
            {
                s_sbDragStartX[nodeID]    = mousePos.x;
                s_sbDragStartFrac[nodeID] = scrollFrac;
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && s_sbDragStartX.count(nodeID))
            {
                float dx = ImGui::GetMousePos().x - s_sbDragStartX[nodeID];
                scrollFrac = std::max(0.f, std::min(1.f, s_sbDragStartFrac[nodeID] + dx / trackRange));
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                s_sbDragStartX.erase(nodeID);
                s_sbDragStartFrac.erase(nodeID);
            }
        }
        curY += sbH + 4.f * z;

        // --- history-length combobox (18 px) ---
        {
            float btnH2 = 18.f * z;
            float btnX  = px + 4.f * z;
            float btnW  = pw - 8.f * z;
            ImVec2 btnMin(btnX, curY);
            ImVec2 btnMax(btnX + btnW, curY + btnH2);
            bool hov = mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                       mousePos.y >= btnMin.y && mousePos.y <= btnMax.y;
            dl->AddRectFilled(btnMin, btnMax, hov ? IM_COL32(60,60,70,255) : IM_COL32(40,40,48,255));
            dl->AddRect(btnMin, btnMax, IM_COL32(120,120,130,255), 0.f, 0, 1.f);
            if (fontSize >= 6.f)
            {
                char label[64];
                snprintf(label, sizeof(label), "History: %s", kHistLabels[histIdx]);
                float lfsz = fontSize * 0.9f;
                dl->AddText(pickFont(lfsz), lfsz,
                            ImVec2(btnX + 4.f*z, curY + (btnH2 - lfsz) * 0.5f),
                            IM_COL32(220,220,230,255), label);
            }
            float arMidX = btnMax.x - 10.f*z, arMidY = curY + btnH2 * 0.5f, arH = 4.f*z;
            dl->AddTriangleFilled(
                ImVec2(arMidX - arH, arMidY - arH*0.5f),
                ImVec2(arMidX + arH, arMidY - arH*0.5f),
                ImVec2(arMidX,       arMidY + arH*0.5f),
                IM_COL32(200,200,210,255));
            char popupId[64];
            snprintf(popupId, sizeof(popupId), "##svhist%d", nodeID);
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hov)
                ImGui::OpenPopup(popupId);
            {
                float itemH = fontSize * 1.35f;
                float padY  = ImGui::GetStyle().WindowPadding.y * 2.f;
                ImGui::SetNextWindowSizeConstraints(ImVec2(0,0), ImVec2(FLT_MAX, itemH * 9.f + padY));
            }
            ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y));
            ImGui::PushFont(pickFont(fontSize));
            if (ImGui::BeginPopup(popupId))
            {
                ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
                for (int i = 0; i < kHistCount; i++)
                {
                    bool sel = (i == histIdx);
                    if (ImGui::Selectable(kHistLabels[i], sel, 0, ImVec2(btnW, 0)))
                        sc->setInputMode((DWORD)nodeID, SIGNAL_VISUALIZER_MODE,
                                         (DWORD)(i << SIGNAL_VISUALIZER_HISTSHIFT),
                                         (DWORD)SIGNAL_VISUALIZER_HISTMASK);
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndPopup();
            }
            ImGui::PopFont();
            curY += btnH2 + 4.f * z;
        }
        return;
    }

    float vizH = 120.f * z;
    ImVec2 vizMin(px + 4.f * z, curY);
    ImVec2 vizMax(vizMin.x + vizW, curY + vizH);
    dl->AddRectFilled(vizMin, vizMax, IM_COL32(0, 0, 0, 255));
    dl->AddRect(vizMin, vizMax, kColPanelBorder, 0.f, 0, 1.f);

    if (vizDisp == (int)SIGNAL_VISUALIZER_VU)
    {
        float peakL = 0.f, peakR = 0.f;
        float rawL  = 0.f, rawR  = 0.f;
        if (vizNode && vizNode->customMem)
        {
            peakL = *((float*)(vizNode->customMem + 1));
            peakR = *((float*)(vizNode->customMem + 2));
            rawL  = *((float*)(vizNode->customMem + 5));
            rawR  = *((float*)(vizNode->customMem + 6));
        }
        auto dbNorm = [](float amp) -> float {
            if (amp <= 0.f) return 0.f;
            float db = 20.f * std::log10f(amp);
            return std::max(0.f, std::min(1.f, (db + 60.f) / 60.f));
        };
        float normL  = dbNorm(rawL),  normR  = dbNorm(rawR);
        float normPL = dbNorm(peakL), normPR = dbNorm(peakR);

        auto drawBar = [&](float bx, float bw2, float norm, float pkNorm)
        {
            const float zG = 0.80f, zY = 0.90f;
            float botY = vizMax.y;
            if (norm > 0.f)
            {
                float gH = std::min(vizH * norm, vizH * zG);
                dl->AddRectFilled(ImVec2(bx, botY - gH), ImVec2(bx + bw2, botY),
                                  IM_COL32(40, 200, 40, 255));
            }
            if (norm > zG)
            {
                float yH = vizH * std::min(norm - zG, zY - zG);
                float yB = botY - vizH * zG;
                dl->AddRectFilled(ImVec2(bx, yB - yH), ImVec2(bx + bw2, yB),
                                  IM_COL32(220, 180, 40, 255));
            }
            if (norm > zY)
            {
                float rH = vizH * (norm - zY);
                float rB = botY - vizH * zY;
                dl->AddRectFilled(ImVec2(bx, rB - rH), ImVec2(bx + bw2, rB),
                                  IM_COL32(220, 50, 50, 255));
            }
            if (pkNorm > 0.01f)
            {
                float pY = botY - vizH * pkNorm;
                ImU32 pc = pkNorm < zG ? IM_COL32(40,200,40,255)
                         : pkNorm < zY ? IM_COL32(220,180,40,255)
                                       : IM_COL32(220,50,50,255);
                dl->AddLine(ImVec2(bx, pY), ImVec2(bx + bw2, pY), pc, 1.5f);
            }
            dl->AddRect(ImVec2(bx, vizMin.y), ImVec2(bx + bw2, botY),
                        IM_COL32(60, 60, 60, 255), 0.f, 0, 0.5f);
        };
        float bpad = 4.f * z;
        float bw2  = vizW * 0.5f - bpad * 1.5f;
        drawBar(vizMin.x + bpad,                   bw2, normL, normPL);
        drawBar(vizMin.x + vizW*0.5f + bpad*0.5f,  bw2, normR, normPR);
        if (fontSize >= 6.f)
        {
            float lfsz = fontSize * 0.8f;
            dl->AddText(pickFont(lfsz), lfsz,
                ImVec2(vizMin.x + bpad + bw2*0.5f - 3.f*z, vizMax.y - lfsz - 2.f*z),
                IM_COL32(180,180,180,255), "L");
            dl->AddText(pickFont(lfsz), lfsz,
                ImVec2(vizMin.x + vizW*0.5f + bpad*0.5f + bw2*0.5f - 3.f*z, vizMax.y - lfsz - 2.f*z),
                IM_COL32(180,180,180,255), "R");
        }
    }
    else if (vizDisp == (int)SIGNAL_VISUALIZER_SCOPE) // Oscilloscope
    {
        if (vizNode && vizNode->customMem)
        {
            DWORD* dw   = vizNode->customMem;
            float* ring = (float*)(dw + SIGVIZ_HEADER_DW);
            const int DISP = 512;
            int syncPos = (int)(dw[7] & (SIGVIZ_BUF_SIZE - 1));
            float midY = vizMin.y + vizH * 0.5f;
            dl->AddLine(ImVec2(vizMin.x, midY), ImVec2(vizMax.x, midY),
                        IM_COL32(128, 128, 128, 255), 0.5f);
            for (int ch = 0; ch < 2; ch++)
            {
                ImU32 col = (ch == 0) ? IM_COL32(0,255,255,220) : IM_COL32(255,255,0,220);
                float px0 = 0.f, py0 = 0.f;
                for (int si = 0; si < DISP; si++)
                {
                    int idx = (syncPos + si) & (SIGVIZ_BUF_SIZE - 1);
                    float s = ring[idx * 2 + ch];
                    if (s >  1.f) s =  1.f;
                    if (s < -1.f) s = -1.f;
                    float scx = vizMin.x + (float)si / (float)(DISP - 1) * vizW;
                    float scy = midY - s * vizH * 0.47f;
                    if (si > 0)
                        dl->AddLine(ImVec2(px0, py0), ImVec2(scx, scy), col, 1.f);
                    px0 = scx; py0 = scy;
                }
            }
        }
    }
    curY += vizH + 4.f * z;
}

} // namespace K64GUI
