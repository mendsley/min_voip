project "webrtc"
	language "C++"
	kind "StaticLib"

	includedirs {
		"webrtc/",
		"webrtc/webrtc/common_audio/signal_processing/include/",
		"webrtc/webrtc/modules/audio_coding/codecs/isac/main/include/",
	}

	files {
		"webrtc/webrtc/common_types.*",
		"webrtc/webrtc/base/checks.*",
		"webrtc/webrtc/base/event.*",
		"webrtc/webrtc/base/platform_file*",
		"webrtc/webrtc/base/platform_thread*",
		"webrtc/webrtc/base/thread_checker**",
		"webrtc/webrtc/base/criticalsection.cc",
		"webrtc/webrtc/system_wrappers/source/**",
		"webrtc/webrtc/system_wrappers/include/**",
		"webrtc/webrtc/modules/audio_processing/aec/**",
		"webrtc/webrtc/modules/audio_processing/aecm/**",
		"webrtc/webrtc/modules/audio_processing/ns/**",
		"webrtc/webrtc/modules/audio_processing/utility/**",
		"webrtc/webrtc/modules/audio_processing/agc/**",
		"webrtc/webrtc/modules/audio_processing/beamformer/**",
		"webrtc/webrtc/modules/audio_processing/include/**",
		"webrtc/webrtc/modules/audio_processing/intelligibility/**",
		"webrtc/webrtc/modules/audio_processing/transient/**",
		"webrtc/webrtc/modules/audio_processing/vad/**",
		"webrtc/webrtc/modules/audio_processing/*.cc",
		"webrtc/webrtc/modules/audio_processing/*.h",
		"webrtc/webrtc/common_audio/include/**",
		"webrtc/webrtc/common_audio/resampler/**",
		"webrtc/webrtc/common_audio/vad/**",
		"webrtc/webrtc/common_audio/signal_processing/**",
		"webrtc/webrtc/common_audio/*.c",
		"webrtc/webrtc/common_audio/*.cc",
		"webrtc/webrtc/common_audio/*.h",
		"webrtc/webrtc/modules/audio_coding/codecs/isac/main/include/**",
		"webrtc/webrtc/modules/audio_coding/codecs/isac/main/source/**",
	}

	excludes {
		"webrtc/webrtc/base/**_unittest.cc",
		"webrtc/webrtc/system_wrappers/**_unittest.cc",
		"webrtc/webrtc/system_wrappers/**_unittest_disabled.cc",
		"webrtc/webrtc/system_wrappers/source/logcat_trace_context.cc",
		"webrtc/webrtc/system_wrappers/source/data_log.cc",
		"webrtc/webrtc/common_audio/**_unittest.cc",
		"webrtc/webrtc/common_audio/wav_file.cc",
		"webrtc/webrtc/modules/audio_coding/**.cc",
	}

	excludes {
		"webrtc/webrtc/system_wrappers/**_posix.cc",
		"webrtc/webrtc/system_wrappers/**_mac.cc",
		"webrtc/webrtc/system_wrappers/**_android.c",
		"webrtc/webrtc/modules/audio_processing/**_mips.c",
		"webrtc/webrtc/modules/audio_processing/**_neon.c",
		"webrtc/webrtc/modules/audio_processing/**_unittest.cc",
		"webrtc/webrtc/modules/audio_processing/**_test.cc",
		"webrtc/webrtc/modules/audio_processing/intelligibility/test/**",
		"webrtc/webrtc/modules/audio_processing/transient/test/**",
		"webrtc/webrtc/common_audio/**_neon.c",
		"webrtc/webrtc/common_audio/**_neon.cc",
		"webrtc/webrtc/common_audio/**_openmax.cc",
		"webrtc/webrtc/common_audio/**_arm.S",
		"webrtc/webrtc/common_audio/**_armv7.S",
		"webrtc/webrtc/common_audio/**_mips.c",
	}

	defines {
		"WEBRTC_WIN",
		"WEBRTC_NS_FLOAT",
		"WIN32_LEAN_AND_MEAN",
		--"NOMINMAX"
		"_CRT_SECURE_NO_WARNINGS",
	}

	buildoptions {
		"/wd4244",
		"/wd4267",
		"/wd4334",
	}
