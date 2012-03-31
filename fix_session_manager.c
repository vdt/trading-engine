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

#include <pthread.h>
#include <stdio.h>

#include <libcore/map.h>
#include <libcore/string.h>

#include "fix_session.h"
#include "fix_session_manager.h"
#include "fix_parser.h"

#define DEBUG   0
#define DBG(...) \
    do { if(DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

/* Private scope */
static Map *sessions = NULL;
static int is_initialized = 0;
static pthread_mutex_t mgr_mutex = PTHREAD_MUTEX_INITIALIZER;

void fix_session_manager_init(void)
{
    printf("FIX Session Manager init\n");

    pthread_mutex_lock(&mgr_mutex);

    if(!is_initialized) {
        sessions = map_create((CompareFn)string_compare);
        is_initialized = 1;
    }

    pthread_mutex_unlock(&mgr_mutex);
}

void fix_session_manager_destroy(void)
{
    printf("FIX Session Manager destroy\n");

    pthread_mutex_lock(&mgr_mutex);

    if(is_initialized) {
        is_initialized = 0;
        map_free_all(sessions, (FreeFn)fix_session_free);
    }

    pthread_mutex_unlock(&mgr_mutex);
}

int fix_session_manager_lookup_session(String *fix_msg, FixSession **session)
{
    String *senderCompId;
    MapIterator *it;
    int ret;

    DBG("Session manager lookup session\n");

    *session = NULL;
    ret = 0;

    pthread_mutex_lock(&mgr_mutex);

    if(is_initialized && fix_parse_is_msg_valid(fix_msg)) {
        senderCompId = fix_parse_SenderCompId(fix_msg);
        DBG("Looking up session for: '%s'\n", string_get_chars(senderCompId));
        if((NULL == senderCompId) || string_is_empty(senderCompId)) {
            ret = -1;
        } else {
            it = map_find(sessions, senderCompId);
            if(NULL != it) {
                DBG("Found existing session object\n");
                *session = (FixSession *)map_get_value(it);
            } else {
                DBG("Creating new session object\n");
                *session = fix_session_create(senderCompId,
                        fix_parse_MsgSeqNum(fix_msg));
                if(NULL == *session) {
                    ret = -1;
                } else {
                    /* Store new session object in the table */
                    map_insert(sessions, fix_session_get_SenderCompId(*session),
                            *session);
                }
            }
        }
    } else {
        ret = -1;
    }

    pthread_mutex_unlock(&mgr_mutex);

    return ret;
}
