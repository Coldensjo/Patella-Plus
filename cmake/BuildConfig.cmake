# The values someone fixes when building the client for their own server, src/BuildConfig.h and
# src/BuildConfigDefaults.h.
#
# PATELLA_BUILD_CONFIG names a developer's own copy of src/BuildConfigDefaults.h (unset by default): the whole build compiles
# with it through the definition PATELLA_BUILD_CONFIG_FILE, so pulling updates never conflicts with its values. src/BuildConfig.h
# applies the same compile-time checks to it. README.md, "Building for your own server".

include_guard(GLOBAL)

set(PATELLA_BUILD_CONFIG "" CACHE FILEPATH "A developer's own copy of src/BuildConfigDefaults.h (a client for their own server; empty: the defaults)")

if(PATELLA_BUILD_CONFIG)
	get_filename_component(patella_build_config_file "${PATELLA_BUILD_CONFIG}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
	if(NOT EXISTS "${patella_build_config_file}" OR IS_DIRECTORY "${patella_build_config_file}")
		message(FATAL_ERROR "PATELLA_BUILD_CONFIG: no such file: ${patella_build_config_file}")
	endif()
	message(STATUS "PATELLA_BUILD_CONFIG: ${patella_build_config_file} replaces src/BuildConfigDefaults.h")
	add_compile_definitions("PATELLA_BUILD_CONFIG_FILE=\"${patella_build_config_file}\"")
endif()
