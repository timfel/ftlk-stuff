AC_INIT([[flPhase6]], [[0.1]], [[timfelgentreff@gmail.com]])

# Hier werden die Abfragen gemacht und die Werte festgelegt,
# die in die Dateien des Projektes eingetragen werden sollen. 

AC_PROG_CXX
AC_PATH_PROG([FLTK_CONFIG],[fltk-utf8-config])
AC_SUBST(FLTK_CXX)
AC_SUBST(FLTK_LD)
FLTK_CXX=`"$FLTK_CONFIG" --use-images --cxxflags`
FLTK_LD=`"$FLTK_CONFIG" --use-images --ldstaticflags  | tr -d '\n'`

AC_ARG_VAR([FLTK_CONFIG], [fltk-config path])
AC_ARG_VAR([FLTK_CXX], [C++ flags of fltk])
AC_ARG_VAR([FLTK_LD], [linker flags of fltk])

AC_ARG_WITH(debug,[  --with-debug            turn debugging code on],[mode=debug],[mode=comp])
AC_ARG_WITH(macx, [  --with-macx			 turn MacOS X specific code on],[mac=mac],[mac=no])

case "$mode" in
	debug)
		AC_DEFINE(DEBUG)
		CXXFLAGS=-Wall;;
	comp)
		CXXFLAGS=-O2;;
esac

case "$mac" in
	mac)
		AC_DEFINE(MACX);;
	comp)
		AC_DEFINE(LINUX);;
esac

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
