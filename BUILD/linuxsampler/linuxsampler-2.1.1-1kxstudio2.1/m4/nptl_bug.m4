dnl Christian Schoenebeck  2007-10-18
dnl --------------------------------------------------------------------------
dnl
dnl Synopsis:
dnl
dnl    ACX_NPTL_GLIBC_BUG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Check for NPTL glibc bug which caused crashs on Gentoo systems, see
dnl Gentoo bug report #194076 for details. This test is ignored on cross-
dnl compilations. On Windows systems this test always succeeds.
dnl
dnl Note: this test requires LIBS, CFLAGS and CC already been set, i.e. by
dnl using ACX_PTHREAD before calling this macro.
dnl
dnl --------------------------------------------------------------------------
AC_DEFUN([ACX_NPTL_GLIBC_BUG],
[

  AC_MSG_CHECKING(for NPTL bug)

  AC_LANG_SAVE
  AC_LANG_C

  AC_TRY_RUN([

#include <stdlib.h>

#if WIN32

int main() {
    exit(0);
}

#else /* POSIX system */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_attr_t  __thread_attr;
pthread_t       __thread_id;

pthread_mutex_t     __posix_mutex;
pthread_mutexattr_t __posix_mutexattr;

/// entry point for the spawned thread
void* __pthread_launcher(void* p) {
    // let the thread be killable under any circumstances
    // (without this function call, this test always succeeds !)
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // this will block this 2nd thread, since we already
    // locked this mutex by the main thread
    pthread_mutex_lock(&__posix_mutex);

    // here we would access shared resources, etc.

    // just pro forma, the remainder of this function is actually not of
    // interest, since this thread is always terminated at its mutex lock call
    pthread_mutex_unlock(&__posix_mutex);
    return NULL;
}

int main() {

    // initialize mutex and thread attribute
    pthread_mutexattr_init(&__posix_mutexattr);
    pthread_mutex_init(&__posix_mutex, &__posix_mutexattr);
    pthread_attr_init(&__thread_attr);

    // already lock the mutex by the main thread ...
    pthread_mutex_lock(&__posix_mutex);

    int res;

    // create and run a 2nd thread
    res = pthread_create(&__thread_id, &__thread_attr, __pthread_launcher, NULL);
    if (res) {
        exit(-1);
    }

    // give the other thread a chance to spawn
    usleep(400000);

    // kill the other thread
    pthread_cancel(__thread_id);
    pthread_detach(__thread_id);

    // give the other thread a chance to finish its execution
    usleep(400000);

    // cleanup
    // (just pro forma, doesnt actually matter for this test case)
    pthread_attr_destroy(&__thread_attr);
    pthread_mutex_destroy(&__posix_mutex);
    pthread_mutexattr_destroy(&__posix_mutexattr);

    exit(0);
}
#endif /* WIN32 */

  ],[
    AC_MSG_RESULT(no)
    ifelse([$2], , :, [$2])
  ], [
    AC_MSG_RESULT(yes)
    ifelse([$1], , :, [$1])
  ], [
    AC_MSG_RESULT(disabled)
    ifelse([$2], , :, [$2])
  ])

  AC_LANG_RESTORE
])
