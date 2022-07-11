#include "functions.h"

int mytruncate(MINODE *mip){
    //release mip->INODE's data blocks;
    //a file may have 12 direct blocks, 256 indirect blocks and 256*256
    //double indirect data blocks. release them all.

    //direct blocks
    for(int i = 0; i < 12; i++){
        if(mip->INODE.i_block[i] == 0)
            break;
        bdalloc(dev, mip->INODE.i_block[i]);
        mip->INODE.i_block[i] = 0;
    }

    //indirect blocks
    char buf[BLKSIZE];
    if(mip->INODE.i_block[12] != 0){
        get_block(dev, mip->INODE.i_block[12], buf);
        for(int i = 0; i < 256 + 12; i++){
            int j = i * 4;
            if (buf[j] == 0)
                break;
            bdalloc(dev, buf[j]);
            buf[j] = 0;
        }
        bdalloc(dev, mip->INODE.i_block[12]);
        mip->INODE.i_block[12] = 0;
    }

    //double indirect blocks
    if (mip->INODE.i_block[13] != 0){
        get_block(dev, mip->INODE.i_block[13], buf);
        for (int i = 0; i < 256 + 12; i++){
            int j = i * 4;
            if (buf[j] == 0) break;
            bdalloc(dev, buf[j]);
            buf[j] = 0;
        }
        bdalloc(dev, mip->INODE.i_block[13]);
        mip->INODE.i_block[13] = 0;
    }
    
    //update INODE's time field
    mip->INODE.i_atime = time(0L);

    //set INODE's size to 0 and mark Minode[ ] dirty
    mip->INODE.i_blocks = 0;
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    iput(mip);
}

int open_file(char *pathname, int mode){
    char dname[128], bname[128];
    int ino, smallest_i;
    //ask for a pathname and mode to open:
    //mode = 0|1|2|3 for R|W|RW|APPEND
    if(mode < 0 || mode > 3) {
        printf("ERROR: mode %d is not valid\n", mode);
        return -1;
    }

    //get pathname's inumber, minode pointer
    parse_tokens(pathname, dname, bname);
    ino = getino(pathname); 
    if(ino == -1){                                      //if file does not exist, create
        printf("WARNING: file %s does not exist\n", bname);
        mycreat();
        ino = getino(pathname);
    }
    
    MINODE * mip = iget(dev, ino);  
    
    //check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
    if(!S_ISREG(mip->INODE.i_mode)) {
        printf("ERROR: file %s is not REG\n", bname);
        iput(mip);
        return -1;
    }
    
    //Check whether the file is ALREADY opened with INCOMPATIBLE mode:
           //If it's already opened for W, RW, APPEND : reject.
           //(that is, only multiple R are OK)
    for(int i = 0; i < NFD; i++){
        if(running->fd[i] == 0)
            break;
        if(running->fd[i]->minodePtr == mip){
            if (mode != 0) { //only R is okay for open file(R = 0)
                printf("ERROR: file %s already open with incompatible mode %d\n", bname, mode);
                return -1;
            }
        }
    }
    
    //allocate a FREE OpenFileTable (OFT) and fill in values:
    OFT *oftp = (OFT *) malloc(sizeof(OFT));
    oftp->mode = mode;      //mode = 0|1|2|3 for R|W|RW|APPEND 
    oftp->refCount = 1;
    oftp->minodePtr = mip;  //point at the file's minode[]

    //Depending on the open mode 0|1|2|3, set the OFT's offset accordingly
    switch(mode){
        case 0 : 
            oftp->offset = 0;     //R: offset = 0
            break;
        case 1 : 
            mytruncate(mip);      //W: truncate file to 0 size
            oftp->offset = 0;
            break;
        case 2 : 
            oftp->offset = 0;     //RW: do NOT truncate file
            break;
        case 3 : 
            oftp->offset =  mip->INODE.i_size;  //APPEND mode
            break;
        default: 
            printf("ERROR: invalid mode %d\n", mode);
            return(-1);
      }

    //find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
    //Let running->fd[i] point at the OFT entry
    smallest_i = -1;
    for(int i = 0; i < NFD; i++){
        if(running->fd[i] == NULL){
            smallest_i = i;
            break;
        }
    }
    if(smallest_i != -1)
        running->fd[smallest_i] = oftp;

    //update INODE's time field
    //for R: touch atime. 
    mip->INODE.i_atime = time(0L);
    
    //for W|RW|APPEND mode : touch atime and mtime
    if (mode > 0)
        mip->INODE.i_mtime = time(0L);

    //mark Minode[ ] dirty
    mip->dirty = 1;
    iput(mip);
    return smallest_i;
}
      
int close_file(int fd){
    //verify fd is within range.

    //verify running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == 0){
        printf("ERROR: fd is not pointing to OFT entry\n");
        return -1;
    }
    //The following code segments should be fairly obvious:
    OFT *oftp = (OFT *) malloc(sizeof(OFT));
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if (oftp->refCount > 0)
        return 0;

    //last user of this OFT entry ==> dispose of the Minode[]
    MINODE *mip = oftp->minodePtr;
    mip->dirty = 1;
    iput(mip);
    free(oftp);
    return 0; 
}

int mylseek(int fd, int position){
    //From fd, find the OFT entry. 
    //change OFT entry's offset to position but make sure NOT to over run either end of the file.
    //return originalPosition
}

int pfd(){
    /*
    This function displays the currently opened files as follows:

        fd     mode    offset    INODE
        ----    ----    ------   --------
            0     READ    1234   [dev, ino]  
            1     WRITE      0   [dev, ino]
        --------------------------------------
    to help the user know what files has been opened.
    */
    char *modes[4] = {"READ", "WRITE", "RW", "APPEND"};  //mode lookup table
    int i = 0;                                          //number of open files
    
    printf("fd\tmode\toffset\tINODE\n");
    printf("----    ----    ------   --------\n");
    for(i = 0; i < NFD; i++){
        if(running->fd[i] == 0)
            break;
        printf("%d\t%s\t%d\t[%d, %d]\n", i, modes[running->fd[i]->mode], running->fd[i]->offset, running->fd[i]->minodePtr->dev, running->fd[i]->minodePtr->ino);
    }
    return i;
}

int dup(int fd){
    //verify fd is an opened descriptor;
    //duplicates (copy) fd[fd] into FIRST empty fd[ ] slot;
    //increment OFT's refCount by 1;
}

int dup2(int fd, int gd){
  //CLOSE gd fisrt if it's already opened;
  //duplicates fd[fd] into fd[gd]; 
}