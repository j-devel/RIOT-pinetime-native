/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

 /*
  * Copyright (C) 2020 j-devel <j@w3reality.com>
  *
  * Added support for board `native`.
  */

#include "log.h"
#include "xtimer.h"
#include "lvgl.h"
#include "controller.h"

#ifdef USE_BOARD_NATIVE
  #include "lvgl_sdl.h"
#else
  #include "hal.h"
  #include "bleman.h"
  #include "storage.h"
#endif

#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int lvgl_thread_create(void);

int main(void)
{
#if 0
    lvgl_sdl_simulator();
    return 0;
#endif

#ifndef USE_BOARD_NATIVE
    storage_init();
#endif
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    LOG_DEBUG("Starting PineTime application");
    lv_init();

#ifndef USE_BOARD_NATIVE
    hal_init();
#endif

    lvgl_thread_create();

#ifndef USE_BOARD_NATIVE
    bleman_thread_create();
#endif

    controller_thread_create();

#ifdef USE_BOARD_NATIVE
    lvgl_sdl_monitor_init("bosmoment native");
    lvgl_sdl_mouse_init();

    lvgl_sdl_memory_monitor(2000);
#endif

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
