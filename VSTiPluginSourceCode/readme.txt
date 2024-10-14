64klang is a modular, nodegraph based software synthesizer package intended to easily produce music for 64k intros or 32k executable music.

Requirements:
- 64klang2 VSTi plugin is currently 32bit only. If possible use a 32bit DAW, 64bit DAWs and bridging will most likely lead to crashes sooner or later.
- The system PATH environment variable must point to the directory where the DLLs are located, so add that directory to the PATH.
- If 64klang2 is not listed in your DAW plugins list, check the file properties of all dlls with windows explorer. windows has the habit to block execution for downloaded dlls sometimes. 
- .NET Client Framework 4.0 installed
- Microsoft Visual C++ 2015 Redistributable Package (x86) installed
- SSE4.1 capable CPU

Plugin startup info:
64klang2 is a singleton VSTi, which means you may only have one physical instance of the plugin loaded in your VST host (similar to 4klang)
To use more than 1 channel with 64klang2 you need to route the other midi channel to that one physical plugin instance (in Renoise this can be done via alias Instruments).

Basic Navigation:
- panning       : press (hold) right mouse button and move the mouse
- zooming       : rotate the mouse wheel
- select node(s): press left mouse button on a node(optional: SHIFT to select input subtree nodes as well. CTRL key to add or remove to existing selection), or draw selection rectangle on the background
- move node(s)  : press (hold) left mouse button on a selected node to move all current selected nodes when moving the mouse
- create node   : press right mouse button on the background to open the context menu. select a node type in the menu to create it (optionally press CTRL to force it to become a global node (pink))
- delete node(s): press the red x on the node (all currently selected nodes will be deleted as well).
- connect nodes : click left mouse button on the output of a node (the red connector line now follows the mouse), left click on the input of a target node to establish connection. left click on the background to stop connecting.
- insert nodes  : right click directly on a connection (the connection turns green) to open the context menu for node insertion. the selected node will be placed between the 2 existing nodes of that connection.
- show params   : click on the button at the bottom of the node to open the edit window next to the node (optionally press CTRL to add the edit window to the overlay dock panel on the left side of the window)
- change params : rotate the knobs in the edit window (optionally press CTRL to rotate with the high precision mode)
- jump to       : select a midi channel from the "Jump To" combo box in the menu to quick jump to and highlight the selected channel node
- rename node	: double click left mouse button on the node title, you can then edit the text of the node.
- search	: type text into the "Search" text box  in the menu to highlight nodes with (partially) matching names (zooming out helps also)
- copy node(s)  : selected nodes can be instantly pasted via right mouse button context menu "Paste Selection" (linux style)
- load/save     : complete patches can be loaded/saved from the main menu. channels can be loaded/saved via the respective channel root node (use jump to for fast access). selected nodes (with connection to each other) can be saved as selection as well (e.g. for a good filter bank or even just a node with good parameter set).

Some useful Information:
- your host must use 44100hz sampling rate. Anything else will probably glitch at some point as internal constants are tweaked to 44100hz.
- pink nodes are global nodes, so they exist only once.
- blue nodes are voice nodes. they will be instanciated from the connected global "VoiceManager" for each note that it triggers.
- therefore it is allowed to have global nodes as input for voice nodes, but not the other way round (global to global and voice to voice always is allowed).
- usually the GUI will let you know when you try to connect things in a wrong way.
- 64klang2 uses stereo signals throughout, therefore you have usually 2 knobs in the node edit window which can be totally independent from each other.
- 64klang2 processes the whole signal network on a per sample basis, so signal feedback loops are perfectly possible
- same goes for cross channel connections, also possible (e.g. for sidechaining)


More to come ...

gopher/alcatraz 2018