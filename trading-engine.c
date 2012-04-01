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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fix_server.h"
#include "fix_session_manager.h"

#include "market.h"

#define WAIT_SECONDS    5

static int done = 0;

void sigint_handler(int sig)
{
    done = 1;
}

int main(int argc, char *argv[])
{
    unsigned long long total_volume, last_volume;
    unsigned long long total_filled, last_filled;

    signal(SIGINT, sigint_handler);

    market_open();
    fix_session_manager_init();
    fix_server_init();

    total_volume = last_volume = 0;
    total_filled = last_filled = 0;

    while(!done) {
        total_volume = market_get_total_volume();
        total_filled = market_get_total_orders_filled();

        printf("Market total volume: %llu\n", total_volume);
        printf("Volume per second: %llu\n",
                (total_volume - last_volume) / WAIT_SECONDS);
        printf("Market total orders filled: %llu\n", total_filled);
        printf("Orders filled per second: %llu\n\n",
                (total_filled - last_filled) / WAIT_SECONDS);

        last_volume = total_volume;
        last_filled = total_filled;

        sleep(5);
    }

    fix_server_destroy();
    fix_session_manager_destroy();
    market_close();

    return 0;
}
