find_program(SCDOC scdoc)
find_program(GZIP gzip)

if(SCDOC)
	message(STATUS "creation of manual pages")
	unset(DEPS)
	foreach(SECTION 1 3 5 7 8)

		file(GLOB MPGS LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.${SECTION}.scd)
		foreach(INSCD ${MPGS})
			if(GZIP)
				string(REPLACE ".scd" ".gz" OUTMAN ${INSCD})
				add_custom_command(
					OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTMAN}
					COMMAND ${SCDOC} < ${CMAKE_CURRENT_SOURCE_DIR}/${INSCD}
						| ${GZIP} > ${CMAKE_CURRENT_BINARY_DIR}/${OUTMAN}
					DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${INSCD})
			else(GZIP)
				string(REPLACE ".scd" "" OUTMAN ${INSCD})
				add_custom_command(
					OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTMAN}
					COMMAND ${SCDOC} < ${CMAKE_CURRENT_SOURCE_DIR}/${INSCD}
						> ${CMAKE_CURRENT_BINARY_DIR}/${OUTMAN}
					DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${INSCD})
			endif(GZIP)
			install(
				FILES ${CMAKE_CURRENT_BINARY_DIR}/${OUTMAN}
				DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man${SECTION})
			list(APPEND DEPS ${CMAKE_CURRENT_BINARY_DIR}/${OUTMAN})
		endforeach(INSCD)
	endforeach(SECTION)
	if(DEPS)
		add_custom_target(manuals ALL DEPENDS ${DEPS})
	endif(DEPS)
else(SCDOC)
	message(STATUS "can't create manual pages")
endif(SCDOC)
