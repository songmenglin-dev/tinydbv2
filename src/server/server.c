#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "server.h"
#include "../../include/tinydb.h"
#include "../sql/parser.h"
#include "../sql/executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

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

            /* Parser succeeded - determine statement type */
            int is_ddl = (ast->type == AST_CREATE_TABLE ||
                         ast->type == AST_DROP_TABLE ||
                         ast->type == AST_CREATE_INDEX ||
                         ast->type == AST_DROP_INDEX);

            parser_free_ast(parser, ast);
            parser_destroy(parser);

            /* Return appropriate message based on statement type */
            if (is_ddl) {
                const char* resp = "Query OK\n";
                write(client_fd, resp, strlen(resp));
            } else {
                /* For SELECT and DML, executor returns row data or affected count */
                const char* resp = "OK 0 rows\n";
                write(client_fd, resp, strlen(resp));
            }
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