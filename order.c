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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <libcore/string.h>

#include "order.h"

#define MAX_SYMBOL_LEN  4

struct _order {
    unsigned long timestamp;
    unsigned long long id;

    String *symbol;
    float price;
    unsigned long quantity;

    ORDER_TYPE type;
    ORDER_SIDE side;
};


/* Constructor and Destructor */

Order* order_create(ORDER_TYPE type,
        ORDER_SIDE side,
        String *symbol,
        float price,
        unsigned long quantity)
{
    struct timeval tv;
    Order *new_order;

    assert(symbol != NULL);

    new_order = malloc(sizeof(struct _order));
    if(NULL == new_order) {
        fprintf(stderr, "Out of memory (%s:%d)\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    /* Order ID is assigned on Market entry */

    /* Current time, in milliseconds
     *
     * FIXME Probably want to use a more reliable mechanism than
     * gettimeofday. Also, need to understand securities exchange
     * business requirements in more detail to see how order time
     * plays a role
     */
    gettimeofday(&tv, NULL);
    new_order->timestamp    = (tv.tv_sec * 1000) + (tv.tv_usec / 1000) ;

    new_order->symbol       = symbol;
    new_order->price        = price;
    new_order->quantity     = quantity;
    new_order->type         = type;
    new_order->side         = side;

    return new_order;
}

void order_free(Order *o)
{
    assert(o != NULL);

    string_free(o->symbol);
    free(o);
}


/* Accessors */

unsigned long order_get_timestamp(const Order *o)
{
    assert(o != NULL);

    return o->timestamp;
}

unsigned long long order_get_id(const Order *o)
{
    assert(o != NULL);

    return o->id;
}

const String* order_get_symbol(const Order *o)
{
    assert(o != NULL);

    return o->symbol;
}

float order_get_price(const Order *o)
{
    assert(o != NULL);

    return o->price;
}

unsigned long order_get_quantity(const Order *o)
{
    assert(o != NULL);

    return o->quantity;
}

ORDER_TYPE order_get_type(const Order *o)
{
    assert(o != NULL);

    return o->type;
}

ORDER_SIDE order_get_side(const Order *o)
{
    assert(o != NULL);

    return o->side;
}


/* Mutators */

int order_set_id(Order *o, unsigned long long id)
{
    assert(o != NULL);

    o->id = id;

    return 0;
}

int order_set_price(Order *o, float price)
{
    assert(o != NULL);

    o->price = price;

    return 0;
}

int order_set_quantity(Order *o, unsigned long quantity)
{
    assert(o != NULL);

    o->quantity = quantity;

    return 0;
}

int order_set_type(Order *o, ORDER_TYPE type)
{
    assert(o != NULL);

    o->type = type;

    return 0;
}

int order_set_side(Order *o, ORDER_SIDE side)
{
    assert(o != NULL);

    o->side = side;

    return 0;
}


/* Converters */

ORDER_TYPE order_convert_from_fix_ordtype(FIX_ORDER_TYPE ordtype)
{
    ORDER_TYPE ret;

    switch(ordtype) {
        /* TODO Only Limit orders are supported for now */
        case FIX_ORDER_TYPE_LIMIT:
            ret = ORDER_TYPE_LIMIT;
            break;
        default:
            ret = ORDER_TYPE_INVALID;
            break;
    }

    return ret;
}

ORDER_SIDE order_convert_from_fix_side(FIX_ORDER_SIDE side)
{
    ORDER_SIDE ret;

    switch(side) {
        case FIX_ORDER_SIDE_BUY:
            ret = ORDER_SIDE_BUY;
            break;
        case FIX_ORDER_SIDE_SELL:
            ret = ORDER_SIDE_SELL;
            break;
        default:
            ret = ORDER_SIDE_INVALID;
            break;
    }

    return ret;
}
