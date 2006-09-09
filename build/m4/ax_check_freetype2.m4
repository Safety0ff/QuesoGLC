AC_DEFUN([AX_CHECK_FREETYPE2], [
  PKG_CHECK_MODULES([FREETYPE2], [freetype2 >= 9.8] , ,[

  if test -z "$FREETYPE_CONFIG"; then
    AC_PATH_PROG(FREETYPE_CONFIG, freetype-config, no)
  fi

  if test "$FREETYPE_CONFIG" = "no" ; then
     no_ft2="yes"
  else
    ax_save_CPPFLAGS="${CPPFLAGS}"
    CPPFLAGS="`freetype-config --cflags`"
    AC_CHECK_LIB(freetype, FT_Init_FreeType,
                 [LIBS="`freetype-config --libs` $LIBS";
                  CPPFLAGS="`freetype-config --cflags` ${ax_save_CPPFLAGS}"],
                 no_ft2="yes"; CPPFLAGS=${ax_save_CPPFLAGS}
                )
  fi
  ])
])
