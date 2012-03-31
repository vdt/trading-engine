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
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>

#include <libcore/string.h>
#include <libcore/queue.h>

#include "fix_session.h"
#include "fix_parser.h"
#include "fix_server.h"

#include "order.h"
#include "market.h"

#define DEBUG   0
#define DBG(...) \
    do { if(DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

struct _fix_session {
    String *SenderCompId;

    int socket;

    int is_active;

    Queue *rx_queue;
    Queue *tx_queue;

    pthread_t socket_thread;
    pthread_t rx_thread;
    pthread_t tx_thread;

    pthread_mutex_t mutex;
    pthread_cond_t rx_cond;
    pthread_cond_t tx_cond;

    unsigned long rx_seq_num;
    unsigned long tx_seq_num;
};

static void _fix_session_message_process(FixSession *session, String *msg)
{
    Order *o;

    DBG("Processing message: '%s'\n", string_get_chars(msg));

    if(fix_parse_is_msg_valid(msg)) {
        /* Validate the RX sequence number */
        if(session->rx_seq_num != fix_parse_MsgSeqNum(msg)) {
            fprintf(stderr, "Sequence number doesn't match: expect %lu got %lu\n",
                    session->rx_seq_num, fix_parse_MsgSeqNum(msg));
            /* TODO Should do a Sequence Reset-Gap Fill/Reset here */
            fix_session_deactivate(session);
            return;
        }

        /* Increment RX sequence number */
        session->rx_seq_num++;

        switch(fix_parse_MsgType(msg)) {
            /* Session Messages */
            case FIX_MSG_TYPE_LOGON:
                DBG("Received logon message\n");
                fix_session_send_message(session, FIX_MSG_TYPE_LOGON,
                        fix_message_generate_logon(FIX_ENCRYPT_METHOD_NONE, 0));
                break;
            case FIX_MSG_TYPE_LOGOUT:
                DBG("Received logout message\n");
                fix_session_send_message(session, FIX_MSG_TYPE_LOGOUT, NULL);
                break;

            /* TODO Other session-level messages here */


            /* Administrative and Application Messages */
            case FIX_MSG_TYPE_NEW_ORDER_SINGLE:
                DBG("Parsing new order\n");
                o = fix_parse_order(msg);
                if(NULL != o) {
                    DBG("Sending order into the market\n");
                    /* Send the order into the market */
                    market_process_order(o);
                }
                break;

            default:
                fprintf(stderr, "Received unsupported message\n");
                fix_session_deactivate(session);
                break;
        }
    } else {
        fprintf(stderr, "Received invalid message\n");
        fix_session_deactivate(session);
    }
}

static void _fix_session_message_send(FixSession *session, String *msg)
{
    DBG("Sending message: '%s'\n", string_get_chars(msg));

    send(session->socket, string_get_chars(msg), string_length(msg), 0);
}

#define BUFSZ           256
static void* _fix_session_socket_thread(void *data)
{
    unsigned long msg_start_idx, msg_end_idx;
    FixSession *session = (FixSession *)data;
    String *buffer, *fix_msg;
    char buf[BUFSZ];
    ssize_t n;

    if(NULL == session) {
        /* TODO Proper error log message */
        fprintf(stderr, "Valid session object not passed to _fix_session_thread\n");
        return NULL;
    }

    if(fix_session_get_socket(session) < 0) {
        fprintf(stderr, "Session has invalid socket\n");
        return NULL;
    }

    buffer = string_create();
    fix_msg = NULL;

    while(fix_session_is_active(session)) {
        if((n = recv(fix_session_get_socket(session), buf, BUFSZ, 0)) > 0) {
            string_append_buf(buffer, buf, n);
            /* Find BeginString tag and CheckSum tag */
            if((string_find(buffer, "8=", &msg_start_idx) == 0) &&
                    (string_find(buffer, "\00110=", &msg_end_idx) == 0)) {
                /* Length of "<SOH>10=xxx<SOH>" is 8 characters */
                if((string_length(buffer) - msg_end_idx) >= 8) {
                    /* Extract FIX message. Index of final SOH is
                     * msg_end_idx + 7.
                     */
                    fix_msg = string_remove_substring(buffer, msg_start_idx,
                            (msg_end_idx + 7));

                    DBG("New msg: '%s'\n", string_get_chars(fix_msg));
                    fix_session_receive_message(session, fix_msg);

                    /* Clear out buffer for new message */
                    if(msg_start_idx > 0) {
                        string_delete_substring(buffer, 0, msg_start_idx - 1);
                    }
                }
            }
        } else {
            /* Assume client disconnected.
             *
             * TODO Check more thoroughly the reason for
             * recv failure
             */
            DBG("Client disconnected\n");
            fix_session_deactivate(session);
        }
    }

    DBG("socket thread exiting\n");

    return NULL;
}

void* _fix_session_rx_thread(void *data)
{
    FixSession *session = (FixSession *)data;
    String *msg;

    if(NULL == session) {
        /* TODO Proper error log message */
        fprintf(stderr, "Invalid session object passed to _fix_session_tx_thread\n");
        return NULL;
    }

    pthread_mutex_lock(&session->mutex);

    while(session->is_active) {
        while(!queue_is_empty(session->rx_queue)) {
            msg = (String *)queue_dequeue(session->rx_queue);
            _fix_session_message_process(session, msg);
            string_free(msg);
        }

        pthread_cond_wait(&session->rx_cond, &session->mutex);
    }

    pthread_mutex_unlock(&session->mutex);

    DBG("Rx Thread: Exiting\n");

    return NULL;
}

void* _fix_session_tx_thread(void *data)
{
    FixSession *session = (FixSession *)data;
    String *msg;

    if(NULL == session) {
        /* TODO Proper error log message */
        fprintf(stderr, "Invalid session object passed to _fix_session_rx_thread\n");
        return NULL;
    }

    pthread_mutex_lock(&session->mutex);

    while(session->is_active) {
        while(!queue_is_empty(session->tx_queue)) {
            msg = (String *)queue_dequeue(session->tx_queue);
            _fix_session_message_send(session, msg);
            string_free(msg);
        }

        pthread_cond_wait(&session->tx_cond, &session->mutex);
    }

    pthread_mutex_unlock(&session->mutex);

    DBG("Tx Thread: Exiting\n");

    return NULL;
}


FixSession* fix_session_create(String *SenderCompId,
        unsigned long client_seq_start)
{
    FixSession *session;
    pthread_mutexattr_t mutex_attr;

    assert(SenderCompId != NULL);

    session = malloc(sizeof(FixSession));
    if(NULL == session) {
        /* TODO Log and error */
        return NULL;
    }

    session->SenderCompId = SenderCompId;
    session->socket = -1;
    session->is_active = 0;

    session->rx_queue = queue_create();
    session->tx_queue = queue_create();

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&session->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    pthread_cond_init(&session->rx_cond, NULL);
    pthread_cond_init(&session->tx_cond, NULL);

    session->rx_seq_num = 1;
    session->tx_seq_num = 1;

    return session;
}

void fix_session_free(FixSession *session)
{
    assert(session != NULL);

    fix_session_deactivate(session);

    pthread_cond_destroy(&session->rx_cond);
    pthread_cond_destroy(&session->tx_cond);
    pthread_mutex_destroy(&session->mutex);

    string_free(session->SenderCompId);

    queue_free_all(session->rx_queue, (FreeFn)string_free);
    queue_free_all(session->tx_queue, (FreeFn)string_free);

    free(session);
}

int fix_session_set_socket(FixSession *session, int socket)
{
    assert(session != NULL);
    assert(socket >= 0);

    pthread_mutex_lock(&session->mutex);
    session->socket = socket;
    pthread_mutex_unlock(&session->mutex);

    return 0;
}

int fix_session_activate(FixSession *session)
{
    assert(session != NULL);

    pthread_mutex_lock(&session->mutex);

    if(!session->is_active) {
        session->is_active = 1;

        printf("FIX Session: Activating session for '%s'\n",
                string_get_chars(session->SenderCompId));

        if(pthread_create(&session->socket_thread, NULL,
                    &_fix_session_socket_thread, (void *)session) != 0) {
            session->is_active = 0;
            pthread_mutex_unlock(&session->mutex);
            return -1;
        }

        if(pthread_create(&session->tx_thread, NULL,
                    &_fix_session_tx_thread, (void *)session) != 0) {
            /* FIXME Need to kill socket thread */
            session->is_active = 0;
            pthread_mutex_unlock(&session->mutex);
            return -1;
        }

        if(pthread_create(&session->rx_thread, NULL,
                    &_fix_session_rx_thread, (void *)session) != 0) {
            /* FIXME Need to kill socket and rx threads */
            session->is_active = 0;
            pthread_mutex_unlock(&session->mutex);
            return -1;
        }
    }

    pthread_mutex_unlock(&session->mutex);

    return 0;
}

int fix_session_deactivate(FixSession *session)
{
    assert(session != NULL);

    pthread_mutex_lock(&session->mutex);

    if(session->is_active) {
        session->is_active = 0;

        printf("FIX Session: Deactivating session for '%s'\n",
                string_get_chars(session->SenderCompId));

        pthread_cond_signal(&session->tx_cond);
        pthread_cond_signal(&session->rx_cond);
        pthread_mutex_unlock(&session->mutex);

        if(pthread_equal(pthread_self(), session->rx_thread) != 0) {
            pthread_join(session->rx_thread, NULL);
        }

        if(pthread_equal(pthread_self(), session->tx_thread) != 0) {
            pthread_join(session->tx_thread, NULL);
        }

        shutdown(session->socket, SHUT_RDWR);
        close(session->socket);
        session->socket = -1;

        if(pthread_equal(pthread_self(), session->socket_thread) != 0) {
            pthread_join(session->socket_thread, NULL);
        }
    } else {
        pthread_mutex_unlock(&session->mutex);
    }

    return 0;
}

int fix_session_receive_message(FixSession *session, String *message)
{
    if((NULL == session) ||
            (NULL == message) ||
            string_is_empty(message)) {
        return -1;
    }

    pthread_mutex_lock(&session->mutex);

    queue_enqueue(session->rx_queue, message);

    pthread_cond_signal(&session->rx_cond);
    pthread_mutex_unlock(&session->mutex);

    return 0;
}

int fix_session_send_message(FixSession *session,
        FIX_MSG_TYPE type, String *payload)
{
    String *header_and_payload, *trailer;
    String *fix_msg, *header;

    DBG("Sending message\n");

    if((NULL == session) ||
            (type >= FIX_MSG_TYPE_LAST)) {
        return -1;
    }

    fix_msg = NULL;

    pthread_mutex_lock(&session->mutex);

    if((NULL == payload) || (string_length(payload) == 0)) {
        header = fix_message_generate_header(type,
                0, /* payload length */
                fix_server_get_id(),
                session->SenderCompId,
                session->tx_seq_num++);

        header_and_payload = string_duplicate(header);
    } else {
        header = fix_message_generate_header(type,
                string_length(payload),
                fix_server_get_id(),
                session->SenderCompId,
                session->tx_seq_num++);

        header_and_payload = string_concat(header, payload);
    }

    trailer = fix_message_generate_trailer(header_and_payload);

    fix_msg = string_concat(header_and_payload, trailer);

    queue_enqueue(session->tx_queue, fix_msg);

    pthread_cond_signal(&session->tx_cond);
    pthread_mutex_unlock(&session->mutex);

    string_free(header);
    string_free(payload);
    string_free(header_and_payload);
    string_free(trailer);

    return 0;
}

const String* fix_session_get_SenderCompId(FixSession *session)
{
    String *ret;

    assert(session != NULL);

    pthread_mutex_lock(&session->mutex);
    ret = session->SenderCompId;
    pthread_mutex_unlock(&session->mutex);

    return ret;
}

int fix_session_is_active(FixSession *session)
{
    int ret;

    assert(session != NULL);

    pthread_mutex_lock(&session->mutex);
    ret = session->is_active;
    pthread_mutex_unlock(&session->mutex);

    return ret;
}

int fix_session_get_socket(FixSession *session)
{
    int ret;

    assert(session != NULL);

    pthread_mutex_lock(&session->mutex);
    ret = session->socket;
    pthread_mutex_unlock(&session->mutex);

    return ret;
}
