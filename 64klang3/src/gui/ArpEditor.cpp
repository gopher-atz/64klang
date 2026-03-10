#include "ArpEditor.h"
#include "core/SynthController.h"

#include <algorithm>
#include <cstdio>

namespace K64GUI {

bool ArpEditor::draw(int nodeID)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return false;

    bool changed = false;

    // Get loop length from step -1 (stored as special index)
    int loopLen = sc->getArpStepData((DWORD)nodeID, (DWORD)-1);
    if (loopLen <= 0) loopLen = 16;
    if (loopLen > kMaxSteps) loopLen = kMaxSteps;

    // Loop length slider
    if (ImGui::SliderInt("Steps", &loopLen, 1, kMaxSteps))
    {
        sc->setArpStepData((DWORD)nodeID, (DWORD)-1, (DWORD)loopLen);
        changed = true;
    }

    // Get current play position
    int playPos = sc->getArpPlayPos((DWORD)nodeID);

    // Draw piano roll grid
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float totalW = (float)loopLen * kStepWidth;
    float totalH = (float)(kOctaves * kNotesPerOctave) * kNoteHeight;

    // Background
    dl->AddRectFilled(origin, ImVec2(origin.x + totalW, origin.y + totalH),
                      IM_COL32(30, 30, 35, 255));

    // Horizontal lines for each note (highlight C notes)
    for (int note = 0; note <= kOctaves * kNotesPerOctave; note++)
    {
        float y = origin.y + (float)note * kNoteHeight;
        ImU32 lineColor = (note % 12 == 0) ? IM_COL32(100, 100, 110, 255) : IM_COL32(50, 50, 55, 255);
        dl->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + totalW, y), lineColor);
    }

    // Vertical lines for each step
    for (int step = 0; step <= loopLen; step++)
    {
        float x = origin.x + (float)step * kStepWidth;
        ImU32 lineColor = (step % 4 == 0) ? IM_COL32(100, 100, 110, 255) : IM_COL32(50, 50, 55, 255);
        dl->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + totalH), lineColor);
    }

    // Draw existing notes and play cursor
    for (int step = 0; step < loopLen; step++)
    {
        int stepData = sc->getArpStepData((DWORD)nodeID, (DWORD)step);
        int transpose = getTranspose(stepData);
        int velocity = getVelocity(stepData);

        if (velocity > 0 && transpose < kOctaves * kNotesPerOctave)
        {
            float x = origin.x + (float)step * kStepWidth;
            float y = origin.y + (float)(kOctaves * kNotesPerOctave - 1 - transpose) * kNoteHeight;
            int alpha = 100 + (velocity * 155 / 255);
            dl->AddRectFilled(ImVec2(x + 1, y + 1),
                              ImVec2(x + kStepWidth - 1, y + kNoteHeight - 1),
                              IM_COL32(100, 180, 255, alpha));
        }

        // Play cursor
        if (step == playPos)
        {
            float x = origin.x + (float)step * kStepWidth;
            dl->AddRectFilled(ImVec2(x, origin.y),
                              ImVec2(x + kStepWidth, origin.y + totalH),
                              IM_COL32(255, 255, 255, 40));
        }
    }

    // Handle mouse interaction
    ImGui::InvisibleButton("##arpGrid", ImVec2(totalW, totalH));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        int clickStep = (int)((mousePos.x - origin.x) / kStepWidth);
        int clickNote = kOctaves * kNotesPerOctave - 1 - (int)((mousePos.y - origin.y) / kNoteHeight);

        if (clickStep >= 0 && clickStep < loopLen && clickNote >= 0 && clickNote < kOctaves * kNotesPerOctave)
        {
            int stepData = sc->getArpStepData((DWORD)nodeID, (DWORD)clickStep);
            int oldTranspose = getTranspose(stepData);
            int oldVelocity = getVelocity(stepData);

            if (ImGui::GetIO().KeyCtrl)
            {
                // Ctrl+click: clear step
                sc->setArpStepData((DWORD)nodeID, (DWORD)clickStep, 0);
            }
            else if (oldVelocity > 0 && oldTranspose == clickNote)
            {
                // Click on existing note: remove it
                sc->setArpStepData((DWORD)nodeID, (DWORD)clickStep, 0);
            }
            else
            {
                // Place note with default velocity 127 and gate length 1
                int newData = packStepData(clickNote, 1, 127);
                sc->setArpStepData((DWORD)nodeID, (DWORD)clickStep, (DWORD)newData);
            }
            changed = true;
        }
    }

    return changed;
}

} // namespace K64GUI
