/*
 * Minimal xattr utility for Android OS
 *
 * Copyright 2017, Anki, Inc.
 * Brian Chapados <chapados@anki.com>
 *
 */

#include <errno.h>
#include <getopt.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   
#include <sys/types.h>
#include <sys/xattr.h>

#ifdef NDEBUG
#define debug(M, ...)                                                                               
#else                                                                                               
#define clean_errno() (errno == 0?"NULL":strerror(errno))                                           
#define debug(M, ...) fprintf(stderr,"[DEBUG] %s %d:" M "\n",__FILE__,__LINE__,##__VA_ARGS__)       
#endif


int cmd_listxattr(const char* path);
ssize_t cmd_getxattr(const char* path, const char* name);
int cmd_setxattr(const char* path, const char* name, const char* value, size_t size);

static const char* k_user_namespace = "user.";

void usage()
{
    printf("usage:\n");
    printf("txattr [-lnv] [file]\n");
    printf(" -l             list all extended attributes\n");
    printf(" -n             name of extended attribute\n");
    printf(" -v             value of extended attribute\n");
    printf("get attribute by passing -n and omitting -v\n");
}

static int verbose_flag = 0;

int main(int argc, char* argv[])
{
    char* path = NULL;

    int list_flag = 0;

    const struct option long_options[] = {
        { "list",       no_argument,            &list_flag,     'l' },
        { "name",       required_argument,      NULL,           'n' },
        { "value",      required_argument,      NULL,           'v' },
        { "verbose",    no_argument,            &verbose_flag,   0  },
        { NULL,         no_argument,            NULL,            0  }
    };

    enum command_name {
        CMD_LIST,
        CMD_GET,
        CMD_SET
    };
    typedef enum command_name command_name_t;

    if (argc < 2) {
        usage();
        return 1;
    }

    command_name_t cmd_name;
    char* opt_xattr_name = NULL;
    char* opt_xattr_val = NULL;

    int cmd_argc = argc;
    char** cmd_argv = &argv[0];

    int c;
    while(1) {
        int option_index = 0;
        c = getopt_long(cmd_argc, cmd_argv, "ln:v:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch(c) {
            case 'l':
            {
                list_flag = 1;
                break;
            }
            case 'n':
            {
                opt_xattr_name = optarg;
                break;
            }
            case 'v':
            {
                opt_xattr_val = optarg;
                break;
            }
        }
    }

    if (optind < cmd_argc) {
        path = cmd_argv[optind];
    } else {
        printf("invalid arguments: path required but not specified\n");
        usage();
        return 1;
    }
    debug("name: %s", opt_xattr_name);
    debug("val: %s", opt_xattr_val);
    debug("path: %s", path);

    if (opt_xattr_name && opt_xattr_val) {
        cmd_name = CMD_SET;
    } else if (opt_xattr_name && !opt_xattr_val) {
        cmd_name = CMD_GET;
    } else if (!opt_xattr_name && !opt_xattr_val) {
        cmd_name = CMD_LIST;
    }

    int r = 0;
    switch(cmd_name) {
        case CMD_LIST:
        {
            int err = cmd_listxattr(path);
            if (err != 0) {
                r = err;
            }
            break;
        }
        case CMD_GET:
        {
            int err = cmd_getxattr(path, opt_xattr_name);
            if (err < 0) {
                r = err;
            }
            break;
        }
        case CMD_SET:
        {
            int err = cmd_setxattr(path, opt_xattr_name, opt_xattr_val, strlen(opt_xattr_val));
            if (err < 0) {
                r = err;
            }
            break;
        }
    }

    return r;
}

bool has_prefix(const char *pre, const char *str)
{
    return ( strncmp(pre, str, strlen(pre)) == 0 );
}

ssize_t cmd_getxattr(const char* path, const char* name)
{
    ssize_t len = getxattr(path, name, NULL, 0);
    if (len <= 0) {
        if (errno != ENODATA) {
            // Do not print an error for missing entries
            perror("getxattr");
        }
        return len;
    }

    char* v = malloc((len+1) * sizeof(char));
    if (NULL == v) {
        perror("malloc");
        return -1;
    }

    ssize_t r = getxattr(path, name, v, len);
    if (r != len) {
        // attr value changed between calls
    }

    if (len > 0) {
        v[len] = 0;
        fprintf(stdout, "%s\n", v);
    }
    
    free(v);

    return r;
}

int cmd_setxattr(const char* path, const char* name, const char* value, size_t size)
{
    int r = setxattr(path, name, value, size, 0);
    if (r != 0) {
        if (!has_prefix(k_user_namespace, name)) {
            printf("[WARN] attribute names must start with the prefix: '%s'\n",
                    k_user_namespace);    
        }
        perror("setxattr");
    }
    return r;
}

// Based on example code from:
// http://man7.org/linux/man-pages/man2/listxattr.2.html
int cmd_listxattr(const char* path)
{
   ssize_t buflen, keylen, vallen;
   char *buf, *key, *val;

   /*
    * Determine the length of the buffer needed.
    */
   buflen = listxattr(path, NULL, 0);
   if (buflen == -1) {
       perror("listxattr");
       exit(EXIT_FAILURE);
   }
   if (buflen == 0) {
       if (verbose_flag) {
           printf("%s has no attributes.\n", path);
       }
       exit(EXIT_SUCCESS);
   }

   /*
    * Allocate the buffer.
    */
   buf = malloc(buflen);
   if (buf == NULL) {
       perror("malloc");
       exit(EXIT_FAILURE);
   }

   /*
    * Copy the list of attribute keys to the buffer.
    */
   buflen = listxattr(path, buf, buflen);
   if (buflen == -1) {
       perror("listxattr");
       exit(EXIT_FAILURE);
   }

   /*
    * Loop over the list of zero terminated strings with the
    * attribute keys. Use the remaining buffer length to determine
    * the end of the list.
    */
   key = buf;
   while (buflen > 0) {

       printf("%s: ", key);

       vallen = getxattr(path, key, NULL, 0);
       if (vallen == -1)
           perror("getxattr");

       if (vallen > 0) {

           val = malloc(vallen + 1);
           if (val == NULL) {
               perror("malloc");
               exit(EXIT_FAILURE);
           }

           vallen = getxattr(path, key, val, vallen);
           if (vallen == -1)
               perror("getxattr");
           else {
               val[vallen] = 0;
               printf("%s", val);
           }

           free(val);
       } else if (vallen == 0)
           printf("<no value>");

       printf("\n");

       keylen = strlen(key) + 1;
       buflen -= keylen;
       key += keylen;
   }

   free(buf);
   exit(EXIT_SUCCESS);
}
