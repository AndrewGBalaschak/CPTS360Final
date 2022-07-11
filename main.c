/****************************************************************************
*                        mount root file system                             *
*****************************************************************************/
// C LIBS:
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>


// USR LIBS:
#include "globals.h"
#include "type.h"
#include "functions.h"

// forward declarations of funcs in main.c
int init();
int quit();
int mount_root();

char *disk = "diskimage";

int main(int argc, char *argv[])
{
    int ino;
    char buf[BLKSIZE];
    if(argc > 1) disk = argv[1];

    printf("checking EXT2 FS ....");
    if ((fd = open(disk, O_RDWR)) < 0)
    {
        printf("open %s failed\n", disk);
        exit(1);
    }

    dev = fd; // global dev same as this fd

    /********** read super block  ****************/
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    /* verify it's an ext2 file system ***********/
    if (sp->s_magic != 0xEF53)
    {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }
    printf("EXT2 FS OK\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    get_block(dev, 2, buf);
    gp = (GD *)buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    iblk = gp->bg_inode_table;
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

    init();
    mount_root();
    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as running process\n");
    running = &proc[0];
    running->cwd = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);

    // WRTIE code here to create P1 as a USER process

    while (1)
    {
        printf("\n--------------------------------------------------\n");
        printf("Level 1: [ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|quit]\n");
        printf("Level 2: [open|close|pfd|cat|read|write|mv|cp]\n");
        printf("input command: ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;

        if (line[0] == 0)
            continue;
        pathname[0] = 0;
        pathname2[0] = 0;

        sscanf(line, "%s %s %s", cmd, pathname, pathname2);
        printf("cmd=%s pathname=%s", cmd, pathname);
        if(strlen(pathname2) > 0)
            printf(" pathname2=%s", pathname2);
        printf("\n");

        if (strcmp(cmd, "ls") == 0)
            ls();
        else if (strcmp(cmd, "cd") == 0)
            cd();
        else if (strcmp(cmd, "pwd") == 0)
            pwd(running->cwd);
        else if(strcmp(cmd, "mkdir") == 0)
            mymkdir(pathname);
        else if(strcmp(cmd, "rmdir") == 0)
            myrmdir(pathname);
        else if(strcmp(cmd, "creat") == 0)
            mycreat();
        else if(strcmp(cmd, "link") == 0)
            mylink(pathname, pathname2);
        else if(strcmp(cmd, "unlink") == 0)
            myunlink(pathname);
        else if(strcmp(cmd, "symlink") == 0)
            mysymlink(pathname, pathname2);
        else if(strcmp(cmd, "readlink") == 0)
            myreadlink(pathname, pathname2);
        else if(strcmp(cmd, "open") == 0)
            open_file(pathname, atoi(pathname2));
        else if(strcmp(cmd, "close") == 0)
            close_file(atoi(pathname));
        else if(strcmp(cmd, "pfd") == 0)
            pfd();
        else if(strcmp(cmd, "cat") == 0)
            mycat(pathname);
        else if(strcmp(cmd, "read") == 0)
            myread(atoi(pathname), buf, atoi(pathname2));
        else if(strcmp(cmd, "write") == 0)
            write_file(atoi(pathname), atoi(pathname2));
        else if(strcmp(cmd, "mv") == 0)
            mymv(pathname, pathname2);
        else if(strcmp(cmd, "cp") == 0)
            mycp(pathname, pathname2);
        else if (strcmp(cmd, "quit") == 0)
            quit();
        else
            printf("Command not recognized\n");
    }
}

int quit()
{
    int i;
    MINODE *mip;
    for (i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        if (mip->refCount > 0)
            iput(mip);
    }
    exit(0);
}


int init()
{
    int i, j;
    MINODE *mip;
    PROC *p;

    printf("init()\n");

    for (i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mptr = 0;
    }
    for (i = 0; i < NPROC; i++)
    {
        p = &proc[i];
        p->pid = i;
        p->uid = p->gid = 0;
        p->cwd = 0;
    }
}

// load root INODE and set root pointer to it
int mount_root()
{
    printf("mount_root()\n");
    root = iget(dev, 2);
}