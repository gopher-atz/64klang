#include "Widgets.h"
#include "imgui.h"
#include "core/SynthNode.h"
#include "core/SynthController.h"

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

    // For voice-level nodes: copy the live voice's ring buffer into the template node's
    // customMem so the data persists after the voice is destroyed. Guard under the mutex
    // so this cannot race with DestroyVoiceNodes + DeferredSynthFree in the audio thread.
    // A 1ms try_lock is safe: audio buffers are ~11ms apart at 44100 Hz.
    {
        bool locked = SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(1));
        if (locked)
        {
            SynthNode* liveNode = sc->getLiveNode((DWORD)nodeID);
            if (liveNode && tmplNode && liveNode != tmplNode &&
                liveNode->customMem && tmplNode->customMem)
            {
                const DWORD kTotal = (SIGVIZ_HEADER_DW + SIGVIZ_BUF_SIZE * 2u) * sizeof(float);
                memcpy(tmplNode->customMem, liveNode->customMem, kTotal);
            }
            SynthController::DataAccessMutex.unlock();
        }
    }
    // All display sections use tmplNode exclusively — never a potentially-freed voice pointer.
    SynthNode* vizNode = tmplNode;

    // ── Spectrum mode: scrolling spectrogram (X=time, Y=frequency, color=magnitude) ──
    if (vizDisp == (int)SIGNAL_VISUALIZER_SPECTRUM)
    {
        // ---------- decode mode bits ----------
        int winSel  = (vizMode & SIGNAL_VISUALIZER_WINMASK)  >> SIGNAL_VISUALIZER_WINSHIFT;   // 0=BH,1=Rect,2=Hamming,3=Blackman
        int chanSel = (vizMode & SIGNAL_VISUALIZER_CHANMASK) >> SIGNAL_VISUALIZER_CHANSHIFT;   // 0=L+R,1=L,2=R
        int fftSel  = (vizMode & SIGNAL_VISUALIZER_FFTMASK)  >> SIGNAL_VISUALIZER_FFTSHIFT;    // 0=2048,1=256,2=512,3=1024,4=4096
        int histIdx = (vizMode & SIGNAL_VISUALIZER_HISTMASK) >> SIGNAL_VISUALIZER_HISTSHIFT;

        static const int kFFTSizes[] = { 256, 512, 1024, 2048, 4096 };
        int fftSize = (fftSel >= 0 && fftSel < 5) ? kFFTSizes[fftSel] : 2048;
        int fftHalf = fftSize / 2;

        static const DWORD kHistLengths[] = { 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256 };
        static const char* kHistLabels[]  = { "65536 (~1.5s)", "32768 (~0.74s)", "16384 (~0.37s)",
                                              "8192 (~0.19s)",  "4096 (~93ms)",  "2048 (~46ms)",
                                              "1024 (~23ms)",   "512 (~12ms)",   "256 (~6ms)" };
        static const int   kHistCount     = 9;
        if (histIdx < 0 || histIdx >= kHistCount) histIdx = 0;
        DWORD windowSize = kHistLengths[histIdx];

        // ---------- persistent per-node spectrogram cache ----------
        struct SpecCache {
            std::vector<uint8_t> pixels;   // RGBA, numCols * numRows * 4
            int   numCols   = 0;
            int   numRows   = 0;
            DWORD lastWp    = 0;           // ring-buffer write-pos last time we updated
            int   lastFFTSize = 0;
            int   lastWinSel  = -1;
            int   lastChanSel = -1;
            int   lastHistIdx = -1;
        };
        static std::unordered_map<int, SpecCache> s_specCache;
        SpecCache& cache = s_specCache[nodeID];

        // ---------- layout ----------
        float specH = 120.f * z;
        int numCols = std::max(8, std::min(512, (int)(vizW / z + 0.5f)));
        int numRows = std::max(8, std::min(fftHalf, (int)(specH / z + 0.5f)));

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

        // ---------- check if full recompute needed ----------
        bool fullRecomp = (!vizNode || !vizNode->customMem ||
                           cache.numCols != numCols || cache.numRows != numRows ||
                           cache.lastFFTSize != fftSize || cache.lastWinSel != winSel ||
                           cache.lastChanSel != chanSel || cache.lastHistIdx != histIdx);

        DWORD curWp = 0;
        float* ring = nullptr;
        if (vizNode && vizNode->customMem) {
            DWORD* dw = vizNode->customMem;
            ring = (float*)(dw + SIGVIZ_HEADER_DW);
            curWp = dw[0];
        }

        if (fullRecomp) {
            cache.numCols = numCols;
            cache.numRows = numRows;
            cache.lastFFTSize = fftSize;
            cache.lastWinSel  = winSel;
            cache.lastChanSel = chanSel;
            cache.lastHistIdx = histIdx;
            cache.lastWp = curWp;
            cache.pixels.assign(numCols * numRows * 4, 0);
        }

        // ---------- compute FFT columns ----------
        // step = how many ring-buffer samples per spectrogram column
        int step = std::max(1, (int)windowSize / numCols);
        // how many new columns since last frame
        int newCols = 0;
        if (ring && !fullRecomp) {
            int elapsed = (int)((curWp - cache.lastWp) & (SIGVIZ_BUF_SIZE - 1));
            newCols = std::min(elapsed / step, numCols);
            if (newCols <= 0) newCols = 0;
        } else if (ring && fullRecomp) {
            newCols = numCols;
        }

        if (newCols > 0 && ring) {
            // shift existing columns left by newCols
            if (newCols < numCols) {
                int keepCols = numCols - newCols;
                for (int row = 0; row < numRows; row++) {
                    memmove(&cache.pixels[row * numCols * 4],
                            &cache.pixels[(row * numCols + newCols) * 4],
                            keepCols * 4);
                }
            }

            // pre-compute log frequency mapping (Y pixel -> FFT bin)
            std::vector<int> binMap(numRows);
            float logmin = logf(2.f), logmax = logf((float)fftHalf);
            for (int y = 0; y < numRows; y++) {
                float logdata = ((float)y / numRows * (logmax - logmin)) + logmin;
                binMap[y] = std::min((int)expf(logdata), fftHalf - 1);
            }

            // pre-compute window function coefficients
            std::vector<float> winCoeff(fftSize);
            float sumW = 0.f;
            for (int i = 0; i < fftSize; i++) {
                float a = 2.f * 3.14159265358979f * i / (fftSize - 1);
                float w;
                switch (winSel) {
                    case 1:  w = 0.54f - 0.46f * cosf(a); break; // Hamming
                    case 2:  w = 0.42f - 0.5f * cosf(a) + 0.08f * cosf(2.f * a); break; // Blackman
                    case 3:  w = 0.35875f - 0.48829f * cosf(a) + 0.14128f * cosf(2.f * a) - 0.01168f * cosf(3.f * a); break; // Blackman-Harris
                    default: w = 1.f; break; // Rectangular (0)
                }
                winCoeff[i] = w;
                sumW += w;
            }
            float scaling = 1.f / sumW;

            // allocate FFT buffer (complexsample_t, real-valued input: im=0)
            std::vector<complexsample_t> fftBuf(fftSize);

            // compute each new column
            for (int col = 0; col < newCols; col++) {
                int colIdx = numCols - newCols + col; // destination column
                // end of this FFT window: newest column ends at curWp-1 (last written sample)
                int agoSamples = (newCols - 1 - col) * step;
                DWORD center = (curWp - agoSamples) & (SIGVIZ_BUF_SIZE - 1);

                // fill FFT input: window = [center-fftSize .. center-1], all within written data
                for (int i = 0; i < fftSize; i++) {
                    DWORD pos = (center - fftSize + i) & (SIGVIZ_BUF_SIZE - 1);
                    float L = ring[pos * 2];
                    float R = ring[pos * 2 + 1];
                    float s;
                    switch (chanSel) {
                        case 1:  s = L; break;
                        case 2:  s = R; break;
                        default: s = L + R; break;
                    }
                    fftBuf[i] = complexsample_t((double)(s * winCoeff[i]));  // real-valued: im=0
                }

                c_fft(fftBuf.data(), fftSize);

                // convert to magnitude and map to pixels for each row (log freq)
                for (int y = 0; y < numRows; y++) {
                    int bin = binMap[y];
                    double re = fftBuf[bin].re.d[0];
                    double im = fftBuf[bin].im.d[0];
                    float power = (float)sqrt(re * re + im * im) * scaling;
                    // 0 dBFS: full-scale sine (power≈0.5 after window norm) * 2 = 1.0 → 0 dB.
                    // Floor at -100 dB;
                    float db = log2f(power * 2.f + 1e-9f) * 6.f;          // ≈ 20*log10, power*2 → 0dBFS ref
                    float tn = 1.f - std::max(std::min(db, 0.f), -100.f) / -100.f;  // 0=floor,1=0dBFS
                    float t = tn * tn;                                              // gamma 2: suppress low-level

                    // pack into pixel buffer (row 0 = bottom = low freq, stored top-down for rendering)
                    int pixIdx = ((numRows - 1 - y) * numCols + colIdx) * 4;
                    ImU32 c = plasmaColor(t);
                    cache.pixels[pixIdx + 0] = (c >> 0) & 0xFF;
                    cache.pixels[pixIdx + 1] = (c >> 8) & 0xFF;
                    cache.pixels[pixIdx + 2] = (c >> 16) & 0xFF;
                    cache.pixels[pixIdx + 3] = 255;
                }
            }
            cache.lastWp = curWp;
        }

        // ---------- render spectrogram ----------
        ImVec2 specMin(px + 4.f * z, curY);
        ImVec2 specMax(specMin.x + vizW, curY + specH);
        dl->AddRectFilled(specMin, specMax, IM_COL32(0, 0, 0, 255));

        if (cache.numCols > 0 && cache.numRows > 0 && !cache.pixels.empty()) {
            float colW = vizW / (float)cache.numCols;
            float rowH = specH / (float)cache.numRows;
            for (int row = 0; row < cache.numRows; row++) {
                for (int col = 0; col < cache.numCols; col++) {
                    int pixIdx = (row * cache.numCols + col) * 4;
                    ImU32 c = IM_COL32(cache.pixels[pixIdx + 0],
                                       cache.pixels[pixIdx + 1],
                                       cache.pixels[pixIdx + 2], 255);
                    if (c == IM_COL32(0,0,0,255)) continue; // skip black for perf
                    float x0 = specMin.x + col * colW;
                    float y0 = specMin.y + row * rowH;
                    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + colW + 0.5f, y0 + rowH + 0.5f), c);
                }
            }
        }
        dl->AddRect(specMin, specMax, kColPanelBorder, 0.f, 0, 1.f);
        curY += specH + 4.f * z;

        // ---------- history-length combobox ----------
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
            snprintf(popupId, sizeof(popupId), "##spechist%d", nodeID);
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
    if (vizDisp == (int)SIGNAL_VISUALIZER_TIMELINE)
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
