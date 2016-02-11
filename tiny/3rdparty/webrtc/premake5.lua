solution "webrtc"
	configurations {
		"Debug"
	}
	platforms "x64"

	project "webrtc"
		language "C++"
		kind "StaticLib"

		files {
			"webrtc/**",			
		}
		includedirs {
			"webrtc/",
		}
