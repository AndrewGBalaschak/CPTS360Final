/*********** util.c file ****************/

#include "functions.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <fcntl.h>
// #include <ext2fs/ext2_fs.h>
// #include <string.h>
// #include <libgen.h>
// #include <sys/stat.h>
// #include <time.h>

// #include "type.h"

/**** globals defined in main.c file ****/
// extern MINODE minode[NMINODE];
// extern MINODE *root;
// extern PROC   proc[NPROC], *running;

// extern char gpath[128];
// extern char *name[64];
// extern int n;

// extern int fd, dev;
// extern int nblocks, ninodes, bmap, imap, iblk;

// extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

//splits pathname into dname and bname
int parse_tokens(char *pathname, char *dname, char *bname){
    tokenize(pathname);
    if(pathname[0] == '/'){                     //for absolute paths
        strcpy(dname, "/");
    }

    for(int i = 0; i < n-1; i++){               //copy rest of dname
        strcat(dname, name[i]);
        strcat(dname, "/");
    }

    if(pathname[0] != '/'){                     //for relative paths
        char temp_path[128] = "";
        get_pwd(running->cwd, temp_path);       //get path to cwd
        
        printf("PWD: %s\n", temp_path);

        if(n > 1)                               //if there's stuff in dname
            strcat(temp_path, dname);           //append dname to cwd
        strcpy(dname, temp_path);               //copy cwd + dname to dname
        
    }
    strcpy(bname, name[n-1]);                   //copy bname

    printf("dname: %s\n", dname);
    printf("bname: %s\n", bname);
    return 0;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino){
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + offset;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip){
    int i, block, offset;
    char buf[BLKSIZE];
    INODE *ip;

    if (mip==0) 
        return;

    mip->refCount--;

    if (mip->refCount > 0) return;
    if (!mip->dirty)       return;

    /* write INODE back to disk */
    /**************** NOTE ******************************
    For mountroot, we never MODIFY any loaded INODE
                    so no need to write it back
    FOR LATER WORK: MUST write INODE back to disk if refCount==0 && DIRTY

    Write YOUR code here to write INODE back to disk
    *****************************************************/

    block = (mip->ino - 1) / 8 + iblk;
    offset = (mip->ino -1) % 8;

    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->INODE;
    put_block(mip->dev, block, buf);
    midalloc(mip);
} 

int search(MINODE *mip, char *name){
    int i; 
    char *cp, c, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    INODE *ip;

    printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
    ip = &(mip->INODE);

    //search for name in mip's data blocks
    for(int i = 0; i < 12;i++){
        if(mip->INODE.i_block[i] == 0){
            printf("Search: %s not found\n", name);
            return 0;
        }
        get_block(dev, ip->i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        printf("  ino   rlen  nlen  name\n");

        while (cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            if(dp->inode == 0 && dp->rec_len == 0 && dp->name_len == 0)
            break;
            printf("%4d  %4d  %4d    %s\n", 
            dp->inode, dp->rec_len, dp->name_len, dp->name);
            if (strcmp(temp, name)==0){
            printf("found %s : ino = %d\n", temp, dp->inode);
            return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    printf("Search: %s not found\n", name);
    return 0;
}

int getino(char *pathname){
    int i, ino, blk, offset;
    char buf[BLKSIZE];
    INODE *ip;
    MINODE *mip;

    printf("getino: pathname=%s\n", pathname);
    if (strcmp(pathname, "/")==0)
        return 2;

    // starting mip = root OR CWD
    if (pathname[0]=='/')
        mip = root;
    else
        mip = running->cwd;

    mip->refCount++;         // because we iput(mip) later

    tokenize(pathname);

    for (i=0; i<n; i++){
        printf("===========================================\n");
        printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

        ino = search(mip, name[i]);

        if (ino==0){
            iput(mip);
            printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);
        mip = iget(dev, ino);
    }

    iput(mip);
    return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]){
    char *cp, c, sbuf[BLKSIZE], temp[256] = "";
    DIR *dp;
    INODE *ip;
    ip = &(parent->INODE);

    for(int i = 0; i < 12; i++){
        get_block(dev, ip->i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;

        while (cp < sbuf + BLKSIZE){
            if (dp->inode == myino){
                strncpy(temp, dp->name, dp->name_len);
                temp[dp->name_len] = 0;
                strcpy(myname, temp);
                return 1;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        return 0;
    }
}

int findino(MINODE *mip, u32 *myino){ // myino = i# of . return i# of ..
    // myino = ino of .  return ino of ..
   *myino = search(mip, ".");
   return search(mip, "..");
}