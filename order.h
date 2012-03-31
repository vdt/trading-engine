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

#ifndef __ORDER_H__
#define __ORDER_H__

#include "fix_message.h"

/* Opaque forward declaration */
typedef struct _order Order;

typedef enum {
    ORDER_TYPE_MARKET,
    ORDER_TYPE_LIMIT,
    ORDER_TYPE_CANCEL,
    ORDER_TYPE_REPLACE,

    /* Add new order types before this point */
    ORDER_TYPE_INVALID
} ORDER_TYPE;

typedef enum {
    ORDER_SIDE_NONE,

    ORDER_SIDE_BUY,
    ORDER_SIDE_SELL,

    ORDER_SIDE_INVALID
} ORDER_SIDE;

/* Constructor and Destructor */
Order*  order_create    (ORDER_TYPE type,
                         ORDER_SIDE side,
                         String *symbol,
                         float price,
                         unsigned long quantity);

void    order_free      (Order *o);

/* Accessors */
unsigned long       order_get_timestamp (const Order *o);
unsigned long long  order_get_id        (const Order *o);
const String*       order_get_symbol    (const Order *o);
float               order_get_price     (const Order *o);
unsigned long       order_get_quantity  (const Order *o);
ORDER_TYPE          order_get_type      (const Order *o);
ORDER_SIDE          order_get_side      (const Order *o);

/* Mutators */
int order_set_id        (Order *o, unsigned long long id);
int order_set_price     (Order *o, float price);
int order_set_quantity  (Order *o, unsigned long quantity);
int order_set_type      (Order *o, ORDER_TYPE type);
int order_set_side      (Order *o, ORDER_SIDE side);

/* Converters */
ORDER_TYPE  order_convert_from_fix_ordtype  (FIX_ORDER_TYPE ordtype);
ORDER_SIDE  order_convert_from_fix_side     (FIX_ORDER_SIDE side);

#endif
