#include "Widgets.h"
#include "imgui.h"
#include "core/SynthNode.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>

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

    dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), kColPanelBorder, 0.5f);
    curY += 4.f * z;
    float vizW = pw - 8.f * z;
    float vizH = 120.f * z;
    ImVec2 vizMin(px + 4.f * z, curY);
    ImVec2 vizMax(vizMin.x + vizW, curY + vizH);
    dl->AddRectFilled(vizMin, vizMax, IM_COL32(0, 0, 0, 255));
    dl->AddRect(vizMin, vizMax, kColPanelBorder, 0.f, 0, 1.f);

    int vizMode = sc->getInputMode((DWORD)nodeID, SIGNAL_VISUALIZER_MODE);
    SynthNode* vizNode = sc->getLiveNode((DWORD)nodeID);

    if (vizMode == (int)SIGNAL_VISUALIZER_VU)
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
    else // Oscilloscope
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
