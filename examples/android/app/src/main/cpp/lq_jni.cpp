// =============================================================================
// The LQ Digital Mode Family — Reference Implementation (lq_lib)
//
// Author:  Luis Quesada (HB9IPH)
// Web:     https://luisquesada.com
// Portal:  https://lquesada.github.io/lq_lib/
// GitHub:  https://github.com/lquesada/lq_lib
// App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
//
// License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
//
// Copyright (c) 2026 Luis Quesada (HB9IPH)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// =============================================================================

#include <jni.h>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <android/log.h>

#include "lq/lq.h"
#include "lq/types.h"
#include "lq/message.h"
#include "lq/transport.h"
#include "lq/varicode.h"
#include "lq/ldpc.h"
#include "lq/hash.h"

#define LOG_TAG "LQ_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::string jstring_to_string(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string str(chars ? chars : "");
    if (chars) env->ReleaseStringUTFChars(jstr, chars);
    return str;
}

static jstring string_to_jstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

static lq::Protocol get_protocol(int modeId) {
    if (modeId == 1) return lq::Protocol::LQ8;
    if (modeId == 2) return lq::Protocol::LQ4;
    if (modeId == 3) return lq::Protocol::LQ2;
    if (modeId == 4) return lq::Protocol::LQ16;
    return lq::Protocol::LQ8;
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_qft8_lq_LqNative_isNativeLoaded(JNIEnv* env, jclass clazz) {
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_qft8_lq_LqNative_computeHash(JNIEnv* env, jclass clazz, jstring jcall) {
    std::string call = jstring_to_string(env, jcall);
    return static_cast<jint>(lq::hash_callsign_24(call));
}

JNIEXPORT jint JNICALL
Java_com_qft8_lq_LqNative_computeHash14(JNIEnv* env, jclass clazz, jstring jcall) {
    std::string call = jstring_to_string(env, jcall);
    return static_cast<jint>(lq::hash_callsign_14(call));
}

static void read_multi_targets_from_java(JNIEnv* env, jobject msgObj, jclass msgClass, lq::Message& cppMsg) {
    jfieldID multiTargetsField = env->GetFieldID(msgClass, "multiTargets", "Ljava/util/List;");
    if (!multiTargetsField) return;
    jobject multiList = env->GetObjectField(msgObj, multiTargetsField);
    if (!multiList) return;

    jclass listClass = env->GetObjectClass(multiList);
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    int sz = env->CallIntMethod(multiList, sizeMethod);

    jclass targetClass = env->FindClass("com/qft8/lq/LqMessage$MultiTarget");
    if (!targetClass) return;
    jfieldID tCallField = env->GetFieldID(targetClass, "call", "Ljava/lang/String;");
    jfieldID tHashField = env->GetFieldID(targetClass, "hash", "I");
    jfieldID tRstField  = env->GetFieldID(targetClass, "rstDb", "I");

    for (int i = 0; i < sz && i < 3; ++i) {
        jobject tObj = env->CallObjectMethod(multiList, getMethod, i);
        if (tObj) {
            lq::MultiTarget mt;
            mt.call = jstring_to_string(env, (jstring) env->GetObjectField(tObj, tCallField));
            mt.hash = env->GetIntField(tObj, tHashField);
            mt.rst_db = env->GetIntField(tObj, tRstField);
            cppMsg.multi_targets.push_back(mt);
        }
    }
}

static void write_multi_targets_to_java(JNIEnv* env, jobject outMsgObj, jclass msgClass, const lq::Message& cppMsg) {
    if (cppMsg.multi_targets.empty()) return;
    jfieldID multiTargetsField = env->GetFieldID(msgClass, "multiTargets", "Ljava/util/List;");
    if (!multiTargetsField) return;
    jobject multiList = env->GetObjectField(outMsgObj, multiTargetsField);
    if (!multiList) return;

    jclass listClass = env->GetObjectClass(multiList);
    jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");
    jclass targetClass = env->FindClass("com/qft8/lq/LqMessage$MultiTarget");
    if (!targetClass) return;
    jmethodID targetInit = env->GetMethodID(targetClass, "<init>", "(Ljava/lang/String;II)V");

    for (const auto& mt : cppMsg.multi_targets) {
        jstring cStr = string_to_jstring(env, mt.call);
        jobject tObj = env->NewObject(targetClass, targetInit, cStr, static_cast<jint>(mt.hash), static_cast<jint>(mt.rst_db));
        env->CallBooleanMethod(multiList, addMethod, tObj);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_qft8_lq_LqNative_encodeMessage(JNIEnv* env, jclass clazz, jobject msgObj, jbyteArray outPayload) {
    if (!msgObj || !outPayload) return JNI_FALSE;

    jclass msgClass = env->GetObjectClass(msgObj);
    jfieldID typeField  = env->GetFieldID(msgClass, "type", "Lcom/qft8/lq/LqMessage$Type;");
    jfieldID call1Field = env->GetFieldID(msgClass, "call1", "Ljava/lang/String;");
    jfieldID call2Field = env->GetFieldID(msgClass, "call2", "Ljava/lang/String;");
    jfieldID hash1Field = env->GetFieldID(msgClass, "hash1", "I");
    jfieldID hash2Field = env->GetFieldID(msgClass, "hash2", "I");
    jfieldID modField   = env->GetFieldID(msgClass, "modifier", "Ljava/lang/String;");
    jfieldID locField   = env->GetFieldID(msgClass, "locator", "Ljava/lang/String;");
    jfieldID rstField   = env->GetFieldID(msgClass, "rstDb", "I");
    jfieldID textField  = env->GetFieldID(msgClass, "text", "Ljava/lang/String;");

    jobject typeEnum = env->GetObjectField(msgObj, typeField);
    jclass enumClass = env->GetObjectClass(typeEnum);
    jfieldID idField = env->GetFieldID(enumClass, "id", "I");
    int typeId = env->GetIntField(typeEnum, idField);

    lq::Message cppMsg;
    cppMsg.type = static_cast<lq::MessageType>(typeId);
    cppMsg.call_1 = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, call1Field));
    cppMsg.call_2 = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, call2Field));
    cppMsg.hash_1 = env->GetIntField(msgObj, hash1Field) & 0xFFFFFF;
    cppMsg.hash_2 = env->GetIntField(msgObj, hash2Field) & 0xFFFFFF;
    cppMsg.modifier = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, modField));
    cppMsg.locator = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, locField));
    cppMsg.rst_db = env->GetIntField(msgObj, rstField);
    cppMsg.text = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, textField));
    read_multi_targets_from_java(env, msgObj, msgClass, cppMsg);

    uint8_t payload[lq::PAYLOAD_BYTES] = {0};
    if (!lq::encode_message(cppMsg, payload)) {
        return JNI_FALSE;
    }

    env->SetByteArrayRegion(outPayload, 0, lq::PAYLOAD_BYTES, reinterpret_cast<const jbyte*>(payload));
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_qft8_lq_LqNative_decodeMessage(JNIEnv* env, jclass clazz, jbyteArray inPayload, jobject outMsgObj) {
    if (!inPayload || !outMsgObj) return JNI_FALSE;

    uint8_t payload[lq::PAYLOAD_BYTES] = {0};
    env->GetByteArrayRegion(inPayload, 0, lq::PAYLOAD_BYTES, reinterpret_cast<jbyte*>(payload));

    lq::Message cppMsg;
    if (!lq::decode_message(payload, cppMsg)) {
        return JNI_FALSE;
    }

    jclass msgClass = env->GetObjectClass(outMsgObj);
    jfieldID typeField  = env->GetFieldID(msgClass, "type", "Lcom/qft8/lq/LqMessage$Type;");
    jfieldID call1Field = env->GetFieldID(msgClass, "call1", "Ljava/lang/String;");
    jfieldID call2Field = env->GetFieldID(msgClass, "call2", "Ljava/lang/String;");
    jfieldID hash1Field = env->GetFieldID(msgClass, "hash1", "I");
    jfieldID hash2Field = env->GetFieldID(msgClass, "hash2", "I");
    jfieldID modField   = env->GetFieldID(msgClass, "modifier", "Ljava/lang/String;");
    jfieldID locField   = env->GetFieldID(msgClass, "locator", "Ljava/lang/String;");
    jfieldID rstField   = env->GetFieldID(msgClass, "rstDb", "I");
    jfieldID textField  = env->GetFieldID(msgClass, "text", "Ljava/lang/String;");

    // Resolve Enum type object
    jclass enumClass = env->FindClass("com/qft8/lq/LqMessage$Type");
    jmethodID valuesMethod = env->GetStaticMethodID(enumClass, "values", "()[Lcom/qft8/lq/LqMessage$Type;");
    jobjectArray enumArray = (jobjectArray) env->CallStaticObjectMethod(enumClass, valuesMethod);
    int numEnums = env->GetArrayLength(enumArray);
    jfieldID idField = env->GetFieldID(enumClass, "id", "I");
    jobject matchedEnum = nullptr;

    for (int i = 0; i < numEnums; ++i) {
        jobject e = env->GetObjectArrayElement(enumArray, i);
        int id = env->GetIntField(e, idField);
        if (id == static_cast<int>(cppMsg.type)) {
            matchedEnum = e;
            break;
        }
    }

    if (matchedEnum) {
        env->SetObjectField(outMsgObj, typeField, matchedEnum);
    }
    env->SetObjectField(outMsgObj, call1Field, string_to_jstring(env, cppMsg.call_1));
    env->SetObjectField(outMsgObj, call2Field, string_to_jstring(env, cppMsg.call_2));
    env->SetIntField(outMsgObj, hash1Field, static_cast<jint>(cppMsg.hash_1));
    env->SetIntField(outMsgObj, hash2Field, static_cast<jint>(cppMsg.hash_2));
    env->SetObjectField(outMsgObj, modField, string_to_jstring(env, cppMsg.modifier));
    env->SetObjectField(outMsgObj, locField, string_to_jstring(env, cppMsg.locator));
    env->SetIntField(outMsgObj, rstField, static_cast<jint>(cppMsg.rst_db));
    env->SetObjectField(outMsgObj, textField, string_to_jstring(env, cppMsg.text));
    write_multi_targets_to_java(env, outMsgObj, msgClass, cppMsg);

    return JNI_TRUE;
}

