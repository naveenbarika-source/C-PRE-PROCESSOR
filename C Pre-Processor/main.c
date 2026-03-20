/*
 * main.c
 * ======
 * Entry point and I/O orchestration for mypreprocessor.
 *
 * Responsibilities
 * ----------------
 *  1. Parse command-line arguments and derive the output filename.
 *  2. Read the entire source file into a dynamic line-buffer (readfile).
 *  3. Orchestrate the three preprocessing passes in order:
 *       a) File inclusion   – inline #include <…> headers.
 *       b) Macro collection – gather all #define directives.
 *       c) Macro expansion  – replace macro names in every line.
 *       d) Comment removal  – strip // and block comments.
 *  4. Write the processed lines (excluding #include / #define lines)
 *     to the .i output file (write_on_prep).
 *
 * Usage
 * -----
 *   ./mypreprocessor inputfile.c
 *   -> produces inputfile.i in the current directory
 */

#include "preprocessor.h"

/* ------------------------------------------------------------------ */
/*  Global counters (extern-declared in preprocessor.h)               */
/* ------------------------------------------------------------------ */

/** Total number of lines currently stored in the line-buffer. */
int count = 0;

/** Number of lines accumulated from the most-recently included header. */
int store = 0;

/* ================================================================== */
/*  readfile                                                            */
/* ================================================================== */

/**
 * readfile – read every line of @source into a heap-allocated 2-D char
 *            array and return a pointer to it.
 *
 * The buffer grows by one row (MAX_LINE_LEN bytes) per line via realloc.
 * The global @count is incremented for each line stored.
 *
 * @source : path of the C source file to read.
 * @ptr    : initial pointer value (pass NULL on first call).
 * @return : reallocated pointer; the caller must not free intermediate
 *           values – only the final returned pointer should be freed.
 */
char (*readfile(const char *source, char (*ptr)[MAX_LINE_LEN]))[MAX_LINE_LEN]
{
    FILE *src = NULL;
    char  readline[MAX_LINE_LEN];

    src = fopen(source, "r");
    if (src == NULL) {
        fprintf(stderr, "Error: could not open source file '%s'\n", source);
        exit(EXIT_FAILURE);
    }

    /* Read one line at a time, growing the buffer on every iteration. */
    while (fgets(readline, MAX_LINE_LEN, src)) {
        ptr = realloc(ptr, (count + 1) * sizeof(*ptr));
        if (ptr == NULL) {
            fprintf(stderr, "Error: memory allocation failed in readfile\n");
            fclose(src);
            exit(EXIT_FAILURE);
        }
        strcpy(ptr[count++], readline);
    }

    fclose(src);
    return ptr;
}

/* ================================================================== */
/*  write_on_prep                                                       */
/* ================================================================== */

/**
 * write_on_prep – run all preprocessing passes then write the result.
 *
 * Pass order
 * ----------
 *  1. File inclusion  : #include lines are expanded into @prepi and
 *                       noted so they can be skipped later.
 *  2. Macro collection: every #define line is parsed and stored.
 *  3. Macro expansion : macro names in all remaining lines are replaced.
 *  4. Comment removal : // and block comments are stripped in-place.
 *  5. Output          : lines that are not #include / #define directives
 *                       are written to the .i file.
 *
 * @ptr  : line-buffer populated by readfile().
 * @prep : path of the .i output file.
 */
void write_on_prep(char (*ptr)[MAX_LINE_LEN], const char *prep)
{
    FILE    *prepi       = NULL;
    Macro_t  macros[MAX_MACROS];
    int      macro_count = 0;

    prepi = fopen(prep, "w");
    if (prepi == NULL) {
        fprintf(stderr, "Error: could not create output file '%s'\n", prep);
        exit(EXIT_FAILURE);
    }

    /* -------------------------------------------------------------- */
    /* Pass 1 – File inclusion                                         */
    /* Expand each #include <…> line by inlining the named header.    */
    /* -------------------------------------------------------------- */
    for (int i = 0; i < count; i++) {
        if (strstr(ptr[i], "#include<") || strstr(ptr[i], "#include <")) {
            process_includes(prepi, ptr[i]);
        }
    }

    /* -------------------------------------------------------------- */
    /* Pass 2 – Macro collection                                       */
    /* Parse every #define line and store name/body in macros[].      */
    /* -------------------------------------------------------------- */
    for (int i = 0; i < count; i++) {
        if (strstr(ptr[i], "#define")) {
            if (macro_count < MAX_MACROS) {
                collect_macro(ptr[i], macros, &macro_count);
            }
        }
    }

    /* -------------------------------------------------------------- */
    /* Pass 3 – Macro expansion                                        */
    /* Replace every macro name occurrence with its definition.       */
    /* -------------------------------------------------------------- */
    apply_macros(ptr, count, macros, macro_count);

    /* -------------------------------------------------------------- */
    /* Pass 4 – Comment removal                                        */
    /* Strip // single-line and block comments from the line-buffer.  */
    /* -------------------------------------------------------------- */
    remove_comments(ptr, &count);

    /* -------------------------------------------------------------- */
    /* Pass 5 – Write output                                           */
    /* Skip preprocessor directives; write everything else.           */
    /* -------------------------------------------------------------- */
    for (int i = 0; i < count; i++) {
        if (strstr(ptr[i], "#define"))  continue;
        if (strstr(ptr[i], "#include")) continue;
        fputs(ptr[i], prepi);
    }

    fclose(prepi);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

/**
 * main – validate arguments, build the output filename, then drive the
 *        two top-level operations: readfile and write_on_prep.
 *
 * Expected invocation:
 *   ./mypreprocessor inputfile.c
 *
 * The output file is derived by replacing the ".c" suffix of the input
 * filename with ".i".  If no ".c" suffix is found, ".i" is appended.
 */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source.c>\n", argv[0]);
        fprintf(stderr, "       Produces <source.i> in the current directory.\n");
        exit(EXIT_FAILURE);
    }

    /* Build output filename: replace trailing ".c" with ".i". */
    char outfile[300];
    strncpy(outfile, argv[1], sizeof(outfile) - 3);
    outfile[sizeof(outfile) - 3] = '\0';

    char *dot = strrchr(outfile, '.');
    if (dot && strcmp(dot, ".c") == 0) {
        strcpy(dot, ".i");          /* replace extension */
    } else {
        strcat(outfile, ".i");      /* append extension  */
    }

    /* Read the source file into the dynamic line-buffer. */
    char (*ptr)[MAX_LINE_LEN] = NULL;
    ptr = readfile(argv[1], ptr);

    /* Run all passes and write the .i file. */
    write_on_prep(ptr, outfile);

    printf("Preprocessing complete: '%s' -> '%s'\n", argv[1], outfile);

    free(ptr);
    return EXIT_SUCCESS;
}
