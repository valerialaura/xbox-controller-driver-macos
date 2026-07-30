/*******************************************************************************
 * osc.c - OSC over UDP output
 *
 * Minimal OSC 1.0 encoder: padded address + typetag strings, big-endian
 * arguments, one message per UDP datagram. No external dependencies.
 ******************************************************************************/

#include "../../include/osc.h"
#include "../../include/log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*******************************************************************************
 * Backend State
 ******************************************************************************/
static int g_sock = -1;
static struct sockaddr_in g_target;
static bool g_ready = false;

int osc_set_target(const char *host, uint16_t port) {
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        LOG_ERROR("OSC: cannot resolve host %s", host);
        return -1;
    }
    memcpy(&g_target, res->ai_addr, sizeof(g_target));
    freeaddrinfo(res);

    g_ready = (g_sock >= 0);
    LOG_INFO("OSC target: %s:%u", host, port);
    return 0;
}

int osc_init(const char *host, uint16_t port) {
    if (g_sock < 0) {
        g_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (g_sock < 0) {
            LOG_ERROR("OSC: socket() failed");
            return -1;
        }
    }
    return osc_set_target(host, port);
}

void osc_cleanup(void) {
    if (g_sock >= 0) {
        close(g_sock);
        g_sock = -1;
    }
    g_ready = false;
}

bool osc_is_ready(void) {
    return g_ready;
}

/*******************************************************************************
 * OSC 1.0 Encoding
 ******************************************************************************/
#define OSC_MSG_MAX 160

static size_t write_padded_string(uint8_t *buf, const char *s) {
    size_t len = strlen(s) + 1;                    // include terminator
    size_t padded = (len + 3) & ~(size_t)3;
    memcpy(buf, s, len);
    memset(buf + len, 0, padded - len);
    return padded;
}

static size_t write_be32(uint8_t *buf, uint32_t bits) {
    buf[0] = (uint8_t)(bits >> 24);
    buf[1] = (uint8_t)(bits >> 16);
    buf[2] = (uint8_t)(bits >> 8);
    buf[3] = (uint8_t)bits;
    return 4;
}

static size_t write_float(uint8_t *buf, float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return write_be32(buf, bits);
}

static void osc_send_raw(const uint8_t *buf, size_t len) {
    if (!g_ready) return;
    sendto(g_sock, buf, len, 0, (const struct sockaddr *)&g_target, sizeof(g_target));
}

void osc_send_i(const char *address, int32_t value) {
    uint8_t buf[OSC_MSG_MAX];
    size_t off = write_padded_string(buf, address);
    off += write_padded_string(buf + off, ",i");
    off += write_be32(buf + off, (uint32_t)value);
    osc_send_raw(buf, off);
}

void osc_send_f(const char *address, float value) {
    uint8_t buf[OSC_MSG_MAX];
    size_t off = write_padded_string(buf, address);
    off += write_padded_string(buf + off, ",f");
    off += write_float(buf + off, value);
    osc_send_raw(buf, off);
}

void osc_send_ff(const char *address, float a, float b) {
    uint8_t buf[OSC_MSG_MAX];
    size_t off = write_padded_string(buf, address);
    off += write_padded_string(buf + off, ",ff");
    off += write_float(buf + off, a);
    off += write_float(buf + off, b);
    osc_send_raw(buf, off);
}
