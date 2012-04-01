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
#include <stdio.h>
#include <pthread.h>

#include <libcore/map.h>
#include <libcore/string.h>

#include "market.h"
#include "book.h"

static Map *book_table = NULL;
static int is_open = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long long order_id = 0;

void market_open(void)
{
    printf("Market Open\n");

    pthread_mutex_lock(&mutex);

    if(!is_open) {
        book_table = map_create((CompareFn)string_compare);
        is_open = 1;
    }

    pthread_mutex_unlock(&mutex);
}

void market_close(void)
{
    printf("Market Close\n");

    pthread_mutex_lock(&mutex);

    if(is_open) {
        is_open = 0;
        map_free_all(book_table, (FreeFn)book_close);
    }

    pthread_mutex_unlock(&mutex);
}

int market_process_order(Order *o)
{
    MapIterator *it;
    Book *b;
    int ret;

    assert(o != NULL);

    ret = 0;

    pthread_mutex_lock(&mutex);

    if(is_open) {
        it = map_find(book_table, order_get_symbol(o));
        if(NULL == it) {
            /* New ticker symbol, so lets open a new book */
            b = book_open(order_get_symbol(o));
            map_insert(book_table, book_get_symbol(b), b);
        } else {
            b = map_get_value(it);
        }

        /* Market-specifc order ID */
        order_set_id(o, order_id++);
        ret = book_process_order(b, o);
    } else {
        fprintf(stderr, "ERROR: Market not open\n");
    }

    pthread_mutex_unlock(&mutex);

    return ret;
}

int market_is_open(void)
{
    int ret;

    pthread_mutex_lock(&mutex);
    ret = is_open;
    pthread_mutex_unlock(&mutex);

    return ret;
}

unsigned long long market_get_total_volume(void)
{
    unsigned long long volume;
    MapIterator *it;

    volume = 0;

    pthread_mutex_lock(&mutex);
    if(is_open) {
        for(it = map_begin(book_table); it != NULL; it = map_next(it)) {
            volume += book_get_volume((const Book *)map_get_value(it));
        }
    }
    pthread_mutex_unlock(&mutex);

    return volume;
}

unsigned long long market_get_total_orders_filled(void)
{
    unsigned long long orders;
    MapIterator *it;

    orders = 0;

    pthread_mutex_lock(&mutex);
    if(is_open) {
        for(it = map_begin(book_table); it != NULL; it = map_next(it)) {
            orders += book_get_orders_filled((const Book *)map_get_value(it));
        }
    }
    pthread_mutex_unlock(&mutex);

    return orders;
}
