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

#ifndef __FIX_SESSION_H__
#define __FIX_SESSION_H__

#if __cplusplus
extern "C" {
#endif

#include <libcore/string.h>

#include "fix_message.h"

/* Opaque forward declaration */
typedef struct _fix_session FixSession;

FixSession* fix_session_create      (String *SenderCompId,
                                     unsigned long client_seq_start);

void        fix_session_free        (FixSession *session);

int         fix_session_set_socket  (FixSession *session, int socket);

int         fix_session_activate    (FixSession *session);
int         fix_session_deactivate  (FixSession *session);

int         fix_session_receive_message (FixSession *session, String *message);
int         fix_session_send_message    (FixSession *session,
                                         FIX_MSG_TYPE type,
                                         String *payload);

const String*   fix_session_get_SenderCompId    (FixSession *session);
int             fix_session_is_active           (FixSession *session);
int             fix_session_get_socket          (FixSession *session);

#if __cplusplus
}
#endif

#endif
