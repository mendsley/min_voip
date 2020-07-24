solution "ovr_audio_test"
	configurations {
		"Debug",
	}
	platforms {
		"x64",
	}
	flags {
		"Symbols",
	}

-- Dependency: tiny
group "depend"

dofile "tiny/premake/tiny.lua"
group "depend"
location ".generated"

-- Test project
solution "ovr_audio_test"
project "test"
	group ""
	kind "ConsoleApp"
	language "C++"
	location ".generated"

	includedirs {
		"tiny/include",
	}
	files {
		"main.cpp"
	}
	links {
		"tiny",
		"winmm",
		"ws2_32",
	}

solution "ovr_audio_test"
startproject "test"
