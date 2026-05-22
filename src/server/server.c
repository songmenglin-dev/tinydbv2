#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "server.h"
#include "../../include/tinydb.h"
#include "../sql/parser.h"
#include "../sql/executor.h"
#include "../sql/storage.h"
#include "../sql/catalog.h"
#include "../storage/btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef SERVER_DEBUG
#define SERVER_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define SERVER_DEBUG_PRINT(...) ((void)0)
#endif

static Server* g_server = NULL;

void server_signal_handler(int sig) {
    (void)sig;
    if (g_server != NULL) {
        g_server->running = false;
    }
}

void server_setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = server_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
}

int server_create_run_directory(void) {
    const char* run_dir = "/run/tinydb";
    struct stat st;

    if (stat(run_dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }
        return -1;
    }

    if (mkdir(run_dir, 0755) == 0) {
        return 0;
    }

    return (errno == EEXIST) ? 0 : -1;
}

int server_write_pid(const char* pid_file) {
    FILE* f = fopen(pid_file, "w");
    if (f == NULL) {
        return -1;
    }
    fprintf(f, "%d\n", getpid());
    fclose(f);
    return 0;
}

int server_remove_pid(const char* pid_file) {
    unlink(pid_file);
    return 0;
}

int server_write_lock(const char* lock_file) {
    int fd = open(lock_file, O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        return -1;
    }
    close(fd);
    return 0;
}

int server_remove_lock(const char* lock_file) {
    unlink(lock_file);
    return 0;
}

int server_init(Server** server_out) {
    Server* server = calloc(1, sizeof(Server));
    if (server == NULL) {
        return -1;
    }

    server->server_fd = -1;
    server->running = false;
    server->client_count = 0;

    strcpy(server->socket_path, DEFAULT_SOCKET_PATH);
    snprintf(server->pid_file, sizeof(server->pid_file), "/run/tinydb/tinydb.pid");
    snprintf(server->lock_file, sizeof(server->lock_file), "/run/tinydb/tinydb.lock");

    /* Check if auth is enabled via environment variable */
    server->auth_enabled = (getenv("TINYDB_AUTH_PASSWORD") != NULL);
    server->auth_file[0] = '\0';

    *server_out = server;
    return 0;
}

void server_shutdown(Server* server) {
    if (server == NULL) return;

    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }

    if (strlen(server->socket_path) > 0) {
        unlink(server->socket_path);
    }

    server_remove_pid(server->pid_file);
    server_remove_lock(server->lock_file);

    /* Storage cleanup deferred until full integration */
    /* if (server->storage != NULL) { tinydb_close(server->storage); } */

    g_server = NULL;
    free(server);
}

int server_bind_and_listen(Server* server, const char* socket_path) {
    if (socket_path != NULL) {
        strncpy(server->socket_path, socket_path, sizeof(server->socket_path) - 1);
        server->socket_path[sizeof(server->socket_path) - 1] = '\0';
    }

    unlink(server->socket_path);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, server->socket_path);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, SERVER_BACKLOG) < 0) {
        close(fd);
        return -1;
    }

    server->server_fd = fd;
    return 0;
}

int server_accept_client(Server* server, int* client_fd_out) {
    int client_fd = accept(server->server_fd, NULL, NULL);
    if (client_fd < 0) {
        return -1;
    }

    *client_fd_out = client_fd;
    server->client_count++;
    return 0;
}

int server_close_client(int client_fd) {
    if (client_fd >= 0) {
        close(client_fd);
    }
    return 0;
}

