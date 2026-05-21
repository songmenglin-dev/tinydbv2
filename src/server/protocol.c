#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PROTOCOL_BUFFER_SIZE 8192

int protocol_init(ProtocolBuffer* proto, int fd) {
    if (proto == NULL || fd < 0) {
        return -1;
    }

    proto->fd = fd;
    proto->buffer = malloc(PROTOCOL_BUFFER_SIZE);
    if (proto->buffer == NULL) {
        return -1;
    }

    proto->buffer_size = PROTOCOL_BUFFER_SIZE;
    proto->buffer_pos = 0;
    proto->data_len = 0;

    return 0;
}

void protocol_close(ProtocolBuffer* proto) {
    if (proto == NULL) return;

    if (proto->fd >= 0) {
        close(proto->fd);
        proto->fd = -1;
    }

    if (proto->buffer != NULL) {
        free(proto->buffer);
        proto->buffer = NULL;
    }
}

int protocol_read_request(ProtocolBuffer* proto, char* sql_buf, size_t buf_size) {
    if (proto == NULL || sql_buf == NULL || buf_size == 0) {
        return -1;
    }

    ssize_t n = read(proto->fd, proto->buffer + proto->data_len,
                     proto->buffer_size - proto->data_len - 1);

    if (n <= 0) {
        return (n == 0) ? 0 : -1;
    }

    proto->data_len += n;
    proto->buffer[proto->data_len] = '\0';

    char* newline = strchr(proto->buffer, '\n');
    if (newline != NULL) {
        size_t line_len = newline - proto->buffer;
        if (line_len >= buf_size) {
            line_len = buf_size - 1;
        }

        memcpy(sql_buf, proto->buffer, line_len);
        sql_buf[line_len] = '\0';

        size_t remaining = proto->data_len - line_len - 1;
        if (remaining > 0) {
            memmove(proto->buffer, newline + 1, remaining);
        }
        proto->data_len = remaining;

        return 1;
    }

    if (proto->data_len >= proto->buffer_size - 1) {
        fprintf(stderr, "Request too large\n");
        proto->data_len = 0;
        return -1;
    }

    return -1;
}

int protocol_write_response(ProtocolBuffer* proto, uint8_t type, uint32_t changes) {
    if (proto == NULL) {
        return -1;
    }

    ResponseHeader hdr;
    hdr.type = type;
    hdr.flags = 0;
    hdr.column_count = 0;
    hdr.row_count = 0;
    hdr.changes = changes;

    ssize_t written = write(proto->fd, &hdr, sizeof(hdr));
    return (written == sizeof(hdr)) ? 0 : -1;
}

int protocol_write_error(ProtocolBuffer* proto, uint32_t error_code, const char* message) {
    if (proto == NULL || message == NULL) {
        return -1;
    }

    ResponseHeader hdr;
    hdr.type = RESPONSE_ERROR;
    hdr.flags = 1;
    hdr.column_count = 0;
    hdr.row_count = 0;
    hdr.changes = error_code;

    if (write(proto->fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        return -1;
    }

    uint16_t msg_len = (uint16_t)strlen(message);
    if (write(proto->fd, &msg_len, sizeof(msg_len)) != sizeof(msg_len)) {
        return -1;
    }

    if (write(proto->fd, message, msg_len) != (ssize_t)msg_len) {
        return -1;
    }

    return 0;
}

int protocol_write_rows(ProtocolBuffer* proto, uint16_t column_count, uint32_t row_count) {
    if (proto == NULL) {
        return -1;
    }

    ResponseHeader hdr;
    hdr.type = RESPONSE_ROWS;
    hdr.flags = 0;
    hdr.column_count = column_count;
    hdr.row_count = row_count;
    hdr.changes = 0;

    ssize_t written = write(proto->fd, &hdr, sizeof(hdr));
    return (written == sizeof(hdr)) ? 0 : -1;
}

size_t protocol_escape_string(const char* src, char* dst, size_t dst_size) {
    if (src == NULL || dst == NULL || dst_size == 0) {
        return 0;
    }

    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 2 < dst_size; i++) {
        switch (src[i]) {
            case '\t':
                dst[j++] = '\\';
                dst[j++] = 't';
                break;
            case '\n':
                dst[j++] = '\\';
                dst[j++] = 'n';
                break;
            case '\\':
                dst[j++] = '\\';
                dst[j++] = '\\';
                break;
            case '\0':
                dst[j++] = '\\';
                dst[j++] = '0';
                break;
            default:
                dst[j++] = src[i];
                break;
        }
    }

    dst[j] = '\0';
    return j;
}

size_t protocol_unescape_string(const char* src, char* dst, size_t dst_size) {
    if (src == NULL || dst == NULL || dst_size == 0) {
        return 0;
    }

    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dst_size - 1; i++) {
        if (src[i] == '\\' && src[i + 1] != '\0') {
            i++;
            switch (src[i]) {
                case 't':
                    dst[j++] = '\t';
                    break;
                case 'n':
                    dst[j++] = '\n';
                    break;
                case '\\':
                    dst[j++] = '\\';
                    break;
                case '0':
                    dst[j++] = '\0';
                    break;
                default:
                    dst[j++] = src[i];
                    break;
            }
        } else {
            dst[j++] = src[i];
        }
    }

    dst[j] = '\0';
    return j;
}