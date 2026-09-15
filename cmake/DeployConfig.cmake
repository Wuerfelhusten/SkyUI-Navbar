if(NOT IS_ABSOLUTE "${ROOT}" OR NOT IS_ABSOLUTE "${BACKUP_DIR}")
	message(FATAL_ERROR "Deploy root and backup directory must be absolute")
endif()
file(MAKE_DIRECTORY "${ROOT}/SKSE/Plugins")
file(REAL_PATH "${ROOT}" ROOT)
file(MAKE_DIRECTORY "${BACKUP_DIR}")
file(REAL_PATH "${BACKUP_DIR}" BACKUP_DIR)
cmake_path(IS_PREFIX ROOT "${BACKUP_DIR}" NORMALIZE BACKUP_INSIDE_MOD)
if(BACKUP_INSIDE_MOD)
	message(FATAL_ERROR "Legacy backups must be outside the deployed mod")
endif()
string(TIMESTAMP BACKUP_STAMP "%Y%m%d-%H%M%S" UTC)
set(BACKUP_ROOT "${BACKUP_DIR}/${BACKUP_STAMP}")

function(backup_file RELATIVE_FILE)
	get_filename_component(PARENT "${BACKUP_ROOT}/${RELATIVE_FILE}" DIRECTORY)
	file(MAKE_DIRECTORY "${PARENT}")
	if(NOT EXISTS "${BACKUP_ROOT}/${RELATIVE_FILE}")
		file(COPY_FILE "${ROOT}/${RELATIVE_FILE}" "${BACKUP_ROOT}/${RELATIVE_FILE}")
	endif()
endfunction()

set(CONFIG_REL "SKSE/Plugins/SkyUINavbar.json")
set(CONFIG "${ROOT}/${CONFIG_REL}")
if(NOT EXISTS "${CONFIG}")
	file(COPY_FILE "${SOURCE}" "${CONFIG}")
endif()

# Exact obsolete navbar artifacts only; no recursive directory cleanup.
foreach(LEGACY IN ITEMS
	"Interface/NavBarForSkyUI/NavBar.swf"
	"SKSE/Plugins/SkyUINavbar.default.json"
	"SKSE/Plugins/SkyUINavbar.ui.json"
	"SKSE/Plugins/SkyUINavbar.tutorial.json"
	"SKSE/Plugins/SkyUINavbar.ui.default.json")
	if(EXISTS "${ROOT}/${LEGACY}")
		backup_file("${LEGACY}")
		file(REMOVE "${ROOT}/${LEGACY}")
		message(STATUS "Removed obsolete ${LEGACY}; backup: ${BACKUP_ROOT}")
	endif()
endforeach()
