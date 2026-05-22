#define _GNU_SOURCE
#include "cli.h"
#include "../../include/tinydb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <time.h>

int cli_init(CLI** cli_out) {
    CLI* cli = calloc(1, sizeof(CLI));
    if (cli == NULL) {
        return -1;
    }

    cli->socket_fd = -1;
    cli->connected = false;
    cli->mode = OUTPUT_MODE_BOX;
    cli->show_headers = true;
    cli->show_timer = false;
    cli->null_string = "";
    cli->pager_enabled = true;
    cli->pager_cmd = "less -R";
    cli->verbose = false;
    cli->quiet = false;
    cli->executed_command = false;

    *cli_out = cli;
    return 0;
}

void cli_shutdown(CLI* cli) {
    if (cli == NULL) return;

    if (cli->connected || cli->socket_fd >= 0) {
        cli_disconnect(cli);
    }

    free(cli);
}

int cli_connect(CLI* cli, const char* socket_path) {
    if (cli == NULL) return -1;

    if (socket_path == NULL) {
        socket_path = CLI_DEFAULT_SOCKET;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    cli->socket_fd = fd;
    cli->connected = true;

    return 0;
}

void cli_disconnect(CLI* cli) {
    if (cli == NULL) return;

    if (cli->socket_fd >= 0) {
        close(cli->socket_fd);
        cli->socket_fd = -1;
    }

    cli->connected = false;
}

int cli_execute_sql(CLI* cli, const char* sql) {
    if (cli == NULL || sql == NULL) return -1;

    if (!cli->connected) {
        if (cli_connect(cli, NULL) != 0) {
            fprintf(stderr, "Error: not connected to server\n");
            return -1;
        }
    }

    char buf[CLI_MAX_LINE + 64];
    snprintf(buf, sizeof(buf), "QUERY:%s\n", sql);

    ssize_t written = write(cli->socket_fd, buf, strlen(buf));
    if (written != (ssize_t)strlen(buf)) {
        return -1;
    }

    char resp[4096];
    size_t resp_len = 0;
    ssize_t n;

    // Read until we get the full response (ends with newline or END marker)
    while ((n = read(cli->socket_fd, resp + resp_len, sizeof(resp) - resp_len - 1)) > 0) {
        resp_len += n;
        resp[resp_len] = '\0';
        // Check for END marker to know when data is complete
        if (resp_len > 3 && strstr(resp, "\nEND\n") != NULL) {
            break;
        }
        // Also break on regular OK response without row data
        if (resp_len > 0 && resp[resp_len - 1] == '\n' &&
            strstr(resp, "row(s)") != NULL && strstr(resp, "ROW:") == NULL) {
            break;
        }
    }

    if (resp_len <= 0) {
        fprintf(stderr, "Error: no response from server\n");
        return -1;
    }

    // Remove trailing newlines only (preserve END marker)
    // Don't trim past "\nEND\n" so that strstr(resp, "\nEND\n") still works
    while (resp_len > 0 && (resp[resp_len - 1] == '\n' || resp[resp_len - 1] == '\r')) {
        if (resp_len >= 5 && strncmp(resp + resp_len - 4, "END\n", 4) == 0) {
            // Stop trimming - we need the final \n before END
            break;
        }
        resp_len--;
    }
    resp[resp_len] = '\0';

    // Parse response type
    if (strncmp(resp, "ERROR", 5) == 0) {
        fprintf(stderr, "Error: %s\n", resp + 6);
        return -1;
    }

    if (strncmp(resp, "OK", 2) == 0 || strncmp(resp, "Query OK", 8) == 0) {
        // Check if this is a SELECT (has "returned") or DML (has "affected")
        char* returned_part = strstr(resp, "returned");
        char* affected_part = strstr(resp, "affected");

        if (returned_part != NULL) {
            // SELECT query - parse row count from "OK N row(s) returned"
            int rows = 0;
            sscanf(resp + 3, "%d", &rows);
            printf("Query OK, %d row(s) returned\n", rows);

            /* Extract and display row data from ROW: lines */
            /* Protocol: OK N row(s) returned\n, ROW: lines, \nEND\n */
            /* Find the OK line and END marker */
            char* ok_line = strstr(resp, "OK ");
            char* end_marker = strstr(resp, "\nEND\n");

            if (ok_line && end_marker) {
                /* Terminate at end marker */
                *end_marker = '\0';

                /* Find the newline after OK line to get to row data */
                char* line_start = strchr(ok_line, '\n');
                if (line_start) {
                    line_start++;  /* Skip the newline */
                } else {
                    /* No newline after OK line - find end of OK line */
                    line_start = ok_line;
                    while (*line_start && *line_start != '\n') line_start++;
                    if (*line_start == '\n') line_start++;
                }

                /* Process each ROW: line */
                char* line = line_start;
                while (line && *line) {
                    char* next_line = strchr(line, '\n');
                    if (next_line) *next_line = '\0';

                    /* Parse ROW:col1\tcol2\tcol3 format */
                    if (strncmp(line, "ROW:", 4) == 0) {
                        char* cols = line + 4;
                        /* Replace tabs with spaces for display */
                        char* p = cols;
                        while (*p) {
                            if (*p == '\t') *p = ' ';
                            p++;
                        }
                        printf("%s\n", cols);
                    }
                    line = next_line ? next_line + 1 : NULL;
                }
            }
            return rows;
        } else if (affected_part != NULL) {
            // DML query (INSERT/UPDATE/DELETE)
            int affected = 0;
            char* p = resp;
            while (*p && (*p < '0' || *p > '9')) p++;
            sscanf(p, "%d", &affected);
            cli_display_ok(affected);
            return affected;
        } else {
            // Empty OK response
            printf("Query OK\n");
            return 0;
        }
    }

    fprintf(stderr, "Error: unknown response format\n");
    return -1;
}

int cli_execute_file(CLI* cli, const char* filepath) {
    if (cli == NULL || filepath == NULL) return -1;

    FILE* f = fopen(filepath, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open file %s\n", filepath);
        return -1;
    }

    char line[CLI_MAX_LINE];
    int count = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0 || line[0] == '#') {
            continue;
        }

        if (line[0] == '.') {
            cli_handle_meta_command(cli, line);
        } else {
            if (cli_execute_sql(cli, line) == 0) {
                count++;
            }
        }
    }

    fclose(f);
    return count;
}

