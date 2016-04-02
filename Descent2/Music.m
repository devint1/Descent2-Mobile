//
//  Music.m
//  Descent
//
//  Created by Devin Tuchsen on 10/24/15.
//


#import <Foundation/Foundation.h>
#import "Music.h"

#include "cfile.h"

@import AVFoundation;

@interface LoopOperation: NSOperation

@end

@implementation LoopOperation {
	MusicPlayer *musicPlayer;
	unsigned int interval;
}

- (id)initWithInterval:(unsigned int)interv withMusicPlayer:(MusicPlayer*)player {
	self->interval = interv;
	self->musicPlayer = player;
	return [super init];
}

- (void)main {
	@autoreleasepool {
		while (1) {
			sleep(interval);
			if([self isCancelled]) {
				return;
			}
			MusicPlayerSetTime(*musicPlayer, 0);
		}
	}
}
@end

AVAudioPlayer *audioPlayer;
LoopOperation *looper;
NSOperationQueue *queue;
float redbookVolume;

//  This is a really gross way of doing looping since kSequenceTrackProperty_LoopInfo
//  was not working. Let me know if you can get it to work...
void startMidiLoop(unsigned int interval, MusicPlayer *musicPlayer) {
	looper = [[LoopOperation alloc] initWithInterval:interval withMusicPlayer:musicPlayer];
	if (!queue) {
		queue = [[NSOperationQueue alloc] init];
	}
	[queue addOperation:looper];
}

void stopMidiLoop() {
	[queue cancelAllOperations];
}

long getRedbookTrackCount() {
	NSString *directoryPath =[NSString stringWithFormat:@"%s/MUSIC", Resource_path];
	NSArray *directoryContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:directoryPath error:nil];
	if ([directoryContents count] <= 0) {
		return 0;
	}
	return [[[directoryContents objectAtIndex:[directoryContents count] - 1] substringToIndex:2] integerValue];
}

void setRedbookVolume(float volume) {
	redbookVolume = volume;
	[audioPlayer setVolume:volume];
}

int playRedbookTrack(int tracknum, int loop) {
	NSString *directoryPath =[NSString stringWithFormat:@"%s/MUSIC", Resource_path];
	NSString *trackString = [NSString stringWithFormat:@"%02d.*", tracknum];
	NSPredicate *predicate = [NSPredicate predicateWithFormat:@"SELF like %@", trackString];
	NSArray *directoryContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:directoryPath error:nil];
	NSArray *results = [directoryContents filteredArrayUsingPredicate:predicate];
	if ([results count] <= 0) {
		return 0;
	}
	NSURL *fileURL = [[NSURL alloc] initFileURLWithPath:[NSString stringWithFormat:@"%@/%@", directoryPath, [results objectAtIndex:0]]];
	audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:nil];
	if (loop) {
		audioPlayer.numberOfLoops = -1;
	}
	[audioPlayer setVolume:redbookVolume];
	[audioPlayer play];
	return [audioPlayer isPlaying];
}

void stopRedbook() {
	[audioPlayer stop];
}
