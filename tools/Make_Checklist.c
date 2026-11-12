/* C port of c/Make_Checklist.Asc - walks the token tables of the main
 * instruction set plus the extension libraries and writes out a sorted,
 * one-line-per-instruction checklist ("Instruction_List.Asc") showing which
 * instructions still need attention.
 *
 * Each token table entry is a pair of lines between the file's "TOKEN_START"
 * and "TOKEN_END" markers:
 *
 *     dc.w    L_Something,L_FnSomething      // ...
 *     dc.b    "keyword","x"+$80,"flags",-1
 *
 * The first quoted string on the dc.b line is the instruction's keyword;
 * the flag letters quoted after it say what kind of token it is - "O"
 * marks a plain operator/punctuation token (not a real instruction, and
 * skipped entirely), "I" marks an indirect-call token, "0"/"1"/"2" mark a
 * function token, "V" marks a variable-style token, and none of those
 * marks a plain statement token. Whether the L_ names on the dc.w line are
 * both "L_Nul" says whether the instruction is still unimplemented.
 *
 * The original's output prefixes were the French abbreviations "OK "
 * (already implemented) and "AF " (a faire - still to do); this port keeps
 * "OK " (already meaningful in English) but spells "AF " out as "TODO ".
 * All the console/report text is translated to English as requested; the
 * parsing logic itself is otherwise a straight, line-for-line port.
 *
 * Dropped from the original: the CLI-window setup, the live "\r"-updating
 * progress counter (replaced with a plain per-file summary line - the
 * original's carriage-return trick doesn't buy anything in a batch tool's
 * captured output), and the second half of the AMOS source (sorting
 * instructions into "already tested" / "to test" files against
 * Instructions_Testees.Asc / Instructions_ATester.Asc). That second half
 * sits after an unconditional `End` in the original and can never run -
 * it's dead code, so it isn't ported. The unused _DATE$/_TIME$/_BREAK/
 * _EXTRACT helpers (only there for the CLI console and the dead code) are
 * dropped too.
 *
 * Usage: Make_Checklist <output> <source.s>... - the original hardcoded
 * both the output name and the list of sources. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"

#define MAX_INSTRUCTIONS 4096

static char *instructions[MAX_INSTRUCTIONS];
static int ninst = 0;
static int nfait = 0;

FILE *in = NULL;

/* Uppercases a NUL-terminated string in place. */
static void upper(char *s)
{
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

/* Parses one dc.b line, mirroring the AMOS source's Instr$/Mid$ dance, and
 * returns the instruction's raw keyword (before the OK/TODO/prefix stage)
 * in out, or an empty string if the line has no quoted keyword at all. */
static void extract_keyword(const char *row, char *out, size_t outsz)
{
    const char *p, *close;
    size_t len;

    out[0] = '\0';

    p = strchr(row, '"');
    if (!p) return;

    close = strchr(p + 1, '"');
    if (!close) return;

    len = (size_t)(close - (p + 1));
    if (len >= outsz) len = outsz - 1;
    memcpy(out, p + 1, len);
    out[len] = '\0';

    /* The character right after the closing quote: if it isn't the "+" of
     * a bare "+$80" (no second quoted literal), one more character is
     * pulled from inside the second quoted literal - e.g. given
     * "arra","y"+$80,"03",-1, close[1] is the "," separator, close[2] is
     * the second literal's opening quote, and close[3] is the "y" that
     * completes the keyword to "array". Given "to"," "+$80,-1 the same
     * offsets land on the " " that completes "to" to "to ". */
    if (close[1] && close[1] != '+' && close[2] && close[3])
    {
        size_t klen = strlen(out);
        if (klen + 1 < outsz)
        {
            out[klen] = close[3];
            out[klen + 1] = '\0';
        }
    }
}

/* Prepends prefix to s in place (s must have room). */
static void prepend(char *s, const char *prefix)
{
    size_t plen = strlen(prefix), slen = strlen(s);
    memmove(s + plen, s, slen + 1);
    memcpy(s, prefix, plen);
}

static void scan_file(const char *filename)
{
    char row[512], t2[512];
    char keyword[512], entry[600];
    char upper_row[512];
    const char *dcb;
    int found_token_start = 0;
    int is_indirect, is_func, is_var, is_operator;

    printf("File %s\n", filename);
    open_input_file(&in, filename);

    while (fgets(row, sizeof(row), in) != NULL)
    {
        if (strstr(row, "TOKEN_START") != NULL)
        {
            found_token_start = 1;
            break;
        }
    }
    if (!found_token_start)
    {
        printf("String hunt aborted!\n");
        die(1);
    }

    while (fgets(row, sizeof(row), in) != NULL)
    {
        if (strstr(row, "TOKEN_END") != NULL) break;

        if (strstr(row, "dc.w") == NULL) continue;

        /* the paired dc.b line - if it's missing (truncated file), just
         * stop scanning this file rather than treating it as an error,
         * same as the original's bare "Exit If Errtrap" here */
        if (fgets(t2, sizeof(t2), in) == NULL) break;

        dcb = strstr(t2, "dc.b");
        if (dcb == NULL) continue;

        /* skip pure-numeric entries like "dc.b $80,-1" - no keyword */
        if (dcb[5] == '$') continue;

        extract_keyword(t2, keyword, sizeof(keyword));
        if (keyword[0] == '\0') continue;

        is_operator = strstr(t2, "\"O") != NULL;
        if (is_operator) continue;

        is_indirect = strstr(t2, "\"I") != NULL;
        is_func = strstr(t2, "\"0") != NULL || strstr(t2, "\"1") != NULL || strstr(t2, "\"2") != NULL;
        is_var = strstr(t2, "\"V") != NULL;

        if (keyword[0] == '!') memmove(keyword, keyword + 1, strlen(keyword));

        if (is_func) prepend(keyword, "= ");
        if (is_indirect) prepend(keyword, "i ");
        if (is_var) prepend(keyword, "v ");
        if (!is_func && !is_indirect && !is_var) prepend(keyword, "s ");

        snprintf(upper_row, sizeof(upper_row), "%s", row);
        upper(upper_row);
        if (strstr(upper_row, "L_NUL,L_NUL") != NULL)
        {
            prepend(keyword, "TODO ");
        }
        else
        {
            prepend(keyword, "OK ");
            nfait++;
        }

        {
            size_t klen = strlen(keyword);
            if (klen && keyword[klen - 1] == ' ') keyword[klen - 1] = '\0';
        }

        if (ninst >= MAX_INSTRUCTIONS)
        {
            printf("Too many instructions (max %d)\n", MAX_INSTRUCTIONS);
            die(1);
        }

        snprintf(entry, sizeof(entry), "%s", keyword);
        instructions[ninst] = strdup(entry);
        ninst++;
    }

    fclose(in);
    in = NULL;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(int argc, char **argv)
{
    const char *out_name;
    FILE *out;
    char dt[100];
    time_t t;
    int i;

    if (argc < 3)
    {
        printf("Usage: %s <output> <source.s>...\n", argv[0]);
        return 1;
    }
    out_name = argv[1];

    for (i = 2; i < argc; i++)
    {
        scan_file(argv[i]);
    }

    printf("Total number of instructions: %d\n", ninst);
    printf("Instructions already implemented: %d\n", nfait);

    qsort(instructions, (size_t)ninst, sizeof(char *), cmp_str);

    time(&t);
    strftime(dt, sizeof(dt), "%d-%m-%Y %H:%M:%S", localtime(&t));

    printf("Writing %s\n", out_name);
    if ((out = fopen(out_name, "wb")) == NULL)
    {
        printf("Could not open file '%s' for output\n", out_name);
        return 1;
    }

    fprintf(out, "' ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    fprintf(out, "' Amos Professional Unity %d instructions on the %s\n", ninst, dt);
    fprintf(out, "' %d%% complete...\n", ninst ? (nfait * 100) / ninst : 0);
    fprintf(out, "'\n");
    fprintf(out, "' ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    for (i = 0; i < ninst; i++)
    {
        fprintf(out, "%s\n", instructions[i]);
    }
    fprintf(out, "'\n");
    fclose(out);

    return 0;
}
