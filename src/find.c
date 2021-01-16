/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "cmdline.h"
#include "common.h"
#include "expressions.h"

/**
 * @brief Do correct stat according to the given symbolic links handling @p options.
 *
 * @param[in] filepath Path of the file to stat.
 * @param[in] options Options for handling symbolic links.
 * @param[in] explicit Flag if the given filepath was explicitly provided on command line.
 * @param[out] st Pointer to the stat structure to fill.
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
static int
find_stat(const char *filepath, int options, int explicit, struct stat *st)
{
    int rc;

    if ((options == EXPR_FOLLOW_SYMLINKS) ||
            (explicit && (options & EXPR_FOLLOW_EXPLICIT_SYMLINKS))) {
        rc = stat(filepath, st);
    } else {
        rc = lstat(filepath, st);
    }
    if (rc == -1) {
        LOG("unable to get file %s information (%s).", filepath, strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Stack for currently visited directories to be able to detect symlinks cycles.
 */
static struct find_dir_stack {
    ino_t inode;        /**< inode of the directory (not the symlink, the directory itself) */
    const char *name;   /**< name of the file (symlink, not the target directory) */
} *dir_stack = NULL;
/** @brief counter of the currently visited directory in the stack */
static unsigned int dir_stack_count = 0;
/** @brief allocated size of the directory stack */
static unsigned int dir_stack_size = 0;
/** @brief Step for reallocating directory stack */
#define DIR_STACK_STEP 8

/**
 * @brief Push a new directory record into the stack.
 *
 * @param[in] inode The inode number of the opened directory (dir itself, not the symlink pointing to it)
 * @param[in] name The name of the opened directory (symlink which is being opened, not necessary the dir itself)
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
static int
dir_stack_push(ino_t inode, const char *name)
{
    /* allocate, enough space in stack */
    if (dir_stack_count == dir_stack_size) {
        void *x = realloc(dir_stack, (dir_stack_size + DIR_STACK_STEP) * sizeof *dir_stack);
        if (!x) {
            LOG("%s", strerror(errno));
            return EXIT_FAILURE;
        }
        dir_stack = x;
        dir_stack_size += DIR_STACK_STEP;
    }
    /* insert new record */
    dir_stack[dir_stack_count].inode = inode;
    dir_stack[dir_stack_count].name = name;
    dir_stack_count++;

    return EXIT_SUCCESS;
}

/**
 * @brief Pop (throw out) the directory record from the stack.
 *
 * Frees the stack when empty.
 */
static void
dir_stack_pop(void)
{
    assert(dir_stack_count);

    dir_stack_count--;
    if (!dir_stack_count) {
        free(dir_stack);
        dir_stack = NULL;
        dir_stack_size = 0;
    }
}

/**
 * @brief Recursively evaluate expressions on files and subdirectories inside the given @p dir.
 *
 * @param[in] dir Directory to process.
 * @param[in] path Path of the directory being processed.
 * @param[in] options Options for handling symbolic links.
 * @param[in] expressions Tree for evaluating expressions.
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
static int
find_indir(DIR *dir, const char *path, int options, struct expr *expressions)
{
    int ret = EXIT_FAILURE;
    struct dirent *file;
    char *filepath = NULL;

    while ((file = readdir(dir))) {
        int rc;
        struct stat st;

        /* skip . and .. */
        if (!strcmp(".", file->d_name)) {
            continue;
        } else if (!strcmp("..", file->d_name)) {
            continue;
        }

        if (asprintf(&filepath, "%s%s%s", path, path[strlen(path) - 1] == '/' ? "" : "/", file->d_name) == -1) {
            LOG("unable to compound complete file path, asprintf() failed.");
            goto cleanup;
        }

        /* apply expressions on the file */
        if (find_stat(filepath, options, 0, &st)) {
            continue;
        }
        for (unsigned int i = 0; i < dir_stack_count; i++) {
            if (st.st_ino == dir_stack[dir_stack_count - 1].inode) {
                LOG("File system loop detected; '%s' is part of the same file system loop as '%s'.",
                    filepath, dir_stack[dir_stack_count - 1].name);
                goto next_dirent;
            }
        }
        expr_eval(filepath, file->d_name, &st, expressions);

        if (st.st_mode & S_IFDIR) {
            /* go recursively into directory */
            DIR *subdir;

            subdir = opendir(filepath);
            if (!subdir) {
                return EXIT_FAILURE;
            }

            dir_stack_push(st.st_ino, filepath);
            rc = find_indir(subdir, filepath, options, expressions);
            dir_stack_pop();
            closedir(subdir);

            if (rc) {
                goto cleanup;
            }
        }
next_dirent:
        free(filepath);
        filepath = NULL;
    }

    ret = EXIT_SUCCESS;

cleanup:
    free(filepath);
    return ret;
}

/**
 * @brief Do the main job of find - filter files in paths and do actions.
 *
 * @param[in] paths List of paths where to search.
 * @param[in] options Options for handling symbolic links.
 * @param[in] expressions Tree for evaluating expressions.
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
static int
find(const char **paths, int options, struct expr *expressions)
{
    int rc;
    DIR *dir;

    for (unsigned int i = 0; paths[i]; i++) {
        struct stat st;

        /* evaluate expressions on the path itself */
        if (find_stat(paths[i], options, 1, &st)) {
            continue;
        }
        expr_eval(paths[i], basename(paths[i]), &st, expressions);

        if (st.st_mode & S_IFDIR) {
            dir = opendir(paths[i]);
            if (!dir) {
                /* not accessible */
                continue;
            }

            dir_stack_push(st.st_ino, paths[i]);

            /* evaluate expressions on files and subdirectories inside the directory */
            rc = find_indir(dir, paths[i], options, expressions);
            dir_stack_pop();
            closedir(dir);
            if (rc) {
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    int argpos = 1; /* skip program name */
    int options;
    const char **paths = NULL;
    struct expr *expressions = NULL;

    /* parse -H, -L, -P options */
    if (parse_options(argc, argv, &argpos, &options)) {
        goto cleanup;
    }

    /* get paths */
    if (parse_paths(argc, argv, &argpos, &paths)) {
        goto cleanup;
    }

    /* parse expressions */
    if (parse_expressions(argc, argv, &argpos, &expressions)) {
        goto cleanup;
    }

    /* process the files */
    if (find(paths, options, expressions)) {
        goto cleanup;
    }

    ret = EXIT_SUCCESS;
cleanup:
    /* cleanup */
    free(paths);
    expr_free(expressions);

    return ret;
}
