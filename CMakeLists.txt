# Patella: the build.
#
# One workflow preset per platform configures and builds (CMakePresets.json):
#	cmake --workflow --preset windows-x64
#	cmake --workflow --preset linux-x64-gcc
# The MSVC presets need the compiler set up (vcvarsall.bat); tools/build.py does that and runs the
# workflow from any shell: python -I tools/build.py <preset> (issue #10).
# windows-x64 (MSVC x64) builds the client, a 64-bit Patella.exe on SDL3 and the Vulkan engine (cmake/WindowsX64.cmake,
# issue #5); linux-x64-gcc (GCC 16) the Linux client, Patella (cmake/LinuxX64.cmake, issue #131; on Windows
# tools/build.py runs it in the Docker image of docker/linux-x64).
# Every product preset builds patella_core (cmake/Core.cmake), the portable code. Its targets take the language, warning
# and hardening settings of cmake/Warnings.cmake and cmake/Hardening.cmake (issue #8).

cmake_minimum_required(VERSION 3.30)

project(Patella
	DESCRIPTION "Patella-Modern, the modernized reconstruction of the Tibia 7.72 client"
	LANGUAGES C CXX)

option(PATELLA_CLIENT "Build the client (MSVC x64 and the Linux presets); needs Tibia.exe for its resources (PATELLA_PLACEHOLDER_RESOURCES=ON: none, the hosted CI)" ON)
option(PATELLA_PACKAGE "The install rules and CPack packages (cmake/Package.cmake, issue #135); refused without PATELLA_PLACEHOLDER_RESOURCES=ON: the presets package-windows-x64 and package-linux-x64" OFF)

# Reproducible Release builds, for every target below, FetchContent's included (issue #136).
include(cmake/Reproducible.cmake)
# The values a build for another server fixes at compile time, PATELLA_BUILD_CONFIG, and their checks (issue #169).
include(cmake/BuildConfig.cmake)
include(cmake/Core.cmake)
if(PATELLA_CLIENT AND MSVC)
	include(cmake/WindowsX64.cmake)
elseif(PATELLA_CLIENT AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# The Linux client (issue #131, plan M8): SDL3 and the Vulkan engine.
	include(cmake/LinuxX64.cmake)
endif()
if(PATELLA_PACKAGE)
	include(cmake/Package.cmake)
endif()
