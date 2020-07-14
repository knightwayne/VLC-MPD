/* Compile this sample with:
 * gcc/clang string2func.c -Wall -Werror -std=gnu11
 * (-std=gnu11 needed for getline)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* context shared between all commands */
//context sharing ->ctx, variable for sharing 
struct ctx
{
    int somecontext;
    bool quit;
};

/* struct definition for one element, matching a string to a function */
// this struct declaration is for handling what is to be done by my read command, has a string and a function declaration
struct strfunc
{
    const char *str;
    int (*func)(struct ctx *);
};

static int cmd1(struct ctx *ctx)
{
    fprintf(stdout, "executing cmd1 (%d)\n", ctx->somecontext);
    return 0;
}

static int cmd2(struct ctx *ctx)
{
    fprintf(stdout, "executing cmd2 (%d)\n", ctx->somecontext);
    return 0;
}

static int cmd3(struct ctx *ctx)
{
    fprintf(stdout, "executing cmd3 (%d)\n", ctx->somecontext);
    return 0;
}

static int quit(struct ctx *ctx)
{
    fprintf(stdout, "quit received, terminating...\n");
    ctx->quit = true;
    return 0;
}

static int err(struct ctx *ctx)
{
    (void) ctx;
    return -1;
}

/* static struct list, matching every comand to a function */
static const struct strfunc strfunc_list[] =
{
    /* strs should be in alphabetical order, cf. man bsearch */
    { "cmd1", cmd1 },
    { "cmd2", cmd2 },
    { "cmd3", cmd3 },
    { "err", err },
    { "quit", quit },
};

/* bsearch callback, cf. man bsearch */
static int strfunc_cmp(const void *key, const void *other)
{
    const struct strfunc *strfunc = other;
    return strcmp(key, strfunc->str/*each list item's string binary search*/);
}

//
struct strfunc * strfunc_search(const char *str)
{
    return bsearch(str/*key*/, strfunc_list, ARRAY_SIZE(strfunc_list),sizeof(*strfunc_list), strfunc_cmp);
}

int main()
{
    //struct declared for context sharing
    struct ctx ctx = {
        .somecontext = 42,
        .quit = false,
    };

    //line reading
    char *linebuf = NULL;
    size_t linesize = 0;

    /* Safer way to read a line since this function handle buffer allocation. */

    //line reading continous loop
    while (!ctx.quit && getline(&linebuf, &linesize, stdin) > 0)
    {
        /* remove '\n' */
        linebuf[strlen(linebuf) - 1] = '\0';

        //search my command using binary search
        struct strfunc *strfunc = strfunc_search(linebuf);
        //when this is returned, strfunc has str, and *func both filled with correct value

        //when command not found
        if (strfunc == NULL)
        {
            fprintf(stderr, "command: '%s' not found\n", linebuf);
            continue;
        }

        //printf("Return Value %s",strfunc->str);
        //accessing pointer struct using -> and passing a ctx struct in the function
        int ret = strfunc->func(&ctx);//------------------------------------------------------------------>what does it do?
        //if strfunc->func has some error
        if (ret != 0)
        {
            //clear linebuf and returning ret, this will exit the main;
            free(linebuf);
            return ret;
        }
        //continue
    }

    /* The buffer used by getline need to be freed even in case of error. */
    free(linebuf);

    return 0;
}
