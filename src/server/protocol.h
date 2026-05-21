#ifndef TINYDB_PROTOCOL_H
#define TINYDB_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

/*============================================================================
 * Protocol constants
 *============================================================================*/
#define PROTOCOL_MAGIC 0x54494E59  // 'TINY'
#define PROTOCOL_VERSION 1

/* Request types */
typedef enum {
    REQUEST_EXECUTE = 1,
    REQUEST_PING = 2,
    REQUEST_SHUTDOWN = 3,
    REQUEST_BEGIN = 4,
    REQUEST_COMMIT = 5,
    REQUEST_ROLLBACK = 6
} RequestType;

/* Response types */
typedef enum {
    RESPONSE_OK = 1,
    RESPONSE_ROWS = 2,
    RESPONSE_ERROR = 3,
    RESPONSE_BUSY = 4,
    RESPONSE_SHUTDOWN_ACK = 5
} ResponseType;

/*============================================================================
 * Request/Response structures
 *============================================================================*/
typedef struct {
    uint8_t type;
    uint8_t flags;
    uint16_t body_length;
    uint32_t request_id;
} RequestHeader;

typedef struct {
    uint8_t type;
    uint8_t flags;
    uint16_t column_count;
    uint32_t row_count;
    uint32_t changes;
} ResponseHeader;

/*============================================================================
 * Protocol buffer
 *============================================================================*/
typedef struct {
    int fd;
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    size_t data_len;
} ProtocolBuffer;

/*============================================================================
 * Protocol operations
 *============================================================================*/
int protocol_init(ProtocolBuffer* proto, int fd);
void protocol_close(ProtocolBuffer* proto);
int protocol_read_request(ProtocolBuffer* proto, char* sql_buf, size_t buf_size);
int protocol_write_response(ProtocolBuffer* proto, uint8_t type, uint32_t changes);
int protocol_write_error(ProtocolBuffer* proto, uint32_t error_code, const char* message);
int protocol_write_rows(ProtocolBuffer* proto, uint16_t column_count, uint32_t row_count);

/*============================================================================
 * Escaping utilities
 *============================================================================*/
size_t protocol_escape_string(const char* src, char* dst, size_t dst_size);
size_t protocol_unescape_string(const char* src, char* dst, size_t dst_size);

#endif /* TINYDB_PROTOCOL_H */