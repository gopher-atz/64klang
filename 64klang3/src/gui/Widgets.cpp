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

// ── Knob Widget ──────────────────────────────────────────────────────────

bool Knob(const char* id, const InputDef& inputDef, float* valueL, float* valueR,
          float modL, float modR, bool synced, bool singleInput, int nodeTypeID, int currentMode)
{
    bool changed = false;
    const float knobDiam = 40.f;
    const float knobRadius = knobDiam * 0.5f;
    const float sweepDeg = 280.f;
    const float startAngle = (180.f + (360.f - sweepDeg) * 0.5f); // degrees from up
    const float PI = 3.14159265359f;

    float range = (float)inputDef.range;
    if (range <= 0.f) range = 128.f;
    float minVal = (float)(inputDef.minVal * range);
    float maxVal = (float)(inputDef.maxVal * range);

    ImGui::PushID(id);

    // --- Left knob ---
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 center(pos.x + knobRadius, pos.y + knobRadius);

        // Background circle
        dl->AddCircleFilled(center, knobRadius, IM_COL32(40, 40, 45, 255));
        dl->AddCircle(center, knobRadius, IM_COL32(80, 80, 85, 255), 0, 1.5f);

        // Value needle
        float normL = (maxVal > minVal) ? (*valueL - minVal) / (maxVal - minVal) : 0.f;
        normL = std::max(0.f, std::min(1.f, normL));
        float angleDeg = startAngle + normL * sweepDeg;
        float angleRad = angleDeg * PI / 180.f;
        ImVec2 needleEnd(center.x + sinf(angleRad) * (knobRadius - 4.f),
                         center.y - cosf(angleRad) * (knobRadius - 4.f));
        dl->AddLine(center, needleEnd, IM_COL32(255, 255, 255, 255), 2.f);

        // Modulator needle (red)
        if (modL != 0.f)
        {
            float modNorm = std::max(0.f, std::min(1.f, (modL - minVal) / (maxVal - minVal)));
            float modAngleDeg = startAngle + modNorm * sweepDeg;
            float modAngleRad = modAngleDeg * PI / 180.f;
            ImVec2 modEnd(center.x + sinf(modAngleRad) * (knobRadius - 4.f),
                          center.y - cosf(modAngleRad) * (knobRadius - 4.f));
            dl->AddLine(center, modEnd, IM_COL32(255, 60, 60, 180), 1.5f);
        }

        // Invisible button for interaction
        ImGui::InvisibleButton("##knobL", ImVec2(knobDiam, knobDiam));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            float delta = -ImGui::GetIO().MouseDelta.y * 0.5f;
            if (ImGui::GetIO().KeyCtrl)
                delta /= 128.f;
            float step = delta / range;
            *valueL += step * (maxVal - minVal);
            *valueL = std::max(minVal, std::min(maxVal, *valueL));
            if (synced)
                *valueR = *valueL;
            changed = true;
        }

        // Text display below knob
        double normDisplay = *valueL / range;
        KnobLabel lbl = formatKnobValue(*valueL, range, inputDef.displayMapping, currentMode, nodeTypeID);
        std::string text = lbl.line2.empty() ? lbl.line1 : (lbl.line1 + "\n" + lbl.line2);
        ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImGui::SetCursorScreenPos(ImVec2(pos.x + (knobDiam - textSize.x) * 0.5f, pos.y + knobDiam + 2.f));
        ImGui::TextUnformatted(text.c_str());
    }

    if (!singleInput)
    {
        ImGui::SameLine(0.f, 8.f);

        // --- Right knob ---
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 center(pos.x + knobRadius, pos.y + knobRadius);

            dl->AddCircleFilled(center, knobRadius, IM_COL32(40, 40, 45, 255));
            dl->AddCircle(center, knobRadius, IM_COL32(80, 80, 85, 255), 0, 1.5f);

            float normR = (maxVal > minVal) ? (*valueR - minVal) / (maxVal - minVal) : 0.f;
            normR = std::max(0.f, std::min(1.f, normR));
            float angleDeg = startAngle + normR * sweepDeg;
            float angleRad = angleDeg * PI / 180.f;
            ImVec2 needleEnd(center.x + sinf(angleRad) * (knobRadius - 4.f),
                             center.y - cosf(angleRad) * (knobRadius - 4.f));
            dl->AddLine(center, needleEnd, IM_COL32(255, 255, 255, 255), 2.f);

            if (modR != 0.f)
            {
                float modNorm = std::max(0.f, std::min(1.f, (modR - minVal) / (maxVal - minVal)));
                float modAngleDeg = startAngle + modNorm * sweepDeg;
                float modAngleRad = modAngleDeg * PI / 180.f;
                ImVec2 modEnd(center.x + sinf(modAngleRad) * (knobRadius - 4.f),
                              center.y - cosf(modAngleRad) * (knobRadius - 4.f));
                dl->AddLine(center, modEnd, IM_COL32(255, 60, 60, 180), 1.5f);
            }

            ImGui::InvisibleButton("##knobR", ImVec2(knobDiam, knobDiam));
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                float delta = -ImGui::GetIO().MouseDelta.y * 0.5f;
                if (ImGui::GetIO().KeyCtrl)
                    delta /= 128.f;
                float step = delta / range;
                *valueR += step * (maxVal - minVal);
                *valueR = std::max(minVal, std::min(maxVal, *valueR));
                if (synced)
                    *valueL = *valueR;
                changed = true;
            }

            double normDisplay = *valueR / range;
            KnobLabel lbl = formatKnobValue(*valueR, range, inputDef.displayMapping, currentMode, nodeTypeID);
            std::string text = lbl.line2.empty() ? lbl.line1 : (lbl.line1 + "\n" + lbl.line2);
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            ImGui::SetCursorScreenPos(ImVec2(pos.x + (knobDiam - textSize.x) * 0.5f, pos.y + knobDiam + 2.f));
            ImGui::TextUnformatted(text.c_str());
        }
    }

    ImGui::PopID();
    return changed;
}

