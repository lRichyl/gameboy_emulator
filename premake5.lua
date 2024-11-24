 workspace "MyWorkspace"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "build"


project "Gameboy"
   objdir ("build/obj/%{cfg.platform}/%{cfg.buildcfg}")
   targetdir ("build/bin/%{cfg.platform}/%{cfg.buildcfg}")
   debugdir ""

   -- kind "WindowedApp"
   kind "ConsoleApp"
   language "C++"

   files {"src/**.cpp", "src/**.c", "src/**.h"}

    -- Common settings
   libdirs { "vendor/sdl/lib"}
   includedirs {"src", "vendor/sdl/include"}

   postbuildcommands {
      "{COPYFILE} %{wks.location}/../vendor/sdl/lib/SDL3.dll %{wks.location}/%{cfg.targetdir}",
     "{COPYFILE} %{wks.location}/../vendor/sdl/lib/SDL2_mixer.dll %{wks.location}/%{cfg.targetdir}",
     "{COPYFILE} %{wks.location}/../vendor/sdl/lib/SDL2_ttf.dll %{wks.location}/%{cfg.targetdir}",
     "{COPYFILE} %{wks.location}/%{cfg.targetdir}/Gameboy.exe %{wks.location}/.."
   }

   -- Compiler-specific settings
   -- filter "toolset:gcc or toolset:clang"
   --    links {"mingw32", "SDL2main", "SDL2",  "SDL2_mixer", "SDL2_ttf", "shell32"}
   --    buildoptions { "-Wall", "-Wextra"}
   --    linkoptions { "-pthread", "-mwindows" }

   filter "toolset:msc*"
      links { "SDL3",  "SDL2_mixer", "SDL2_ttf", "shell32"}
      buildoptions { "/W3", "/std:c++20" }
      -- linkoptions { "/SUBSYSTEM:CONSOLE" }

   filter "platforms:x64"
      architecture "x64"

    -- Configuration-specific settings
   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

project "Tests"
   objdir ("tests/build/obj/%{cfg.platform}/%{cfg.buildcfg}")
   targetdir ("tests/build/bin/%{cfg.platform}/%{cfg.buildcfg}")
   debugdir ""

   -- kind "WindowedApp"
   kind "ConsoleApp"
   language "C++"

   files {"src/**.cpp", "src/**.c", "src/**.h", "tests/src/**.cpp"}
   includedirs {"src"}

   filter "toolset:msc*"
      buildoptions { "/W3", "/std:c++20" }
      -- linkoptions { "/SUBSYSTEM:CONSOLE" }

   filter "platforms:x64"
      architecture "x64"

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"