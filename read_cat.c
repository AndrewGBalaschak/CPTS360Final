#include "functions.h"

int myread(int fd, char *buf, int nbytes){
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    int count=0, lbk, blk, offset, start_byte, remain, avail, *ip;
    char read_buf[BLKSIZE], buf12[BLKSIZE], buf13[BLKSIZE];
    
    offset = oftp->offset;                      //byte offset in file to READ
    avail = mip->INODE.i_size - oftp->offset;

    if(nbytes < 1){
        printf("ERROR: cannot read %d bytes\n", nbytes);
        return -1;
    }
    if(oftp == 0){
        printf("ERROR: file not open\n");
        return -1;
    }
    if(oftp->mode != 0 && oftp->mode != 2){
        printf("ERROR: file not open in READ mode\n");
        return -1;
    }

    while(nbytes && avail){
        lbk = oftp->offset / BLKSIZE;           //logical block number
        start_byte = oftp->offset % BLKSIZE;    //start_byte for block

        //direct
        if(lbk < 12){
            blk = mip->INODE.i_block[lbk];      //convert lbk to physical block number
        }

        //indirect
        else if(lbk >=12 && lbk < 256+12){
            get_block(mip->dev, mip->INODE.i_block[12], buf12);
            blk = buf12[lbk-12];                //get the blk number
            //getchar();
        }

        //double indirect
        else{
            lbk -= (12 + 256);
            get_block(mip->dev, mip->INODE.i_block[13], buf13);
            //which 256 chuck is lbk in?
            //which blk is lbk in that chuck? MAILMAN's algorithm
            ip = buf13 + (lbk / 256);
            get_block(mip->dev, *ip, buf13);
            blk = buf13 + (lbk % 256);
        }
        if(blk <= 0)                            //if we are at an empty block
            return count;
            
        get_block(mip->dev, blk, read_buf);
        char *cp = read_buf + start_byte;
        remain = BLKSIZE - start_byte;

        //copy data
        /*
        while(remain>0){
            if(*cp == -1){
                avail = 0;
                break;
            }

            *buf = *cp;                         //copy to buf
            
            //advance pointers and offsets
            buf++;
            cp++;
            oftp->offset++;
            offset++;
            count++;

            //decrement remaining bytes
            remain--;
            avail--;
            nbytes--;

            if(avail <= 0 || nbytes <= 0)
                break;
        }*/

        //don't copy more than remaining bytes
        if(nbytes > remain)
            nbytes = remain;
        
        memmove(buf, cp, nbytes);               //copy to buf

        //advance pointers and offsets
        buf += nbytes;
        cp += nbytes;
        oftp->offset += nbytes;
        offset += nbytes;
        count += nbytes;

        //decrement remaining bytes
        avail -= nbytes;
        remain -= nbytes;
        nbytes -= nbytes;
    }
    return count;
}

int mycat(char *pathname){
	char mybuf[BLKSIZE], dummy = 0;
	int i;

	int fd = open_file(pathname, 0);            //open pathname for READ

	if (fd == -1) {
        close_file(fd); 
        printf("ERROR: %s failed to open\n", pathname, fd);
        return -1;
    }

	printf("\n");
	while(i = myread(fd, mybuf, 1024)){
		mybuf[i] = 0;                           //as a null terminated string
		printf("%s", mybuf);                    //spit out chars from mybuf[]
	}
	printf("\n");
	close_file(fd);                             //close fd

	return 0;
}