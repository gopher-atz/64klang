#include "Widgets.h"
#include "imgui.h"

#include <cmath>
#include <cstdio>
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

static const char* DELAY_TIME_NAMES[] = {
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
            return { DELAY_TIME_NAMES[i], "" };
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
} // namespace K64GUI
