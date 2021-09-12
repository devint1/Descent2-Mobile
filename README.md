Descent II Mobile
=================

This is a source port of the original DOS/Windows/Mac game "Descent II" for iOS and Android. You will need the game files from a licensed copy in order to play it.

**NOTE: I am not currently maintaining/testing this on iOS. It may work, but your mileage may vary.**

Licence
-------
Please see "COPYING."

Required Game Data
------------------
The following files need to be copied to `Descent2/Data` (iOS) or `Descent2/src/main/assets` (Android) prior to building:

- alien1.pig
- alien2.pig
- descent2.ham
- descent2.hog
- descent2.s22 -or- descent2.s11
- fire.pig
- groupa.pig
- ice.pig
- intro-h.mvl -or- intro-l.mvl
- other-h.mvl -or- other-l.mvl
- robots-h.mvl -or- robots-l.mvl
- water.pig

Rebook Music
------------
Redbook music can be used instead of MIDI music, if desired. Descent2-Mobile will search for music in `Descent2/Data/music` (iOS) or `Descent2/src/main/assets/music` (Android) with the pattern `<tracknum>.*`, where `<tracknum>` is the track number as it would appear on the original Descent II CD. Descent2-Mobile uses the same scheme as the original version of Descent II, where track 02 is the title track, track 03 is the credits track, and the remaining tracks are used for the levels. The files can be in any format supported by `AVAudioPlayer` (iOS) or `MediaPlayer` (Android).

Building (Android)
------------------
Copy the Descent II data files to `Descent2/src/main/assets`, then open the project in Android Studio. If needed, install the required build tools, SDK, and NDK. From there, simply build/run the project. The Android version only supports the OpenGL ES renderer. Tested on an NVIDIA Shield Tablet K1 running Android 6.0.1 (Marshmallow) and a Nexus 6P running Android 6.0.1.

Building (iOS)
--------------
Copy the Descent II data files to `Descent2/Data`, then open the Xcode workspace “Descent2-Mobile.xcworkspace.” You may wish to select the renderer (OpenGL ES or software) prior to building; select a scheme from the Product → Scheme menu. Once this is done, select your platform and build! This was tested on an iPhone 5s running iOS 9.3. Apple now allows you to run on an iDevice for free, so long as you have an Apple ID.

Issues and Limitations
----------------------
Please see the “Issues” section in GitHub.

Special Thanks
--------------
- Merlin Silver soundfont for MIDI instrument playback - http://www.soundfonts.gonet.biz
- HMP to MIDI conversion code - Hans de Goede, d1x project
- LIBMVE, d2x project contributors

