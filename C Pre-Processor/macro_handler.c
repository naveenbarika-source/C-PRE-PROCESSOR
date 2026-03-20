/*
 * macro_handler.c
 * ===============
 * Implements collection and expansion of C preprocessor macros.
 *
 * Overview
 * --------
 * This module is responsible for two distinct operations:
 *
 *  1. collect_macro()
 *     Parses a "#define NAME BODY" (or "#define NAME(args) BODY") line
 *     and appends the resulting Macro_t record to the caller-supplied
 *     array.  Both object-like and function-like macros are stored as
 *     plain name→body pairs; no argument expansion is performed at
 *     collection time.
 *
 *  2. apply_macros()
 *     Iterates over every line in the line-buffer and replaces every
 *     occurrence of each collected macro name with its body text.
 *     Replacement is done using memmove/strncpy so that the line length
 *     is properly adjusted after each substitution.
 *
 * Macro name matching
 * -------------------
 * Matching is purely textual (strstr).  This means a macro named "MAX"
 * will also match inside "MAXIMUM"; for correctness in production code a
 * word-boundary check would be needed, but the current implementation
 * mirrors the behaviour of the original single-file preprocessor.
 *
 * Function-like macros
 * --------------------
 * If the #define line contains a '(' immediately after the name token,
 * collect_macro() includes everything up to and including the closing ')'
 * as part of the name (e.g. "SQUARE(x)").  The caller must ensure the
 * source code uses exactly the same syntactic form for substitution to
 * work.
 */

#include "preprocessor.h"

/* ================================================================== */
/*  collect_macro                                                       */
/* ================================================================== */

/**
 * collect_macro – parse one #define line and add it to @macros[].
 *
 * Tokenising strategy
 * -------------------
 *  Token 0 : "#define"  – discarded.
 *  Token 1 : macro name (and optional argument list for function macros).
 *  Token 2 : macro body (replacement text).
 *  Tokens 3+: part of a multi-token body – appended with a space.
 *
 * The trailing newline of the body, if present, is stripped before the
 * record is committed.
 *
 * @line        : the raw "#define …\n" line from the source file.
 * @macros      : array of Macro_t records; new entry appended at index
 *                *macro_count.
 * @macro_count : updated to reflect the newly added record.
 */
void collect_macro(const char *line, Macro_t *macros, int *macro_count)
{
    /* Work on a local mutable copy so we don't clobber the buffer. */
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';

    Macro_t *m   = &macros[*macro_count];
    m->name[0]   = '\0';
    m->def[0]    = '\0';

    int   flag = 0;           /* 0 = first token (#define), 1 = name, 2+ = body */
    char *word = strtok(buf, " \t");

    while (word != NULL) {
        if (flag == 0) {
            /* Skip the "#define" keyword itself. */
            flag = 1;
        } else if (flag == 1) {
            /* Second token is the macro name (may include "(args)"). */
            strncpy(m->name, word, MAX_MACRO_NAME - 1);
            m->name[MAX_MACRO_NAME - 1] = '\0';
            flag = 2;
        } else if (flag == 2) {
            /* Third token starts the replacement body. */
            strncpy(m->def, word, MAX_MACRO_DEF - 1);
            m->def[MAX_MACRO_DEF - 1] = '\0';
            flag = 3;
        } else {
            /* Subsequent tokens: append with a space (multi-token bodies). */
            strncat(m->def, " ", MAX_MACRO_DEF - strlen(m->def) - 1);
            strncat(m->def, word, MAX_MACRO_DEF - strlen(m->def) - 1);
        }
        word = strtok(NULL, " \t");
    }

    /* Strip any trailing newline from the body. */
    size_t def_len = strlen(m->def);
    if (def_len > 0 && m->def[def_len - 1] == '\n') {
        m->def[def_len - 1] = '\0';
    }

    /* Only register the macro if both name and body are non-empty. */
    if (m->name[0] != '\0' && m->def[0] != '\0') {
        (*macro_count)++;
    }
}

/* ================================================================== */
/*  apply_macros                                                        */
/* ================================================================== */

/**
 * apply_macros – substitute every macro name with its body in @ptr.
 *
 * For each line in the buffer, for each macro, the function repeatedly
 * calls strstr() to find occurrences and uses memmove()+strncpy() to
 * splice in the replacement text.  The loop continues until no further
 * occurrences of the current macro name are found on the current line.
 *
 * @ptr         : line-buffer to process in-place.
 * @lines       : total number of lines in @ptr.
 * @macros      : array of collected Macro_t records.
 * @macro_count : number of entries in @macros.
 */
void apply_macros(char (*ptr)[MAX_LINE_LEN], int lines,
                  const Macro_t *macros, int macro_count)
{
    for (int i = 0; i < lines; i++) {
        for (int m = 0; m < macro_count; m++) {
            const char *name    = macros[m].name;
            const char *def     = macros[m].def;
            long int    lold    = (long int)strlen(name);
            long int    lnew    = (long int)strlen(def);
            char       *p       = NULL;

            /*
             * Replace every occurrence of the macro name on this line.
             * After each replacement, advance @p past the newly inserted
             * body so we don't infinitely re-match our own substitution.
             */
            while ((p = strstr(ptr[i], name)) != NULL) {
                /*
                 * Shift the tail of the line left or right to make room
                 * for (or close the gap left by) the replacement text,
                 * then copy the replacement in.
                 */
                memmove(p + lnew, p + lold,
                        strlen(p + lold) + 1);
                strncpy(p, def, lnew);

                /* Advance past the replacement to avoid infinite loops. */
                p += lnew;

                /*
                 * Safety: if the resulting line is longer than the buffer
                 * allows, truncate it and break out.
                 */
                if (strlen(ptr[i]) >= MAX_LINE_LEN - 1) {
                    ptr[i][MAX_LINE_LEN - 1] = '\0';
                    break;
                }
            }
        }
    }
}
