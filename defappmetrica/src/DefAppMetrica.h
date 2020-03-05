#pragma once
#include <dmsdk/sdk.h>

struct TrackData
{
  char* key;
  char* value;
};

extern void DefAppMetrica_setAppMetricaKey(const char*appMetricaKey);
extern void DefAppMetrica_setIsDebug(bool is_debug);
extern void DefAppMetrica_trackEvent(const char*eventName, dmArray<TrackData>* trackData);
