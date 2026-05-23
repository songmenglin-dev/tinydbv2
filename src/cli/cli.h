#ifndef TINYDB_CLI_H
#define TINYDB_CLI_H

#include <stdbool.h>
#include <stddef.h>

/*============================================================================
 * CLI configuration
 *============================================================================*/
#define CLI_DEFAULT_SOCKET "/run/tinydb/tinydb.sock"
#define CLI_PROMPT "tinydb> "
#define CLI_MULTILINE_PROMPT "   ...> "
#define CLI_MAX_LINE 4096

/*============================================================================
 * Output modes
 *============================================================================*/
typedef enum {
    OUTPUT_MODE_BOX,
    OUTPUT_MODE_CSV,
    OUTPUT_MODE_LINE,
    OUTPUT_MODE_LIST
} OutputMode;

/*============================================================================
 * CLI state
 *============================================================================*/
typedef struct {
    int socket_fd;
    bool connected;
    OutputMode mode;
    bool show_headers;
    bool show_timer;
    const char* null_string;
    bool pager_enabled;
    const char* pager_cmd;
    bool verbose;
    bool quiet;
    bool executed_command;  /* Set when -c or -f is used */
    char socket_path[256];   /* Stored socket path for reuse */
} CLI;

/*============================================================================
 * CLI lifecycle
 *============================================================================*/
int cli_init(CLI** cli_out);
void cli_shutdown(CLI* cli);

/*============================================================================
 * Connection
 *============================================================================*/
int cli_connect(CLI* cli, const char* socket_path);
void cli_disconnect(CLI* cli);

/*============================================================================
 * Query execution
 *============================================================================*/
int cli_execute_sql(CLI* cli, const char* sql);
int cli_execute_file(CLI* cli, const char* filepath);

/*============================================================================
 * Interactive mode
 *============================================================================*/
int cli_interactive_loop(CLI* cli);
int cli_single_query(CLI* cli, const char* sql);
int cli_batch_mode(CLI* cli, const char* filepath);

/*============================================================================
 * Meta commands (dot commands)
 *============================================================================*/
int cli_handle_meta_command(CLI* cli, const char* cmd);
void cli_print_help(void);
void cli_print_tables(CLI* cli);
void cli_print_schema(CLI* cli, const char* table);
void cli_print_indexes(CLI* cli, const char* table);

/*============================================================================
 * Output formatting
 *============================================================================*/
void cli_set_output_mode(CLI* cli, OutputMode mode);
void cli_set_headers(CLI* cli, bool show);
void cli_set_timer(CLI* cli, bool show);
void cli_set_null_string(CLI* cli, const char* s);
void cli_set_pager(CLI* cli, const char* cmd);
void cli_set_verbose(CLI* cli, bool verbose);

const char* cli_get_output_mode_name(OutputMode mode);
OutputMode cli_parse_output_mode(const char* mode);

/*============================================================================
 * Result display
 *============================================================================*/
void cli_display_result(CLI* cli, int rows, int cols, char** data, char** headers);
void cli_display_error(const char* error);
void cli_display_ok(int changes);

/*============================================================================
 * Table formatting (MySQL-style box drawing)
 *============================================================================*/
int cli_format_table(char* resp, size_t resp_len, double elapsed_sec);
int cli_print_box_row(char** cols, int col_count, int* widths);
void cli_format_footer(int row_count, double elapsed_sec);

/*============================================================================
 * Argument parsing
 *============================================================================*/
int cli_parse_args(CLI* cli, int argc, char** argv);

#endif /* TINYDB_CLI_H */