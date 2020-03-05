#define EXTENSION_NAME DefAppMetrica
#define LIB_NAME "DefAppMetrica"
#define MODULE_NAME LIB_NAME

#define DLIB_LOG_DOMAIN LIB_NAME

#if defined(DM_PLATFORM_ANDROID)
#include "utils/LuaUtils.h"
#include "DefAppMetrica.h"
#include <stdlib.h>
#include <jni.h>
#include <dmsdk/sdk.h>

dmArray<TrackData> list;

const char* JAR_PATH = "com/chilaykov/defappmetrica/DefAppMetrica";

static JNIEnv* Attach()
{
  JNIEnv* env;
  JavaVM* vm = dmGraphics::GetNativeAndroidJavaVM();
  vm->AttachCurrentThread(&env, NULL);
  return env;
}

static bool Detach(JNIEnv* env)
{
  bool exception = (bool) env->ExceptionCheck();
  env->ExceptionClear();
  JavaVM* vm = dmGraphics::GetNativeAndroidJavaVM();
  vm->DetachCurrentThread();
  return !exception;
}

namespace {
struct AttachScope
{
  JNIEnv* m_Env;
  AttachScope() : m_Env(Attach())
  {
  }
  ~AttachScope()
  {
    Detach(m_Env);
  }
};
}

