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
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libcore/heap.h>

#include "book.h"
#include "order.h"

#define DEBUG   0
#define DBG(...) \
    do { if(DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)

struct _book {
    String *symbol;
    Heap *buy;
    Heap *sell;

    unsigned long orders_filled;
    unsigned long long volume;

    int book_is_open;

    pthread_t       matcher_thread;
    pthread_mutex_t matcher_mutex;
    pthread_cond_t  matcher_cond;
};

/* The buy order heap is a max-heap, with the highest
 * bid at the top of the heap. If two or more orders have
 * the highest bid, then we compare the timestamps to
 * place the oldest order at the top of the heap.
 */
int _book_buy_compare(const Order *o1, const Order *o2)
{
    float price_diff = fabsf(order_get_price(o1) - order_get_price(o2));
    int ret;

    if(price_diff < FLT_EPSILON) {
        /* Prices are equal, so compare timestamps */
        if(order_get_timestamp(o1) < order_get_timestamp(o2)) {
            ret = 1;
        } else {
            ret = -1;
        }
    } else if(order_get_price(o1) > order_get_price(o2)) {
        ret = 1;
    } else {
        ret = -1;
    }

    return ret;
}

/* The sell order heap is a min-heap, with the lowest
 * quote at the top of the heap. If two or more orders have
 * the lowest quote, then we compare the timestamps to
 * place the oldest order at the top of the heap.
 */
int _book_sell_compare(const Order *o1, const Order *o2)
{
    float price_diff = fabsf(order_get_price(o1) - order_get_price(o2));
    int ret;

    if(price_diff < FLT_EPSILON) {
        /* Prices are equal, so compare timestamps */
        if(order_get_timestamp(o1) < order_get_timestamp(o2)) {
            ret = 1;
        } else {
            ret = -1;
        }
    } else if(order_get_price(o1) < order_get_price(o2)) {
        ret = 1;
    } else {
        ret = -1;
    }

    return ret;
}

void* _book_fill_orders(void *arg)
{
    unsigned long bid_quantity, quote_quantity;
    Order *bid, *quote;
    float price_diff;
    Book *b;

    if(NULL == arg) {
        fprintf(stderr, "(%s:%d) ERROR: Matcher thread received invalid Book.\n",
                __FUNCTION__, __LINE__);
        return NULL;
    }

    b = (Book *)arg;

    pthread_mutex_lock(&b->matcher_mutex);

    while(b->book_is_open) {
        bid   = (Order *)heap_top(b->buy);
        quote = (Order *)heap_top(b->sell);

        if((NULL != bid) && (NULL != quote) &&
            (((price_diff =
               fabsf(order_get_price(bid) - order_get_price(quote))) < FLT_EPSILON) ||
             (order_get_price(bid) > order_get_price(quote)))) {

            bid_quantity    = order_get_quantity(bid);
            quote_quantity  = order_get_quantity(quote);

            if(bid_quantity == quote_quantity) {
                /* Bid and quote orders can be completely filled */

                /* TODO Create new transaction record and log it */
                DBG("(%s:%d) Filled %lu of \"%s\" at price $%f\n",
                        __FUNCTION__, __LINE__, bid_quantity,
                        string_get_chars(b->symbol),
                        order_get_price(quote));

                b->orders_filled += 2;
                b->volume += bid_quantity;

                order_free((Order *)heap_pop(b->buy));
                order_free((Order *)heap_pop(b->sell));
            } else if(bid_quantity > quote_quantity) {
                /* Quote order can be completely filled */

                /* TODO Create new transaction record and log it */
                DBG("(%s:%d) Filled %lu of \"%s\" at price $%f\n",
                       __FUNCTION__, __LINE__, (bid_quantity - quote_quantity),
                        string_get_chars(b->symbol),
                        order_get_price(quote));

                b->orders_filled++;
                b->volume += quote_quantity;

                order_set_quantity(bid, (bid_quantity - quote_quantity));
                order_free((Order *)heap_pop(b->sell));
            } else {
                /* Bid order can be completely filled */

                /* TODO Create new transaction record and log it */
                DBG("(%s:%d) Filled %lu of \"%s\" at price $%f\n",
                        __FUNCTION__, __LINE__, (quote_quantity - bid_quantity),
                        string_get_chars(b->symbol),
                        order_get_price(quote));

                b->orders_filled++;
                b->volume += bid_quantity;

                order_set_quantity(quote, (quote_quantity - bid_quantity));
                order_free((Order *)heap_pop(b->buy));
            }
        } else {
            pthread_cond_wait(&b->matcher_cond, &b->matcher_mutex);
        }
    }

    pthread_mutex_unlock(&b->matcher_mutex);

    return NULL;
}

Book* book_open(const String *symbol)
{
    struct sched_param sched;
    pthread_attr_t attr;
    Book *new_book;

    assert(symbol != NULL);

    printf("Book: Open new book for: '%s'\n", string_get_chars(symbol));

    new_book = malloc(sizeof(struct _book));
    if(NULL == new_book) {
        fprintf(stderr, "(%s:%d) Out of memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    new_book->symbol = string_duplicate(symbol);
    new_book->orders_filled = 0;
    new_book->volume = 0;

    new_book->buy  = heap_create((CompareFn)_book_buy_compare);
    if(NULL == new_book->buy) {
        fprintf(stderr, "(%s:%d) Couldn't create buy heap\n",
                __FUNCTION__, __LINE__);
        free(new_book);
        return NULL;
    }

    new_book->sell = heap_create((CompareFn)_book_sell_compare);
    if(NULL == new_book->sell) {
        fprintf(stderr, "(%s:%d) Couldn't create sell heap\n",
                __FUNCTION__, __LINE__);
        free(new_book);
        return NULL;
    }

    new_book->book_is_open = 1;

    pthread_mutex_init(&new_book->matcher_mutex, NULL);
    pthread_cond_init(&new_book->matcher_cond, NULL);

    /* Create matcher thread with a high priority */
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_getschedparam(&attr, &sched);
    sched.sched_priority = sched_get_priority_max(SCHED_RR) - 1;
    assert(pthread_attr_setschedparam(&attr, &sched) == 0);

    assert(pthread_create(&new_book->matcher_thread,
                &attr, _book_fill_orders, new_book) == 0);

    return new_book;
}

void book_close(Book *b)
{
    assert(b != NULL);

    printf("Book: Closing book for: '%s'\n", string_get_chars(b->symbol));

    /* Tell the matcher thread that the book is closing */
    b->book_is_open = 0;
    pthread_cond_signal(&b->matcher_cond);

    pthread_join(b->matcher_thread, NULL);

    pthread_cond_destroy(&b->matcher_cond);
    pthread_mutex_destroy(&b->matcher_mutex);

    string_free(b->symbol);
    heap_free(b->buy);
    heap_free(b->sell);

    free(b);
}

int book_process_order(Book *b, Order *o)
{
    assert(b != NULL);
    assert(o != NULL);

    pthread_mutex_lock(&b->matcher_mutex);

    if(string_compare(order_get_symbol(o), b->symbol) != 0) {
        /* ERROR: Symbols don't match. Wrong book? */
        fprintf(stderr, "(%s:%d) Symbols don't match: Book=\"%s\" Order=\"%s\"\n",
                __FUNCTION__, __LINE__,
                string_get_chars(b->symbol),
                string_get_chars(order_get_symbol(o)));
        pthread_mutex_unlock(&b->matcher_mutex);
        return -1;
    }

    switch(order_get_type(o)) {
        case ORDER_TYPE_LIMIT:
            switch(order_get_side(o)) {
                case ORDER_SIDE_BUY:
                    DBG("Adding buy order\n");
                    heap_push(b->buy, o);
                    break;
                case ORDER_SIDE_SELL:
                    DBG("Adding sell order\n");
                    heap_push(b->sell, o);
                    break;
                default:
                    /* ERROR: Unknown order side */
                    fprintf(stderr, "Unknown order side\n");
                    pthread_mutex_unlock(&b->matcher_mutex);
                    return -1;
                    break;
            }

            /* Signal the matcher thread */
            pthread_cond_signal(&b->matcher_cond);
            break;

        case ORDER_TYPE_CANCEL:
        case ORDER_TYPE_REPLACE:
        default:
            /* ERROR: Unsupported order type */
            fprintf(stderr, "Unsupported order type\n");
            pthread_mutex_unlock(&b->matcher_mutex);
            return -1;
            break;
    }

    pthread_mutex_unlock(&b->matcher_mutex);

    return 0;
}

String* book_get_symbol(const Book *b)
{
    assert(b != NULL);

    return b->symbol;
}

unsigned long long book_get_volume(const Book *b)
{
    assert(b != NULL);

    return b->volume;
}

unsigned long book_get_orders_filled(const Book *b)
{
    assert(b != NULL);

    return b->orders_filled;
}
