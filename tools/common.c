#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *LAB0 = "Lib_";
const char *LAB1 = "Lib_Def";
const char *LAB2 = "Lib_Int";
const char *LAB3 = "Lib_Empty";
const char *LAB4 = "Lib_Ext";
const char *LAB5 = "Lib_Par";
const char *LABSET1 = "Lib_Pos";
const char *LABSET2 = "Lib_Ini";

#define MAX_TRACKED_FILES 8
static FILE **tracked_files[MAX_TRACKED_FILES];
static int tracked_count = 0;

void register_close(FILE **fp)
{
    if (tracked_count < MAX_TRACKED_FILES)
    {
        tracked_files[tracked_count++] = fp;
    }
}

void die(int code)
{
    int i;

    for (i = 0; i < tracked_count; i++)
    {
        if (*tracked_files[i])
        {
            fclose(*tracked_files[i]);
            *tracked_files[i] = NULL;
        }
    }
    exit(code);
}

void open_input_file(FILE **in, const char *filename)
{
    register_close(in);

    if ((*in = fopen(filename, "rb")) == NULL)
    {
        printf("Could not open file '%s' for input\n", filename);
        die(1);
    }
}

void open_output_file(FILE **out, const char *filename, const char *title)
{
    char dt[100];
    time_t t;

    register_close(out);

    if ((*out = fopen(filename, "wb")) == NULL)
    {
        printf("Could not open file '%s' for output\n", filename);
        die(1);
    }

    if (title)
    {
        time(&t);
        strftime(dt, sizeof(dt), "%d-%m-%Y %H:%M:%S", localtime(&t));

        fprintf(*out, "; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        fprintf(*out, "; %s on the %s\n", title, dt);
        fprintf(*out, ";\n");
        fprintf(*out, "; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }

    printf("Writing %s\n", filename);
}

int read_token(const char *ad, char *out, size_t outsz)
{
    const char *a, *token_end;
    size_t len;

    a = ad;
    while (*a > ' ') a++; /* skip past the rest of the matched keyword */
    while (*a <= ' ' && *a) a++; /* skip the whitespace separating it from the token */
    if (!*a) return -1; /* line ended before a token appeared */

    token_end = a;
    while (*token_end > ' ') token_end++;

    len = (size_t)(token_end - a);
    if (len >= outsz) len = outsz - 1;
    memcpy(out, a, len);
    out[len] = '\0';

    return (int)len;
}

void write_label_line(FILE *f, const char *name, int lab)
{
    int prefix_len = 2 + (int)strlen(name) + 1; /* "L_" + name + ":" */

    fprintf(f, "L_%s:", name);
    if (prefix_len < 8) fprintf(f, "\t\t\t");
    else if (prefix_len < 16) fprintf(f, "\t\t");
    else if (prefix_len < 24) fprintf(f, "\t");
    fprintf(f, "set\t%d\n", lab);
}

void write_lib_size_line(FILE *f, int count)
{
    fprintf(f, "Lib_Size\t\tequ\t %d\n", count);
}

void *read_file(const char *filename, size_t *size)
{
    FILE *f;
    void *buf = NULL;

    *size = 0;

    if ((f = fopen(filename, "rb")) == NULL)
    {
        printf("Could not open file '%s' for input\n", filename);
        die(1);
    }

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(*size);
    if (buf)
    {
        fread(buf, 1, *size, f);
    }
    fclose(f);

    return buf;
}
