#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#define _GNU_SOURCE
#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    CLI* cli = NULL;

    if (cli_init(&cli) != 0) {
        fprintf(stderr, "Failed to initialize CLI\n");
        return 1;
    }

    if (cli_parse_args(cli, argc, argv) != 0) {
        cli_shutdown(cli);
        return 0;
    }

    /* Only show banner in interactive mode, not when executing commands */
    if (!cli->quiet && !cli->executed_command && isatty(fileno(stdin))) {
        printf("TinyDB CLI v2.0.0\n");
        printf("Type \".help\" for help.\n\n");
    }

    if (isatty(fileno(stdin))) {
        cli_interactive_loop(cli);
    } else {
        char line[4096];
        while (fgets(line, sizeof(line), stdin) != NULL) {
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
                line[--len] = '\0';
            }

            if (len == 0) continue;

            if (line[0] == '.') {
                cli_handle_meta_command(cli, line);
            } else {
                cli_single_query(cli, line);
            }
        }
    }

    cli_shutdown(cli);
    return 0;
}