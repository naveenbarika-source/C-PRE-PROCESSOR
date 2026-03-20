/*
 * preprocessor.h
 * ==============
 * Central header file for the mypreprocessor project.
 *
 * This header declares all shared types, constants, and function
 * prototypes used across the modular source files:
 *   - comment_removal.c
 *   - file_inclusion.c
 *   - macro_handler.c
 *   - main.c
 *
 * All modules include this file to access the shared line-buffer
 * type and the global counters that track its size.
 */

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */

/** Maximum characters on a single source line (including NUL). */
#define MAX_LINE_LEN 200

/** Maximum characters when reading a header-file line. */
#define MAX_HDR_LINE 300

/** Maximum length of a macro name  (e.g. "PI", "SQUARE(x)"). */
#define MAX_MACRO_NAME 64

/** Maximum length of a macro replacement body. */
#define MAX_MACRO_DEF 128

/** Maximum number of #define macros supported per translation unit. */
#define MAX_MACROS 64

/* ------------------------------------------------------------------ */
/*  Global counters (defined in main.c, extern-declared here)          */
/* ------------------------------------------------------------------ */

/** count – number of lines currently in the dynamic line-buffer. */
extern int count;

/** store – number of lines accumulated from an included header. */
extern int store;

/* ------------------------------------------------------------------ */
/*  Macro record                                                        */
/* ------------------------------------------------------------------ */

/**
 * Macro_t – one #define entry.
 *
 * name : the identifier being defined  (e.g. "PI", "SQUARE(x)")
 * def  : the replacement text           (e.g. "3.14", "((x)*(x))")
 */
typedef struct {
    char name[MAX_MACRO_NAME];
    char def [MAX_MACRO_DEF];
} Macro_t;

/* ------------------------------------------------------------------ */
/*  Function prototypes                                                 */
/* ------------------------------------------------------------------ */

/* main.c / file I/O helpers */
char (*readfile(const char *source, char (*ptr)[MAX_LINE_LEN]))[MAX_LINE_LEN];
void  write_on_prep(char (*ptr)[MAX_LINE_LEN], const char *prep);

/* comment_removal.c */
void remove_comments  (char (*ptr)[MAX_LINE_LEN], int *lines);
void remove_line_range(char (*ptr)[MAX_LINE_LEN], int i, int j, int *lines);

/* file_inclusion.c */
void process_includes(FILE *prepi, const char *line);

/* macro_handler.c */
void collect_macro(const char *line, Macro_t *macros, int *macro_count);
void apply_macros  (char (*ptr)[MAX_LINE_LEN], int lines,
                    const Macro_t *macros, int macro_count);

#endif /* PREPROCESSOR_H */
