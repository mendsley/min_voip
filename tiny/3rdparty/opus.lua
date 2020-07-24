project "opus"
	language "C"
	kind "StaticLib"

	includedirs {
		"opus/include/",
		"opus/celt/",
		"opus/silk/",
		"opus/silk/float",
		"opus/src/",
		"opus_contrib/",
	}

	files {
		"opus/celt/*",
		"opus/silk/*",
		"opus/silk/float/**",
		"opus/src/**",
	}

	excludes {
		"opus/celt/*_demo.c",
		"opus/src/*_demo.c",
		"opus/src/mlp_train.c",
		"opus/src/opus_compare.c",
	}


	defines {
		"USE_ALLOCA",
		"HAVE_CONFIG_H",
	}