// ── VU Meter ─────────────────────────────────────────────────────────────

void VUMeter(float levelL, float levelR, float width, float height)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float barW = (width - 3.f) * 0.5f;

    // Background
    dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(20, 20, 25, 255));

    // Helper to draw a VU bar
    auto drawBar = [&](float x, float level) {
        level = std::max(0.f, std::min(1.f, level));
        float barH = level * height;
        float barY = pos.y + height - barH;

        // Green→yellow→red gradient based on level
        ImU32 color;
        if (level < 0.6f)
            color = IM_COL32(40, 200, 40, 255);
        else if (level < 0.85f)
            color = IM_COL32(220, 200, 40, 255);
        else
            color = IM_COL32(220, 40, 40, 255);

        dl->AddRectFilled(ImVec2(x, barY), ImVec2(x + barW, pos.y + height), color);
    };

    drawBar(pos.x, levelL);
    drawBar(pos.x + barW + 3.f, levelR);

    // Border
    dl->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(80, 80, 85, 255));

    ImGui::Dummy(ImVec2(width, height));
}

// ── Bit Pattern ──────────────────────────────────────────────────────────

bool BitPattern(const char* label, unsigned int* pattern)
{
    bool changed = false;
    ImGui::PushID(label);

    for (int i = 0; i < 8; i++)
    {
        if (i > 0) ImGui::SameLine(0.f, 2.f);
        bool bit = (*pattern >> i) & 1;
        ImGui::PushID(i);
        if (ImGui::Button(bit ? "#" : ".", ImVec2(20.f, 20.f)))
        {
            *pattern ^= (1u << i);
            changed = true;
        }
        ImGui::PopID();
    }

    ImGui::PopID();
    return changed;
}

} // namespace Widgets
} // namespace K64GUI
