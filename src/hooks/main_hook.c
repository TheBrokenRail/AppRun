#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "exec_utils.h"

/* Trampoline for the real main() */
static int (*apprun_main_orig)(int, char **, char **);


/* Update working directory */
__attribute__((constructor)) static void apprun_update_working_directory() {
    apprun_restore_workdir_if_needed();
}


/* Wrapper for main() that gets called by __libc_start_main() */
int apprun_main_hook(int argc, char **argv, char **envp) {
#ifdef DEBUG
    char exec_path[PATH_MAX] = {0x0};
    realpath("/proc/self/exe", exec_path);

    fprintf(stderr, "APPRUN_HOOK_DEBUG: --- Before main %s ---\n", exec_path);
#endif

    int ret = apprun_main_orig(argc, argv, envp);

#ifdef DEBUG
    fprintf(stderr, "APPRUN_HOOK_DEBUG: --- After main ----\n");
    fprintf(stderr, "APPRUN_HOOK_DEBUG: main() returned %d\n", ret);
    fflush(stderr);
#endif

    return ret;
}


/*
 * Wrapper for __libc_start_main() that replaces the real main
 * function with our hooked version.
 */
int __libc_start_main(
        int (*main)(int, char **, char **),
        int argc,
        char **argv,
        int (*init)(int, char **, char **),
        void (*fini)(void),
        void (*rtld_fini)(void),
        void *stack_end) {
    /* Save the real main function address */
    apprun_main_orig = main;

    /* Find the real __libc_start_main()... */
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    /* ... and call it with our custom main function */
    return orig(apprun_main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}