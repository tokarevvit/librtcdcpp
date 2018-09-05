if (NOT TARGET LibNice::LibNice)
    set(LIBNICE_DEFINITIONS ${PC_LIBNICE_CFLAGS_OTHER})

	find_path(LIBNICE_INCLUDE nice/agent.h
		    HINTS ${NICE_INCLUDE_DIR}
            PATH_SUFFICES libnice)
		find_library(LIBNICE_LIBRARY NAMES nice libnice HINTS ${NICE_LIB_DIR})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Libnice DEFAULT_MSG
		    LIBNICE_LIBRARY LIBNICE_INCLUDE)
		mark_as_advanced(LIBNICE_INCLUDE LIBNICE_LIBRARY)

    set(LIBNICE_LIBRARIES ${LIBNICE_LIBRARY})
	set(LIBNICE_INCLUDES ${LIBNICE_INCLUDE})

    find_package(GLIB REQUIRED COMPONENTS gio gobject gmodule gthread)

	list(APPEND LIBNICE_INCLUDES ${GLIB_INCLUDE_DIRS})
    list(APPEND LIBNICE_LIBRARIES ${GLIB_GOBJECT_LIBRARIES} ${GLIB_LIBRARIES})

    if (LIBNICE_FOUND)
        add_library(LibNice::LibNice UNKNOWN IMPORTED)
        set_target_properties(LibNice::LibNice PROPERTIES
                IMPORTED_LOCATION "${LIBNICE_LIBRARY}"
                INTERFACE_COMPILE_DEFINITIONS "_REENTRANT"
				INTERFACE_INCLUDE_DIRECTORIES "${LIBNICE_INCLUDES}"
                INTERFACE_LINK_LIBRARIES "${LIBNICE_LIBRARIES}"
                IMPORTED_LINK_INTERFACE_LANGUAGES "C")
    endif ()
endif ()
