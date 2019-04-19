#include "assert.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 
// get html
// include the actual C file here
#include "doc_text.c"

//
// assume html_array contains standard html
// emit directly to stdout
void render_docs(void)
{
    fprintf(stdout, "%s", html_array);
}

/*
int main(void)
{
    render_docs();
    return 0;
}
*/
