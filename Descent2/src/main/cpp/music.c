//
// Created by devin on 6/12/16.
//

#include <jni.h>

extern JavaVM *jvm;
extern jobject Activity;

long getRedbookTrackCount() {
	JNIEnv *env;
	jclass clazz;
	jmethodID method;
	jint result;

	(*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
	clazz = (*env)->FindClass(env, "tuchsen/descent2/DescentActivity");
	method = (*env)->GetMethodID(env, clazz, "getRedbookTrackCount", "()I");
	result = (*env)->CallIntMethod(env, Activity, method);
	(*env)->DeleteLocalRef(env, clazz);
	return result;
}

void setMusicVolume(float volume) {
	JNIEnv *env;
	jclass clazz;
	jmethodID method;

	(*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
	clazz = (*env)->FindClass(env, "tuchsen/descent2/DescentActivity");
	method = (*env)->GetMethodID(env, clazz, "setMusicVolume", "(F)V");
	(*env)->CallVoidMethod(env, Activity, method, volume);
	(*env)->DeleteLocalRef(env, clazz);
}

int playRedbookTrack(int tracknum, int loop) {
	JNIEnv *env;
	jclass clazz;
	jmethodID method;
	jboolean result;

	(*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
	clazz = (*env)->FindClass(env, "tuchsen/descent2/DescentActivity");
	method = (*env)->GetMethodID(env, clazz, "playRedbookTrack", "(IZ)Z");
	result = (*env)->CallBooleanMethod(env, Activity, method, tracknum, loop);
	(*env)->DeleteLocalRef(env, clazz);
	return result;
}

void stopMusic(){
	JNIEnv *env;
	jclass clazz;
	jmethodID method;

	(*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
	clazz = (*env)->FindClass(env, "tuchsen/descent2/DescentActivity");
	method = (*env)->GetMethodID(env, clazz, "stopMusic", "()V");
	(*env)->CallVoidMethod(env, Activity, method);
	(*env)->DeleteLocalRef(env, clazz);
}
