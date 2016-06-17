//
//  Music.h
//  Descent
//
//  Created by Devin Tuchsen on 10/24/15.
//

#ifndef ANDROID_NDK
#import <AudioToolbox/AudioToolbox.h>

void startMidiLoop(unsigned int interval, MusicPlayer *musicPlayer);
void stopMidiLoop();
#endif

long getRedbookTrackCount();
void setMusicVolume(float volume);
int playRedbookTrack(int tracknum, int loop);
void stopMusic();
