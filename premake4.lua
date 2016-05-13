#!lua

-- A solution contains projects, and defines the available configurations
solution "Sofia-PS"
	configurations { "Release", "Debug" }

	if _ACTION == "clean" then
		os.rmdir("bin/")
		os.rmdir("lib/")
		os.rmdir("obj/")
	end	

	project "SharedTools"
		kind "StaticLib"
		language "C++"
		includedirs { 
			"Tools/inc/",
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
		}
		files{ "Tools/inc/**.h", "Tools/src/**.cpp" }

		bindir = "lib/"

		configuration "Debug"
			debugBindir= bindir .. "Debug/"
			os.mkdir(debugBindir)
			targetdir( debugBindir )
			targetname( "SharedTools-D" )
			defines { "DEBUG" }
			flags { "Symbols" }

		configuration "Release"
			releaseBindir= bindir .. "Release/"
			os.mkdir(releaseBindir)
			targetdir( releaseBindir )
			targetname( "SharedTools" )
			defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
			flags { "Optimize" }

	-- A project defines one build target
	project "Sofia-PS"
		kind "ConsoleApp"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/",
			"Tools/inc/", 
			"../LibGastonForSofia/inc", 
		}
		links{ "SharedTools" }
		excludes { "FCAPS/src/fcaps/premodules/**", "rapidjson/**", "boost/**" }
		files { "Sofia-PS/**.cpp", "FCAPS/**.h", "FCAPS/**.inl", "FCAPS/**.cpp" }

		bindir = "bin/"

		libdirs {
			"boost/stage/libs/",
		}

		rapidjsonDir="rapidjson/"
		if ( not os.isdir(rapidjsonDir)  ) then
			os.execute( "git clone https://github.com/miloyip/rapidjson.git " .. rapidjsonDir )
			os.execute( "git --git-dir " .. rapidjsonDir .. ".git --work-tree " .. rapidjsonDir .. " reset --hard c745c953adf68" )
			os.execute( "git --git-dir " .. rapidjsonDir .. ".git --work-tree " .. rapidjsonDir .. " apply --whitespace=fix rapidjson.patch" )
		end

		configuration "Debug"
			debugBindir= bindir .. "Debug/"
			os.mkdir(debugBindir)
			targetdir( debugBindir )
			targetname( "Sofia-PS-D" )
			--libdirs { "libs/Debug/" }
			defines { "DEBUG" }
			flags { "Symbols" }
			links{ 
				"boost_filesystem",
				"boost_system",
				"boost_thread",
				"dl",
				"pthread"
			}

		configuration "Release"
			releaseBindir= bindir .. "Release/"
			os.mkdir(releaseBindir)
			targetdir( releaseBindir )
			targetname( "Sofia-PS" )
			--libdirs { "libs/Release/" }
			defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
			flags { "Optimize" }
			links{ 
				"boost_filesystem",
				"boost_system",
				"boost_thread",
				"dl",
				"pthread"
			}

