package com.chilyakov.defappmetrica;

import android.app.Activity;

import com.yandex.metrica.YandexMetrica;
import com.yandex.metrica.YandexMetricaConfig;

import java.util.Map;

public class DefAppMetrica {

	public static void DefAppMetrica_setAppMetricaKey(final Activity appActivity, final String appMetricaKey) {
		
		YandexMetricaConfig config = YandexMetricaConfig.newConfigBuilder(appMetricaKey)
			.withSessionTimeout(30)
			.withLogs()
			.build();
		YandexMetrica.activate(appActivity.getApplicationContext(), config);
		YandexMetrica.enableActivityAutoTracking(appActivity.getApplication());
	}

	public static void DefAppMetrica_trackEvent(Activity appActivity, String eventName, Map<String, Object> eventValue) {
		YandexMetrica.reportEvent(eventName, eventValue);
	}
}
