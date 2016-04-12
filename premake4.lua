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
      excludes { "FCAPS/src/fcaps/premodules/**", "rapidjson/**", "boost/**" }
      files { "**.h", "**.inl", "**.cpp" }

      bindir = "bin/"
	  
	  rapidjsonDir="rapidjson/"
	  if ( not os.isdir(rapidjsonDir)  ) then
		os.execute( "git clone https://github.com/miloyip/rapidjson.git " .. rapidjsonDir )
		os.execute( "git --git-dir " .. rapidjsonDir .. ".git --work-tree " .. rapidjsonDir .. " reset --hard c745c953adf68" )
		os.execute( "git --git-dir " .. rapidjsonDir .. ".git --work-tree " .. rapidjsonDir .. " apply --whitespace=fix rapidjson.patch" )
	  end
	  
	  libdirs {
		 "boost/bin.v2/libs/filesystem/build/*/release/link-static/threading-multi/",
		 "boost/bin.v2/libs/system/build/*/release/link-static/threading-multi/",
		 "boost/bin.v2/libs/thread/build/*/release/link-static/threading-multi/"
	  }

      configuration "Debug"
         debugBindir= bindir .. "Debug/"
	 os.mkdir(debugBindir)
      	 targetdir( debugBindir )
	 targetname( "Sofia-PS-D" )
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
         defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
         flags { "Optimize" }
	 links{ 
		 "boost_filesystem",
		 "boost_system",
		 "boost_thread",
		 "dl",
		 "pthread"
	 }

