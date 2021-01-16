/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE /* S_IFDIR */
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "test_empty.h"

#include "common.h"

enum expr_result
expr_test_empty_clb(const char *path, const char *UNUSED(name), const struct stat *st, const char *UNUSED(arg))
{
    if (st->st_mode & S_IFDIR) {
        DIR *dir;
        struct dirent *file;

        /* directory has always some size, so it is evaluated as empty if there are no files inside */

        dir = opendir(path);
        if (!dir) {
            return EXPR_FALSE;
        }

        while ((file = readdir(dir))) {
            if (!strcmp(".", file->d_name) || !strcmp("..", file->d_name)) {
                /* ignore . and .. */
                continue;
            }
            closedir(dir);
            return EXPR_FALSE;
        }
        closedir(dir);
    } else if (st->st_size) {
        return EXPR_FALSE;
    }

    return EXPR_TRUE;
}
