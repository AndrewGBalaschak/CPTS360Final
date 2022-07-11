#include "functions.h"

int mysymlink(char *old_file, char *new_file){
    int oino, ino, pino;
    char parent[128], child[128];
    MINODE *mip;
    char buf[BLKSIZE];
    char new_old_file[128];
    
    //check that old file exists
    oino = getino(old_file);
    if(!oino){
        printf("ERROR: %s does not exist\n", old_file);
        return -1;
    }
    //new file must not exist yet
    if(getino(new_file) != 0){
        printf("ERROR: %s already exists\n", new_file);
        return -1;
    }

    //creat new_file
    strcpy(new_old_file, old_file);
    strcpy(pathname, new_file);                 //since creat works on pathname
    mycreat();
    ino = getino(new_file);

    if(!ino){
        printf("ERROR: creat failed, symlink failed\n");
        return -1;
    }

    mip = iget(dev, ino);
    mip->INODE.i_mode = 0xA1ED;                 //change new_file to LNK type
    
    strncpy((char *) mip->INODE.i_block, new_old_file, 60);//store old_file name in newfile’s INODE.i_block[ ] area
    mip->INODE.i_size = strlen(new_old_file);       //set file size to length of old_file name
    mip->dirty = 1;                             //mark new_file’s minode dirty
    memset(buf, 0, BLKSIZE);                    //clear buffer
    iput(mip);                                  //iput new_file’s minode
    return 0;
}

int myreadlink(char * file, char *buffer){
    int ino;
    MINODE *mip;
    //get file’s INODE in memory; verify it’s a LNK file
    ino = getino(file);
    mip = iget(dev,ino);

    if(!S_ISLNK(mip->INODE.i_mode)){
        printf("ERROR: file is not LINK type\n");
        return -1;
    }
    //copy target filename from INODE.i_block[ ] into buffer;

    strncpy(buffer,mip->INODE.i_block, mip->INODE.i_size);
    
    //return file size
    return mip->INODE.i_size;
}