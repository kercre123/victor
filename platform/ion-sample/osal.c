/* Copyright (c) 2011-2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include "osal.h"
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

OSAL_API int clst_printf(const char *format, ...)
{
    int     chars_written = 0;
    va_list args;

    va_start(args, format);
    chars_written = vprintf(format, args);
    va_end(args);

    return chars_written;
}

OSAL_API int clst_sprintf(char *dest, const char *format, ...)
{
    int     chars_written = 0;
    va_list args;

    va_start(args, format);
    chars_written = vsprintf(dest, format, args);
    va_end(args);

    return chars_written;
}

OSAL_API int clst_snprintf(char *dest, size_t count, const char *format, ...)
{
    int     chars_written = 0;
    va_list args;

    va_start(args, format);
    chars_written = vsnprintf(dest, count, format, args);
    va_end(args);

    return chars_written;
}

OSAL_API size_t clst_strlen(const char *str)
{
    return strlen(str);
}

OSAL_API void* clst_calloc(unsigned num_bytes)
{
    return calloc(1, num_bytes);
}

OSAL_API void* clst_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

OSAL_API void* clst_memcpy(void *s1, const void *s2, size_t n)
{
    return memcpy(s1, s2, n);
}

OSAL_API int clst_memcmp(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

OSAL_API void  clst_free(void* ptr)
{
    free(ptr);
}

OSAL_API long64 clst_clock_start(void)
{
    struct timeval start;
    long64 result;
    gettimeofday(&start, NULL);
    result = (long64)(1000000 * start.tv_sec + start.tv_usec);
    return result;
}

OSAL_API double clst_clock_diff(long64 start)
{
    // First measure the time
    struct timeval current, compensation;
    long64 useconds_diff, comp, comp_start;
    gettimeofday(&current, NULL);
    useconds_diff = (long64)(1000000 * current.tv_sec + current.tv_usec) - start;

    // Then collect overhead time
    comp_start = clst_clock_start();
    gettimeofday(&compensation, NULL);
    comp = (long64)(1000000 * compensation.tv_sec + compensation.tv_usec) - comp_start;

    // Compensate
    if (useconds_diff > comp)
    {
        useconds_diff -= comp;
    }

    // Convert & return
    double seconds_diff = (double)useconds_diff / (double)1000000;
    return seconds_diff;
}

OSAL_API void clst_usleep(unsigned useconds)
{
#ifdef USE_BUSY_WAIT
    struct timeval now, start;
    gettimeofday(&start, NULL);
    do
    {
        gettimeofday(&now, NULL);
        useconds_diff = (long64)(now.tv_sec - start.tv_sec) * 1000000;
        useconds_diff += now.tv_usec - start.tv_usec;
    }
    while (useconds_diff < useconds);
#else
    usleep (useconds);
#endif
}

OSAL_API void clst_set_console_color(clst_color color)
{
#if 0
// On Android, these control characters go via the console
// and mess-up the user's color scheme.
// The test does not return the colors to it's original
// scheme since Android can't query the remote terminal
// for it's color scheme.
// So keep it simple and don't change colors

    const char *foreground_red_bold = "\033[31;1m";
    const char *foreground_green_bold = "\033[32;1m";
    const char *foreground_gray_nobold = "\033[37;22m";

    switch(color)
    {
    default:
    case CLST_COLOR_DEFAULT:
        printf("%s", foreground_gray_nobold);
        break;
    case CLST_COLOR_RED:
        printf("%s", foreground_red_bold);
        break;
    case CLST_COLOR_GREEN:
        printf("%s", foreground_green_bold);
        break;
    }
#endif
}

OSAL_API void clst_get_filelist(char* find_path, char* file_extension, filelist* list_head)
{
    DIR* dir;
    struct dirent* ent;
    struct stat buf;
    filelist current = NULL;
    char namebuf[FULLPATH_BUFFER_SIZE];
    char pathwithslash[FULLPATH_BUFFER_SIZE];
    size_t base_len = strlen(find_path);

    //Append trailing foreslash if necessary
    if(find_path[base_len - 1] != '/')
    {
        snprintf(pathwithslash, FULLPATH_BUFFER_SIZE, "%s/", find_path);
    }else{
        snprintf(pathwithslash, FULLPATH_BUFFER_SIZE, "%s", find_path);
    }

    //If the path is missing, return
    dir = opendir(pathwithslash);
    if (!dir)
    {
        return;
    }

    //Continue finding files and appending
    ent = readdir(dir);
    while (ent != NULL)
    {
        // ignore .. and .
        if (!strcmp(ent->d_name, "..") || !strcmp(ent->d_name, "."))
        {
            ent = readdir(dir);
            continue;
        }

        //Fill namebuf with the full path
        snprintf(namebuf, FULLPATH_BUFFER_SIZE, "%s%s", pathwithslash, ent->d_name);

        //Get stats of the file, put stats in buf
        if (lstat(namebuf, &buf) != 0)
        {
            perror(namebuf);
            ent = readdir(dir);
            continue;
        }

        // If it's a file, potentially add to list
        if (S_ISREG(buf.st_mode))
        {
            if (strchr(ent->d_name, '.') && (0 == strcmp(strchr(ent->d_name, '.') + 1, file_extension)))
            {
                current = (filelist)clst_calloc(sizeof(struct _list_of_files_node));
                strncpy(current->path, namebuf, FULLPATH_BUFFER_SIZE);
                strncpy(current->filename, ent->d_name, FULLPATH_BUFFER_SIZE);
                current->next = *(list_head);
                *(list_head) = current;
            }
        }

        // If it's a directory, recurse
        else if (S_ISDIR(buf.st_mode))
        {
            clst_get_filelist(namebuf, file_extension, list_head);
        }

        ent = readdir(dir);
    }

    closedir(dir);
}

OSAL_API void clst_read_file(char* path, char** data)
{
    FILE*   file = NULL;
    size_t  result = 0;
    size_t size;

    *data = NULL;
    file = fopen(path, "rb");

    if (file == NULL)// error opening file
    {
        goto cleanup;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    *data = (char*)clst_calloc(size + 1);
    if(*data == NULL)
    {
        goto cleanup;
    }

    result = fread(*data, 1, size, file);
    if (result != size)// error reading file
    {
        perror("fread error");
        free(*data);
        *data = NULL;
        goto cleanup;
    }
    else
    {
        (*data)[size] = '\0';
    }

cleanup:
    fclose(file);
}

OSAL_API  int clst_thread_create(void **thread_handle,
                                 int  stack_size,        // 0 = default stack size
                                 clst_thread_func_type func,
                                 void *args)
{
    pthread_t *newthread = (pthread_t*)clst_calloc(sizeof(thread_handle));
    if (!newthread)
    {
        goto failed;
    }

    if (pthread_create(newthread, 0, (void *)func, (void*)args))
    {
        goto failed;
    }

    *thread_handle = (void *)newthread;
    return 0;

failed:
    clst_free(newthread);
    *thread_handle = NULL;
    return -1;
}


OSAL_API int clst_thread_terminate(void * thread_handle)
{
    if (!thread_handle) return -1;

    pthread_exit(NULL);

    return 0;
}


OSAL_API void clst_thread_join(void *thread_handle)
{
    if (!thread_handle) return;

    pthread_t* join_thread = (pthread_t*)thread_handle;

    pthread_join(*join_thread, NULL);
}

OSAL_API int clst_abs(int number)
{
    return abs(number);
}
