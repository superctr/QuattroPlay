# QuattroPlay

## About

This program implements the sound drivers used by Namco arcade games from the mid 80s to the late 90s. It can play music and sound effects from over 140 games, with a tracker-style pattern visualization, channel muting and more.

QuattroPlay is free software licensed under the GPL version 2.

### Supported Sound Drivers

*   Thunder Ceptor (FM only)
*   System 86 (FM only)
*   System 1 (Except some games)
*   System 2/21 (all known games supported)
*   NA-1/2 (all known games supported)
*   Quattro (System 11/12/22/23/NB/FL, all known games supported)

## Build instructions

Only dependency is SDL2 (sudo apt-get install libsdl2-dev)

The makefile is tested on Linux Mint, Ubuntu and Windows (using MSYS).

The program works on macOS, but you might have to do modifications to the makefile. I can't help you there.

## Usage

Currently zipped MAME ROMs are not supported, you will have to store them in a subdirectory under /roms.

Then run it from command line/terminal:

	bin/QuattroPlay.exe <gamename>
or

	./bin/QuattroPlay <gamename>
 
Running without the <gamename> argument will allow you to select a game from a menu. It is also possible to load .ini files with associated data and wave files by drag and drop while the program is running.

## GUI usage

The current user interface is a bit complicated... Currently it only uses the keyboard.

Use the arrow keys to move the selection and press __Enter__ once to edit the value. Use the arrow keys to set the value and press __Enter__ again to set it.

There are three groups of values. The first are _Track_ slots. By entering a value here you can play music or sound effects. The second group control _Registers_. These are used to control various parameters of the song. The third group is not editable but used to display information on the 32 _Voices_ (sound channels).

To simply play sounds, simply move the selection to the top left value (Track 0), and press __I__ or __D__ to modify the value. Press __S__ to stop a sound or __R__ to restart.

By highlighting one of the values in the last group you can mute or solo channels by highlighting them and pressing __M__ or __S__.

## Command line usage

	./bin/QuattroPlay [options] <gamename> [<song ID>]

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
*	__Esc__: Go to main screen
*	__F1__: Go to about screen. If no game is loaded, select audio device.
*	__F3__: Reset sound driver. If no game is loaded, refresh game definitions.
*	__F5__: toggle stereo/quad output
*	__F6__: unmute all voices
*	__F7__: decrease volume
*	__F8__: increase volume
*	__F10__: toggle fast forward
*	__F11__: log sound to file
	*	Logs started from the GUI have filenames hardcoded to `qp_log.wav`. Don't log for too long; 30 seconds = 30 MB.
	*	Format: 32-bit float, 4 channels, rate is either 85333 or 88200.
*	__F12__: display rendering stats
*	__Space__: Go to playlist screen
*	__Arrow keys__: move selection

### Playlist screen

*	__Enter__: Play selected song
*	__R__: Restart current song
*	__N__: Play next song
*	__B__: Play previous song
*   __1__: Make song selection follow the playlist (could be useful for videos)
*	__L__: Display keyboard, while active:
	*	__8__: Show pitch modulation
	*	__9__: Show volume modulation
	*   __I__: Transpose one octave down (shift notes right)
	*   __D__: Transpose one octave up (shift notes left)

### Main screen

*	__0__-__9__: Preset action (song triggers, etc)
	*	These are configured in the game config files, check the `/ini` directory.
	
#### if Track selected

*	__L__: Change between display modes (e.g. pattern view)

#### if Track or Register selected

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

#### if Voice selected

*	__M__: Mute voice
*	__S__: Solo voice
*	__R__: Reset mute/solo for all voices. Same as __F6__.

## Notes

*	Playlists are all defined in the .INI files. If you find any errors or have any suggestions, please create an issue in the github repository.
*   The C30 chip is not emulated, instead the C352 is used. This makes VGM logging possible.
*   The C140 chip is not emulated, instead the C352 is used. The compressed sample format differs, breaking VGM logs in a few games.
*   The C219 chip is not emulated, instead the C352 is used. This actually improves the sound quality of VGM logs, as the C219 chip is not accurately emulated in VGM players.
*	Position envelopes (used by a few songs in _Cyber Commando_) are not supported.

## Copyright

&copy; 2016-2017 Ian Karlsson

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
