# The client of the preset windows-x64: a 64-bit Patella.exe with MSVC x64 on SDL3. Its only graphics engine is the
# Vulkan engine (cmake/Vulkan.cmake).
#
# - The static CRT and no manifest (a manifest would change the common controls and the DPI handling the user sees).
# - The product settings: cmake/Warnings.cmake (C++23, /W4 with the baseline list, /WX) and cmake/Hardening.cmake
#   (/guard:cf, /CETCOMPAT); CMake's default optimisation flags; a PDB in Release and Debug.
# - The resource pictures are extracted from the local Tibia.exe at configure time.
#
# Configure with MSVC for x64 set up (vcvarsall.bat x64; `python -I tools/build.py windows-x64` does
# it). Without Tibia.exe (PATELLA_TIBIA_EXE) the configure stops; PATELLA_CLIENT=OFF builds only the
# portable libraries, for machines without it.
#
# PATELLA_PLACEHOLDER_RESOURCES=ON (hosted CI, which has no Tibia.exe and must never get it): the full
# Patella.exe with neutral placeholder pictures (tools/placeholder_resources.py, generated at
# configure time, no CipSoft file read). The option refuses to run where a Tibia.exe is present (the source tree's or
# PATELLA_TIBIA_EXE's), so it can never quietly replace the real pictures in a local build; the picture files then
# go to a folder of their own (res-placeholder), and the client logs a warning at start.

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "windows-x64 needs MSVC's x64 compiler: run python -I tools/build.py windows-x64, or "
		"configure from a developer prompt set up with vcvarsall.bat x64 (found ${CMAKE_SIZEOF_VOID_P}-byte "
		"pointers)")
endif()

enable_language(RC)

# The Python on PATH before any registered elsewhere.
set(Python3_FIND_REGISTRY LAST)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

option(PATELLA_PLACEHOLDER_RESOURCES "Build the client with neutral placeholder pictures instead of Tibia.exe's (the hosted CI's build; refused where a Tibia.exe is present)" OFF)

set(PATELLA_TIBIA_EXE "${PROJECT_SOURCE_DIR}/Tibia.exe" CACHE FILEPATH
	"The original Tibia.exe: the resource pictures are extracted from it")
include("${PROJECT_SOURCE_DIR}/cmake/Resources.cmake")
if(PATELLA_PLACEHOLDER_RESOURCES)
	foreach(tibia_exe "${PATELLA_TIBIA_EXE}" "${PROJECT_SOURCE_DIR}/Tibia.exe")
		if(EXISTS "${tibia_exe}")
			message(FATAL_ERROR "PATELLA_PLACEHOLDER_RESOURCES=ON refused: ${tibia_exe} exists. The placeholders are "
				"for machines without Tibia.exe (the hosted CI), and must never replace the real pictures in a local build.")
		endif()
	endforeach()
	set(PATELLA_RES_DIR "${PROJECT_BINARY_DIR}/res-placeholder")
	patella_placeholder_resources("${PATELLA_RES_DIR}")
else()
	if(NOT EXISTS "${PATELLA_TIBIA_EXE}")
		message(FATAL_ERROR "${PATELLA_TIBIA_EXE} is missing: the resource pictures are extracted from it "
			"(set PATELLA_TIBIA_EXE to use another copy, PATELLA_CLIENT=OFF to build only "
			"the portable libraries, or PATELLA_PLACEHOLDER_RESOURCES=ON on a machine without Tibia.exe)")
	endif()
	set(PATELLA_RES_DIR "${PROJECT_BINARY_DIR}/res")
	patella_extract_resources("${PATELLA_TIBIA_EXE}" "${PATELLA_RES_DIR}")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/Warnings.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Hardening.cmake")

# rc.exe: English (United States) where Patella.rc names no language.
set(CMAKE_RC_FLAGS "/l0x0409 /nologo")

