/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>

/**
 * @brief macro to cover unused arguments (usually in callbacks)
 */
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))

/**
 * @brief application identifier
 */
#define FIND_ID "rfind"

/**
 * @brief Logging macro
 * @param[in] FORMAT formatting string
 */
#define LOG(FORMAT, ...) \
    fprintf(stderr, FIND_ID ": " FORMAT "\n", ##__VA_ARGS__)

#endif /* _COMMON_H */
