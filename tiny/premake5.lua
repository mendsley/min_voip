local ROOT_DIR = path.join(path.getdirectory(_SCRIPT), ".") .. "/"
local EXAMPLES_DIR = (ROOT_DIR .. "examples/")

solution "tiny_sln"
	location (".build/projects/" .. _ACTION)
	objdir (".build/%{cfg.platform}_" .. _ACTION .. "/obj/%{cfg.buildcfg}/%{prj.name}")

	platforms {
		"x32",
		"x64",
	}
	language "C++"
	configurations {
		"Debug",
		"Release",
	}

	flags {
		"NoMinimalRebuild",
		"Symbols",
		"StaticRuntime",
	}
	warnings "Extra"

	configuration "Release"
		optimize "full"

	configuration {}

	dofile "premake/tiny.lua"
	dofile "premake/webrtc.lua"

function example_project(name)
	project ("example_" .. name)
		kind "ConsoleApp"

		files {
			EXAMPLES_DIR .. name .. "/**",
		}
		includedirs {
			ROOT_DIR .. "include/",
		}

		links {
			"tiny",
			"webrtc",
		}

		configuration "windows"
			links {
				"ws2_32",
				"Iphlpapi",
				"winmm",
			}

		configuration {}
end

example_project("test")
example_project("twopeers")
example_project("audio_sin")
example_project("audio_micecho")
example_project("voip")
example_project("voip_net")