int server_handle_client(Server* server, int client_fd) {
    (void)server;  /* Reserved for future storage integration */
    char buffer[SERVER_BUFFER_SIZE];
    ssize_t n;

    /* Check if auth is required - if so, require AUTH: command first */
    if (server->auth_enabled) {
        n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n <= 0) return 0;
        buffer[n] = '\0';

        /* Check for AUTH: command */
        if (strncmp(buffer, "AUTH:", 5) != 0) {
            const char* resp = "ERROR authentication required\n";
            write(client_fd, resp, strlen(resp));
            return 0;
        }

        /* Simple password check - in production use hashed passwords */
        const char* expected_pw = getenv("TINYDB_AUTH_PASSWORD");
        if (expected_pw == NULL) expected_pw = "tinydb_default";  /* fallback */

        /* Compare password (without trailing newline) */
        size_t pw_len = n - 5;
        while (pw_len > 0 && (buffer[5 + pw_len - 1] == '\n' || buffer[5 + pw_len - 1] == '\r')) {
            pw_len--;
        }

        if (pw_len != strlen(expected_pw) ||
            strncmp(buffer + 5, expected_pw, pw_len) != 0) {
            const char* resp = "ERROR authentication failed\n";
            write(client_fd, resp, strlen(resp));
            return 0;
        }

        /* Auth successful - send OK */
        const char* resp = "OK auth\n";
        write(client_fd, resp, strlen(resp));
    }

    while ((n = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';

        if (strncmp(buffer, "PING", 4) == 0) {
            const char* resp = "PONG\n";
            write(client_fd, resp, strlen(resp));
            continue;
        }

        if (strncmp(buffer, "SHUTDOWN", 8) == 0) {
            const char* resp = "OK shutdown\n";
            write(client_fd, resp, strlen(resp));
            return 1;
        }

        if (strncmp(buffer, "QUERY:", 6) == 0) {
            /* Extract SQL from after "QUERY:" prefix */
            const char* sql = buffer + 6;
            size_t sql_len = n - 6;

            /* Trim trailing newline/whitespace */
            while (sql_len > 0 && (sql[sql_len - 1] == '\n' || sql[sql_len - 1] == '\r' || sql[sql_len - 1] == ' ')) {
                sql_len--;
            }

            if (sql_len == 0) {
                const char* resp = "ERROR empty query\n";
                write(client_fd, resp, strlen(resp));
                continue;
            }

            /* Create parser and parse SQL */
            Parser* parser = parser_create(sql, sql_len);
            if (!parser) {
                const char* resp = "ERROR failed to create parser\n";
                write(client_fd, resp, strlen(resp));
                continue;
            }

            AstNode* ast = parser_parse(parser);
            if (!ast) {
                char err_buf[256];
                snprintf(err_buf, sizeof(err_buf), "ERROR parse error: %s\n", parser_error(parser));
                parser_destroy(parser);
                write(client_fd, err_buf, strlen(err_buf));
                continue;
            }

            SERVER_DEBUG_PRINT("[SERVER] Parsed SQL: ast=%p type=%d\n", (void*)ast, ast->type);
            /* Execute statement through executor if storage is available */
            int exec_result = SUCCESS;
            int row_count = 0;
            AstNodeType saved_type = AST_SELECT;  /* Save type BEFORE executor call */
            char* select_result_buf = NULL;  /* Buffer for SELECT row data */

            if (ast) saved_type = ast->type;

            SERVER_DEBUG_PRINT("[SERVER] Before executor_create storage=%p\n", (void*)server->storage);
            if (server->storage != NULL) {
                SERVER_DEBUG_PRINT("[SERVER] Creating executor\n");
                Executor* exec = executor_create(server->storage);
                SERVER_DEBUG_PRINT("[SERVER] executor_create returned exec=%p\n", (void*)exec);
                if (exec) {
                    SERVER_DEBUG_PRINT("[SERVER] Calling executor_exec type=%d\n", saved_type);
                    /* For SELECT, pass pointer to receive result buffer */
                    if (saved_type == AST_SELECT) {
                        char** result_ptr = &select_result_buf;
                        exec_result = executor_exec(exec, ast, NULL, &result_ptr);
                    } else {
                        exec_result = executor_exec(exec, ast, NULL, NULL);
                    }
                    SERVER_DEBUG_PRINT("[SERVER] executor_exec returned result=%d\n", exec_result);
                    SERVER_DEBUG_PRINT("[SERVER] Calling executor_destroy\n");
                    executor_destroy(exec);
                    SERVER_DEBUG_PRINT("[SERVER] executor_destroy returned\n");

                    /* Save AST info BEFORE freeing */
                    AstNodeType type_for_rows = saved_type;
                    char* table_name_for_select = NULL;
                    if (saved_type == AST_SELECT && exec_result == SUCCESS) {
                        AstSelect* select = (AstSelect*)ast;
                        if (select->table_name) {
                            table_name_for_select = strdup(select->table_name);
                        }
                    }

                    SERVER_DEBUG_PRINT("[SERVER] Calling parser_free_ast\n");
                    parser_free_ast(parser, ast);
                    SERVER_DEBUG_PRINT("[SERVER] parser_free_ast returned\n");
                    SERVER_DEBUG_PRINT("[SERVER] Calling parser_destroy\n");
                    parser_destroy(parser);
                    SERVER_DEBUG_PRINT("[SERVER] parser_destroy returned\n");

                    if (type_for_rows == AST_SELECT && exec_result == SUCCESS && table_name_for_select) {
                        Catalog* catalog = storage_get_catalog(server->storage);
                        if (catalog) {
                            CatalogEntry* entry = catalog_lookup_type_name(catalog, CATALOG_TYPE_TABLE, table_name_for_select);
                            if (entry && entry->root_page > 0) {
                                Pager* pager = storage_get_pager(server->storage);
                                PageCache* cache = storage_get_cache(server->storage);
                                if (pager && cache) {
                                    BTree* tree = btree_open(pager, cache, entry->root_page);
                                    if (tree) {
                                        BTreeCursor* cursor = btree_first(tree);
                                        char row_buf[2048];
                                        while (cursor && btree_cursor_valid(cursor)) {
                                            uint64_t key;
                                            uint32_t len;
                                            if (btree_get(cursor, &key, row_buf, &len) == SUCCESS && len > 0) {
                                                row_count++;
                                            }
                                            btree_cursor_next(cursor);
                                        }
                                        if (cursor) btree_cursor_free(cursor);
                                        btree_close(tree);
                                    }
                                }
                            }
                            if (entry) free(entry);
                        }
                        free(table_name_for_select);
                    } else if (type_for_rows == AST_INSERT && exec_result == SUCCESS) {
                        row_count = 1;
                    } else if (exec_result == SUCCESS) {
                        row_count = 1;
                    }

                    SERVER_DEBUG_PRINT("[SERVER] Building response resp_buf\n");
                    /* NOTE: executor_destroy already called above */
                } else {
                    exec_result = ERR_INTERNAL;
                }
            } else {
                exec_result = ERR_INTERNAL;
            }

            char resp_buf[128];
            SERVER_DEBUG_PRINT("[SERVER] Response: exec_result=%d saved_type=%d row_count=%d\n", exec_result, saved_type, row_count);
            if (exec_result != SUCCESS) {
                snprintf(resp_buf, sizeof(resp_buf), "ERROR execution failed (%d)\n", exec_result);
                SERVER_DEBUG_PRINT("[SERVER] Calling write for ERROR\n");
                write(client_fd, resp_buf, strlen(resp_buf));
                SERVER_DEBUG_PRINT("[SERVER] write ERROR done\n");
            } else if (saved_type == AST_SELECT) {
                /* Send OK with row count first */
                snprintf(resp_buf, sizeof(resp_buf), "OK %d row(s) returned\n", row_count);
                SERVER_DEBUG_PRINT("[SERVER] Calling write for SELECT OK\n");
                write(client_fd, resp_buf, strlen(resp_buf));
                SERVER_DEBUG_PRINT("[SERVER] write SELECT OK done\n");

                /* Then send ROW: lines if we have data */
                if (select_result_buf && strlen(select_result_buf) > 0) {
                    SERVER_DEBUG_PRINT("[SERVER] Calling write for ROW data\n");
                    write(client_fd, select_result_buf, strlen(select_result_buf));
                    SERVER_DEBUG_PRINT("[SERVER] write ROW data done\n");
                }

                /* Send END to mark end of data */
                const char* end_marker = "END\n";
                SERVER_DEBUG_PRINT("[SERVER] Calling write for END\n");
                write(client_fd, end_marker, strlen(end_marker));
                SERVER_DEBUG_PRINT("[SERVER] write END done\n");

                if (select_result_buf) free(select_result_buf);
            } else {
                snprintf(resp_buf, sizeof(resp_buf), "Query OK, %d row(s) affected\n", row_count);
                SERVER_DEBUG_PRINT("[SERVER] Calling write for DML OK\n");
                write(client_fd, resp_buf, strlen(resp_buf));
                SERVER_DEBUG_PRINT("[SERVER] write DML OK done\n");
            }
            SERVER_DEBUG_PRINT("[SERVER] Response sent, continuing\n");
            continue;
        }

        const char* err = "ERROR unknown command\n";
        write(client_fd, err, strlen(err));
    }

    return 0;
}

int server_run(Server* server) {
    server->running = true;

    while (server->running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server->server_fd, &read_fds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ready = select(server->server_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ready < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (ready == 0) continue;

        int client_fd;
        if (server_accept_client(server, &client_fd) == 0) {
            int should_shutdown = server_handle_client(server, client_fd);
            server_close_client(client_fd);

            if (should_shutdown) {
                server->running = false;
                break;
            }
        }
    }

    return 0;
}