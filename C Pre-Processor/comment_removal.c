/*
 * comment_removal.c
 * =================
 * Implements in-place removal of C-style comments from the line-buffer.
 *
 * Supported comment forms
 * -----------------------
 *  //  Single-line comments - everything from "//" to the end of the
 *      physical line is erased, UNLESS a semicolon appears before the
 *      "//" on the same line (heuristic to keep "http://" in strings).
 *
 *  Block comments (slash-star ... star-slash) - may span one or more lines.
 *      The entire comment is excised; any code on the same line before the
 *      opening or after the closing delimiter is preserved.
 *
 * Design notes
 * ------------
 *  - All modifications are done in-place on the (*ptr)[MAX_LINE_LEN]
 *    array that was originally allocated by readfile().
 *  - The global @count / caller-supplied *lines value is decremented
 *    whenever one or more lines are physically removed.
 *  - remove_line_range() is the low-level helper used by the block-
 *    comment logic; it can also be called from other modules if needed.
 */

#include "preprocessor.h"

/* ================================================================== */
/*  remove_line_range                                                   */
/* ================================================================== */

/**
 * remove_line_range – delete lines [i..j] from the buffer, keeping any
 *                     code that follows the closing "* /" on line j.
 *
 * Algorithm
 * ---------
 *  1. Locate the closing "*\/" on line j.
 *  2. If there is non-whitespace text after "*\/", move it to line i
 *     so it is not lost, and decrement j (line j is now "consumed").
 *  3. Shift all lines after j down by (j - i + 1) positions using
 *     memmove, repeating once per removed line, and decrement *lines.
 *
 * @ptr   : the line-buffer array.
 * @i     : index of the first line to remove (the one with the opening delimiter).
 * @j     : index of the last  line to remove (the one with the closing delimiter).
 * @lines : pointer to the current line count; decremented accordingly.
 */
void remove_line_range(char (*ptr)[MAX_LINE_LEN], int i, int j, int *lines)
{
    int num = i;

    /* Locate the closing delimiter on line j. */
    char *last = strstr(ptr[j], "*/");
    if (last == NULL) {
        /* Malformed: no closing delimiter found – remove up to j anyway. */
        last = ptr[j] + strlen(ptr[j]);
    } else {
        last += 2; /* step past the closing delimiter */
    }

    /* If there is remaining code after the close delimiter, preserve it on line j. */
    char *end_of_j = ptr[j] + strlen(ptr[j]);
    if (last < end_of_j) {
        /* Move trailing content to the start of line j; adjust j so
           that line j is kept (we only remove lines i..j-1). */
        memmove(ptr[j], last, strlen(last) + 1);
        j--;
    }

    /* Shift lines [j+1 .. *lines-1] left to fill the gap [i .. j]. */
    while (num <= j) {
        memmove(ptr + i, ptr + i + 1, (*lines - i - 1) * sizeof(*ptr));
        (*lines)--;
        num++;
    }
}

/* ================================================================== */
/*  remove_comments                                                     */
/* ================================================================== */

/**
 * remove_comments – main entry point: strip all comments from @ptr.
 *
 * The function makes two sub-passes in a single loop iteration per line:
 *
 *  Sub-pass A – single-line comments (//)
 *    Find the first "//" on the line.  If no ";" appears before it
 *    (heuristic), erase from "//" to the end of the line (keeping
 *    the newline character if one exists).
 *
 *  Sub-pass B - block comments (slash-star ... star-slash)
 *    Find the opening delimiter on the current line. If no matching closing
 *    delimiter is on the same line, scan forward through subsequent lines.
 *    Then call remove_line_range() to collapse those lines.
 *    If the closing delimiter is on the same line, erase between the two.
 *
 * @ptr   : line-buffer to process in-place.
 * @lines : pointer to the current line count (may decrease).
 */
void remove_comments(char (*ptr)[MAX_LINE_LEN], int *lines)
{
    for (int i = 0; i < *lines; i++) {

        /* ---------------------------------------------------------- */
        /* Sub-pass A: single-line comment "//"                        */
        /* ---------------------------------------------------------- */
        char *comment = strstr(ptr[i], "//");
        if (comment != NULL) {
            /* Only remove the comment if no semicolon precedes it on
               this line (avoids wrongly cutting "http://...").        */
            char *semicolon = strstr(ptr[i], ";");
            if (!(semicolon != NULL && semicolon < comment)) {
                /* Preserve the trailing newline character if present. */
                char *newline = strrchr(comment, '\n');
                if (newline != NULL) {
                    memmove(comment, newline, strlen(newline) + 1);
                } else {
                    *comment = '\0';
                }
            }
        }

        /* ---------------------------------------------------------- */
        /* Sub-pass B: block comments (opening to closing delimiter)        */
        /* ---------------------------------------------------------- */
        comment = strstr(ptr[i], "/*");
        if (comment != NULL) {
            int   j    = i;
            char *last = NULL;

            /* Search for the closing "*\/" starting from line i. */
            while (j < *lines) {
                last = strstr(ptr[j], "*/");
                if (last != NULL) break;
                j++;
            }

            if (last == NULL) {
                /* Unclosed comment – erase to end of line i and stop. */
                char *newline = strrchr(comment, '\n');
                if (newline != NULL)
                    memmove(comment, newline, strlen(newline) + 1);
                else
                    *comment = '\0';
            } else if (j == i) {
                /* Opening and closing on the SAME line: erase in-place. */
                char *after_close = last + 2; /* character after "*\/" */
                memmove(comment, after_close,
                        strlen(after_close) + 1);
            } else {
                /* Block spans multiple lines: erase from opening delimiter on line i
                   and let remove_line_range handle the rest.           */
                char *newline = strrchr(comment, '\n');
                if (newline != NULL)
                    memmove(comment, newline, strlen(newline) + 1);
                else
                    *comment = '\0';

                /* Remove lines (i+1)..j, preserving trailing code on j. */
                remove_line_range(ptr, i + 1, j, lines);

                /* Re-examine line i (it may now start another comment). */
                i--;
            }
        }
    }
}