JNIEXPORT jintArray JNICALL
Java_com_qft8_lq_LqNative_encodeTones(JNIEnv* env, jclass clazz, jobject msgObj, jint modeId) {
    if (!msgObj) return nullptr;

    jclass msgClass = env->GetObjectClass(msgObj);
    jfieldID typeField  = env->GetFieldID(msgClass, "type", "Lcom/qft8/lq/LqMessage$Type;");
    jfieldID call1Field = env->GetFieldID(msgClass, "call1", "Ljava/lang/String;");
    jfieldID call2Field = env->GetFieldID(msgClass, "call2", "Ljava/lang/String;");
    jfieldID hash1Field = env->GetFieldID(msgClass, "hash1", "I");
    jfieldID hash2Field = env->GetFieldID(msgClass, "hash2", "I");
    jfieldID modField   = env->GetFieldID(msgClass, "modifier", "Ljava/lang/String;");
    jfieldID locField   = env->GetFieldID(msgClass, "locator", "Ljava/lang/String;");
    jfieldID rstField   = env->GetFieldID(msgClass, "rstDb", "I");
    jfieldID textField  = env->GetFieldID(msgClass, "text", "Ljava/lang/String;");

    jobject typeEnum = env->GetObjectField(msgObj, typeField);
    jclass enumClass = env->GetObjectClass(typeEnum);
    jfieldID idField = env->GetFieldID(enumClass, "id", "I");
    int typeId = env->GetIntField(typeEnum, idField);

    lq::Message cppMsg;
    cppMsg.type = static_cast<lq::MessageType>(typeId);
    cppMsg.call_1 = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, call1Field));
    cppMsg.call_2 = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, call2Field));
    cppMsg.hash_1 = env->GetIntField(msgObj, hash1Field) & 0xFFFFFF;
    cppMsg.hash_2 = env->GetIntField(msgObj, hash2Field) & 0xFFFFFF;
    cppMsg.modifier = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, modField));
    cppMsg.locator = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, locField));
    cppMsg.rst_db = env->GetIntField(msgObj, rstField);
    cppMsg.text = jstring_to_string(env, (jstring) env->GetObjectField(msgObj, textField));
    read_multi_targets_from_java(env, msgObj, msgClass, cppMsg);

    lq::Protocol proto = get_protocol(modeId);
    lq::ToneSequence seq;
    if (!lq::encode_tones(cppMsg, proto, seq)) {
        return nullptr;
    }

    jintArray result = env->NewIntArray(seq.tones.size());
    jint* toneElems = new jint[seq.tones.size()];
    for (size_t i = 0; i < seq.tones.size(); ++i) {
        toneElems[i] = static_cast<jint>(seq.tones[i]);
    }
    env->SetIntArrayRegion(result, 0, seq.tones.size(), toneElems);
    delete[] toneElems;
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_qft8_lq_LqNative_decodeTones(JNIEnv* env, jclass clazz, jintArray inTones, jint modeId, jobject outMsgObj) {
    if (!inTones || !outMsgObj) return JNI_FALSE;

    int len = env->GetArrayLength(inTones);
    jint* toneElems = env->GetIntArrayElements(inTones, nullptr);
    lq::ToneSequence seq;
    seq.protocol = get_protocol(modeId);
    seq.tones.resize(len);
    for (int i = 0; i < len; ++i) {
        seq.tones[i] = static_cast<uint8_t>(toneElems[i]);
    }
    env->ReleaseIntArrayElements(inTones, toneElems, JNI_ABORT);

    lq::Message cppMsg;
    if (!lq::decode_tones(seq, cppMsg)) {
        return JNI_FALSE;
    }

    jclass msgClass = env->GetObjectClass(outMsgObj);
    jfieldID typeField  = env->GetFieldID(msgClass, "type", "Lcom/qft8/lq/LqMessage$Type;");
    jfieldID call1Field = env->GetFieldID(msgClass, "call1", "Ljava/lang/String;");
    jfieldID call2Field = env->GetFieldID(msgClass, "call2", "Ljava/lang/String;");
    jfieldID hash1Field = env->GetFieldID(msgClass, "hash1", "I");
    jfieldID hash2Field = env->GetFieldID(msgClass, "hash2", "I");
    jfieldID modField   = env->GetFieldID(msgClass, "modifier", "Ljava/lang/String;");
    jfieldID locField   = env->GetFieldID(msgClass, "locator", "Ljava/lang/String;");
    jfieldID rstField   = env->GetFieldID(msgClass, "rstDb", "I");
    jfieldID textField  = env->GetFieldID(msgClass, "text", "Ljava/lang/String;");

    jclass enumClass = env->FindClass("com/qft8/lq/LqMessage$Type");
    jmethodID valuesMethod = env->GetStaticMethodID(enumClass, "values", "()[Lcom/qft8/lq/LqMessage$Type;");
    jobjectArray enumArray = (jobjectArray) env->CallStaticObjectMethod(enumClass, valuesMethod);
    int numEnums = env->GetArrayLength(enumArray);
    jfieldID idField = env->GetFieldID(enumClass, "id", "I");
    jobject matchedEnum = nullptr;

    for (int i = 0; i < numEnums; ++i) {
        jobject e = env->GetObjectArrayElement(enumArray, i);
        int id = env->GetIntField(e, idField);
        if (id == static_cast<int>(cppMsg.type)) {
            matchedEnum = e;
            break;
        }
    }

    if (matchedEnum) {
        env->SetObjectField(outMsgObj, typeField, matchedEnum);
    }
    env->SetObjectField(outMsgObj, call1Field, string_to_jstring(env, cppMsg.call_1));
    env->SetObjectField(outMsgObj, call2Field, string_to_jstring(env, cppMsg.call_2));
    env->SetIntField(outMsgObj, hash1Field, static_cast<jint>(cppMsg.hash_1));
    env->SetIntField(outMsgObj, hash2Field, static_cast<jint>(cppMsg.hash_2));
    env->SetObjectField(outMsgObj, modField, string_to_jstring(env, cppMsg.modifier));
    env->SetObjectField(outMsgObj, locField, string_to_jstring(env, cppMsg.locator));
    env->SetIntField(outMsgObj, rstField, static_cast<jint>(cppMsg.rst_db));
    env->SetObjectField(outMsgObj, textField, string_to_jstring(env, cppMsg.text));
    write_multi_targets_to_java(env, outMsgObj, msgClass, cppMsg);

    return JNI_TRUE;
}

