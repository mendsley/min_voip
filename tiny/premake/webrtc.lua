local THIRD_PARTY_DIR = path.join(path.join(path.getdirectory(_SCRIPT), "..") , "3rdparty") .. "/"

project "webrtc"
	kind "StaticLib"
	language "C++"

	flags {
		"NoExceptions",
	}

	files {
		-- opus
		THIRD_PARTY_DIR .. "opus/celt/*",
		THIRD_PARTY_DIR .. "opus/silk/*",
		THIRD_PARTY_DIR .. "opus/silk/float/**",
		THIRD_PARTY_DIR .. "opus/src/**",

		-- webrtc
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_types.*",
		THIRD_PARTY_DIR .. "webrtc/webrtc/base/checks.*",
		THIRD_PARTY_DIR .. "webrtc/webrtc/base/platform_file*",
		THIRD_PARTY_DIR .. "webrtc/webrtc/base/platform_thread*",
		THIRD_PARTY_DIR .. "webrtc/webrtc/base/thread_checker**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/base/criticalsection.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/source/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/include/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/aec/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/aecm/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/ns/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/utility/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/agc/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/beamformer/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/include/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/intelligibility/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/transient/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/vad/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/*.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/*.h",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/include/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/resampler/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/vad/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/signal_processing/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/*.c",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/*.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/*.h",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_coding/codecs/isac/main/include/**",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_coding/codecs/isac/main/source/**",
	}

	includedirs {
		-- opus
		THIRD_PARTY_DIR .. "opus/include/",
		THIRD_PARTY_DIR .. "opus/celt/",
		THIRD_PARTY_DIR .. "opus/silk/",
		THIRD_PARTY_DIR .. "opus/silk/float/",
		THIRD_PARTY_DIR .. "opus/src/",
		THIRD_PARTY_DIR .. "opus/",

		-- webrtc
		THIRD_PARTY_DIR .. "webrtc/",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/signal_processing/include/",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_coding/codecs/isac/main/include/",
	}

	excludes {
		-- webrtc
		THIRD_PARTY_DIR .. "webrtc/webrtc/base/**_unittest.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/**_unittest.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/**_unittest_disabled.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/source/logcat_trace_context.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/source/data_log.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_unittest.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/wav_file.cc",
		THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_coding/**.cc",
	}

	configuration "windows"
		defines {
			-- opus
			"USE_ALLOCA",
			"HAVE_CONFIG_H",

			-- webrtc
			"WEBRTC_WIN",
			"WEBRTC_NS_FLOAT",
			"WIN32_LEAN_AND_MEAN",
		}

		excludes {
			-- webrtc
			THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/**_posix.cc",
			THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/**_mac.cc",
			THIRD_PARTY_DIR .. "webrtc/webrtc/system_wrappers/**_android.c",
			THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/**_mips.c",
			THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/**_neon.c",
			THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/**_unittest.cc",
			THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/**_test.cc",
			THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/intelligibility/test/**",
			THIRD_PARTY_DIR .. "webrtc/webrtc/modules/audio_processing/transient/test/**",
			THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_neon.c",
			THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_neon.cc",
			THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_openmax.cc",
			THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_arm.S",
			THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_armv7.S",
			THIRD_PARTY_DIR .. "webrtc/webrtc/common_audio/**_mips.c",
		}

	configuration "vs*"
		defines {
			-- webrtc
			"COMPILER_MSVC",
			"_SCL_SECURE_NO_WARNINGS",
			"_CRT_SECURE_NO_WARNINGS",
		}

		buildoptions {
			"/wd4100", -- warning C4100: 'T' : unreferenced formal parameter
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4244", -- warning C4244: '=' : conversion from 'T' to 'U', possible loss of data
			"/wd4351", -- warning C4351: new behavior: elements of array 'webrtc::TransientDetector::last_first_moment_' will be default initialized
			"/wd4701", -- warning C4701: potentially uninitialized local variable 'T' used
			"/wd4703", -- warning C4703: potentially uninitialized local pointer variable 'pcm_transition' used
			"/wd4706", -- warning C4706: assignment within conditional expression
 			"/wd4310", -- warning C4310: cast truncates constant value
		}

	configuration {}
