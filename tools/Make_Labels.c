#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

FILE *in = NULL;
FILE *out1 = NULL;
FILE *out2 = NULL;
FILE *out3 = NULL;
FILE *out4 = NULL;

/* Persists across both search() calls, same as LAB/NLAB in the AMOS source -
 * only Lib_Pos/Lib_Ini resets it, so the second file continues numbering
 * where the first left off. */
int lab_count = 0;

void search()
{
    char row[512], *ad, *i;
    char name[512];
    int name_len;

    while (fgets(row, sizeof(row), in) != NULL)
    {
        /* only lines mentioning "Lib_" at all are of interest */
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
            i = NULL;
            if (
                ((ad = strstr(row, LAB1))) ||
                ((ad = strstr(row, LAB4))) ||
                ((ad = strstr(row, LAB5))) ||
                (((ad = strstr(row, LAB2))) && ((i = ad)))

            )
            {
                name_len = read_token(ad, name, sizeof(name));
                if (name_len < 0)
                {
                    printf("Parse error\n");
                    die(10);
                }

                if (i && out2)
                {
                    fprintf(out2, "\tdc.w\tL_%s\n", name);
                    fprintf(out2, "\tdc.l\t%s\n", name);
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

    if (out2)
    {
        fprintf(out2, "\tdc.w\t0\n");
        fprintf(out2, "\tdc.l\t0\n");
    }

    if (out3) write_lib_size_line(out3, lab_count);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    open_input_file(&in, "+ILib.s");
    open_output_file(&out1, "+LEqu.s", "AMOSPro Internal Library functions");
    open_output_file(&out2, "+Internal_Jumps.s", "AMOSPro Internal Jump list");
    open_output_file(&out3, "+ILib_Size.s", "AMOSPro Internal Library Size");
    open_output_file(&out4, "+ILib_Functions.s", "AMOSPro Internal Library Definition");

    search();

    /* not part of search() in the original - written by the caller right
     * after the first _SEARCH call returns */
    if (out4)
    {
        /* same leading-space quirk as write_lib_size_line(): Print #4,...;0;...
         * prints the literal integer 0 directly, with its sign column, unlike
         * Lib_Pos below which strips it via Mid$(Str$(NLAB),2) */
        fprintf(out4, "\tLib_Ini\t 0\n");
        fprintf(out4, "\tLib_Pos\t%d\n", lab_count);
    }

    /* out1 (+LEqu.s) stays open across both scans - the original only
     * closes channels 2 and 3 here, and appends a small divider header to
     * channel 1 instead of reopening it. */
    if (out2)
    {
        fclose(out2);
        out2 = NULL;
    }
    if (out3)
    {
        fclose(out3);
        out3 = NULL;
    }
    if (out4)
    {
        fclose(out4);
        out4 = NULL;
    }
    if (in)
    {
        fclose(in);
        in = NULL;
    }

    if (out1)
    {
        fprintf(out1, "; Main library\n");
        fprintf(out1, "; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }

    open_input_file(&in, "+Lib.s");
    open_output_file(&out3, "+Lib_Size.s", "AMOSPro Main Library Size");

    search();

    printf("Number of labels: %d\n", lab_count);

    die(0);
    return 0;
}