static jclass GetClass(JNIEnv* env, const char* classname)
{
  jclass activity_class = env->FindClass("android/app/NativeActivity");
  jmethodID get_class_loader = env->GetMethodID(activity_class,"getClassLoader", "()Ljava/lang/ClassLoader;");
  jobject cls = env->CallObjectMethod(dmGraphics::GetNativeAndroidActivity(), get_class_loader);
  jclass class_loader = env->FindClass("java/lang/ClassLoader");
  jmethodID find_class = env->GetMethodID(class_loader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

  jstring str_class_name = env->NewStringUTF(classname);
  jclass outcls = (jclass)env->CallObjectMethod(cls, find_class, str_class_name);

  env->DeleteLocalRef(str_class_name);
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(activity_class);
  env->DeleteLocalRef(class_loader);
  return outcls;
}

void DefAppMetrica_setAppMetricaKey(const char*appMetricaKey)
{
  AttachScope attachscope;
  JNIEnv* env = attachscope.m_Env;
  jclass cls = GetClass(env, JAR_PATH);
  jstring apkey = env->NewStringUTF(appMetricaKey);
  jmethodID method = env->GetStaticMethodID(cls, "DefAppMetrica_setAppMetricaKey", "(Landroid/app/Activity;Ljava/lang/String;)V");
  env->CallStaticVoidMethod(cls, method, dmGraphics::GetNativeAndroidActivity(), apkey);
  dmLogWarning("Call DefAppMetrica_setAppMetricaKey", apkey);
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(afkey);
}

void DefAppMetrica_setIsDebug(bool is_debug)
{
  AttachScope attachscope;
  JNIEnv* env = attachscope.m_Env;
  jclass cls = GetClass(env, JAR_PATH);
  jmethodID method = env->GetStaticMethodID(cls, "DefAppMetrica_setIsDebug", "(Z)V");
  env->CallStaticVoidMethod(cls, method, is_debug ? JNI_TRUE : JNI_FALSE);

  env->DeleteLocalRef(cls);
}

void DefAppMetrica_trackEvent(const char*eventName, dmArray<TrackData>* trackData)
{
  AttachScope attachscope;
  JNIEnv* env = attachscope.m_Env;
  jclass hashMapClass = env->FindClass("java/util/HashMap");
  jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "(I)V");
  jobject hashMapObj = env->NewObject(hashMapClass, hashMapInit, trackData->Size());
  jmethodID hashMapId = env->GetMethodID(hashMapClass, "put","(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

  TrackData data;
  jstring key;
  jstring value;
  for(uint32_t i = 0; i != trackData->Size(); i++)
  {
    data = (*trackData)[i];
    key = env->NewStringUTF(data.key);
    value = env->NewStringUTF(data.value);
    env->CallObjectMethod(hashMapObj, hashMapId, key, value);

    env->DeleteLocalRef(key);
    env->DeleteLocalRef(value);
  }
  jclass cls = GetClass(env, JAR_PATH);
  jmethodID method = env->GetStaticMethodID(cls, "DefAppMetrica_trackEvent","(Landroid/app/Activity;Ljava/lang/String;Ljava/util/Map;)V");
  jstring jEventName = env->NewStringUTF(eventName);
  env->CallStaticVoidMethod(cls, method, dmGraphics::GetNativeAndroidActivity(), jEventName, hashMapObj);

  env->DeleteLocalRef(hashMapClass);
  env->DeleteLocalRef(hashMapObj);
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(jEventName);
}

static int setIsDebug(lua_State* L)
{
  DM_LUA_STACK_CHECK(L, 0);
  bool enableDebugMode_lua = luaL_checkbool(L, 1);
  DefAppMetrica_setIsDebug(enableDebugMode_lua);
  return 0;
}

static int trackEvent(lua_State* L)
{
  DM_LUA_STACK_CHECK(L, 0);
  const char *eventName = luaL_checkstring(L, 1);
  if(lua_type(L, 2) == LUA_TTABLE)
  {
    lua_pushvalue(L, 2);
    lua_pushnil(L);

    while (lua_next(L, -2) != 0)
    {
      TrackData data;
      const char* k = lua_tostring(L, -2);
      const char* s = lua_tostring(L, -1);
      if (!s)
      {
        char msg[256];
        snprintf(msg, sizeof(msg), "Wrong type for table attribute '%s'. Expected string, got %s", lua_tostring(L, -2), luaL_typename(L, -1) );
        luaL_error(L, msg);
        return 0;
      }
      data.key = strdup(k);
      data.value = strdup(s);
      if(list.Full())
      {
        list.OffsetCapacity(2);
      }
      list.Push(data);
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }
  DefAppMetrica_trackEvent(eventName, &list);
  for(int i = list.Size() - 1; i >= 0; --i)
  {
    free(list[i].key);
    free(list[i].value);
    list.EraseSwap(i);
  }
  return 0;
}

static const luaL_reg Module_methods[] =
{
  {"setIsDebug", setIsDebug},
  {"trackEvent", trackEvent},
  {0, 0}
};

static void LuaInit(lua_State* L)
{
  int top = lua_gettop(L);
  luaL_register(L, MODULE_NAME, Module_methods);
  lua_pop(L, 1);
  assert(top == lua_gettop(L));
}

const char* appMetricaKey;

static dmExtension::Result AppInitilizeDefAppMetricaDefAppMetrica(dmExtension::AppParams* params)
{
  int isDebug = dmConfigFile::GetInt(params->m_ConfigFile, "app_metrica.is_debug", 0);
  if (isDebug && isDebug > 0)
  {
    DefAppMetrica_setIsDebug(true);
  }
  appMetricaKey = dmConfigFile::GetString(params->m_ConfigFile, "app_metrica.key", 0);
  if (appMetricaKey)
  {
    dmLogWarning("Call AppInitilizeDefAppMetricaDefAppMetrica", appMetricaKey);
    DefAppMetrica_setAppMetricaKey(appMetricaKey);
  }
  else
  {
    dmLogError("Pls add app_metrica.key to game.project\n");
  }

  return dmExtension::RESULT_OK;
}

dmExtension::Result InitilizeDefAppMetrica(dmExtension::Params* params)
{
  LuaInit(params->m_L);
  return dmExtension::RESULT_OK;
}

static void OnEventDefAppMetrica(dmExtension::Params* params, const dmExtension::Event* event)
{

  if (event->m_Event == dmExtension::EVENT_ID_ACTIVATEAPP)
  {
    DefAppMetrica_setAppMetricaKey(appMetricaKey);
  }

}

dmExtension::Result FinalizeDefAppMetrica(dmExtension::Params* params)
{
  return dmExtension::RESULT_OK;
}

#endif

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitilizeDefAppMetricaDefAppMetrica, 0, InitilizeDefAppMetrica, 0, OnEventDefAppMetrica, FinalizeDefAppMetrica)
