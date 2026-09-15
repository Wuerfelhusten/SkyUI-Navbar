# Adapted from SkyUI-Community's source/swfsources.cmake. The original macro
# rebuilds an XML-exported SWF and then injects its ActionScript sources in an
# isolated FFDec staging directory.
macro(add_navbar_swf TARGET_NAME SWF_REL XML_PATH)
	set(_sources ${ARGN})
	set(_base_swf "${CMAKE_CURRENT_BINARY_DIR}/interface/base/${SWF_REL}")
	set(_final_swf "${CMAKE_CURRENT_BINARY_DIR}/interface/${SWF_REL}")

	Add_XML_Base(
		OUTPUT_SWF "${_base_swf}"
		XML_PATH "${XML_PATH}"
	)

	set(_icon_swf "${CMAKE_CURRENT_BINARY_DIR}/interface/base/${TARGET_NAME}-icons.swf")
	set(_background_svg "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/assets/navbar-background.svg")
	set(_menu_icons "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/assets/icons")
	add_custom_command(OUTPUT "${_icon_swf}"
		COMMAND "${FFDEC_CLI}" -replace "${_base_swf}" "${_icon_swf}"
			78 "${_background_svg}" nofill
			82 "${_menu_icons}/inventory.svg" nofill 84 "${_menu_icons}/map.svg" nofill
			86 "${_menu_icons}/skills.svg" nofill 88 "${_menu_icons}/magic.svg" nofill
			90 "${_menu_icons}/bestiary.svg" nofill 92 "${_menu_icons}/achievements.svg" nofill
			94 "${_menu_icons}/wait.svg" nofill 96 "${_menu_icons}/custom-skills.svg" nofill
			100 "${_menu_icons}/character.svg" nofill
		DEPENDS "${_base_swf}" "${_background_svg}"
			"${_menu_icons}/inventory.svg" "${_menu_icons}/map.svg"
			"${_menu_icons}/skills.svg" "${_menu_icons}/magic.svg"
			"${_menu_icons}/bestiary.svg" "${_menu_icons}/achievements.svg"
			"${_menu_icons}/wait.svg" "${_menu_icons}/custom-skills.svg"
			"${_menu_icons}/character.svg"
		COMMENT "Importing background and menu-specific vector icons with FFDec" VERBATIM)

	Add_AS(
		TARGET_NAME "AS_${TARGET_NAME}"
		SWF_REL "${SWF_REL}"
		SWF_INPUT "${_icon_swf}"
		SWF_OUTPUT "${_final_swf}"
		SOURCES ${_sources}
	)

	list(APPEND AS_TARGETS "AS_${TARGET_NAME}")
	list(APPEND SWF_COMPILED_OUTPUTS "${_final_swf}")
	set(NAVBAR_SWF_OUTPUT "${_final_swf}")
endmacro()

add_navbar_swf(navbar
	"NavBarForSkyUI/NavBar.swf"
	"navbar/navbar.xml"
	"NavBar/NavbarWidget.as"
)