file(GLOB patella_cpp_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(SORT patella_cpp_sources CASE INSENSITIVE)
file(GLOB patella_c_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/Extern/regexp/*.c")
list(SORT patella_c_sources CASE INSENSITIVE)
set(patella_rc "${PROJECT_SOURCE_DIR}/src/res/Patella.rc")
set_source_files_properties("${patella_rc}" PROPERTIES OBJECT_DEPENDS "${PATELLA_RES_DIR}/resources.tsv")

# The Vulkan engine: cmake/Vulkan.cmake builds the back end and the CPU driver; the client adds
# TSurfaceVulkan (src/vulkan, which the glob of src/*.cpp above does not reach) and the definition
# PATELLA_VULKAN, which no source tests. The graphics device choice: src/vulkan/DeviceSettings.cpp, the
# advanced options' buttons and Patella.ini (src/settings/Settings.cpp, patella_core's parser compiled
# here again for the static CRT); the environment variable PATELLA_VULKAN=auto|gpu|gpuN|cpu overrides it. The
# developer log's keys in Patella.ini: src/log/LogSettings.cpp, patella_core's, compiled here again too, and so
# are the options' keys in it (src/settings/PreferenceKeys.cpp, Preferences.cpp's Patella.ini).
include("${PROJECT_SOURCE_DIR}/cmake/Vulkan.cmake")
# SDL3 (cmake/Sdl3.cmake): the window and the event loop of src/SdlFrame.cpp. Patella.exe's Platform() has the SDL3
# window's input (src/platform/Win32Platform.cpp) and its clipboard and desktop (src/platform/SdlPlatform.cpp).
include("${PROJECT_SOURCE_DIR}/cmake/Sdl3.cmake")
# The platform's Win32 implementations (src/platform: the clock, the files, the network; Patella.exe's Platform())
# are compiled with them.
# The engine's window system (src/vulkan/WindowSystem.h): SDL3's (SDL_Vulkan_CreateSurface).
set(patella_vulkan_window_sources "${PROJECT_SOURCE_DIR}/src/vulkan/WindowSystemSdl.cpp")
set(patella_vulkan_client_sources "${PROJECT_SOURCE_DIR}/src/vulkan/SurfaceVulkan.cpp" ${patella_vulkan_window_sources}
	"${PROJECT_SOURCE_DIR}/src/vulkan/DeviceSettings.cpp" "${PROJECT_SOURCE_DIR}/src/settings/Settings.cpp"
	"${PROJECT_SOURCE_DIR}/src/settings/PreferenceKeys.cpp" "${PROJECT_SOURCE_DIR}/src/log/LogSettings.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/Win32Platform.cpp"
	# The SDL3 window's clipboard and desktop (message boxes, display modes).
	"${PROJECT_SOURCE_DIR}/src/platform/SdlPlatform.cpp" "${PROJECT_SOURCE_DIR}/src/platform/SdlClipboard.cpp"
	# The network, TPortableSocket (patella_platform's source, compiled here again for the static CRT) and
	# the Windows client's glue (the lookups' replies to the window, the modal loops' polling).
	"${PROJECT_SOURCE_DIR}/src/platform/PortableSocket.cpp" "${PROJECT_SOURCE_DIR}/src/platform/NetworkWin32.cpp"
	# Where the data files and Patella.ini are (settings::ChooseLocations' inputs from the process).
	"${PROJECT_SOURCE_DIR}/src/platform/DataLocationWin32.cpp"
	# The process's entry, WinMain.
	"${PROJECT_SOURCE_DIR}/src/platform/EntryWin32.cpp")

# The client: Patella.exe.
function(patella_client_compile_settings target)
	set_target_properties(${target} PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
	target_include_directories(${target} PRIVATE
		"$<$<COMPILE_LANGUAGE:C,CXX>:${PROJECT_SOURCE_DIR}/src>"
		"$<$<COMPILE_LANGUAGE:RC>:${PATELLA_RES_DIR}>")
	target_compile_definitions(${target} PRIVATE
		$<$<CONFIG:Debug>:_DEBUG>
		$<$<CONFIG:Release>:NDEBUG>
		$<$<COMPILE_LANGUAGE:C,CXX>:_WIN32_WINNT=0x0601>)
	patella_target_language(${target})
	patella_target_warnings(${target})
	patella_target_hardening(${target})
	target_compile_definitions(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:PATELLA_VULKAN>)
	target_link_libraries(${target} PRIVATE patella_render_vulkan)
	if(PATELLA_PLACEHOLDER_RESOURCES) # the client's warning at start (Player.cpp)
		target_compile_definitions(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:PATELLA_PLACEHOLDER_RESOURCES>)
	endif()
	# SDL3's headers (the library itself is linked by patella_client).
	target_link_libraries(${target} PRIVATE SDL3::Headers)
endfunction()

function(patella_client target)
	add_executable(${target} WIN32 ${patella_cpp_sources} ${patella_c_sources} "${patella_rc}"
		${patella_vulkan_client_sources} ${ARGN})
	patella_client_compile_settings(${target})
	target_link_libraries(${target} PRIVATE
		winmm kernel32 user32 gdi32 advapi32 shell32 comctl32 ws2_32 SDL3::SDL3-static)
	target_link_options(${target} PRIVATE
		/MANIFEST:NO /DEBUG
		$<$<CONFIG:Release>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>)
	# A Release link never updates the PDB of the link before (cmake/Reproducible.cmake).
	patella_repro_link(${target})
	patella_vulkan_cpu_driver(${target})
endfunction()

patella_client(Patella)