int cli_single_query(CLI* cli, const char* sql) {
    if (cli == NULL || sql == NULL) return -1;

    if (!cli->connected) {
        if (cli_connect(cli, NULL) != 0) {
            return -1;
        }
    }

    return cli_execute_sql(cli, sql);
}

int cli_batch_mode(CLI* cli, const char* filepath) {
    return cli_execute_file(cli, filepath);
}

int cli_interactive_loop(CLI* cli) {
    if (cli == NULL) return -1;

    if (!cli->connected) {
        const char* socket_path = getenv("TINYDB_SOCKET_PATH");
        if (cli_connect(cli, socket_path) != 0) {
            fprintf(stderr, "Warning: could not connect to %s\n",
                    socket_path ? socket_path : CLI_DEFAULT_SOCKET);
        }
    }

    char line[CLI_MAX_LINE];
    int multiline = 0;
    char sql_buf[CLI_MAX_LINE * 2];
    size_t sql_len = 0;

    while (1) {
        printf("%s", multiline ? CLI_MULTILINE_PROMPT : CLI_PROMPT);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            len--;
        }

        if (len == 0) {
            multiline = 0;
            sql_len = 0;
            continue;
        }

        if (line[0] == '.' && !multiline) {
            if (cli_handle_meta_command(cli, line) != 0) {
                break;
            }
            continue;
        }

        if (sql_len + len + 1 >= sizeof(sql_buf)) {
            fprintf(stderr, "SQL statement too long\n");
            sql_len = 0;
            multiline = 0;
            continue;
        }

        if (sql_len > 0) {
            sql_buf[sql_len++] = ' ';
        }
        memcpy(sql_buf + sql_len, line, len);
        sql_len += len;
        sql_buf[sql_len] = '\0';

        if (line[len-1] == ';') {
            cli_execute_sql(cli, sql_buf);
            sql_len = 0;
            multiline = 0;
        } else {
            multiline = 1;
        }
    }

    return 0;
}

