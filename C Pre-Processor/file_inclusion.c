/*
 * file_inclusion.c
 * ================
 * Implements expansion of #include <…> directives.
 *
 * How it works
 * ------------
 * When the main orchestration loop (write_on_prep in main.c) encounters
 * a line that begins with "#include<" or "#include <", it calls
 * process_includes() with the open output FILE* and the raw directive
 * line.
 *
 * process_includes() does the following:
 *  1. Extracts the header name from between the angle brackets.
 *  2. Constructs a path under /usr/include/ for that header.
 *  3. Opens the header file and reads it line-by-line into a temporary
 *     dynamic buffer (std[]).
 *  4. Strips comments from that buffer using remove_comments().
 *  5. Writes all resulting lines directly into the output .i file.
 *
 * Supported headers
 * -----------------
 * The current implementation resolves headers by prepending "/usr/include/"
 * to the name found in the angle brackets.  This covers all standard
 * C library headers available on a typical Linux system (stdio.h,
 * stdlib.h, string.h, math.h, …).
 *
 * Limitations
 * -----------
 *  - Only #include <…> (angle-bracket) form is handled.
 *  - User headers (#include "…") are not yet supported.
 *  - Nested includes inside the included file are not recursed into.
 */

#include "preprocessor.h"

/* ================================================================== */
/*  process_includes                                                    */
/* ================================================================== */

/**
 * process_includes – inline the contents of the header named in @line.
 *
 * @prepi : open output FILE* into which header content is written.
 * @line  : the raw "#include <header.h>" directive line from the source.
 */
void process_includes(FILE *prepi, const char *line)
{
    /* ---------------------------------------------------------------- */
    /* Step 1 – Extract the header filename from the angle brackets.    */
    /* ---------------------------------------------------------------- */
    const char *start = strchr(line, '<');
    const char *end   = strchr(line, '>');

    if (start == NULL || end == NULL || end <= start + 1) {
        /* Malformed directive – skip silently. */
        fprintf(stderr,
                "Warning: malformed #include directive, skipping: %s", line);
        return;
    }

    /* Copy the header name (e.g. "stdio.h") into a local buffer. */
    char header_name[128];
    int  name_len = (int)(end - start - 1);   /* length between < and > */
    if (name_len <= 0 || name_len >= (int)sizeof(header_name)) {
        fprintf(stderr, "Warning: header name too long, skipping: %s", line);
        return;
    }
    strncpy(header_name, start + 1, name_len);
    header_name[name_len] = '\0';

    /* ---------------------------------------------------------------- */
    /* Step 2 – Build the filesystem path to the header.               */
    /* ---------------------------------------------------------------- */
    char header_path[256];
    snprintf(header_path, sizeof(header_path),
             "/usr/include/%s", header_name);

    /* ---------------------------------------------------------------- */
    /* Step 3 – Open the header and read it into a temporary buffer.   */
    /* ---------------------------------------------------------------- */
    FILE *hdr_fp = fopen(header_path, "r");
    if (hdr_fp == NULL) {
        fprintf(stderr,
                "Warning: could not open header '%s' – skipping.\n",
                header_path);
        return;
    }

    /* Dynamically allocate a line-buffer for the header content.
       'store' (global) tracks the number of lines read.             */
    store = 0;
    char (*std)[MAX_LINE_LEN] = NULL;
    char  input[MAX_HDR_LINE];

    while (fgets(input, sizeof(input), hdr_fp)) {
        /* Grow the buffer by one row. */
        char (*tmp)[MAX_LINE_LEN] =
            realloc(std, (store + 1) * sizeof(*std));
        if (tmp == NULL) {
            fprintf(stderr,
                    "Error: out of memory while reading '%s'\n",
                    header_path);
            fclose(hdr_fp);
            free(std);
            exit(EXIT_FAILURE);
        }
        std = tmp;

        /* Truncate to MAX_LINE_LEN – 1 characters to fit the array. */
        strncpy(std[store], input, MAX_LINE_LEN - 1);
        std[store][MAX_LINE_LEN - 1] = '\0';
        store++;
    }
    fclose(hdr_fp);

    /* ---------------------------------------------------------------- */
    /* Step 4 – Strip comments from the header content.                */
    /* ---------------------------------------------------------------- */
    remove_comments(std, &store);

    /* ---------------------------------------------------------------- */
    /* Step 5 – Write processed header lines to the output file.       */
    /* ---------------------------------------------------------------- */
    for (int i = 0; i < store; i++) {
        fputs(std[i], prepi);
    }

    /* Release the temporary header buffer. */
    free(std);
}
