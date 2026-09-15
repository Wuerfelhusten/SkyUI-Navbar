# Keep formatting scoped to owned C++; never recurse into build trees or references.
get_filename_component(NAVBAR_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_program(NAVBAR_CLANG_FORMAT NAMES clang-format
	HINTS "${NAVBAR_COMPILER_DIR}/../../../../../Llvm/x64/bin")

if(NAVBAR_CLANG_FORMAT)
	file(GLOB_RECURSE NAVBAR_STYLE_FILES CONFIGURE_DEPENDS
		"${PROJECT_SOURCE_DIR}/src/skse/*.cpp"
		"${PROJECT_SOURCE_DIR}/src/skse/*.h"
		"${PROJECT_SOURCE_DIR}/tests/*.cpp")
	list(APPEND NAVBAR_STYLE_FILES "${PROJECT_SOURCE_DIR}/cmake/Plugin.h.in")

	add_custom_target(format
		COMMAND "${NAVBAR_CLANG_FORMAT}" --style=file -i ${NAVBAR_STYLE_FILES}
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		COMMENT "Formatting owned C++ with the po3 style template"
		VERBATIM)
	add_custom_target(format-check
		COMMAND "${NAVBAR_CLANG_FORMAT}" --style=file --dry-run --Werror ${NAVBAR_STYLE_FILES}
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		COMMENT "Checking owned C++ formatting"
		VERBATIM)
endif()
