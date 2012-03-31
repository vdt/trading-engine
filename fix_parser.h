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

#ifndef __FIX_PARSER_H__
#define __FIX_PARSER_H__

#if __cplusplus
extern "C" {
#endif

#include <libcore/string.h>

#include "fix_message.h"
#include "order.h"

int             fix_parse_is_msg_valid  (String *msg);
Order*          fix_parse_order         (String *msg);

/* Header and Trailer Fields */
String*         fix_parse_BeginString   (String *msg);
unsigned int    fix_parse_CheckSum      (String *msg);
unsigned long   fix_parse_BodyLength    (String *msg);
FIX_MSG_TYPE    fix_parse_MsgType       (String *msg);
String*         fix_parse_SenderCompId  (String *msg);
String*         fix_parse_TargetCompId  (String *msg);
unsigned long   fix_parse_MsgSeqNum     (String *msg);
//UTCTimestamp    fix_parse_SendingTime   (String *msg);
int             fix_parse_HeartBtInt    (String *msg);

/* New Order Fields */
String*         fix_parse_ClOrdId       (String *msg);
String*         fix_parse_Symbol        (String *msg);
FIX_ORDER_SIDE  fix_parse_Side          (String *msg);
//UTCTimestamp    fix_parse_TransactTime  (String *msg);
float           fix_parse_OrderQty      (String *msg);
FIX_ORDER_TYPE  fix_parse_OrdType       (String *msg);
float           fix_parse_Price         (String *msg);

#if __cplusplus
}
#endif

#endif

