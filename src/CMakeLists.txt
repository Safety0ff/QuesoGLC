set (QUESOGLC_SOURCES
  context.c
  except.c
  font.c
  global.c
  master.c
  measure.c
  misc.c
  oarray.c
  ofacedesc.c
  ofont.c
  oglyph.c
  render.c
  scalable.c
  texture.c
  transform.c
  unicode.c
)

if (WIN32)
  set (QUESOGLC_SOURCES ${QUESOGLC_SOURCES} win32/ocharmap.c win32/ocontext.c win32/omaster.c)
else ()
  set (QUESOGLC_SOURCES ${QUESOGLC_SOURCES} ocharmap.c ocontext.c omaster.c)
endif ()

add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/database.c
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/database/buildDB.py
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/database/buildDB.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_source_files_properties ("${CMAKE_CURRENT_BINARY_DIR}/database.c" PROPERTIES GENERATED 1)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}) # because we're putting database.c in the binary dir.
set (QUESOGLC_SOURCES ${QUESOGLC_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/database.c)

set_source_files_properties (${QUESOGLC_SOURCES} PROPERTIES COMPILE_FLAGS "${HIDE_SYMBOLS_FLAGS}")

add_library (GLC ${QUESOGLC_SOURCES})

target_link_libraries (GLC
  ${FREETYPE_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${FRIBIDI_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${OPENGL_gl_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
)
set_target_properties(GLC PROPERTIES COMPILE_FLAGS "-D_REENTRANT")

install (TARGETS GLC
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install (DIRECTORY ${CMAKE_SOURCE_DIR}/include/GL DESTINATION include)
