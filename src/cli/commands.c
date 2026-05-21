#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cli_execute_command(const char* cmd) {
    (void)cmd;
    return 0;
}

int cli_execute_query(CLI* cli, const char* sql) {
    return cli_execute_sql(cli, sql);
}

int cli_begin_transaction(CLI* cli) {
    return cli_execute_sql(cli, "BEGIN");
}

int cli_commit_transaction(CLI* cli) {
    return cli_execute_sql(cli, "COMMIT");
}

int cli_rollback_transaction(CLI* cli) {
    return cli_execute_sql(cli, "ROLLBACK");
}

void cli_format_result_box(int rows, int cols, char** data, char** headers) {
    if (rows == 0) {
        printf("No rows returned.\n");
        return;
    }

    int* widths = calloc(cols, sizeof(int));
    if (widths == NULL) return;

    for (int j = 0; j < cols; j++) {
        widths[j] = strlen(headers[j] ? headers[j] : "");
        for (int i = 0; i < rows; i++) {
            int len = strlen(data[i * cols + j] ? data[i * cols + j] : "");
            if (len > widths[j]) widths[j] = len;
        }
    }

    int total_width = 1;
    for (int j = 0; j < cols; j++) {
        total_width += widths[j] + 3;
    }

    printf("+");
    for (int j = 0; j < cols; j++) {
        for (int k = 0; k < widths[j] + 2; k++) printf("-");
        printf("+");
    }
    printf("\n");

    printf("|");
    for (int j = 0; j < cols; j++) {
        printf(" %-*s |", widths[j], headers[j] ? headers[j] : "");
    }
    printf("\n");

    printf("+");
    for (int j = 0; j < cols; j++) {
        for (int k = 0; k < widths[j] + 2; k++) printf("-");
        printf("+");
    }
    printf("\n");

    for (int i = 0; i < rows; i++) {
        printf("|");
        for (int j = 0; j < cols; j++) {
            printf(" %-*s |", widths[j], data[i * cols + j] ? data[i * cols + j] : "");
        }
        printf("\n");
    }

    printf("+");
    for (int j = 0; j < cols; j++) {
        for (int k = 0; k < widths[j] + 2; k++) printf("-");
        printf("+");
    }
    printf("\n");

    free(widths);
}

void cli_format_result_csv(int rows, int cols, char** data, char** headers) {
    (void)headers;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (j > 0) printf(",");
            const char* val = data[i * cols + j];
            if (val == NULL || *val == '\0') {
                printf("\"\"");
            } else {
                int needs_quotes = (strchr(val, ',') != NULL) ||
                                   (strchr(val, '"') != NULL) ||
                                   (strchr(val, '\n') != NULL);
                if (needs_quotes) {
                    printf("\"");
                    for (const char* p = val; *p; p++) {
                        if (*p == '"') printf("\"\"");
                        else printf("%c", *p);
                    }
                    printf("\"");
                } else {
                    printf("%s", val);
                }
            }
        }
        printf("\n");
    }
}

void cli_format_result_line(int rows, int cols, char** data, char** headers) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%s = %s\n", headers[j] ? headers[j] : "", data[i * cols + j] ? data[i * cols + j] : "");
        }
        if (i < rows - 1) printf("\n");
    }
}

void cli_format_result_list(int rows, int cols, char** data, char** headers) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (j > 0) printf("\t");
            printf("%s\t%s", headers[j] ? headers[j] : "", data[i * cols + j] ? data[i * cols + j] : "");
        }
        printf("\n");
    }
}