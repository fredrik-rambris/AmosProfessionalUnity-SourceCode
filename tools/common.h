/* Shared helpers for the AMOS "library scanner" family of tools
 * (Make_Labels, Library_Digest, ...) - all of these are C ports of AMOS
 * Basic programs built around the same _SEARCH routine: load a source .s
 * file, walk it line by line looking for "Lib_" declaration keywords, and
 * emit "L_name: set N" label-equate lines plus a "Lib_Size equ N" count. */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stddef.h>

/* The Lib_ keywords the scanners hunt for, shared verbatim from the AMOS
 * source's LAB0$../LABSET2$ constants. */
extern const char *LAB0;      /* "Lib_"       - any line worth looking at */
extern const char *LAB1;      /* "Lib_Def" */
extern const char *LAB2;      /* "Lib_Int" */
extern const char *LAB3;      /* "Lib_Empty" */
extern const char *LAB4;      /* "Lib_Ext" */
extern const char *LAB5;      /* "Lib_Par" */
extern const char *LABSET1;   /* "Lib_Pos"    - reset the label counter */
extern const char *LABSET2;   /* "Lib_Ini"    - reset the label counter */

/* Closes every FILE* registered with register_close() (skipping NULLs),
 * clears them to NULL, then exit()s. Mirrors the AMOS programs' habit of
 * closing every open channel before quitting, on both success and error. */
void die(int code);

/* Registers *fp to be closed by die(). Pass the address of a FILE* that is
 * about to be opened (or is already NULL) - open_input_file()/
 * open_output_file() below call this for you. */
void register_close(FILE **fp);

/* Opens filename for reading, dies with an error message on failure.
 * *out is registered with die() before the open is attempted. */
void open_input_file(FILE **out, const char *filename);

/* Opens filename for writing, dies with an error message on failure, and
 * writes the standard AMOSPro tool banner (title + current local date and
 * time) at the top of the file. *out is registered with die() before the
 * open is attempted. */
void open_output_file(FILE **out, const char *filename, const char *title);

/* Reads the next whitespace-delimited token starting right after the
 * keyword match at 'ad' (skips the rest of the matched keyword, then any
 * separating whitespace) and copies it into out (truncated to fit, always
 * NUL-terminated). Returns the token's length, or -1 if the line ends
 * before a token is found (a "Parse error" in the AMOS source). */
int read_token(const char *ad, char *out, size_t outsz);

/* Writes one "L_name:<tabs>set\t<lab>\n" label line to f, padding with
 * tabs the same way the AMOS source does so the "set" columns line up. */
void write_label_line(FILE *f, const char *name, int lab);

/* Writes the "Lib_Size\t\tequ\t N\n" line (note: AMOS's Print reserves a
 * sign column for positive numbers here since NLAB is printed with Str$
 * directly, not stripped with Mid$). */
void write_lib_size_line(FILE *f, int count);

void *read_file(const char *filename, size_t *size);

#endif
