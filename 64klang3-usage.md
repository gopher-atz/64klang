# 64klang3 Usage Guide — Mouse & Keyboard

Summary of mouse and keyboard interactions in the 64klang3 rewrite (feat/64klang3 branch).

---

## Canvas Navigation

| Action | Result |
|--------|--------|
| **Mouse wheel** | Zoom in/out (1.1× per notch). Zoom pivots around the cursor. Range: 3%–500%. |
| **Right-click + drag** | Pan the canvas. Context menu only appears if you release without panning (no significant movement). |
| **Jump To dropdown** (toolbar) | Center the canvas on a channel (1–16) or SynthRoot. |

---

## Node Selection

| Action | Result |
|--------|--------|
| **Left-click on node** | Select node; clear previous selection if not already selected. |
| **Left-click on empty canvas** | Clear selection and start rubber-band selection. |
| **Shift + left-click on node** | Recursively select upstream nodes. Clears selection first unless Ctrl is held. |
| **Ctrl + left-click on node** | Toggle node in/out of selection. |
| **Ctrl + left-click on empty canvas** | Add to existing selection (no clear). |
| **Left-click + drag on empty area** | Rubber-band selection; Ctrl adds to selection. |
| **Left-click + drag on selected node** | Drag selected node(s) after crossing small threshold. |

---

## Wiring

| Action | Result |
|--------|--------|
| **Left-click on output pin** | Start wire drag (ghost wire follows cursor). |
| **Shift + left-click on output pin** | Recursive select only; no wire drag. |
| **Left-click on input pin** (during wire drag) | Connect wire to that input; stay in wire-drag mode for continuous wiring. |
| **Left-click on input pin** (no wire drag) | Disconnect existing wire from that input. |
| **Left-click on empty canvas** (during wire drag) | Open context menu to insert new node; new node auto-connects. |
| **Left-click on node body** (during wire drag) | Consume click; stay in wire-drag mode. |
| **Left-click on output pin** (during wire drag) | Ignore; stay in wire-drag mode. |
| **Add Input pill** (MultiAdd / NoteController) | Wire can be dropped on it to add a new input slot. |

---

## Context Menu

| Action | Result |
|--------|--------|
| **Right-release on empty canvas** | Open node-creation context menu at that position. |
| **Right-release on wire** | Open menu to insert a node in the middle of that wire. |
| **Ctrl when selecting from menu** | Create a global node (for both regular and wire-insert modes). |
| **Stop wiring** | Menu entry shown during continuous wire mode; ends wire drag without inserting. |

---

## Node Actions

| Action | Result |
|--------|--------|
| **Left-click red X** (node corner) | Delete node. If selected, delete all selected nodes; otherwise delete only that node. |
| **Left-click Edit button** (node bottom) | Open/close parameter edit panel. |
| **Double-click node** (ChannelRoot / VoiceManager) | Toggle mute. |
| **Load Channel / Save Channel** (ChannelRoot) | Native file dialog to load/save `.64k2Channel` for that channel. |
| **Channel name label** (ChannelRoot) | Text below the node showing the loaded channel filename (empty when initial or after reset). |

---

## Edit Panel

| Action | Result |
|--------|--------|
| **Left-click red X** (header) | Close edit panel. |
| **Left-click blue "0"** (header) | Reset all parameters to defaults. |
| **Left-click knob** | Start vertical drag to adjust value; up = increase. |
| **Vertical drag on knob** | Change value. |
| **Ctrl + vertical drag** | Fine adjustment (1/128 steps). |
| **Double-click left knob** | Reset to factory default (both channels if synced). |
| **Double-click right knob** | Reset right channel only (when not synced). |
| **Left-click Sync checkbox** | Toggle L/R sync; when synced, right channel follows left. |
| **Left-click mode dropdown** | Open list; select item to change mode. |

---

## Keyboard Shortcuts (canvas focused)

| Shortcut | Result |
|----------|--------|
| **Delete** | Delete selected nodes (structural nodes protected). |
| **Ctrl+C** | Copy selected nodes (positions, connections, values). |
| **Ctrl+V** | Paste at cursor; select pasted nodes. |
| **Ctrl+Shift+D** | Toggle debug overlay (mouse, zoom, offset, hit info). |
| **Ctrl+W** | Toggle wave file dialog. |

---

## Arpeggiator Editor (VoiceManager edit panel)

| Action | Result |
|--------|--------|
| **Left-click in main grid** (note area) | Create new note and start drag to set duration. |
| **Left-click in loop zone** (top row) | Set loop-start step. |
| **Left-drag in main grid** | Extend or shorten note duration (after small horizontal dead zone). |
| **Ctrl + left-click on note bar** | Delete that note. |
| **Vertical drag on transpose label** | Change transpose value (0–48). |
| **Vertical drag on velocity label** | Change velocity (1–127). |

---

## Toolbar

| Control | Result |
|---------|--------|
| **Load Patch** | Open file dialog; load `.64k2Patch`. |
| **Save Patch** | Save patch to `.64k2Patch`. |
| **Reset Patch** | Reset patch to default. |
| **Wavetables** | Toggle wavetable file dialog. |
| **Export Patch** | Export patch as C header (`.h`). |
| **Export Song** | Export song as C header (`.h`). |
| **Quantization combo** | Choose sample count (16–256) for song export. |
| **Jump To** | Select channel to center canvas. |
| **Search** | Filter nodes by name or type; matching nodes show cyan border. |
| **P.A.N.I.C** | Kill all voices (panic). |

---

## Notes

- **Ctrl / Shift** use `GetKeyState` on Windows so they work even when the plugin window lacks keyboard focus (many hosts don't forward key events).
- Edit panels can be stacked; only the topmost panel under the cursor receives clicks.
- Knob dragging continues across the panel; mouse release anywhere stops it.
- Popups (dropdowns, context menus) suppress canvas hit-testing to avoid accidental canvas actions when closing them.
