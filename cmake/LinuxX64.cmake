# The client of the Linux presets (Patella-Modern issue #131, plan M8; notes/decisions.md, "#131"): the client's sources as
# on Windows (cmake/WindowsX64.cmake), on the SDL3 window and the Vulkan engine, with the platform layer's Linux files
# (src/platform/*Linux.cpp) in place of the Win32 ones. No resource compiler: the cursors and the window icon (issue #162)
# are built in from the local Tibia.exe's (or the placeholders, tools/embed_cursors.py; the MFC-era resources are not used).
#
# The client's code is compiled with PATELLA_CORE (common.h: no Windows headers, as patella_core; the types its platform
# layer still names come from compat/win32_types.h, PATELLA_WIN32_TYPES), and with MSVC's rand (compat/msvc.h,
# PATELLA_MSVC_RAND), so the client's random numbers are the ones of a Windows run with the same seed.
#
# Tibia.exe (PATELLA_TIBIA_EXE, or the environment's, which tools/build.py sets when it mounts the local copy into the
# image) is only read for the cursors and the icon; without it the configure stops, unless PATELLA_PLACEHOLDER_RESOURCES=ON (the hosted
# CI), which refuses where a Tibia.exe is present, as on Windows.

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)

# The default is the environment's (PATELLA_PLACEHOLDER_RESOURCES=1 in the hosted CI's containers, whose workflows cannot
# pass -D through tools/build.py).
if(DEFINED ENV{PATELLA_PLACEHOLDER_RESOURCES} AND NOT "$ENV{PATELLA_PLACEHOLDER_RESOURCES}" STREQUAL "")
	set(patella_placeholder_default ON)
else()
	set(patella_placeholder_default OFF)
endif()
option(PATELLA_PLACEHOLDER_RESOURCES "Build the client with neutral placeholder pictures instead of Tibia.exe's (the hosted CI's build; refused where a Tibia.exe is present, issue #127)" ${patella_placeholder_default})

if(DEFINED ENV{PATELLA_TIBIA_EXE} AND NOT "$ENV{PATELLA_TIBIA_EXE}" STREQUAL "")
	set(patella_tibia_exe_default "$ENV{PATELLA_TIBIA_EXE}")
else()
	set(patella_tibia_exe_default "${PROJECT_SOURCE_DIR}/Tibia.exe")
endif()
set(PATELLA_TIBIA_EXE "${patella_tibia_exe_default}" CACHE FILEPATH
	"The original Tibia.exe: its cursors and icon are built into the Linux client")
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
		message(FATAL_ERROR "${PATELLA_TIBIA_EXE} is missing: the cursors and the icon are taken from it (set PATELLA_TIBIA_EXE, "
			"PATELLA_CLIENT=OFF for only the portable libraries, or PATELLA_PLACEHOLDER_RESOURCES=ON on a machine without Tibia.exe)")
	endif()
	set(PATELLA_RES_DIR "${PROJECT_BINARY_DIR}/res")
	patella_extract_resources("${PATELLA_TIBIA_EXE}" "${PATELLA_RES_DIR}")
endif()
set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	"${PROJECT_SOURCE_DIR}/tools/embed_cursors.py")
execute_process(
	COMMAND "${Python3_EXECUTABLE}" -I "${PROJECT_SOURCE_DIR}/tools/embed_cursors.py" "${PATELLA_RES_DIR}"
		"${PROJECT_BINARY_DIR}/EmbeddedCursors.cpp"
	RESULT_VARIABLE embed_result ERROR_VARIABLE embed_error)
if(NOT embed_result EQUAL 0)
	message(FATAL_ERROR "tools/embed_cursors.py failed (${embed_result}):\n${embed_error}")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/Warnings.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Hardening.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Vulkan.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Sdl3.cmake")

file(GLOB patella_cpp_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(SORT patella_cpp_sources CASE INSENSITIVE)
file(GLOB patella_c_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/Extern/regexp/*.c")
list(SORT patella_c_sources CASE INSENSITIVE)

set(patella_linux_client_sources
	"${PROJECT_SOURCE_DIR}/src/vulkan/SurfaceVulkan.cpp"
	"${PROJECT_SOURCE_DIR}/src/vulkan/WindowSystemSdl.cpp"
	"${PROJECT_SOURCE_DIR}/src/vulkan/DeviceSettings.cpp"
	"${PROJECT_SOURCE_DIR}/src/settings/Settings.cpp"
	"${PROJECT_SOURCE_DIR}/src/settings/PreferenceKeys.cpp"
	"${PROJECT_SOURCE_DIR}/src/log/LogSettings.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/LinuxPlatform.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/SdlPlatform.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/SdlClipboard.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/PortableSocket.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/NetworkLinux.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/DataLocationLinux.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/EntryLinux.cpp"
	"${PROJECT_SOURCE_DIR}/src/platform/WindowIconLinux.cpp"
	"${PROJECT_BINARY_DIR}/EmbeddedCursors.cpp")

# The compile settings of the client's targets and of the header checks (cmake/HeaderCheck.cmake).
function(patella_client_compile_settings target)
	target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")
	target_compile_definitions(${target} PRIVATE
		$<$<CONFIG:Debug>:_DEBUG>
		$<$<CONFIG:Release>:NDEBUG>
		PATELLA_CORE PATELLA_WIN32_TYPES PATELLA_MSVC_RAND PATELLA_VULKAN)
	patella_target_language(${target})
	patella_target_warnings(${target})
	patella_target_hardening(${target})
	target_link_libraries(${target} PRIVATE patella_render_vulkan SDL3::Headers)
	if(PATELLA_PLACEHOLDER_RESOURCES)
		target_compile_definitions(${target} PRIVATE PATELLA_PLACEHOLDER_RESOURCES)
	endif()
endfunction()

function(patella_client target)
	add_executable(${target} ${patella_cpp_sources} ${patella_c_sources} ${patella_linux_client_sources} ${ARGN})
	patella_client_compile_settings(${target})
	target_link_libraries(${target} PRIVATE
		$<IF:$<TARGET_EXISTS:SDL3::SDL3-static>,SDL3::SDL3-static,SDL3::SDL3> Threads::Threads ${CMAKE_DL_LIBS})
endfunction()

patella_client(Patella)
