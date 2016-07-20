Usage:
	put configs and ROMs in the configs and roms directory respectively.
	then run it from command line / terminal:
 
		bin/Debug/QuattroPlay.exe <gamename>
			or
		./bin/Debug/QuattroPlay <gamename>
 
Basic usage:
 To play a song, move the cursor (highlighted value) with the arrow keys to the
 top left entry and press ENTER once to set the song ID with the arrow keys.
 Press ENTER again to play the song. You can also press I and D to increment or
 decrement the song ID and you can press R to restart the song. Once a song is
 playing you can explore tracks/voices with the arrow keys...

Slightly more advanced usage:
 On the left you'll see 3 'groups' of values. First group is tracks/songs,
 second group is registers (variables that can control various parameters) and
 the third group shows the chip voices.
 You can move the cursor (the highlighted value) with the arrow keys.
 If the cursor is in the first two groups, press ENTER once to edit the value
 and ENTER again to set the value.

Controls (a mess)
	Global
		Q: exit
		P: pause
		U: update tick (be sure to pause first)
		F3: Reset sound driver
		F5: toggle stereo/quad output
		F6: unmute all voices
		F7: decrease volume
		F8: increase volume
		F10: toggle fast forward
		F11: log sound to file
			Filename is currently hardcoded to "qp_log.raw"
			Format: (Raw 32-bit float PCM, 4 channels, rate is either 85333 or 88200)
			don't log for too long; 30 seconds = 30 MB.
		F12: display statistics
		Arrow keys: move selection
	if Track/Register selected	
		C: clear register/track
		S: stop track
		I: increase value
		D: decrease value
		Enter: enter edit mode, while in Edit mode:
			up/down: add/remove 0x10
			left/right: add/remove 1
			enter: play song/change register
			Esc: exit edit mode
	if Voice selected:
		M: mute voice
		S: solo voice
		R: reset mute/solo

Notes:
	mostly everything works.
	position envelopes (used by a few songs in Cyber Commando) are not supported.
	No playlist support. It won't happen until the frontend code is rewritten.
