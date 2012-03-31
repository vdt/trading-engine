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
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libcore/string.h>

#include "fix.h"
#include "fix_parser.h"
#include "fix_message.h"

#include "order.h"

static int _fix_valid_version(String *msg)
{
    String *version;
    int ret;

    assert(msg != NULL);

    ret = 0;

    version = fix_parse_BeginString(msg);
    if(strcmp(string_get_chars(version), FIX_VERSION) == 0) {
        ret = 1;
    }
    string_free(version);

    return ret;
}

static int _fix_valid_checksum(String *msg)
{
    unsigned int checksum, msg_checksum;
    unsigned long start_index;
    int ret;

    assert(msg != NULL);

    ret = 0;

    /* What checksum does the message say it has? */
    msg_checksum = fix_parse_CheckSum(msg);

    if(string_find(msg, "\00110=", &start_index) == 0) {
        /* What checksum does the message really have? */
        checksum = fix_message_generate_checksum(string_get_chars(msg),
                start_index + 1);

        if(msg_checksum == checksum) {
            ret = 1;
        }
    }

    return ret;
}

static int _fix_valid_length(String *msg)
{
    unsigned long start_index, end_index, msg_end_index;
    unsigned long length;
    int ret;

    assert(msg != NULL);

    ret = 0;

    /* What length does the message say it has? */
    length = fix_parse_BodyLength(msg);

    if(string_find(msg, "\0019=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            /* What length does the message really have? */
            if(string_find(msg, "\00110=", &msg_end_index) == 0) {
                if(length == (msg_end_index - end_index)) {
                    ret = 1;
                }
            }
        }
    }

    return ret;
}

int fix_parse_is_msg_valid(String *msg)
{
    return (_fix_valid_version(msg) &&
            _fix_valid_length(msg) &&
            _fix_valid_checksum(msg));
}

Order* fix_parse_order(String *msg)
{
    ORDER_TYPE type;
    ORDER_SIDE side;
    Order *o;

    type = order_convert_from_fix_ordtype(fix_parse_OrdType(msg));
    if(ORDER_TYPE_INVALID == type) {
        return NULL;
    }

    side = order_convert_from_fix_side(fix_parse_Side(msg));
    if(ORDER_SIDE_INVALID == side) {
        return NULL;
    }

    o = order_create(type, side,
            fix_parse_Symbol(msg),
            fix_parse_Price(msg),
            fix_parse_OrderQty(msg));

    return o;
}

/* 8: BeginString, must be first field in message */
String* fix_parse_BeginString(String *msg)
{
    unsigned long start_index, end_index;
    String *begin_string;

    assert(msg != NULL);

    begin_string = NULL;

    /* BeginString is always first field in message */
    if(string_find(msg, "=", &start_index) == 0) {
        if(string_find(msg, "\001", &end_index) == 0) {
            begin_string = string_substring(msg, start_index + 1, end_index - 1);
        }
    }

    return begin_string;
}

/* 10: CheckSum */
unsigned int fix_parse_CheckSum(String *msg)
{
    unsigned int msg_checksum;
    unsigned long start_index;
    String *tmp;

    assert(msg != NULL);

    msg_checksum = 0;

    if(string_find(msg, "\00110=", &start_index) == 0) {
        tmp = string_substring(msg, start_index + 4, start_index + 6);
        /* TODO Should check errno here */
        msg_checksum = strtoul(string_get_chars(tmp), NULL, 10);
        string_free(tmp);
    }

    return msg_checksum;
}


/* 9: MsgType, must be second field in message */
unsigned long fix_parse_BodyLength(String *msg)
{
    unsigned long start_index, end_index, length;
    String *tmp;

    assert(msg != NULL);

    length = 0;

    if(string_find(msg, "\0019=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            tmp = string_substring(msg, start_index + 3, end_index - 1);
            /* TODO Should check errno here */
            length = strtoul(string_get_chars(tmp), NULL, 10);
            string_free(tmp);
        }
    }

    return length;
}

/* 35: MsgType, must be third field in message */
FIX_MSG_TYPE fix_parse_MsgType(String *msg)
{
    unsigned long start_index;
    FIX_MSG_TYPE ret;

    assert(msg != NULL);

    ret = FIX_MSG_TYPE_INVALID;

    if(string_find(msg, "\00135=", &start_index) == 0) {
        ret = (FIX_MSG_TYPE)(string_char_at(msg, (start_index + 4)) - '0');
    }

    return ret;
}

/* 49: Assigned value used to identify firm sending message */
String* fix_parse_SenderCompId(String *msg)
{
    unsigned long start_index, end_index;
    String *senderCompId;

    assert(msg != NULL);

    senderCompId = NULL;

    if(string_find(msg, "\00149=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            senderCompId = string_substring(msg, start_index + 4, end_index - 1);
        }
    }

    return senderCompId;
}

