# Package build outputs and the checked-in default, never the user's deployed mod.
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION SKSE/Plugins LIBRARY DESTINATION SKSE/Plugins)
install(FILES "${NAVBAR_SWF_OUTPUT}" DESTINATION Interface RENAME NavBar.swf)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config/SkyUINavbar.json" DESTINATION SKSE/Plugins)
install(FILES ${NAVBAR_TRANSLATION_OUTPUTS} DESTINATION Interface/Translations)
# Documentation and license files stay in the repository, not the release archives.

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME "Navigation Bar for SkyUI")
set(CPACK_PACKAGE_VENDOR "Wuerfelhusten")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "Navigation-Bar-for-SkyUI-${PROJECT_VERSION}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/packages")
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_SOURCE_GENERATOR ZIP)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "Navigation-Bar-for-SkyUI-${PROJECT_VERSION}-source")
set(NAVBAR_SOURCE_REGEX "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(NAVBAR_REGEX_CHAR IN ITEMS "." "+" "(" ")" "[" "]" "^" "$" "?" "*" "|")
	string(REPLACE "${NAVBAR_REGEX_CHAR}" "\\${NAVBAR_REGEX_CHAR}" NAVBAR_SOURCE_REGEX "${NAVBAR_SOURCE_REGEX}")
endforeach()
set(CPACK_SOURCE_IGNORE_FILES
	"^${NAVBAR_SOURCE_REGEX}/[.]git/" "^${NAVBAR_SOURCE_REGEX}/[.]vs/"
	"^${NAVBAR_SOURCE_REGEX}/build[^/]*/" "^${NAVBAR_SOURCE_REGEX}/dist/" "^${NAVBAR_SOURCE_REGEX}/out/"
	"^${NAVBAR_SOURCE_REGEX}/docs/" "^${NAVBAR_SOURCE_REGEX}/README[.]md$" "^${NAVBAR_SOURCE_REGEX}/CHANGELOG[.]md$"
	"^${NAVBAR_SOURCE_REGEX}/licenses/" "^${NAVBAR_SOURCE_REGEX}/LICENSE$" "^${NAVBAR_SOURCE_REGEX}/THIRD_PARTY_NOTICES[.]md$"
	"/CMakeUserPresets[.]json$" "[.]user$" "[.]log$"
)
include(CPack)

if(NOT ENABLE_SKYRIM_SE OR NOT ENABLE_SKYRIM_AE OR ENABLE_SKYRIM_VR)
	message(STATUS "The release target is provided for FLATRIM (SE+AE, no VR).")
	return()
endif()

if(NOT BUILD_TESTING OR NOT NAVBAR_NODE)
	message(STATUS "The release target requires BUILD_TESTING=ON and Node.js; ordinary builds remain available.")
	return()
endif()

add_custom_target(release
	COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_CURRENT_BINARY_DIR}" -C $<CONFIG> --output-on-failure --no-tests=error
	COMMAND "${CMAKE_CPACK_COMMAND}" --config "${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake" -C $<CONFIG>
	COMMAND "${CMAKE_CPACK_COMMAND}" --config "${CMAKE_CURRENT_BINARY_DIR}/CPackSourceConfig.cmake" -C $<CONFIG>
	COMMAND "${NAVBAR_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
		-File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/ValidateRelease.ps1"
		-ProjectDir "${CMAKE_CURRENT_SOURCE_DIR}" -BuildDir "${CMAKE_CURRENT_BINARY_DIR}"
		-Plugin "$<TARGET_FILE:${PROJECT_NAME}>" -Symbols "$<TARGET_PDB_FILE:${PROJECT_NAME}>"
		-Version "${PROJECT_VERSION}" -Configuration "$<CONFIG>"
	DEPENDS ${PROJECT_NAME} deploy_interface
	COMMENT "Testing and packaging the local release, source and debug symbols"
	VERBATIM USES_TERMINAL
)
# Tests are independent targets, not dependencies of the plugin.
if(BUILD_TESTING)
	get_property(NAVBAR_DIRECTORY_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
	foreach(NAVBAR_TARGET IN LISTS NAVBAR_DIRECTORY_TARGETS)
		if(NAVBAR_TARGET MATCHES "^Navbar.*Tests$")
			add_dependencies(release ${NAVBAR_TARGET})
		endif()
	endforeach()
endif()
