cmake_minimum_required(VERSION 3.21)

project(SkyUINavbar VERSION 1.0.0 DESCRIPTION "Navigation Bar for SkyUI" LANGUAGES NONE)

option(ENABLE_SKYRIM_SE "Build for pre-AE Skyrim SE" ON)
option(ENABLE_SKYRIM_AE "Build for Skyrim AE runtimes" ON)
option(ENABLE_SKYRIM_VR "Build for Skyrim VR (not a release target)" OFF)

option(NAVBAR_BUILD_PLUGIN "Build the CommonLibSSE-NG plugin" ON)
option(NAVBAR_BUILD_INTERFACE "Build the ActionScript 2 interface with FFDec" ON)
set(NAVBAR_DEPLOY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist" CACHE PATH
	"Staging or mod-manager directory receiving the built plugin and interface")
set(FFDEC_ROOT "" CACHE PATH "Directory containing ffdec-cli.exe")

# Maintain editable UTF-8 sources; Skyrim consumes UTF-16 LE BOM tab-separated files.
find_program(NAVBAR_POWERSHELL NAMES pwsh powershell REQUIRED)
file(GLOB NAVBAR_TRANSLATIONS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/translations/SkyUINavbar_*.txt")
set(NAVBAR_TRANSLATION_OUTPUTS "")
foreach(TRANSLATION IN LISTS NAVBAR_TRANSLATIONS)
	get_filename_component(TRANSLATION_NAME "${TRANSLATION}" NAME)
	set(TRANSLATION_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/interface/Translations/${TRANSLATION_NAME}")
	add_custom_command(OUTPUT "${TRANSLATION_OUTPUT}"
		COMMAND "${NAVBAR_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
			-File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/BuildTranslations.ps1"
			-Source "${TRANSLATION}" -Output "${TRANSLATION_OUTPUT}"
		DEPENDS "${TRANSLATION}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/BuildTranslations.ps1"
		COMMENT "Encoding ${TRANSLATION_NAME} for Skyrim" VERBATIM)
	list(APPEND NAVBAR_TRANSLATION_OUTPUTS "${TRANSLATION_OUTPUT}")
endforeach()
add_custom_target(deploy_translations ALL
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${NAVBAR_DEPLOY_DIR}/Interface/Translations"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${NAVBAR_TRANSLATION_OUTPUTS} "${NAVBAR_DEPLOY_DIR}/Interface/Translations"
	DEPENDS ${NAVBAR_TRANSLATION_OUTPUTS}
	COMMENT "Deploying standard Skyrim translation files" VERBATIM)

if(NAVBAR_BUILD_PLUGIN)
	enable_language(CXX)
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)

	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
	include(SKSEPlugin)
	include(CodeStyle)

	include(CTest)
	if(BUILD_TESTING)
		get_filename_component(NAVBAR_LINKER_DIR "${CMAKE_LINKER}" DIRECTORY)
		find_program(NAVBAR_DUMPBIN NAMES dumpbin HINTS "${NAVBAR_LINKER_DIR}" REQUIRED)
		add_test(NAME NavbarPluginMetadata COMMAND "${NAVBAR_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
			-File "${CMAKE_CURRENT_SOURCE_DIR}/tests/PluginMetadataTests.ps1"
			-Plugin "$<TARGET_FILE:${PROJECT_NAME}>" -Version "${PROJECT_VERSION}" -Dumpbin "${NAVBAR_DUMPBIN}")
		add_executable(NavbarConfigTests tests/ConfigTests.cpp src/skse/ConfigData.cpp)
		target_include_directories(NavbarConfigTests PRIVATE src/skse)
		target_link_libraries(NavbarConfigTests PRIVATE nlohmann_json::nlohmann_json)
		if(MSVC)
			target_compile_options(NavbarConfigTests PRIVATE /utf-8)
		endif()
		add_test(NAME NavbarConfig COMMAND NavbarConfigTests "${CMAKE_CURRENT_SOURCE_DIR}/config/SkyUINavbar.json")
		add_test(NAME NavbarDeployConfig COMMAND "${CMAKE_COMMAND}"
			"-DPROJECT_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/deploy-config-tests"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/tests/DeployConfigTests.cmake")
		add_executable(NavbarSkillsInputTests tests/SkillsInputTests.cpp)
		target_include_directories(NavbarSkillsInputTests PRIVATE src/skse)
		add_test(NAME NavbarSkillsInput COMMAND NavbarSkillsInputTests)
		add_executable(NavbarAnimationStateTests tests/AnimationStateTests.cpp)
		target_include_directories(NavbarAnimationStateTests PRIVATE src/skse)
		add_test(NAME NavbarAnimationState COMMAND NavbarAnimationStateTests)
		add_executable(NavbarTransitionInputTests tests/TransitionInputTests.cpp)
		target_include_directories(NavbarTransitionInputTests PRIVATE src/skse)
		add_test(NAME NavbarTransitionInput COMMAND NavbarTransitionInputTests)
		add_executable(NavbarMenuRouteTests tests/MenuRouteTests.cpp)
		target_include_directories(NavbarMenuRouteTests PRIVATE src/skse)
		add_test(NAME NavbarMenuRoutes COMMAND NavbarMenuRouteTests)
		add_executable(NavbarCycleTests tests/CycleTests.cpp)
		target_include_directories(NavbarCycleTests PRIVATE src/skse)
		add_test(NAME NavbarCycle COMMAND NavbarCycleTests)
		add_executable(NavbarHoldNavigationTests tests/HoldNavigationTests.cpp)
		target_include_directories(NavbarHoldNavigationTests PRIVATE src/skse)
		add_test(NAME NavbarHoldNavigation COMMAND NavbarHoldNavigationTests)
		add_executable(NavbarMovieAssetPathTests tests/MovieAssetPathTests.cpp)
		target_include_directories(NavbarMovieAssetPathTests PRIVATE src/skse)
		add_test(NAME NavbarMovieAssetPaths COMMAND NavbarMovieAssetPathTests)
		add_executable(NavbarFadeDurationTests tests/FadeDurationTests.cpp)
		target_include_directories(NavbarFadeDurationTests PRIVATE src/skse)
		add_test(NAME NavbarFadeDurations COMMAND NavbarFadeDurationTests)
		add_executable(NavbarMenuFadeTests tests/MenuFadeTests.cpp)
		target_include_directories(NavbarMenuFadeTests PRIVATE src/skse)
		add_test(NAME NavbarMenuFade COMMAND NavbarMenuFadeTests)
		add_executable(NavbarTranslationCatalogTests tests/TranslationCatalogTests.cpp)
		target_include_directories(NavbarTranslationCatalogTests PRIVATE src/skse)
		add_test(NAME NavbarTranslationCatalog COMMAND NavbarTranslationCatalogTests)
		add_executable(NavbarBinaryContractTests tests/BinaryContractTests.cpp)
		target_include_directories(NavbarBinaryContractTests PRIVATE src/skse)
		add_test(NAME NavbarBinaryContracts COMMAND NavbarBinaryContractTests)
		find_program(NAVBAR_NODE NAMES node)
		if(NAVBAR_NODE)
			add_test(NAME NavbarReleaseConfiguration COMMAND "${NAVBAR_NODE}"
				"${CMAKE_CURRENT_SOURCE_DIR}/tests/ReleaseConfigTests.cjs" "${CMAKE_CURRENT_SOURCE_DIR}")
			add_test(NAME NavbarTranslations COMMAND "${NAVBAR_NODE}"
				"${CMAKE_CURRENT_SOURCE_DIR}/tests/TranslationTests.cjs"
				"${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/interface/Translations")
			set(NAVBAR_FADER_REFERENCE "${CMAKE_CURRENT_SOURCE_DIR}/../Reference/SkyUI-Community/source/actionscript/Vanilla/FaderMenu.as")
			if(EXISTS "${NAVBAR_FADER_REFERENCE}")
				add_test(NAME NavbarFaderLogic COMMAND "${NAVBAR_NODE}"
					"${CMAKE_CURRENT_SOURCE_DIR}/tests/FaderLogicTests.cjs" "${NAVBAR_FADER_REFERENCE}")
			endif()
			add_test(NAME NavbarWidgetLogic COMMAND "${NAVBAR_NODE}"
				"${CMAKE_CURRENT_SOURCE_DIR}/tests/WidgetTests.cjs"
				"${CMAKE_CURRENT_SOURCE_DIR}/src/ui/actionscript/NavBar/NavbarWidget.as")
		endif()
	endif()

	add_custom_command(TARGET "${PROJECT_NAME}" POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E make_directory
			"${NAVBAR_DEPLOY_DIR}/SKSE/Plugins"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"$<TARGET_FILE:${PROJECT_NAME}>"
			"${NAVBAR_DEPLOY_DIR}/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
		COMMENT "Deploying CommonLib plugin"
		VERBATIM
	)
endif()

add_custom_target(deploy_config ALL
	COMMAND "${CMAKE_COMMAND}"
		"-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/config/SkyUINavbar.json"
		"-DROOT=${NAVBAR_DEPLOY_DIR}"
		"-DBACKUP_DIR=${CMAKE_CURRENT_BINARY_DIR}/deploy-backups"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployConfig.cmake"
	COMMENT "Deploying minimal navbar configuration (backing up legacy files)"
	VERBATIM
)

if(NAVBAR_BUILD_INTERFACE)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
	include(ImportToSWF)
	include(SWFXMLPatch)

	find_program(FFDEC_CLI
		NAMES ffdec-cli.exe ffdec-cli
		HINTS
			"${FFDEC_ROOT}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/FFDec"
			"${CMAKE_CURRENT_SOURCE_DIR}/../Reference/SkyUI-Community/tools/FFDec"
	)
	if(NOT FFDEC_CLI)
		message(FATAL_ERROR "FFDec CLI was not found. Set FFDEC_ROOT to its directory.")
	endif()

	# SkyUI's importer expects these metadata values while staging AS2 sources.
	set(PROJECT_RELEASE_IDX 0)
	set(PROJECT_VERSION_STRING "${PROJECT_VERSION}")
	set(VERSION_METADATA_TARGETS "")
	set(ENABLE_FPS_PATCH OFF)
	set(AS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/actionscript")
	set(AS_TARGETS "")
	set(SWF_COMPILED_OUTPUTS "")
	AS_GlobalAssemble_Init(AS_SOURCE_DIR "${AS_SOURCE_DIR}")

	include("${CMAKE_CURRENT_SOURCE_DIR}/src/ui/swfsources.cmake")
	AS_GlobalAssemble_Finalize()

	add_custom_target(ActionScript DEPENDS ${AS_TARGETS})

	add_custom_target(deploy_interface ALL
		COMMAND "${CMAKE_COMMAND}" -E make_directory
			"${NAVBAR_DEPLOY_DIR}/Interface"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"${NAVBAR_SWF_OUTPUT}"
			"${NAVBAR_DEPLOY_DIR}/Interface/NavBar.swf"
		COMMENT "Deploying SkyUI navbar interface"
		VERBATIM
	)
	add_dependencies(deploy_interface ActionScript deploy_config deploy_translations)
	if(BUILD_TESTING AND NAVBAR_NODE)
		add_test(NAME NavbarCompiledAssets COMMAND "${NAVBAR_NODE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tests/SwfAssetTests.cjs" "${FFDEC_CLI}"
			"${CMAKE_CURRENT_BINARY_DIR}/interface/base/NavBarForSkyUI/NavBar.swf" "${NAVBAR_SWF_OUTPUT}")
	endif()
endif()

if(NAVBAR_BUILD_PLUGIN AND NAVBAR_BUILD_INTERFACE)
	include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Release.cmake")
endif()