void cli_print_help(void) {
    printf("TinyDB CLI v2.0.0 - Available commands:\n");
    printf("\nMeta-commands:\n");
    printf("  .help           Show this help\n");
    printf("  .quit            Exit the CLI\n");
    printf("  .exit            Exit the CLI\n");
    printf("  .tables          List all tables\n");
    printf("  .schema <table>  Show CREATE TABLE statement\n");
    printf("  .indexes <table> List indexes on a table\n");
    printf("  .plan <sql>      Show query plan\n");
    printf("  .timer on/off    Enable/disable query timing\n");
    printf("  .mode <mode>     Output mode: box, csv, line, list\n");
    printf("  .headers on/off  Show/hide column headers\n");
    printf("  .null <string>   String to display for NULL values\n");
    printf("  .pager <cmd>     Set pager command\n");
    printf("  .shell <cmd>     Execute shell command\n");
    printf("  .read <file>     Read and execute SQL from file\n");
    printf("  .trace on/off    Enable server trace\n");
}

void cli_print_tables(CLI* cli) {
    (void)cli;
    printf("Tables: (use .tables to list)\n");
}

void cli_print_schema(CLI* cli, const char* table) {
    (void)cli;
    (void)table;
    printf("Schema: (not yet implemented)\n");
}

void cli_print_indexes(CLI* cli, const char* table) {
    (void)cli;
    (void)table;
    printf("Indexes: (not yet implemented)\n");
}

void cli_set_output_mode(CLI* cli, OutputMode mode) {
    if (cli != NULL) {
        cli->mode = mode;
    }
}

void cli_set_headers(CLI* cli, bool show) {
    if (cli != NULL) {
        cli->show_headers = show;
    }
}

void cli_set_timer(CLI* cli, bool show) {
    if (cli != NULL) {
        cli->show_timer = show;
    }
}

void cli_set_null_string(CLI* cli, const char* s) {
    if (cli != NULL && s != NULL) {
        cli->null_string = s;
    }
}

void cli_set_pager(CLI* cli, const char* cmd) {
    if (cli != NULL && cmd != NULL) {
        cli->pager_cmd = cmd;
        cli->pager_enabled = (strcmp(cmd, "cat") != 0);
    }
}

void cli_set_verbose(CLI* cli, bool verbose) {
    if (cli != NULL) {
        cli->verbose = verbose;
    }
}

const char* cli_get_output_mode_name(OutputMode mode) {
    switch (mode) {
        case OUTPUT_MODE_BOX: return "box";
        case OUTPUT_MODE_CSV: return "csv";
        case OUTPUT_MODE_LINE: return "line";
        case OUTPUT_MODE_LIST: return "list";
        default: return "unknown";
    }
}

OutputMode cli_parse_output_mode(const char* mode) {
    if (mode == NULL) return OUTPUT_MODE_BOX;

    if (strcmp(mode, "box") == 0) return OUTPUT_MODE_BOX;
    if (strcmp(mode, "csv") == 0) return OUTPUT_MODE_CSV;
    if (strcmp(mode, "line") == 0) return OUTPUT_MODE_LINE;
    if (strcmp(mode, "list") == 0) return OUTPUT_MODE_LIST;

    return OUTPUT_MODE_BOX;
}

void cli_display_result(CLI* cli, int rows, int cols, char** data, char** headers) {
    (void)cli;
    (void)rows;
    (void)cols;
    (void)data;
    (void)headers;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (j > 0) printf("\t");
            printf("%s", data[i * cols + j] ? data[i * cols + j] : "");
        }
        printf("\n");
    }
}

void cli_display_error(const char* error) {
    if (error != NULL) {
        fprintf(stderr, "ERROR: %s\n", error);
    }
}

void cli_display_ok(int changes) {
    printf("OK: %d row(s) affected\n", changes);
}

