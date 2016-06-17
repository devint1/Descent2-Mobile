#include <linux/limits.h>
#include <jni.h>
#include <stdlib.h>
#include "hmp.h"
#include "cfile.h"
#include "Music.h"

extern JavaVM *jvm;
extern jobject Activity;

extern void hmp2mid(hmp_file *hmp, unsigned char **midbuf, unsigned int *midlen);

void hmp_init() {

}

void hmp_stop(hmp_file *hmp) {
	stopMusic();
}

void hmp_setvolume(hmp_file *hmp, int volume) {
	setMusicVolume(volume / 128.0f);
}

int hmp_play(hmp_file *hmp, bool loop) {
	const char *midbuf;
	const char path[PATH_MAX];
	unsigned int midlen;
	FILE *file;
	JNIEnv *env;
	jclass clazz;
	jmethodID method;
	jstring jpath;

	sprintf(path, "%s/%s", Cache_path, "tmp.mid");
	hmp2mid(hmp, &midbuf, &midlen);
	file = fopen(path, "wb");
	fwrite(midbuf, 1, midlen, file);
	fclose(file);
	free(midbuf);
	(*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
	jpath = (*env)->NewStringUTF(env, path);
	clazz = (*env)->FindClass(env, "tuchsen/descent2/DescentActivity");
	method = (*env)->GetMethodID(env, clazz, "playMidi", "(Ljava/lang/String;Z)V");
	(*env)->CallVoidMethod(env, Activity, method, jpath, loop);
	(*env)->DeleteLocalRef(env, clazz);
	(*env)->DeleteLocalRef(env, jpath);
	return 0;
}
