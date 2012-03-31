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
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <libcore/string.h>

#include "fix.h"
#include "fix_server.h"
#include "fix_session.h"
#include "fix_session_manager.h"

#define DEBUG   0
#define DBG(...) \
    do { if(DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

#define BUFSZ           256

static pthread_t server_thread;

static String *fix_server_id = NULL;
static int server_socket = -1;
static int server_done = 0;

static void fix_server_read_logon(int socket)
{
    unsigned long msg_start_idx, msg_end_idx;
    String *buffer, *fix_msg;
    FixSession *session;
    char buf[BUFSZ];
    ssize_t n;
    int done;

    session = NULL;
    done = 0;

    buffer = string_create();
    fix_msg = NULL;

    while(!done) {
        DBG("Server waiting for data from client\n");
        /* Read data onto the end of buffer */
        if((n = recv(socket, buf, BUFSZ, 0)) > 0) {
            string_append_buf(buffer, buf, n);
            /* Find BeginString tag and CheckSum tag */
            if((string_find(buffer, "8=", &msg_start_idx) == 0) &&
                    (string_rfind(buffer, "\00110=", &msg_end_idx) == 0)) {
                /* Length of "<SOH>10=xxx<SOH>" is 8 characters */
                if((string_length(buffer) - msg_end_idx) >= 8) {
                    /* Extract FIX message. Index of final SOH is
                     * msg_end_idx + 7.
                     */
                    fix_msg = string_substring(buffer, msg_start_idx,
                            (msg_end_idx + 7));

                    DBG("New msg: '%s'\n", string_get_chars(fix_msg));

                    /* Pass message to session object */
                    if(NULL == session) {
                        if(fix_session_manager_lookup_session(fix_msg,
                                    &session) < 0) {
                            done = 1;
                        } else {
                            if(NULL == session) {
                                done = 1;
                            } else {
                                if(!fix_session_is_active(session)) {
                                    fix_session_set_socket(session, socket);
                                    fix_session_activate(session);
                                    fix_session_receive_message(session, fix_msg);
                                }

                                done = 1;
                            }
                        }
                    }
                }
            }
        } else {
            /* Assume client disconnected.
             *
             * TODO Check more thoroughly the reason for
             * recv failure
             */
            done = 1;
        }
    }

    /* Free the buffer string since there should
     * be no other valid data in it. When a client
     * connects, the only thing they should send
     * us is a logon message.
     */
    string_free(buffer);

    DBG("Server is done waiting for data from client\n");
}

static void* _fix_server(void *data)
{
    struct sockaddr_in addr;
    int c;

    printf("FIX Server init\n");

    server_done = 0;

    fix_server_id = string_create_from_buf(FIX_SERVER_ID,
            strlen(FIX_SERVER_ID));

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(FIX_SERVER_PORT);

    bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));

    /* Wait for clients to connect */
    listen(server_socket, SOMAXCONN);

    while(!server_done) {
        c = accept(server_socket, NULL, NULL);
        DBG("New client\n");
        fix_server_read_logon(c);
    }

    return NULL;
}

void fix_server_init(void)
{
    pthread_create(&server_thread, NULL, _fix_server, NULL);
}

void fix_server_destroy(void)
{
    printf("FIX Server destroy\n");

    server_done = 1;

    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);

    pthread_join(server_thread, NULL);

    string_free(fix_server_id);
}

const String* fix_server_get_id(void)
{
    return fix_server_id;
}
