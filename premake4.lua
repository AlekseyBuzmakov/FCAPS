#!lua

-- A solution contains projects, and defines the available configurations
solution "Sofia-PS"
   configurations { "Release", "Debug" }

   -- A project defines one build target
   project "Sofia-PS"
      kind "ConsoleApp"
      language "C++"
      includedirs { 
	      "boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
	      "rapidjson/include",
	      "FCAPS/include/", 
	      "FCAPS/src/",
	      "Tools/", 
	      "../LibGastonForSofia/inc", 
	      "Tools/" 
      }
      files { "**.h", "**.inl", "**.cpp" }
      excludes { "FCAPS/src/fcaps/premodules/**", "rapidjson/**", "boost/**" }

      bindir = "bin/"

      configuration "linux or macosx"
	      libdirs {
			 "boost/bin.v2/libs/filesystem/build/gcc-4.8/release/link-static/threading-multi/",
			 "boost/bin.v2/libs/system/build/gcc-4.8/release/link-static/threading-multi/",
			 "boost/bin.v2/libs/thread/build/gcc-4.8/release/link-static/threading-multi/"
	      }

      configuration "Debug"
         debugBindir= bindir .. "Debug/"
	 os.mkdir(debugBindir)
      	 targetname( debugBindir .. "Sofia-PS-D" )
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
      	 targetname( releaseBindir .. "Sofia-PS-D" )
         defines { "NDEBUG" }
         flags { "Optimize" }
	 links{ 
		 "boost_filesystem",
		 "boost_system",
		 "boost_thread",
		 "dl",
		 "pthread"
	 }

