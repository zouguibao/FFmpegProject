#include "opensl_es_context.h"

extern "C" {
#include <android/log.h>
};
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "zouguibao", __VA_ARGS__)

OpenSLESContext* OpenSLESContext::instance = new OpenSLESContext();

void OpenSLESContext::init() {
	LOGE("createEngine");
	SLresult result = createEngine();
	LOGE("createEngine result is s%", ResultToString(result));
	if (SL_RESULT_SUCCESS == result) {
		LOGE("Realize the engine object");
		// Realize the engine object
		result = RealizeObject(engineObject);
		if (SL_RESULT_SUCCESS == result) {
			LOGE("Get the engine interface");
			// Get the engine interface
			result = GetEngineInterface();
		}
	}
}

OpenSLESContext::OpenSLESContext() {
	isInited = false;
}
OpenSLESContext::~OpenSLESContext() {
}

OpenSLESContext* OpenSLESContext::GetInstance() {
	if (!instance->isInited) {
		instance->init();
		instance->isInited = true;
	}
	return instance;
}
