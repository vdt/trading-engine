/* Copyright (c) 2012, Chris Winter <wintercni@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <time.h>

#include <libcore/string.h>
#include <libcore/darray.h>

#include "fix.h"
#include "fix_message.h"

#define BUFSZ           1024

static unsigned long tx_seq_num = 1;
static String *SenderCompId = NULL;
static String *TargetCompId = NULL;

/* Copied from fix_message.c */
#define UTCTIMESTAMP_FORMAT "%Y%m%d-%H:%M:%S"
static String* _make_utctimestamp(void)
{
    String *timestamp;
    char buf[BUFSZ];
    struct tm *tmp;
    ssize_t size;
    time_t t;

    timestamp = NULL;

    t = time(NULL);
    tmp = gmtime(&t);
    if(NULL == tmp) {
        return NULL;
    }

    size = strftime(buf, BUFSZ, UTCTIMESTAMP_FORMAT, tmp);
    if(size > 0) {
        timestamp = string_create_from_buf(buf, size);
    }

    return timestamp;
}

void read_logon(int socket)
{
    char buf[BUFSZ];

    /* Read data onto the end of buffer */
    recv(socket, buf, BUFSZ, 0);
}

int client_socket_init(void)
{
    struct sockaddr_in addr;
    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(3927);

    if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "Cannot connect to trading server: %s\n", strerror(errno));
        s = -1;
    }

    return s;
}

void fix_send_message(int sockfd, FIX_MSG_TYPE type, String *payload)
{
    String *header_and_payload, *trailer;
    String *fix_msg, *header;

    fix_msg = NULL;

    if((NULL == payload) || (string_length(payload) == 0)) {
        header = fix_message_generate_header(type,
                0, /* payload length */
                SenderCompId,
                TargetCompId,
                tx_seq_num++);

        header_and_payload = string_duplicate(header);
    } else {
        header = fix_message_generate_header(type,
                string_length(payload),
                SenderCompId,
                TargetCompId,
                tx_seq_num++);

        header_and_payload = string_concat(header, payload);
    }

    trailer = fix_message_generate_trailer(header_and_payload);

    fix_msg = string_concat(header_and_payload, trailer);

    //printf("Sending msg: '%s'\n", string_get_chars(fix_msg));
    send(sockfd, string_get_chars(fix_msg), string_length(fix_msg), 0);

    string_free(header);
    string_free(payload);
    string_free(header_and_payload);
    string_free(trailer);
    string_free(fix_msg);
}

void send_logon(int sockfd)
{
    fix_send_message(sockfd, FIX_MSG_TYPE_LOGON,
            fix_message_generate_logon(FIX_ENCRYPT_METHOD_NONE, 0));
}

void send_order(int sockfd)
{
    String *cl_ord_id, *symbol;
    FIX_ORDER_SIDE side;
    float price;
    unsigned int quantity;

    cl_ord_id = _make_utctimestamp();
    symbol = string_create_from_buf("AAPL", strlen("AAPL"));

    side = (FIX_ORDER_SIDE)((rand() % 2) + 1);
    quantity = (unsigned int)(rand() % 100);

    //price = (float)((rand() % 1000) / 100.0);
    if(FIX_ORDER_SIDE_BUY == side) {
        price = 10.00;
    } else {
        price = 9.00;
    }

    fix_send_message(sockfd,
            FIX_MSG_TYPE_NEW_ORDER_SINGLE,
            fix_message_generate_new_order_single(cl_ord_id,
                    FIX_HANDL_INST_AUTO_PRIVATE,
                    symbol,
                    side,
                    quantity,
                    FIX_ORDER_TYPE_LIMIT,
                    price));

    string_free(cl_ord_id);
    string_free(symbol);
}

int main(int argc, char *argv[])
{
    int sockfd;

    if(argc < 2 ||
            strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0) {
        printf("Usage: test-client <SenderCompId>\n");
        exit(0);
    }

    SenderCompId = string_create_from_buf(argv[1], strlen(argv[1]));
    TargetCompId = string_create_from_buf("CWTS", strlen("CWTS"));

    sockfd = client_socket_init();

    if(sockfd >= 0) {
        send_logon(sockfd);
        read_logon(sockfd);

        while(1) {
            send_order(sockfd);
            usleep(50 * 1000);
        }
    }

    string_free(SenderCompId);
    string_free(TargetCompId);

    return 0;
}
