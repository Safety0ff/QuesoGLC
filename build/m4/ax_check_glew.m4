AC_DEFUN([AX_CHECK_GLEW], [
  AC_CHECK_HEADER([GL/glew.h])
  if (test "x$enable_glew_multiple_contexts" = "xyes"); then
    AC_CHECK_LIB(GLEW, glewContextInit, , no_glew="yes")
  else
    AC_CHECK_LIB(GLEW, glewInit, , no_glew="yes")
  fi

  if test -z "$no_glew"; then
    if (test "x$enable_glew_multiple_contexts" = "xyes"); then
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <GL/glew.h>
#if !defined(GL_SGIS_texture_lod) || !defined(GL_ARB_vertex_buffer_object) || !defined(GL_ARB_pixel_buffer_object)
# error
#endif
                                    ]],
                                    [[glewContextInit()]])],
                   [LIBS="-lGLEW $LIBS"])
    else
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <GL/glew.h>
#if !defined(GL_SGIS_texture_lod) || !defined(GL_ARB_vertex_buffer_object) || !defined(GL_ARB_pixel_buffer_object)
# error
#endif
                                    ]],
                                    [[glewInit()]])],
                   [LIBS="-lGLEW $LIBS"])
    fi
  fi
])
