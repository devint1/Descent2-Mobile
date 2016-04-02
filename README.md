Descent II for iOS
==================

This is a source port of the original DOS/Windows/Mac game "Descent II" for iOS. You will need the game files from a licensed copy in order to play it.

Licence
-------
Please see "COPYING."

Requirements
------------
- Xcode 7.2 or later
- iOS 9.2 or later
- If you want to run on an iDevice: http://www.apple.com/ios/whats-new/#compatibility

Required Game Data
------------------
The following files need to be copied to Descent2/Data prior to building:

- ALIEN1.PIG
- ALIEN2.PIG
- DESCENT2.HAM
- DESCENT2.HOG
- DESCENT2.S22 -or- DESCENT2.S11
- FIRE.PIG
- GROUPA.PIG
- ICE.PIG
- INTRO-H.MVL -or- INTRO-L.MVL
- OTHER-H.MVL -or- OTHER-L.MVL
- ROBOTS-H.MVL -or- ROBOTS-L.MVL
- WATER.PIG

Rebook Music
—-----------
Redbook music can be used instead of MIDI music, if desired. Descent2-iOS will search for music in `Descent2/Data/MUSIC` with the pattern `<tracknum>.*`, where `<tracknum>` is the track number as it would appear on the original Descent II CD. Descent2-iOS uses the same scheme as the original version of Descent II, where track 02 is the title track, track 03 is the credits track, and the remaining tracks are used for the levels. The files can be in any format supported by `AVAudioPlayer`.

Building
--------
You will need the data files from the full version of Descent II in order to play. Copy these to the `Descent2/Data` directory, then open the Xcode workspace “Descent2-iOS.xcworkspace.” You may wish to select the renderer (OpenGL ES or software) prior to building; select a scheme from the Product → Scheme menu. Once this is done, select your platform and build! This was tested on an iPhone 5s running iOS 9.3. Apple now allows you to run on an iDevice for free, so long as you have an Apple ID.

Issues and Limitations
----------------------
Please see the “Issues” section in GitHub.

Special Thanks
--------------
- Merlin Silver soundfont for MIDI instrument playback - http://www.soundfonts.gonet.biz
- HMP to MIDI conversion code - Hans de Goede, d1x project
- LIBMVE, d2x project contributors