int cli_handle_meta_command(CLI* cli, const char* cmd) {
    if (cli == NULL || cmd == NULL) return -1;

    if (strcmp(cmd, ".quit") == 0 || strcmp(cmd, ".exit") == 0) {
        return 1;
    }

    if (strcmp(cmd, ".help") == 0) {
        cli_print_help();
        return 0;
    }

    if (strcmp(cmd, ".tables") == 0) {
        cli_print_tables(cli);
        return 0;
    }

    if (strncmp(cmd, ".schema", 7) == 0) {
        char table[64] = {0};
        sscanf(cmd + 7, "%63s", table);
        cli_print_schema(cli, table);
        return 0;
    }

    if (strncmp(cmd, ".indexes", 8) == 0) {
        char table[64] = {0};
        sscanf(cmd + 8, "%63s", table);
        cli_print_indexes(cli, table);
        return 0;
    }

    if (strcmp(cmd, ".timer on") == 0) {
        cli_set_timer(cli, true);
        printf("Timer enabled.\n");
        return 0;
    }

    if (strcmp(cmd, ".timer off") == 0) {
        cli_set_timer(cli, false);
        printf("Timer disabled.\n");
        return 0;
    }

    if (strncmp(cmd, ".mode", 5) == 0) {
        char mode[16] = {0};
        sscanf(cmd + 5, "%15s", mode);
        cli_set_output_mode(cli, cli_parse_output_mode(mode));
        printf("Output mode set to %s.\n", cli_get_output_mode_name(cli->mode));
        return 0;
    }

    if (strcmp(cmd, ".headers on") == 0) {
        cli_set_headers(cli, true);
        return 0;
    }

    if (strcmp(cmd, ".headers off") == 0) {
        cli_set_headers(cli, false);
        return 0;
    }

    if (strncmp(cmd, ".null", 5) == 0) {
        char null_str[32] = {0};
        sscanf(cmd + 5, "%31s", null_str);
        cli_set_null_string(cli, null_str);
        return 0;
    }

    if (strncmp(cmd, ".pager", 6) == 0) {
        char cmd_str[128] = {0};
        sscanf(cmd + 6, "%127s", cmd_str);
        cli_set_pager(cli, cmd_str);
        return 0;
    }

    if (strncmp(cmd, ".shell", 6) == 0) {
        /* Validate shell command - only allow safe characters */
        const char* arg = cmd + 6;
        while (*arg == ' ') arg++;  /* skip leading spaces */
        size_t arg_len = strlen(arg);
        if (arg_len == 0 || arg_len > 255) {
            printf("Error: invalid shell command\n");
            return 0;
        }
        /* Check for dangerous characters */
        for (size_t i = 0; i < arg_len; i++) {
            char c = arg[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == ' ' || c == '-' ||
                  c == '_' || c == '.' || c == '/' || c == ':')) {
                printf("Error: invalid characters in shell command\n");
                return 0;
            }
        }
        char shell_cmd[256] = {0};
        snprintf(shell_cmd, sizeof(shell_cmd), "%s", arg);
        system(shell_cmd);
        return 0;
    }

    if (strncmp(cmd, ".read", 5) == 0) {
        const char* arg = cmd + 5;
        while (*arg == ' ') arg++;  /* skip leading spaces */
        /* Validate path - prevent path traversal */
        if (strstr(arg, "..") != NULL) {
            printf("Error: invalid characters in path\n");
            return 0;
        }
        cli_execute_file(cli, arg);
        return 0;
    }

    printf("Unknown command: %s\n", cmd);
    return 0;
}

int cli_parse_args(CLI* cli, int argc, char** argv) {
    if (cli == NULL || argc < 0 || argv == NULL) {
        return -1;
    }

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: tinydb-cli [OPTIONS] [SQL]\n");
            printf("Options:\n");
            printf("  -h, --help             Show help\n");
            printf("  -v, --version          Show version\n");
            printf("  -s, --socket <path>    Connect to socket\n");
            printf("  -c, --command <sql>    Execute SQL command\n");
            printf("  -f, --file <file>      Execute SQL from file\n");
            printf("  -n, --no-pager         Disable pager\n");
            printf("  -t, --stats            Show query statistics\n");
            printf("  -V, --verbose          Verbose output\n");
            printf("  -q, --quiet            Quiet mode\n");
            return -1;
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("TinyDB CLI v2.0.0\n");
            return -1;
        }

        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--socket") == 0) {
            if (i + 1 < argc) {
                cli_connect(cli, argv[++i]);
            }
            i++;
            continue;
        }

        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--command") == 0) {
            if (i + 1 < argc) {
                cli->executed_command = true;
                cli_single_query(cli, argv[++i]);
                return -1;  // Return -1 to indicate exit after command
            }
            i++;
            continue;
        }

        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
            if (i + 1 < argc) {
                cli->executed_command = true;
                cli_batch_mode(cli, argv[++i]);
            }
            i++;
            continue;
        }

        if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-pager") == 0) {
            cli_set_pager(cli, "cat");
            i++;
            continue;
        }

        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--stats") == 0) {
            cli_set_timer(cli, true);
            i++;
            continue;
        }

        if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--verbose") == 0) {
            cli_set_verbose(cli, true);
            i++;
            continue;
        }

        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            cli->quiet = true;
            i++;
            continue;
        }

        i++;
    }

    return 0;
}