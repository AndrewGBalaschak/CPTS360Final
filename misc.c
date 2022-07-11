#include "functions.h"


int mystat(char *pathname){
    struct stat myst;
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);

    myst.st_dev = dev;
    myst.st_ino = ino;
    
    //copy mip->INODE fields to myst fields;

    

    iput(mip);
}

int mychmod(){
// filename mode: (mode = |rwx|rwx|rwx|, e.g. 0644 in octal)
//          get INODE of pathname into memroy:
//              ino = getino(pathname);
//              mip = iget(dev, ino);
//              mip->INODE.i_mode |= mode;
//          mip->dirty = 1;
//          iput(mip);
}

int utime(char *filename){
    //modify atime of INODE

}