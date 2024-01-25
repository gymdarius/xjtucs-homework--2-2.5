#include "EX2.h"
#include "function.c"
#include "struct.c"
#include <stdio.h>
#include <ctype.h>

int main()
{
    ext2_inode cu; /*current user*/
    printf("Hello! Welcome to Ext2_like file system!\n");
    if (initfs(&cu) == 1)return 0;
    if (login() != 0) /***************/
    {
        printf("Wrong password!\n");
        exitdisplay();
        return 0;
    }
    shellloop(cu);
    exitdisplay();
    return 0;
}