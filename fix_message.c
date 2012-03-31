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

#include <string.h>
#include <stdio.h>
#include <time.h>

#include <libcore/string.h>
#include <libcore/darray.h>

#include "fix.h"
#include "fix_server.h"
#include "fix_message.h"

#define BUFSZ   1024
#define UTCTIMESTAMP_FORMAT "%Y%m%d-%H:%M:%S"

static String* _make_tag_equals(FIX_TAG tag)
{
    char buf[BUFSZ];
    int written;

    written = snprintf(buf, BUFSZ, "%d=", (int)tag);
    if(written > BUFSZ) {
        written = BUFSZ;
    }

    return string_create_from_buf(buf, written);
}

static String* _make_field_from_chars(FIX_TAG tag, const char *value)
{
    String *field;

    field = _make_tag_equals(tag);
    string_append_buf(field, value, strlen(value));
    string_append_char(field, '\001');

    return field;
}

static String* _make_field_from_char(FIX_TAG tag, char value)
{
    String *field;

    field = _make_tag_equals(tag);
    string_append_char(field, value);
    string_append_char(field, '\001');

    return field;
}

static String* _make_field_from_string(FIX_TAG tag, const String *value)
{
    String *tag_equals, *field;

    tag_equals = _make_tag_equals(tag);
    field = string_concat(tag_equals, value);
    string_append_char(field, '\001');
    string_free(tag_equals);

    return field;
}

static String* _make_field_from_ulong(FIX_TAG tag, unsigned long value)
{
    char buf[BUFSZ];
    int written;

    written = snprintf(buf, BUFSZ, "%d=%lu\001", (int)tag, value);
    if(written > BUFSZ) {
        return NULL;
    }

    return string_create_from_buf(buf, written);
}

static String* _make_field_from_int(FIX_TAG tag, int value)
{
    char buf[BUFSZ];
    int written;

    written = snprintf(buf, BUFSZ, "%d=%d\001", (int)tag, value);
    if(written > BUFSZ) {
        return NULL;
    }

    return string_create_from_buf(buf, written);
}

static String* _make_field_from_float(FIX_TAG tag, float value)
{
    char buf[BUFSZ];
    int written;

    written = snprintf(buf, BUFSZ, "%d=%4.4f\001", (int)tag, value);
    if(written > BUFSZ) {
        return NULL;
    }

    return string_create_from_buf(buf, written);
}

static String* _make_field_from_checksum(FIX_TAG tag, unsigned long value)
{
    char buf[BUFSZ];
    int written;

    written = snprintf(buf, BUFSZ, "%d=%.3lu\001", (int)tag, value);
    if(written > BUFSZ) {
        return NULL;
    }

    return string_create_from_buf(buf, written);
}

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
        /* TODO log an error */
        return NULL;
    }

    size = strftime(buf, BUFSZ, UTCTIMESTAMP_FORMAT, tmp);
    if(size > 0) {
        timestamp = string_create_from_buf(buf, size);
    }

    return timestamp;
}

/* Adapted from: "Financial Information Exchange Protocol (FIX),
 * Version 4.2 with Errata 20010501" (Fix Protocol Limited, 2001)
 */
unsigned long fix_message_generate_checksum(const char *buf, unsigned long len) {
    unsigned long idx;
    unsigned long cks;

    for(idx = 0, cks = 0; idx < len; cks += (unsigned int)buf[idx++]) {
        /* Empty */
    }

    return (unsigned long)(cks % 256);
}

String* fix_message_generate_header(FIX_MSG_TYPE MsgType,
        unsigned long payload_length, const String *SenderCompId,
        const String *TargetCompId, unsigned long MsgSeqNum)
{
    unsigned long BodyLength, i;
    String *now, *header;
    DArray *fields;

    fields = darray_create();

    now = _make_utctimestamp();

    BodyLength = 0;

    darray_append(fields, _make_field_from_chars(FIX_TAG_BEGIN_STRING, FIX_VERSION));
    darray_append(fields, _make_field_from_char(FIX_TAG_MSG_TYPE, '0' + MsgType));
    darray_append(fields, _make_field_from_string(FIX_TAG_SENDER_COMP_ID, SenderCompId));
    darray_append(fields, _make_field_from_string(FIX_TAG_TARGET_COMP_ID, TargetCompId));
    darray_append(fields, _make_field_from_ulong(FIX_TAG_MSG_SEQ_NUM, MsgSeqNum));
    darray_append(fields, _make_field_from_string(FIX_TAG_SENDING_TIME, now));

    for(i = 1; i < darray_size(fields); i++) {
        BodyLength += string_length((String *)darray_index(fields, i));
    }

    BodyLength += payload_length;

    darray_insert(fields, 1, _make_field_from_ulong(FIX_TAG_BODY_LENGTH, BodyLength));

    header = string_join(fields);

    string_free(now);
    darray_free_all(fields, (FreeFn)string_free);

    return header;
}

String* fix_message_generate_trailer(const String *header_and_payload)
{
    unsigned int checksum;
    String *trailer;

    checksum = fix_message_generate_checksum(
            string_get_chars(header_and_payload),
            string_length(header_and_payload));

    trailer = _make_field_from_checksum(FIX_TAG_CHECKSUM, checksum);

    return trailer;
}

String* fix_message_generate_logon(FIX_ENCRYPT_METHOD encrypt_method,
        int heart_bt_int)
{
    DArray *fields;
    String *logon;

    fields = darray_create();

    darray_append(fields, _make_field_from_int(FIX_TAG_ENCRYPT_METHOD, (int)encrypt_method));
    darray_append(fields, _make_field_from_int(FIX_TAG_HEARTBTINT, (int)heart_bt_int));

    logon = string_join(fields);

    darray_free_all(fields, (FreeFn)string_free);

    return logon;
}

String* fix_message_generate_new_order_single(String *cl_ord_id,
        FIX_HANDL_INST handl_inst,
        String *symbol,
        FIX_ORDER_SIDE side,
        float order_qty,
        FIX_ORDER_TYPE type,
        float price)
{
    DArray *fields;
    String *new_order, *now;

    fields = darray_create();

    now = _make_utctimestamp();

    darray_append(fields, _make_field_from_string(FIX_TAG_CLORDID, cl_ord_id));
    darray_append(fields, _make_field_from_char(FIX_TAG_HANDLINST, '0' + (int)handl_inst));
    darray_append(fields, _make_field_from_string(FIX_TAG_SYMBOL, symbol));
    darray_append(fields, _make_field_from_char(FIX_TAG_SIDE, '0' + (int)side));
    darray_append(fields, _make_field_from_string(FIX_TAG_TRANSACT_TIME, now));
    darray_append(fields, _make_field_from_float(FIX_TAG_ORDER_QTY, order_qty));
    darray_append(fields, _make_field_from_char(FIX_TAG_ORDER_TYPE, '0' + (int)type));
    darray_append(fields, _make_field_from_float(FIX_TAG_PRICE, price));

    new_order = string_join(fields);

    string_free(now);
    darray_free_all(fields, (FreeFn)string_free);

    return new_order;
}
