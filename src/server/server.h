#ifndef TINYDB_SERVER_H
#define TINYDB_SERVER_H

#include "../../include/tinydb.h"
#include "../util/error.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>

/*============================================================================
 * Server configuration
 *============================================================================*/
#define SERVER_BACKLOG 128
#define SERVER_MAX_CLIENTS 64
#define SERVER_BUFFER_SIZE 8192
#define SERVER_pid_FILE "tinydb.pid"
#define SERVER_LOCK_FILE "tinydb.lock"

/*============================================================================
 * Server state
 *============================================================================*/
typedef struct {
    Storage* storage;
    int server_fd;
    char socket_path[256];
    char pid_file[256];
    char lock_file[256];
    bool running;
    int client_count;
    bool auth_enabled;
    char auth_file[256];
} Server;

/*============================================================================
 * Client connection state
 *============================================================================*/
typedef struct {
    int fd;
    char recv_buf[SERVER_BUFFER_SIZE];
    size_t recv_len;
    size_t recv_pos;
} Client;

/*============================================================================
 * Server lifecycle
 *============================================================================*/
int server_init(Server** server);
void server_shutdown(Server* server);
int server_run(Server* server);

/*============================================================================
 * Socket operations
 *============================================================================*/
int server_bind_and_listen(Server* server, const char* socket_path);
int server_accept_client(Server* server, int* client_fd);
int server_close_client(int client_fd);

/*============================================================================
 * Client handling
 *============================================================================*/
int server_handle_client(Server* server, int client_fd);

/*============================================================================
 * Signal handling
 *============================================================================*/
void server_setup_signals(void);
void server_signal_handler(int sig);

/*============================================================================
 * PID file management
 *============================================================================*/
int server_write_pid(const char* pid_file);
int server_remove_pid(const char* pid_file);
int server_write_lock(const char* lock_file);
int server_remove_lock(const char* lock_file);

/*============================================================================
 * Directory creation
 *============================================================================*/
int server_create_run_directory(void);

#endif /* TINYDB_SERVER_H */