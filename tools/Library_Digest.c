/* C port of c/Library_Digest.Asc - counts the Lib_ labels in a single
 * extension source file and emits "<name>_Labels.s" (the label-equate
 * table) and "<name>_Size.s" (the "Lib_Size equ N" line).
 *
 * Unlike Make_Labels, there is no jump-table output (no Lib_Int handling)
 * and no Lib_Ini/Lib_Pos-driven "internal library definition" file - this
 * is meant for third-party extensions, which only ever declare entries with
 * Lib_Def/Lib_Ext/Lib_Par and reserve slots with Lib_Empty.
 *
 * The original AMOS program also accepts the source filename interactively
 * (prompting "Usage: LIBRARY_DIGEST filename" and re-reading the command
 * line if it's missing/invalid) and validates the file/directory with
 * Exist(). It's only ever invoked here from the Makefile with a fixed
 * argument, so that interactive retry loop isn't reproduced - a missing or
 * bad argument just prints usage and exits.
 *
 * The original also derives OUT1$/OUT2$ as PATH$+N$+"_Labels.s" /
 * "_Size.s", where PATH$ is meant to be the directory part of the input
 * filename but is actually computed from an undeclared variable (FILT$)
 * and so is always "". That means the outputs always land next to N$ as
 * given - i.e. in the current directory unless the input path itself has a
 * directory prefix baked into it - regardless of whether the input path
 * had a directory component. We reproduce that observed behaviour (N$
 * keeps whatever directory prefix NAME$ had; nothing extra is prepended)
 * without reproducing the undefined-variable bug itself. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

FILE *in = NULL;
FILE *out1 = NULL;
FILE *out2 = NULL;

int lab_count = 0;

void search()
{
    char row[512], *ad;
    char name[512];

    while (fgets(row, sizeof(row), in) != NULL)
    {
        /* only lines starting with whitespace (i.e. indented Lib_ entries,
         * not column-0 comments/labels) are of interest */
        if ((unsigned char)row[0] > 32) continue;

        if (strstr(row, LAB0) == NULL) continue;

        if (((ad = strstr(row, LABSET1))) || ((ad = strstr(row, LABSET2))))
        {
            if (read_token(ad, name, sizeof(name)) < 0)
            {
                printf("Parse error\n");
                die(10);
            }
            lab_count = atoi(name);
        }
        else
        {
            if (
                ((ad = strstr(row, LAB1))) ||
                ((ad = strstr(row, LAB4))) ||
                ((ad = strstr(row, LAB5)))
            )
            {
                if (read_token(ad, name, sizeof(name)) < 0)
                {
                    printf("Parse error\n");
                    die(10);
                }

                if (out1) write_label_line(out1, name, lab_count);

                lab_count++;
            }
            else if ((ad = strstr(row, LAB3)))
            {
                /* Lib_Empty: consumes a slot, no label emitted */
                lab_count++;
            }
        }
    }

    if (out2) write_lib_size_line(out2, lab_count);
}

int main(int argc, char **argv)
{
    const char *name;
    const char *dot;
    char base[512];
    char out1_name[600], out2_name[600];
    char title1[600], title2[600];

    if (argc != 2)
    {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }
    name = argv[1];

    /* N$: NAME$ with its extension (everything from the last '.' on)
     * stripped, keeping any directory prefix it already had */
    dot = strrchr(name, '.');
    if (dot)
    {
        size_t len = (size_t)(dot - name);
        if (len >= sizeof(base)) len = sizeof(base) - 1;
        memcpy(base, name, len);
        base[len] = '\0';
    }
    else
    {
        snprintf(base, sizeof(base), "%s", name);
    }

    snprintf(out1_name, sizeof(out1_name), "%s_Labels.s", base);
    snprintf(out2_name, sizeof(out2_name), "%s_Size.s", base);
    snprintf(title1, sizeof(title1), "%s, list of the library functions", name);
    snprintf(title2, sizeof(title2), "%s, library size", name);

    open_input_file(&in, name);
    open_output_file(&out1, out1_name, title1);
    open_output_file(&out2, out2_name, title2);

    search();

    printf("Number of labels: %d\n", lab_count);

    die(0);
    return 0;
}
