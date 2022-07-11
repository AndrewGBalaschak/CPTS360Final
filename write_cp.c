#include "functions.h"

int write_file(int fd, int nbytes){
    //ask for a fd and a text string to write;
    //verify fd is indeed opened for WR or RW or APPEND mode
    //copy the text string into a buf[] and get its length as nbytes.
    char buf[BLKSIZE];
    return(mywrite(fd, buf, nbytes));
}

int mywrite(int fd, char buf[ ], int nbytes){
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    int count=0, lbk, blk, start_byte, remain, avail, *ip;
    char wbuf[BLKSIZE], buf12[BLKSIZE], buf13[BLKSIZE];
    int ibuf[256];

    while(nbytes > 0 ){

        //compute LOGICAL BLOCK (lbk) and the startByte in that lbk:
        lbk        = oftp->offset / BLKSIZE;
        start_byte = oftp->offset % BLKSIZE;

        // I only show how to write DIRECT data blocks, you figure out how to 
        // write indirect and double-indirect blocks.

        //direct blocks
        if(lbk < 12){
            if(mip->INODE.i_block[lbk] == 0){   // if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
            }
            blk = mip->INODE.i_block[lbk];      // blk should be a disk block now
        }
        //indirect blocks
        else if(lbk >= 12 && lbk < 256 + 12){
            if(mip->INODE.i_block[12] == 0){
                //allocate a block for it;
                mip->INODE.i_block[12] = balloc(mip->dev);
                //zero out the block on disk !!!!
                bzero(buf, BLKSIZE);
                put_block(dev, mip->INODE.i_block[12], buf);
            }
            //get i_block[12] into an int ibuf[256];
            get_block(dev, mip->INODE.i_block[12], (char *) ibuf);
            blk = ibuf[lbk - 12];
            if(blk==0){
                //allocate a disk block;
                blk = ibuf[lbk - 12] = balloc(dev);
                //record it in i_block[12];
                put_block(dev, mip->INODE.i_block[12], (char *) ibuf);
            }
            //.......
        }
        //double indirect blocks
        else{
            lbk -= (12+256);
            int dbl_ptr = lbk / 256;    //block
			int dbl_dat = lbk % 256;    //offset

            if(mip->INODE.i_block[13] == 0){
                //allocate a block for it;
                mip->INODE.i_block[13] = balloc(mip->dev);
                //zero out the block on disk !!!!
                bzero(buf, BLKSIZE);
                put_block(dev, mip->INODE.i_block[13], buf);
            }

            //get i_block[13] into an int ibuf[256];
            get_block(dev, mip->INODE.i_block[13], (char *) ibuf);

            if(ibuf[dbl_ptr] == 0){
                ibuf[dbl_ptr] = balloc(dev);
                bzero(ibuf[dbl_ptr], BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[13], (char *) ibuf);
            }

            int ibuf2[256];             //holds double indirect pointers
            get_block(mip->dev, ibuf[dbl_ptr], (char *) ibuf2);

            //check the block of pointers' (double indirect)
            if(ibuf2[dbl_dat] == 0){
                ibuf2[dbl_dat] = balloc(dev);
                put_block(mip->dev, ibuf[dbl_dat], (char *) ibuf2);
            }

            //set blk to double indirect data block
            blk = ibuf2[dbl_dat];
        }

        /* all cases come to here : write to the data block */
        get_block(mip->dev, blk, wbuf);     // read disk block into wbuf[ ]  
        char *cp = wbuf + start_byte;       // cp points at startByte in wbuf[]
        remain = BLKSIZE - start_byte;      // number of BYTEs remain in this block
        char * cq = buf;                    // cq points at buf [ ]

        /*
        while(remain > 0){                  // write as much as remain allows  
            *cp++ = *cq++;                  // cq points at buf[ ]
            nbytes--; remain--;             // dec counts
            oftp->offset++;                 // advance offset
            if (offset > INODE.i_size)      // especially for RW|APPEND mode
                mip->INODE.i_size++;        // inc file size (if offset > fileSize)
            if (nbytes <= 0) break;         // if already nbytes, break
        }*/

        if(remain < nbytes)
            nbytes = remain;

        memmove(cp, buf, nbytes);
        buf += nbytes;
        cp += nbytes;
        oftp->offset += nbytes;
        nbytes -= nbytes;

        if (oftp->offset > mip->INODE.i_size)
            mip->INODE.i_size = oftp->offset;

        put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
        
        // loop back to outer while to write more .... until nbytes are written
    }

    mip->dirty = 1;       // mark mip dirty for iput() 
    printf("wrote %d char into file descriptor fd=%d\n", nbytes, fd);           
    return nbytes;
}

int mycp(char *src, char *dest){
    char buf[BLKSIZE];
    int fd = open_file(src, 0); // R = 0
    int gd = open_file(dest, 1); // W = 1
    int n = 0;

    //make sure files opened
    if (fd == -1) {
        printf("ERROR: src file %s failed to open\n", src);
        close_file(fd);
        close_file(gd);
        return -1;
    }

    if (gd == -1) { 
        printf("ERROR: dest file %s failed to open\n", dest);
        close_file(fd);
        close_file(gd);
        return -1;
    }

    while(n = myread(fd, buf, BLKSIZE))
       mywrite(gd, buf, n);  // notice the n in write()

    close_file(fd);
    close_file(gd);
    return 0;
}

int mymv(char *src, char *dest){
    //verify src exists; get its INODE in ==> you already know its dev
    //check whether src is on the same dev as src
    int ino = getino(src);
    if(ino == -1) {
        printf("ERROR: %s does not exist\n", src);
        return -1;
    }

    //CASE 1: same dev:
    //Hard link dst with src (i.e. same INODE number)
    mylink(src, dest);
    //4. unlink src (i.e. rm src name from its parent directory and reduce INODE's link count by 1).
    myunlink(src);
}