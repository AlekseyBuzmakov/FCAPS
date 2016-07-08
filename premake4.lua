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
		kind "SharedLib"
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
			targetname( "SharedTools" )
			defines { "DEBUG" }
			flags { "Symbols" }

		configuration "Release"
			releaseBindir= bindir .. "Release/"
			os.mkdir(releaseBindir)
			targetdir( releaseBindir )
			targetname( "SharedTools" )
			defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
			flags { "Optimize" }

	project "Storages"
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"Tools/inc/", 
		}
		files{ "FCAPS/src/fcaps/Storages/**.h", "FCAPS/src/fcaps/Storages/**.cpp" }

		libdirs {
			"boost/stage/libs/",
		}

		name = "Storages"
		bindir = "lib/"

		configuration "Debug"
			debugBindir= bindir .. "Debug/"
			os.mkdir(debugBindir)
			targetdir( debugBindir )
			targetname( name )
			defines { "DEBUG" }
			flags { "Symbols" }
			links{ 
				--"boost_filesystem",
				--"boost_system",
				--"boost_thread",
				--"dl",
				--"pthread",
				"SharedTools"
			}

		configuration "Release"
			releaseBindir= bindir .. "Release/"
			os.mkdir(releaseBindir)
			targetdir( releaseBindir )
			targetname( name )
			defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
			flags { "Optimize" }
			links{ 
				--"boost_filesystem",
				--"boost_system",
				--"boost_thread",
				--"dl",
				--"pthread",
				"SharedTools"
			}

	project "StdFCAModule"
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/src/", -- For IntentStorage
			"FCAPS/include/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/StdFCA/**.h", "FCAPS/src/fcaps/StdFCA/**.cpp" }

		libdirs {
			"boost/stage/libs/",
		}

		name = "StdFCAModule"
		bindir = "modules/"

		configuration "Debug"
			debugBindir= bindir .. "Debug/"
			os.mkdir(debugBindir)
			targetdir( debugBindir )
			targetname( name  )
			defines { "DEBUG" }
			flags { "Symbols" }
			links{ 
				--"boost_filesystem",
				--"boost_system",
				--"boost_thread",
				--"dl",
				--"pthread",
				"SharedTools",
				"Storages"
			}

		configuration "Release"
			releaseBindir= bindir .. "Release/"
			os.mkdir(releaseBindir)
			targetdir( releaseBindir )
			targetname( name )
			defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
			flags { "Optimize" }
			links{ 
				--"boost_filesystem",
				--"boost_system",
				--"boost_thread",
				--"dl",
				--"pthread",
				"SharedTools",
				"Storages"
			}

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
			"Sofia-PS/inc/",
			"../LibGastonForSofia/inc" 
		}
		excludes { "FCAPS/src/fcaps/premodules/**", "rapidjson/**", "boost/**" }
		files { "Sofia-PS/**.cpp" } --, "FCAPS/**.h", "FCAPS/**.inl", "FCAPS/**.cpp" }

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
			targetname( "Sofia-PS" )
			--libdirs { "libs/Debug/" }
			defines { "DEBUG" }
			flags { "Symbols" }
			links{ 
				"boost_filesystem",
				"boost_system",
				"boost_thread",
				"dl",
				"pthread",
				"SharedTools"
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
				"pthread",
				"SharedTools"
			}