JNIEXPORT jfloatArray JNICALL
Java_com_qft8_lq_LqNative_generateAudio(JNIEnv* env, jclass clazz, jintArray inTones, jint modeId, jfloat centerFreq, jfloat sampleRate) {
    if (!inTones || sampleRate <= 0.0f) return nullptr;

    int numTones = env->GetArrayLength(inTones);
    jint* toneElems = env->GetIntArrayElements(inTones, nullptr);
    lq::ToneSequence seq;
    seq.protocol = get_protocol(modeId);
    auto params = lq::get_protocol_params(seq.protocol);
    seq.symbol_period = params.symbol_period;
    seq.tone_spacing = params.tone_spacing;
    seq.tones.resize(numTones);
    for (int i = 0; i < numTones; ++i) {
        seq.tones[i] = static_cast<uint8_t>(toneElems[i]);
    }
    seq.tx_duration = static_cast<float>(numTones) * seq.symbol_period;
    env->ReleaseIntArrayElements(inTones, toneElems, JNI_ABORT);

    std::vector<float> audio;
    lq::generate_audio(seq, centerFreq, sampleRate, audio);

    jfloatArray result = env->NewFloatArray(audio.size());
    env->SetFloatArrayRegion(result, 0, audio.size(), audio.data());
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_qft8_lq_LqNative_decodeAudio(JNIEnv* env, jclass clazz, jfloatArray inAudio, jfloat centerFreq, jfloat sampleRate, jint modeId, jobject outMsgObj) {
    if (!inAudio || !outMsgObj || sampleRate <= 0.0f) return JNI_FALSE;

    int numSamples = env->GetArrayLength(inAudio);
    jfloat* audioElems = env->GetFloatArrayElements(inAudio, nullptr);
    std::vector<float> audio(audioElems, audioElems + numSamples);
    env->ReleaseFloatArrayElements(inAudio, audioElems, JNI_ABORT);

    lq::Protocol proto = get_protocol(modeId);
    lq::Message cppMsg;

    __android_log_print(ANDROID_LOG_DEBUG, "LqJni", "decodeAudio: mode=%d, centerFreq=%.1f, samples=%zu", modeId, centerFreq, audio.size());

    // Fast multi-threaded search (4 threads)
    bool decoded = lq::audio_to_message(audio, centerFreq, sampleRate, proto, cppMsg, 4, false);
    __android_log_print(ANDROID_LOG_DEBUG, "LqJni", "decodeAudio fast result: %d", decoded ? 1 : 0);
    if (!decoded && centerFreq > 0.0f) {
        // High-sensitivity deep search fallback for targeted channel frequency (+/- 60 Hz)
        decoded = lq::audio_to_message(audio, centerFreq, sampleRate, proto, cppMsg, 4, true);
        __android_log_print(ANDROID_LOG_DEBUG, "LqJni", "decodeAudio deep result: %d", decoded ? 1 : 0);
    }
    if (!decoded) {
        return JNI_FALSE;
    }

    jclass msgClass = env->GetObjectClass(outMsgObj);
    jfieldID typeField  = env->GetFieldID(msgClass, "type", "Lcom/qft8/lq/LqMessage$Type;");
    jfieldID call1Field = env->GetFieldID(msgClass, "call1", "Ljava/lang/String;");
    jfieldID call2Field = env->GetFieldID(msgClass, "call2", "Ljava/lang/String;");
    jfieldID hash1Field = env->GetFieldID(msgClass, "hash1", "I");
    jfieldID hash2Field = env->GetFieldID(msgClass, "hash2", "I");
    jfieldID modField   = env->GetFieldID(msgClass, "modifier", "Ljava/lang/String;");
    jfieldID locField   = env->GetFieldID(msgClass, "locator", "Ljava/lang/String;");
    jfieldID rstField   = env->GetFieldID(msgClass, "rstDb", "I");
    jfieldID textField  = env->GetFieldID(msgClass, "text", "Ljava/lang/String;");

    jclass enumClass = env->FindClass("com/qft8/lq/LqMessage$Type");
    jmethodID valuesMethod = env->GetStaticMethodID(enumClass, "values", "()[Lcom/qft8/lq/LqMessage$Type;");
    jobjectArray enumArray = (jobjectArray) env->CallStaticObjectMethod(enumClass, valuesMethod);
    int numEnums = env->GetArrayLength(enumArray);
    jfieldID idField = env->GetFieldID(enumClass, "id", "I");
    jobject matchedEnum = nullptr;

    for (int i = 0; i < numEnums; ++i) {
        jobject e = env->GetObjectArrayElement(enumArray, i);
        int id = env->GetIntField(e, idField);
        if (id == static_cast<int>(cppMsg.type)) {
            matchedEnum = e;
            break;
        }
    }

    if (matchedEnum) {
        env->SetObjectField(outMsgObj, typeField, matchedEnum);
    }
    env->SetObjectField(outMsgObj, call1Field, string_to_jstring(env, cppMsg.call_1));
    env->SetObjectField(outMsgObj, call2Field, string_to_jstring(env, cppMsg.call_2));
    env->SetIntField(outMsgObj, hash1Field, static_cast<jint>(cppMsg.hash_1));
    env->SetIntField(outMsgObj, hash2Field, static_cast<jint>(cppMsg.hash_2));
    env->SetObjectField(outMsgObj, modField, string_to_jstring(env, cppMsg.modifier));
    env->SetObjectField(outMsgObj, locField, string_to_jstring(env, cppMsg.locator));
    env->SetIntField(outMsgObj, rstField, static_cast<jint>(cppMsg.rst_db));
    env->SetObjectField(outMsgObj, textField, string_to_jstring(env, cppMsg.text));
    write_multi_targets_to_java(env, outMsgObj, msgClass, cppMsg);

    return JNI_TRUE;
}

} // extern "C"
