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

#ifndef __FIX_MESSAGE_H__
#define __FIX_MESSAGE_H__

#if __cplusplus
extern "C" {
#endif

/* Field tags. A small subset of those listed beginning on
 * page 192 of the FIX 4.2 spec.
 */
typedef enum {
    FIX_TAG_BEGIN_STRING = 8,
    FIX_TAG_BODY_LENGTH = 9,
    FIX_TAG_CHECKSUM = 10,
    FIX_TAG_CLORDID = 11,

    FIX_TAG_HANDLINST = 21,

    FIX_TAG_MSG_SEQ_NUM = 34,
    FIX_TAG_MSG_TYPE = 35,

    FIX_TAG_ORDER_QTY = 38,

    FIX_TAG_ORDER_TYPE = 40,

    FIX_TAG_PRICE = 44,
    FIX_TAG_SENDER_COMP_ID = 49,
    FIX_TAG_SENDING_TIME = 52,
    FIX_TAG_SIDE = 54,
    FIX_TAG_SYMBOL = 55,
    FIX_TAG_TARGET_COMP_ID = 56,

    FIX_TAG_TRANSACT_TIME = 60,

    FIX_TAG_ENCRYPT_METHOD = 98,

    FIX_TAG_HEARTBTINT = 108
} FIX_TAG;

/* Message types in FIX.4.2 are enumerated from 0-9,A-Z,a-m (see
 * page 154 of FIX 4.2 spec., referenced above).
 *
 * The enum below is constructed by taking the ASCII value of
 * each message type, and subtracting 48, the decimal value of
 * the ASCI character '0'.
 *
 * TODO This enum list is incomplete, and only lists a few
 * more of the message types actually supported by the server's
 * current capabilities
 */
typedef enum {
    FIX_MSG_TYPE_HEARTBEAT = 0,
    FIX_MSG_TYPE_TEST_REQUEST,
    FIX_MSG_TYPE_RESEND_REQUEST,
    FIX_MSG_TYPE_REJECT,
    FIX_MSG_TYPE_SEQ_RESET,
    FIX_MSG_TYPE_LOGOUT,
    FIX_MSG_TYPE_INDICATION_OF_INTEREST,
    FIX_MSG_TYPE_ADVERT,
    FIX_MSG_TYPE_EXEC_REPORT,
    FIX_MSG_TYPE_ORDER_CANCEL_REJECT,

    FIX_MSG_TYPE_LOGON = 17,

    FIX_MSG_TYPE_NEW_ORDER_SINGLE = 20,

    FIX_MSG_TYPE_ORDER_CANCEL_REQUEST = 22,
    FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST,
    FIX_MSG_TYPE_ORDER_STATUS_REQUEST,

    FIX_MSG_TYPE_PRIVATE = 37,

    /* No new messages after this point */
    FIX_MSG_TYPE_LAST,

    FIX_MSG_TYPE_INVALID
} FIX_MSG_TYPE;

typedef enum {
    FIX_ORDER_SIDE_BUY = 1,
    FIX_ORDER_SIDE_SELL,
    FIX_ORDER_SIDE_BUY_MINUS,
    FIX_ORDER_SIDE_SELL_PLUS,
    FIX_ORDER_SIDE_SELL_SHORT,
    FIX_ORDER_SIDE_SELL_SHORT_EXEMPT,
    FIX_ORDER_SIDE_UNDISCLOSED,
    FIX_ORDER_SIDE_CROSS,
    FIX_ORDER_SIDE_CROSS_SHORT,

    /* No new order sides after this point */
    FIX_ORDER_SIDE_LAST,

    FIX_ORDER_SIDE_INVALID
} FIX_ORDER_SIDE;

typedef enum {
    FIX_ORDER_TYPE_MARKET = 1,
    FIX_ORDER_TYPE_LIMIT,
    FIX_ORDER_TYPE_STOP,
    FIX_ORDER_TYPE_STOP_LIMIT,
    FIX_ORDER_TYPE_MARKET_ON_CLOSE,
    FIX_ORDER_TYPE_WITH_OR_WITHOUT,
    FIX_ORDER_TYPE_LIMIT_OR_BETTER,
    FIX_ORDER_TYPE_LIMIT_WITH_OR_WITHOUT,
    FIX_ORDER_TYPE_ON_BASIS,

    FIX_ORDER_TYPE_ON_CLOSE = 17,
    FIX_ORDER_TYPE_LIMIT_ON_CLOSE,
    FIX_ORDER_TYPE_FOREX_MARKET,
    FIX_ORDER_TYPE_PREV_QUOTED,
    FIX_ORDER_TYPE_PREV_INDICATED,
    FIX_ORDER_TYPE_FOREX_LIMIT,
    FIX_ORDER_TYPE_FOREX_SWAP,
    FIX_ORDER_TYPE_FOREX_PREV_QUOTED,
    FIX_ORDER_TYPE_FUNARI,

    FIX_ORDER_TYPE_PEGGED = 32,

    /* No new order types after this point */
    FIX_ORDER_TYPE_LAST,

    FIX_ORDER_TYPE_INVALID
} FIX_ORDER_TYPE;

typedef enum {
    FIX_ENCRYPT_METHOD_NONE = 0,
    FIX_ENCRYPT_METHOD_PKCS,
    FIX_ENCRYPT_METHOD_DES,
    FIX_ENCRYPT_METHOD_PKCS_DES,
    FIX_ENCRYPT_METHOD_PGP_DES,
    FIX_ENCRYPT_METHOD_PGP_DES_MD5,
    FIX_ENCRYPT_METHOD_PEM_DES_MD5,

    /* No new encryption methods after this point */
    FIX_ENCRYPT_METHOD_LAST,

    FIX_ENCRYPT_METHOD_INVALID
} FIX_ENCRYPT_METHOD;

typedef enum {
    FIX_HANDL_INST_AUTO_PRIVATE = 1,
    FIX_HANDL_INST_AUTO_PUBLIC,
    FIX_HANDL_INST_MANUAL,

    /* No new handling instructions after this point */
    FIX_HANDL_INST_LAST,

    FIX_HANDL_INST_INVALID
} FIX_HANDL_INST;

unsigned long fix_message_generate_checksum(const char *buf, unsigned long len);

String* fix_message_generate_header     (FIX_MSG_TYPE MsgType,
                                         unsigned long payload_length,
                                         const String *SenderCompId,
                                         const String *TargetCompId,
                                         unsigned long MsgSeqNum);

String* fix_message_generate_trailer    (const String *header_and_payload);
String* fix_message_generate_logon      (FIX_ENCRYPT_METHOD encrypt_method,
                                         int heart_bt_int);

String* fix_message_generate_new_order_single   (String *cl_ord_id,
                                                 FIX_HANDL_INST handl_inst,
                                                 String *symbol,
                                                 FIX_ORDER_SIDE side,
                                                 float order_qty,
                                                 FIX_ORDER_TYPE type,
                                                 float price);

#if __cplusplus
}
#endif

#endif
