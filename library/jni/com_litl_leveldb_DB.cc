#include <string.h>
#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>

#include "leveldbjni.h"

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

static jint
nativeOpen(JNIEnv* env,
           jclass clazz,
           jstring dbpath)
{

    const char *path = env->GetStringUTFChars(dbpath, 0);
    LOGI("Opening database %s", path);

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &db);
    if (!status.ok()) {
        throwException(env, status);
    } else {
        LOGI("Opened database");
    }

    return reinterpret_cast<jint>(db);
}

static void
nativeClose(JNIEnv* env,
            jclass clazz,
            jint dbPtr)
{
    leveldb::DB* db = reinterpret_cast<leveldb::DB*>(dbPtr);
    if (db) {
        delete db;
    }

    LOGI("Database closed");
}

static jbyteArray
nativeGet(JNIEnv * env,
          jclass clazz,
          jint dbPtr,
          jbyteArray keyObj)
{
    leveldb::DB* db = reinterpret_cast<leveldb::DB*>(dbPtr);

    size_t keyLen = env->GetArrayLength(keyObj);
    jbyte *buffer = env->GetByteArrayElements(keyObj, NULL);

    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), leveldb::Slice((const char *)buffer, keyLen), &value);
    env->ReleaseByteArrayElements(keyObj, buffer, JNI_ABORT);

    if (status.ok()) {
        size_t len = value.size();
        jbyteArray result = env->NewByteArray(len);
        env->SetByteArrayRegion(result, 0, len, (const jbyte *) value.data());
        return result;
    } else if (status.IsNotFound()) {
        return NULL;
    } else {
        throwException(env, status);
        return NULL;
    }
}

static void
nativePut(JNIEnv *env,
          jclass clazz,
          jint dbPtr,
          jbyteArray keyObj,
          jbyteArray valObj)
{
    leveldb::DB* db = reinterpret_cast<leveldb::DB*>(dbPtr);

    size_t keyLen = env->GetArrayLength(keyObj);
    jbyte *keyBuf = env->GetByteArrayElements(keyObj, NULL);

    size_t valLen = env->GetArrayLength(valObj);
    jbyte *valBuf = env->GetByteArrayElements(valObj, NULL);

    leveldb::Status status = db->Put(leveldb::WriteOptions(),
            leveldb::Slice((const char *) keyBuf, keyLen),
            leveldb::Slice((const char *) valBuf, valLen));

    env->ReleaseByteArrayElements(keyObj, keyBuf, JNI_ABORT);
    env->ReleaseByteArrayElements(valObj, valBuf, JNI_ABORT);

    if (!status.ok()) {
        throwException(env, status);
    }
}

static void
nativeDelete(JNIEnv *env,
             jclass clazz,
             jint dbPtr,
             jbyteArray keyObj)
{
    leveldb::DB* db = reinterpret_cast<leveldb::DB*>(dbPtr);

    size_t keyLen = env->GetArrayLength(keyObj);
    jbyte *buffer = env->GetByteArrayElements(keyObj, NULL);

    leveldb::Status status = db->Delete(leveldb::WriteOptions(), leveldb::Slice((const char *) buffer, keyLen));
    env->ReleaseByteArrayElements(keyObj, buffer, JNI_ABORT);

    if (!status.ok()) {
        throwException(env, status);
    }
}

static void
nativeWrite(JNIEnv *env,
            jclass clazz,
            jint dbPtr,
            jint batchPtr)
{
    leveldb::DB* db = reinterpret_cast<leveldb::DB*>(dbPtr);

    leveldb::WriteBatch *batch = (leveldb::WriteBatch *) batchPtr;
    leveldb::Status status = db->Write(leveldb::WriteOptions(), batch);
    if (!status.ok()) {
        throwException(env, status);
    }
}

static jint
nativeIterator(JNIEnv* env,
               jclass,
               jint dbPtr)
{
    leveldb::DB* db = reinterpret_cast<leveldb::DB*>(dbPtr);
    leveldb::Iterator *iter = db->NewIterator(leveldb::ReadOptions());
    return reinterpret_cast<jint>(iter);
}

static void
nativeDestroy(JNIEnv *env,
              jclass clazz,
              jstring dbpath)
{
    const char* path = env->GetStringUTFChars(dbpath,0);
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = DestroyDB(path, options);
    if (!status.ok()) {
        throwException(env, status);
    }
}

static JNINativeMethod sMethods[] =
{
        { "nativeOpen", "(Ljava/lang/String;)I", (void*) nativeOpen },
        { "nativeClose", "(I)V", (void*) nativeClose },
        { "nativeGet", "(I[B)[B", (void*) nativeGet },
        { "nativePut", "(I[B[B)V", (void*) nativePut },
        { "nativeDelete", "(I[B)V", (void*) nativeDelete },
        { "nativeWrite", "(II)V", (void*) nativeWrite },
        { "nativeIterator", "(I)I", (void *) nativeIterator },
        { "nativeDestroy", "(Ljava/lang/String;)V", (void*) nativeDestroy }
};

int
register_com_litl_leveldb_DB(JNIEnv *env) {
    jclass clazz = env->FindClass("com/litl/leveldb/DB");
    if (!clazz) {
        LOGE("Can't find class com.litl.leveldb.DB");
        return 0;
    }

    return env->RegisterNatives(clazz, sMethods, NELEM(sMethods));
}