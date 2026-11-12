/* C port of c/Check_CLib.Asc - sanity-checks +CLib.s (the compiler support
 * library source) against +LEqu.s (the label-equate table produced by
 * Make_Labels for the main +Lib.s), making sure every Lib_Cmp entry still
 * has a matching L_<name>: label and that no old-style Lib_Def-ish entry
 * is left dangling once Lib_Ini's argument table has started.
 *
 * The original AMOS program opens (or reuses) a CLI window to print its
 * findings and prompts for a keypress-driven console if none is available;
 * none of that applies to a batch CLI tool, so it's dropped along with the
 * unused _DATE$/_TIME$ helpers (only there to timestamp that console). The
 * original also never aborts on a problem, it just keeps scanning and
 * prints one line per issue found - we keep that behaviour but track a
 * global error count and use it as the process exit code instead of
 * silently returning to the AMOS editor.
 *
 * Filenames ("+LEqu.s", "+CLib.s") are hardcoded, same as the original. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static const char *LAB_CMP = "Lib_Cmp";
static const char *LAB_END = "Lib_End";
static const char *LAB_POS = "Lib_Pos";

FILE *in = NULL;

int errors = 0;

/* Bounded substring search - labels_buf comes from read_file() and is not
 * NUL-terminated, so strstr() can't be used on it directly. */
static char *find_bounded(char *start, char *end, const char *needle)
{
    size_t needle_len = strlen(needle);
    char *p;

    if (needle_len == 0 || (size_t)(end - start) < needle_len) return NULL;

    for (p = start; p <= end - needle_len; p++)
    {
        if (memcmp(p, needle, needle_len) == 0) return p;
    }

    return NULL;
}

static void chomp(char *row)
{
    row[strcspn(row, "\n")] = '\0';
}

int main(int argc, char **argv)
{
    const char *lequ_name = "+LEqu.s";
    const char *clib_name = "+CLib.s";
    char *labels_buf;
    size_t labels_size;
    char *hunt_pos, *hunt_end, *found;
    char row[512], name[512], label[600];
    char *ad;
    int found_ini = 0;
    int line_no = 0;

    (void)argc;
    (void)argv;

    printf("Loading %s...\n", lequ_name);
    if ((labels_buf = read_file(lequ_name, &labels_size)) == NULL)
    {
        printf("Could not open file '%s' for input\n", lequ_name);
        return 1;
    }
    hunt_pos = labels_buf;
    hunt_end = labels_buf + labels_size;

    printf("Loading %s...\n", clib_name);
    open_input_file(&in, clib_name);

    /* Skip forward to the line where Lib_Ini's argument table starts;
     * everything before it is header/definition boilerplate. */
    while (fgets(row, sizeof(row), in) != NULL)
    {
        line_no++;
        if (strstr(row, "Lib_Ini") != NULL)
        {
            found_ini = 1;
            break;
        }
    }

    if (!found_ini)
    {
        printf("Bad library!\n");
        errors++;
    }
    else
    {
        while (fgets(row, sizeof(row), in) != NULL)
        {
            line_no++;

            /* only indented Lib_ entries are of interest, same as the
             * original's Peek(ADOLD)<=32 check */
            if ((unsigned char)row[0] > 32) continue;

            if (strstr(row, LAB0) == NULL) continue;

            if ((ad = strstr(row, LAB_CMP)) != NULL)
            {
                if (read_token(ad, name, sizeof(name)) < 0)
                {
                    printf("Line %d: Parse error\n", line_no);
                    errors++;
                    continue;
                }

                snprintf(label, sizeof(label), "L_%s:", name);
                found = find_bounded(hunt_pos, hunt_end, label);
                if (!found)
                {
                    chomp(row);
                    printf("Line %d: Label problem: %s\n", line_no, row);
                    errors++;
                }
                else
                {
                    hunt_pos = found + 1;
                }
            }
            else if (strstr(row, LAB_END) != NULL)
            {
                break;
            }
            else if (strstr(row, LAB_POS) == NULL)
            {
                chomp(row);
                printf("%s Line %d: LIB_DEF still present: %s\n", clib_name, line_no, row);
                errors++;
            }
        }
    }

    free(labels_buf);

    if (errors)
        printf("\nProblems found: %d\n\n", errors);

    die(0);
    return 0;
}
