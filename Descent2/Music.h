//
//  Music.h
//  Descent
//
//  Created by Devin Tuchsen on 10/24/15.
//

#import <AudioToolbox/AudioToolbox.h>

void startMidiLoop(unsigned int interval, MusicPlayer *musicPlayer);
void stopMidiLoop();
long getRedbookTrackCount();
void setRedbookVolume(float volume);
int playRedbookTrack(int tracknum, int loop);
void stopRedbook();
