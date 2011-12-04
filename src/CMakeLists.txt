set (QUESOGLC_SOURCES
  context.c
  database.c
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

set_source_files_properties (${QUESOGLC_SOURCES} PROPERTIES COMPILE_FLAGS "${HIDE_SYMBOLS_FLAGS}")

add_library (quesoglc SHARED ${QUESOGLC_SOURCES})
target_link_libraries (quesoglc
  ${FREETYPE_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${FRIBIDI_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${OPENGL_gl_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
)
set_target_properties(quesoglc PROPERTIES COMPILE_FLAGS "-D_REENTRANT")

if (ENABLE_STATIC)
  add_library (quesoglc_static STATIC ${QUESOGLC_SOURCES})
  target_link_libraries (quesoglc_static
    ${FREETYPE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FRIBIDI_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
  )
  set_target_properties(quesoglc_static PROPERTIES COMPILE_FLAGS "-D_REENTRANT")
endif ()