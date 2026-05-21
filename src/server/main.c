#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_version(void) {
    printf("TinyDB Server v2.0.0\n");
}

static void print_help(const char* prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("\nOptions:\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --version          Show version\n");
    printf("  -d, --db-path <path>    Database file path\n");
    printf("  -s, --socket-path <path>  Unix socket path\n");
    printf("  -l, --log-level <level> Log level (error, warn, info, debug)\n");
    printf("\nEnvironment variables:\n");
    printf("  TINYDB_DB_PATH         Database file path\n");
    printf("  TINYDB_SOCKET_PATH     Unix socket path\n");
    printf("  TINYDB_LOG_LEVEL       Log level\n");
}

int main(int argc, char** argv) {
    const char* db_path = NULL;
    const char* socket_path = NULL;
    const char* log_level = NULL;

    static struct option long_options[] = {
        {"help",        no_argument,       NULL, 'h'},
        {"version",     no_argument,       NULL, 'v'},
        {"db-path",     required_argument, NULL, 'd'},
        {"socket-path", required_argument, NULL, 's'},
        {"log-level",   required_argument, NULL, 'l'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hv d:s:l:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'd':
                db_path = optarg;
                break;
            case 's':
                socket_path = optarg;
                break;
            case 'l':
                log_level = optarg;
                break;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    (void)db_path;
    (void)log_level;

    if (getenv("TINYDB_DB_PATH") && !db_path) {
        db_path = getenv("TINYDB_DB_PATH");
    }
    if (getenv("TINYDB_SOCKET_PATH") && !socket_path) {
        socket_path = getenv("TINYDB_SOCKET_PATH");
    }
    if (getenv("TINYDB_LOG_LEVEL") && !log_level) {
        log_level = getenv("TINYDB_LOG_LEVEL");
    }

    if (!db_path) {
        db_path = "/var/lib/tinydb/main.db";
    }

    if (server_create_run_directory() != 0) {
        fprintf(stderr, "Failed to create run directory /run/tinydb\n");
        return 1;
    }

    Server* server = NULL;
    if (server_init(&server) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    char pid_file[256];
    char lock_file[256];
    snprintf(pid_file, sizeof(pid_file), "/run/tinydb/tinydb.pid");
    snprintf(lock_file, sizeof(lock_file), "/run/tinydb/tinydb.lock");

    if (server_write_lock(lock_file) != 0) {
        fprintf(stderr, "Another instance is already running (lock file exists)\n");
        server_shutdown(server);
        return 1;
    }

    if (server_write_pid(pid_file) != 0) {
        fprintf(stderr, "Failed to write PID file\n");
        server_remove_lock(lock_file);
        server_shutdown(server);
        return 1;
    }

    if (socket_path) {
        strncpy(server->socket_path, socket_path, sizeof(server->socket_path) - 1);
    }

    if (server_bind_and_listen(server, socket_path) != 0) {
        fprintf(stderr, "Failed to bind to socket at %s\n", socket_path ? socket_path : DEFAULT_SOCKET_PATH);
        server_remove_pid(pid_file);
        server_remove_lock(lock_file);
        server_shutdown(server);
        return 1;
    }

    server_setup_signals();

    printf("TinyDB Server v2.0.0\n");
    printf("Database: %s\n", db_path);
    printf("Socket: %s\n", server->socket_path);
    printf("PID file: %s\n", pid_file);

    /* Initialize storage with database path */
    if (tinydb_open(&server->storage, db_path) != 0) {
        fprintf(stderr, "Failed to open database at %s\n", db_path);
        server_remove_pid(pid_file);
        server_remove_lock(lock_file);
        server_shutdown(server);
        return 1;
    }

    printf("Server listening...\n");

    /* NOTE: Full storage integration pending SQL engine implementation */
    /* Server accepts basic protocol commands (PING, SHUTDOWN) in placeholder mode */

    server_run(server);

    printf("Server shutting down...\n");
    if (server->storage) {
        tinydb_close(server->storage);
    }
    server_shutdown(server);

    return 0;
}