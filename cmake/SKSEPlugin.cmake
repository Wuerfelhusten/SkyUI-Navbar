add_library("${PROJECT_NAME}" SHARED)

# Set C++ standard and enable folder organization
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_23)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Include C++ and header files from source directories
include(AddCXXFiles)
add_cxx_files("${PROJECT_NAME}")

# Generate configuration files
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake/version.rc" @ONLY)

# Add generated files to the target
target_sources("${PROJECT_NAME}" PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h ${CMAKE_CURRENT_BINARY_DIR}/cmake/version.rc)

# Precompile headers
target_precompile_headers("${PROJECT_NAME}" PRIVATE src/skse/PCH.h)

# Enable interprocedural optimization
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# Keep release debug symbols for native crash diagnostics.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

# MSVC-specific settings
if(CMAKE_GENERATOR MATCHES "Visual Studio")
	add_compile_definitions(_UNICODE)
	add_compile_options(/MP)

	target_compile_definitions("${PROJECT_NAME}" PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
	target_compile_options("${PROJECT_NAME}" PRIVATE
		/sdl             # Additional security checks
		/utf-8           # Source and execution character sets
		/Zi              # Debug information
		/permissive-     # Standards conformance
		/Zc:preprocessor
		/wd4200          # CommonLib uses zero-sized arrays in native layouts.
		"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)
	target_link_options("${PROJECT_NAME}" PRIVATE
		"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
		"$<$<CONFIG:RELEASE>:/LTCG;/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

# Find required packages
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(DirectXTK CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Include directories and libraries
target_include_directories("${PROJECT_NAME}" PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/src/skse)

# Verify that vcpkg built CommonLib with the requested runtime layouts.
foreach(NAVBAR_RUNTIME IN ITEMS SE AE VR)
	target_compile_definitions("${PROJECT_NAME}" PRIVATE
		"NAVBAR_EXPECT_${NAVBAR_RUNTIME}=$<BOOL:${ENABLE_SKYRIM_${NAVBAR_RUNTIME}}>")
endforeach()

# Link libraries
target_link_libraries("${PROJECT_NAME}" PRIVATE
	CommonLibSSE::CommonLibSSE
	nlohmann_json::nlohmann_json
)
