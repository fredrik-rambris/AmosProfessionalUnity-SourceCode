#include <stdio.h>
#include <stdint.h>
#include <string.h>


static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void MakeToktable(FILE *in, FILE *out)
{
    char buf[256], *cmt, *end;
    int state = 0;
    uint8_t lwrd[4];

    printf("Making Token Table\n");

    while (fgets(buf, sizeof(buf) - 1, in))
    {
        end = buf + strlen(buf);
        while (--end > buf && (*end == '\n' || *end == '\r' || *end == ' '))
        {
            *end = '\0';
            end--;
        }

        if (state == 0 && strstr(buf, "TOKEN_START"))
        {
            state = 1;
            continue;
        }
        if (state == 1 && strstr(buf, "TOKEN_END"))
        {
            state = 2;
            break;
        }
        if (state == 1)
        {
            if ((cmt = strstr(buf, "//")))
            {
                cmt += 2;
                lwrd[0] = (hexval(cmt[1]) << 4) + hexval(cmt[2]);
                lwrd[1] = (hexval(cmt[4]) << 4) + hexval(cmt[5]);
                lwrd[2] = (hexval(cmt[7]) << 4) + hexval(cmt[8]);
                lwrd[3] = (hexval(cmt[10]) << 4) + hexval(cmt[11]);
                if (fwrite(lwrd, 4, 1, out) != 1)
                {
                    printf("Error writing to output file\n");
                    return;
                };
            }
        }
    }
    if (state == 0)
    {
        printf("Did not find TOKEN_START\n");
    }
    else if (state == 1)
    {
        printf("Did not find TOKEN_END\n");
    }
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    if (argc != 3)
    {
        printf("Usage: %s +Lib.s +Toktab_Verif.Bin\n", argv[0]);
        return 1;
    }

    if ((in = fopen(argv[1], "r")))
    {
        if ((out = fopen(argv[2], "wb")))
        {
            MakeToktable(in, out);
            fclose(out);
        }
        else
        {
            printf("Could not open output file '%s'\n", argv[2]);
            return 1;
        }
        fclose(in);
    }
    else
    {
        printf("Could not open input file '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}
