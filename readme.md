# QuattroPlay

## About

This program implements the Quattro sound driver used by Namco games in the mid and late 90s. It can play music and sound effects from over 75 games, with a tracker-style pattern visualization, channel muting and more.

QuattroPlay is free software licensed under the GPL version 2.

## Build instructions

Only dependency is SDL2 (sudo apt-get install libsdl2-dev)

The makefile is tested on Linux Mint and Ubuntu.

The program works on macOS, but you might have to do modifications to the makefile. I can't help you there.
 
For Windows, use the codeblocks project file instead.

## Usage

Currently zipped MAME ROMs are not supported, you will have to store them in a subdirectory under /roms.

Then run it from command line/terminal:

	bin/Debug/QuattroPlay.exe <gamename>
or

	./bin/Debug/QuattroPlay <gamename>
 
## GUI usage

The current user interface is a bit complicated... Currently it only uses the keyboard.

Use the arrow keys to move the selection and press __Enter__ once to edit the value. Use the arrow keys to set the value and press __Enter__ again to set it.

There are three groups of values. The first are _Track_ slots. By entering a value here you can play music or sound effects. The second group control _Registers_. These are used to control various parameters of the song. The third group is not editable but used to display information on the 32 _Voices_ (sound channels).

To simply play sounds, simply move the selection to the top left value (Track 0), and press __I__ or __D__ to modify the value. Press __S__ to stop a sound or __R__ to restart.

By highlighting one of the values in the last group you can mute or solo channels by highlighting them and pressing __M__ or __S__.

## Command line usage

	./bin/Debug/QuattroPlay [options] <gamename> [<song ID>]

If Song ID is specified, the song will automatically start. If enabled with
the -w or -v parameters, the filenames will also contain the game name and
song ID.
 
*	`-ini`: Set game config path
*	`-w`: log to WAV.
*	`-v`: log to VGM.
 
## Key bindings (a mess)

This might not be complete yet.
 
###	Global
*	__Q__: exit
*	__P__: pause
*	__U__: update tick (be sure to pause first)
*	__F3__: Reset sound driver
*	__F5__: toggle stereo/quad output
*	__F6__: unmute all voices
*	__F7__: decrease volume
*	__F8__: increase volume
*	__F10__: toggle fast forward
*	__F11__: log sound to file
	*	Logs started from the GUI have filenames hardcoded to `qp_log.wav`. Don't log for too long; 30 seconds = 30 MB.
	*	Format: 32-bit float, 4 channels, rate is either 85333 or 88200.
*	__F12__: display statistics
*	__Arrow keys__: move selection
*	__0__-__9__: Preset action (song triggers, etc)
	*	These are configured in the game config files, check the `/ini` directory.
###	if Track selected
*	__L__: display mode
### if Track or Register selected
*	__C__: clear register/track
*	__S__: stop track
*	__I__: increase value
*	__D__: decrease value
*	__Enter__: enter edit mode, while in Edit mode:
	*	__Up__/__Down__: add/remove `0x10`
	*	__Left__/__Right__: add/remove `0x01`
	*	__Shift__: Hold down to multiply increment with `0x100`
	*	__Enter__: play song/change register
	*	__Esc__: exit edit mode
### if Voice selected
*	__M__: Mute voice
*	__S__: Solo voice
*	__R__: Reset mute/solo for all voices. Same as __F6__.

## Notes
*	Mostly everything works.
*	Position envelopes (used by a few songs in _Cyber Commando_) are not supported.
*	No playlist support. It won't happen until the frontend code is rewritten.
*	The UI performance isn't great on Linux and macOS. This appears to be due to a prolem with SDL's own rendering API.
