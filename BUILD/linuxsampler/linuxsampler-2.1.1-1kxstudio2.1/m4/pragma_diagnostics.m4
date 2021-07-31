dnl Christian Schoenebeck  2016-10-31
dnl --------------------------------------------------------------------------
dnl
dnl Synopsis:
dnl
dnl    ACX_CXX_PRAGMA_DIAGNOSTICS
dnl
dnl Check the compiler for supported pragma diagnostics capablities.
dnl It takes no arguments.
dnl
dnl At the moment this configure check only checks whether the compiler
dnl supports embedding pragma diagnostic statements between parts of
dnl statements.
dnl
dnl --------------------------------------------------------------------------
AC_DEFUN([ACX_CXX_PRAGMA_DIAGNOSTICS],
[

  AC_MSG_CHECKING(for compiler's pragma diagnostics)

  AC_LANG_PUSH([C++])

  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[

#include <stdlib.h>

int foo(int a, int b) {
    int i =
        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"")
        (a + b)
        _Pragma("GCC diagnostic pop")
        ;
    return i;
}

  ]])],[
    AC_MSG_RESULT(yes)
    have_embedded_pragma_diag=1
  ], [
    AC_MSG_RESULT(no)
    have_embedded_pragma_diag=0
  ])
  AC_DEFINE_UNQUOTED(HAVE_CXX_EMBEDDED_PRAGMA_DIAGNOSTICS,$have_embedded_pragma_diag,[Define to 1 if your compiler supports embedded pragma diagnostics.])

  AC_LANG_POP([C++])
])
