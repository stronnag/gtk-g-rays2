#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


void process_str(char *buf, int len)
{
    static char statbuf[1024];
    char *sp,*ptr;
    sp = buf;

    buf[len] = 0;
    fprintf(stderr,"Start [%s]\n", buf);
    while((ptr = index(sp, '\n')))
    {
        *ptr = 0;
        strcat(statbuf, sp);
//            serial_process(statbuf);
        fprintf(stderr,"Complete IO [%s]\n", statbuf);                    
        *statbuf = 0;
        sp = ptr+1;
    }
    if (*sp != '\0')
    {
        strcat(statbuf, sp);
//        fprintf(stderr, "Finally %s\n", statbuf);
    }
}

int main()
{
    char buf[129] ="abc\ndef\nx";
    int len = strlen(buf);
    process_str(buf, len);
    strcpy(buf,"terminal\n");
    len = strlen(buf);
    process_str(buf, len);
    return 0;
}
