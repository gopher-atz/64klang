# 64klang
Official 64klang repository

Summary
-------

64klang is a modular, nodegraph based software synthesizer package intended to easily produce music for 64k intros (small executables with a maximum filesize of 65536 bytes containing realtime audio and visuals) or 32k executable music.

It consists of a VSTi plugin, a few example songs/instruments, as well as an example C project showing how to include it in your code for playback.

The repository contains the folders:
- VSTiPlugin (containing a .zip file with the precompiled VST plugin and example instruments/songs)
- Player (an example player project for using the exported files by the plugin)

<b>IMPORTANT:</b>

The 64klang2 VSTi plugin is currently 32bit only. If possible use a 32bit DAW, 64bit DAWs and bridging will most likely lead to crashes sooner or later. Also the system PATH environment variable MUST point to the directory where you stored the DLLs, so add that directory to the PATH.

For general requirements and how to get the VST plugin running PLEASE check the readme.txt in the .zip file.

Mostly keep in mind it is targeted for demoscene usage, which is the reason it acts as a singleton plugin managing ALL 16 midi channels in the same instance.

The player project here is based on Visual Studio 2015, so that and above should compile out of the box.

![64klang image](https://raw.githubusercontent.com/hzdgopher/64klang/master/64klang2.png)

Examples
--------

Some 64k intros using 64klang:

- http://www.pouet.net/prod.php?which=69669
- http://www.pouet.net/prod.php?which=69654
- http://www.pouet.net/prod.php?which=71570
- http://www.pouet.net/prod.php?which=71977

Some 32k exe music tracks using 64klang and experiments (by Paul Kraus (pOWL) and Jochen Feldkötter (Virgill)):
- https://soundcloud.com/powl-music/triangularity
- https://soundcloud.com/powl-music/the-last-sequencer
- https://soundcloud.com/virgill/sets/64klang2-modular-synth
- https://soundcloud.com/virgill/sets/64klang2-synthesis-experiments

History
-------

64klang is the big brother of 4klang ( https://github.com/hzdgopher/4klang )
And this version here is actually the second incarnation of 64klang, thats also why the VST is called 64klang2.

The first version of 64klang was created around 2010/2011 and was created specifically for use in a 64k intro together with Fairlight ( http://www.pouet.net/prod.php?which=57449 ). 

It was literally an extended 4klang, just extending the concept of a signal stack to a more generic and understandable nodegraph.
The synth core was build around the 4klang codebase and completely written in Assembler, the GUI was a raped native Win32 GUI with lots of dialog boxes connected by some spline curves (and no zooming capabilities).

It did its job, and convinced me of the nodegraph concept for a modular synth, but was simply not maintainable/extendable, not to speak of the usability of the GUI. So i took the good ideas from that and started again.

Most of the development of the new 64klang2 was done between 2012 and 2014, and since then once in a while some additions/fixes or requests were implemented. The current version has a C++ based synth core making heavy use of SSE4.1 instructions and for GUI i chose .NET WPF.

The probably quite unique thing about 64klang is its ability to have basically unlimited options to connect things for sound creation and processing. The complete nodegraph is evaluated per sample, which makes it possible to even do sample exact feedback loops and offers options such as physical modelling (delay based) on top of the usual AM, FM, subtracive synthesis. 

And now that it has been used in some 64k productions and 32k exemusic tracks it is finally time to relese it to the public.

Credits
-------

64klang was developed by
<br>Dominik Ries (gopher / Alcatraz).

Special thanks go out to Ralph Borson ( https://github.com/revivalizer ).
I stumbeled upon his blog just at the time where i needed inspiration on how to get the needed performance in the new 64klang. Unfortunately his blog seems to be down nowadays, still, these were the links i am grateful for:

http://revivalizer.dk/blog/2013/07/26/art-of-softsynth-development-simd-parallelization/
http://revivalizer.dk/blog/2013/07/28/art-of-softsynth-development-using-sse-in-c-plus-plus-without-the-hassle/

Also 64klang would not be in its current state withou the help and input of many talented friends and demosceners (just to name a few):

pOWL, virgill, xtr1m, muhmac, reed, jco, punqtured, ...



