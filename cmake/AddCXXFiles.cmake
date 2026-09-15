function(add_cxx_files TARGET)
	file(GLOB_RECURSE INCLUDE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/skse/*.h"
		"src/skse/*.hpp"
		"src/skse/*.hxx"
		"src/skse/*.inl"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/skse
		PREFIX "Header Files"
		FILES ${INCLUDE_FILES})

	target_sources("${TARGET}" PUBLIC ${INCLUDE_FILES})

	file(GLOB_RECURSE SOURCE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/skse/*.cpp"
		"src/skse/*.cxx"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/skse
		PREFIX "Source Files"
		FILES ${SOURCE_FILES})

	target_sources("${TARGET}" PRIVATE ${SOURCE_FILES})
endfunction()
