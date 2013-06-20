/*
 * ijkplayer_jni.c
 *
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <jni.h>
#include "ijksdl_android/ijksdl_android.h"
#include "ijkutil/ijkutil.h"
#include "ffplay/ff_ffplay.h"
#include "ijkplayer_android.h"

#define JNI_MODULE_PACKAGE      "tv/danmaku/ijk/media/player"
#define JNI_CLASS_IJKPLAYER     "tv/danmaku/ijk/media/player/IjkMediaPlayer"
#define JNI_IJK_MEDIA_EXCEPTION "tv/danmaku/ijk/media/player/IjkMediaException"

#define IJK_CHECK_MPRET_GOTO(retval, env, label) \
    JNI_CHECK_GOTO((retval != EIJK_INVALID_STATE), env, "java/lang/IllegalStateException", NULL, LABEL_RETURN); \
    JNI_CHECK_GOTO((retval != EIJK_OUT_OF_MEMORY), env, "java/lang/OutOfMemoryError", NULL, LABEL_RETURN); \
    JNI_CHECK_GOTO((retval < 0), env, JNI_IJK_MEDIA_EXCEPTION, NULL, LABEL_RETURN);

static JavaVM* g_jvm;

typedef struct player_fields_t {
    pthread_mutex_t mutex;
    jclass clazz;

    jfieldID mNativeMediaPlayer;

    jfieldID surface_texture;

    jmethodID post_event;
} player_fields_t;
static player_fields_t g_clazz;

static IjkMediaPlayer *jni_get_media_player(JNIEnv* env, jobject thiz)
{
    pthread_mutex_lock(&g_clazz.mutex);

    IjkMediaPlayer *mp = (IjkMediaPlayer *) (intptr_t) (*env)->GetLongField(env, thiz, g_clazz.mNativeMediaPlayer);
    if (mp) {
        ijkmp_inc_ref(mp);
    }

    pthread_mutex_unlock(&g_clazz.mutex);
    return mp;
}

static IjkMediaPlayer *jni_set_media_player(JNIEnv* env, jobject thiz, IjkMediaPlayer *mp)
{
    pthread_mutex_lock(&g_clazz.mutex);

    IjkMediaPlayer *old = (IjkMediaPlayer*) (intptr_t) (*env)->GetLongField(env, thiz, g_clazz.mNativeMediaPlayer);
    if (mp) {
        ijkmp_inc_ref(mp);
    }
    (*env)->SetLongField(env, thiz, g_clazz.mNativeMediaPlayer, (intptr_t) mp);

    pthread_mutex_unlock(&g_clazz.mutex);

    // NOTE: ijkmp_dec_ref may block thread
    if (old != NULL) {
        ijkmp_dec_ref(&old);
    }

    return old;
}

static void
IjkMediaPlayer_setDataSourceAndHeaders(
    JNIEnv *env, jobject thiz, jstring path,
    jobjectArray keys, jobjectArray values)
{
    int retval = 0;
    const char *c_path = NULL;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(path, env, "java/lang/IllegalArgumentException", "mpjni: setDataSource: null path", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setDataSource: null mp", LABEL_RETURN);

    c_path = (*env)->GetStringUTFChars(env, path, NULL);
    JNI_CHECK_GOTO(c_path, env, "java/lang/OutOfMemoryError", "mpjni: setDataSource: path.string oom", LABEL_RETURN);

    ALOGV("setDataSource: path %s", c_path);
    retval = ijkmp_set_data_source(mp, c_path);
    (*env)->ReleaseStringUTFChars(env, path, c_path);

    IJK_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static void
IjkMediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setVideoSurface: null mp", LABEL_RETURN);

    SDL_Vout *vout = ijkmp_get_vout(mp);
    assert(vout);

    SDL_VoutAndroid_SetAndroidSurface(vout, env, jsurface);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
    return;
}

static void
IjkMediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
    int retval = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: prepareAsync: null mp", LABEL_RETURN);

    retval = ijkmp_prepare_async(mp);
    IJK_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static void
IjkMediaPlayer_start(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: start: null mp", LABEL_RETURN);

    ijkmp_start(mp);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static void
IjkMediaPlayer_stop(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: stop: null mp", LABEL_RETURN);

    ijkmp_stop(mp);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static void
IjkMediaPlayer_pause(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: pause: null mp", LABEL_RETURN);

    ijkmp_pause(mp);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static void
IjkMediaPlayer_seekTo(JNIEnv *env, jobject thiz, int msec)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: seekTo: null mp", LABEL_RETURN);

    ijkmp_seek_to(mp, msec);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static jboolean
IjkMediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    jboolean retval = JNI_FALSE;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: isPlaying: null mp", LABEL_RETURN);

    retval = ijkmp_is_playing(mp) ? JNI_TRUE : JNI_FALSE;

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
    return retval;
}

static int
IjkMediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    int retval = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getCurrentPosition: null mp", LABEL_RETURN);

    retval = ijkmp_get_current_position(mp);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
    return retval;
}

static int
IjkMediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    int retval = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getDuration: null mp", LABEL_RETURN);

    retval = ijkmp_get_duration(mp);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
    return retval;
}

static void
IjkMediaPlayer_release(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    if (!mp)
        return;

    // explicit close mp
    ijkmp_shutdown(mp);

    SDL_Vout *vout = ijkmp_get_vout(mp);
    if (vout) {
        ijkmp_set_vout(mp, NULL);
        SDL_VoutFree(vout);
    }

    ijkmp_dec_ref(&mp);
    ijkmp_dec_ref(&mp);
    jni_set_media_player(env, thiz, NULL);
}

static void
IjkMediaPlayer_reset(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: reset: null mp", LABEL_RETURN);

    ijkmp_reset(mp);

    LABEL_RETURN:
    ijkmp_dec_ref(&mp);
}

static void
IjkMediaPlayer_native_init(JNIEnv *env)
{
    // FIXME: implement
}

static void
IjkMediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    SDL_Vout *vout = NULL;
    IjkMediaPlayer *mp = ijkmp_create();
    JNI_CHECK_GOTO(mp, env, "java/lang/OutOfMemoryError", "mpjni: native_setup: ijkmp_create() failed", LABEL_RETURN);

    vout = SDL_VoutAndroid_CreateForAndroidSurface();
    JNI_CHECK_GOTO(mp, env, "java/lang/OutOfMemoryError", "mpjni: native_setup: SDL_VoutAndroid_CreateAndroidSurface() failed", LABEL_RETURN);

    ijkmp_set_vout(mp, vout);
    jni_set_media_player(env, thiz, mp);

    // FIXME: implement

    mp = NULL;
    vout = NULL;

    LABEL_RETURN:
    if (vout)
        SDL_VoutFree(vout);
    if (mp)
        ijkmp_dec_ref(&mp);
    return;
}

static void
IjkMediaPlayer_native_finalize(JNIEnv *env, jobject thiz)
{
    // FIXME: implement
    IjkMediaPlayer_release(env, thiz);
}

// ----------------------------------------------------------------------------

static JNINativeMethod g_methods[] = {
    {
        "_setDataSource",
        "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
        (void *) IjkMediaPlayer_setDataSourceAndHeaders
    },
    { "_setVideoSurface", "(Landroid/view/Surface;)V", (void *) IjkMediaPlayer_setVideoSurface },
    { "prepareAsync", "()V", (void *) IjkMediaPlayer_prepareAsync },
    { "_start", "()V", (void *) IjkMediaPlayer_start },
    { "_stop", "()V", (void *) IjkMediaPlayer_stop },
    { "seekTo", "(J)V", (void *) IjkMediaPlayer_seekTo },
    { "_pause", "()V", (void *) IjkMediaPlayer_pause },
    { "isPlaying", "()Z", (void *) IjkMediaPlayer_isPlaying },
    { "getCurrentPosition", "()J", (void *) IjkMediaPlayer_getCurrentPosition },
    { "getDuration", "()J", (void *) IjkMediaPlayer_getDuration },
    { "_release", "()V", (void *) IjkMediaPlayer_release },
    { "_reset", "()V", (void *) IjkMediaPlayer_reset },
    { "native_init", "()V", (void *) IjkMediaPlayer_native_init },
    { "native_setup", "(Ljava/lang/Object;)V", (void *) IjkMediaPlayer_native_setup },
    { "native_finalize", "()V", (void *) IjkMediaPlayer_native_finalize },
};

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv* env = NULL;

    g_jvm = vm;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    pthread_mutex_init(&g_clazz.mutex, NULL);
    g_clazz.clazz = (*env)->FindClass(env, JNI_CLASS_IJKPLAYER);
    JNI_CHECK_RET(g_clazz.clazz, env, NULL, NULL, -1);

    g_clazz.mNativeMediaPlayer = (*env)->GetFieldID(env, g_clazz.clazz, "mNativeMediaPlayer", "J");
    JNI_CHECK_RET(g_clazz.mNativeMediaPlayer, env, NULL, NULL, -1);

    (*env)->RegisterNatives(env, g_clazz.clazz, g_methods, NELEM(g_methods));

    ijkmp_global_init();

    return JNI_VERSION_1_4;
}

void JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    ijkmp_global_uninit();

    pthread_mutex_destroy(&g_clazz.mutex);
}