/* 56: Assigned value used to identify receiving firm */
String* fix_parse_TargetCompId(String *msg)
{
    unsigned long start_index, end_index;
    String *targetCompId;

    assert(msg != NULL);

    targetCompId = NULL;

    if(string_find(msg, "\00156=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            targetCompId = string_substring(msg, start_index + 4, end_index - 1);
        }
    }

    return targetCompId;
}

/* 34: Integer message sequence number */
unsigned long fix_parse_MsgSeqNum(String *msg)
{
    unsigned long start_index, end_index, msgSeqNum;
    String *tmp;

    assert(msg != NULL);

    msgSeqNum = 0;

    if(string_find(msg, "\00134=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            tmp = string_substring(msg, start_index + 4, end_index - 1);
            /* TODO Should check errno here */
            msgSeqNum = strtoul(string_get_chars(tmp), NULL, 10);
            string_free(tmp);
        }
    }

    return msgSeqNum;
}

/* 52: Time of message transmission (always expressed in
 * UTC (Universal Time Coordinated, also known as
 * "GMT")
 */
//UTCTimestamp fix_parse_SendingTime(String *msg)
//{
//}

/* 108: Heartbeat interval (seconds) */
int fix_parse_HeartBtInt(String *msg)
{
    unsigned long start_index, end_index;
    int heartBtInt;
    String *tmp;

    assert(msg != NULL);

    heartBtInt = -1;

    if(string_find(msg, "\001108=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            tmp = string_substring(msg, start_index + 5, end_index - 1);
            /* TODO Should check errno here */
            heartBtInt = (int)strtol(string_get_chars(tmp), NULL, 10);
            string_free(tmp);
        }
    }

    return heartBtInt;
}


/* New Order Fields */

/* 11: Unique identifier for Order as assigned by institution.
 * Uniqueness must be guaranteed within a single trading day.
 */
String* fix_parse_ClOrdId(String *msg)
{
    unsigned long start_index, end_index;
    String *clOrdId;

    assert(msg != NULL);

    clOrdId = NULL;

    if(string_find(msg, "\00111=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            clOrdId = string_substring(msg, start_index + 4, end_index - 1);
        }
    }

    return clOrdId;
}

/* 21: Instructions for order handling on Broker trading floor */
/*
char fix_parse_HandlInst(String *msg)
{
    TODO Currently unsupported
}
*/

/* 55: Ticker symbol */
String* fix_parse_Symbol(String *msg)
{
    unsigned long start_index, end_index;
    String *symbol;

    assert(msg != NULL);

    symbol = NULL;

    if(string_find(msg, "\00155=", &start_index) == 0) {
        if(string_find_after(msg, "\001", start_index + 1, &end_index) == 0) {
            symbol = string_substring(msg, start_index + 4, end_index - 1);
        }
    }

    return symbol;
}

/* 54: Side of order */
FIX_ORDER_SIDE fix_parse_Side(String *msg)
{
    unsigned long start_index;
    FIX_ORDER_SIDE ret;

    assert(msg != NULL);

    ret = FIX_ORDER_SIDE_INVALID;

    if(string_find(msg, "\00154=", &start_index) == 0) {
        ret = (FIX_ORDER_SIDE)(string_char_at(msg, (start_index + 4)) - '0');
    }

    return ret;
}

/* 60: Time of execution/order creation (expressed in UTC
 * (Universal Time Coordinated, also known as "GMT")
 */
//UTCTimestamp fix_parse_TransactTime(String *msg)
//{
//}

/* 38: Number of shares ordered */
float fix_parse_OrderQty(String *msg)
{
    unsigned long start_index;
    float orderQty;

    assert(msg != NULL);

    orderQty = -1.0;

    if(string_find(msg, "\00138=", &start_index) == 0) {
        /* TODO Should check errno here */
        orderQty = strtof(string_get_chars(msg) + (start_index + 4), NULL);
    }

    return orderQty;
}

/* 40: Order type */
FIX_ORDER_TYPE fix_parse_OrdType(String *msg)
{
    unsigned long start_index;
    FIX_ORDER_TYPE ret;

    assert(msg != NULL);

    ret = FIX_ORDER_TYPE_INVALID;

    if(string_find(msg, "\00140=", &start_index) == 0) {
        ret = (FIX_ORDER_TYPE)(string_char_at(msg, (start_index + 4)) - '0');
    }

    return ret;
}

/* 44: Price */
float fix_parse_Price(String *msg)
{
    unsigned long start_index;
    float price;

    assert(msg != NULL);

    price = -1.0;

    if(string_find(msg, "\00144=", &start_index) == 0) {
        /* TODO Should check errno here */
        price = strtof(string_get_chars(msg) + (start_index + 4), NULL);
    }

    return price;
}
