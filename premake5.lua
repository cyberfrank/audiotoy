-- premake5.lua

workspace "workspace"
    configurations { "Debug", "Release" }
    language "C"
    flags {
        "FatalWarnings", 
        "MultiProcessorCompile"
    }
    warnings "Extra"
    inlining "Auto"
    sysincludedirs { "" }
    editAndContinue "Off"
    targetdir "bin"
    location "bin"
    characterset "MBCS" 

filter "system:windows"
    platforms { "Win64" }
    systemversion("latest")

filter "platforms:Win64"
    defines { 
        "OS_WINDOWS", 
        "_CRT_SECURE_NO_WARNINGS"
    }
    includedirs "src"
    staticruntime "On"
    architecture "x64"

filter "configurations:Debug"
    defines { "DEBUG_MODE", "DEBUG" }
    symbols "On"

filter "configurations:Release"
    defines { "RELEASE_MODE", "RELEASE" }
    optimize "On"

project "audiotoy"
    kind "ConsoleApp"
    targetname "audiotoy"
    files { "src/**.cc", "src/**.h" }
