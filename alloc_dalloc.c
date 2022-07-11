 /*********** alloc-dalloc.c file ****************/

#include "functions.h"

int tst_bit(char *buf, int bit){
    int i = bit/8;
    int j = bit%8;
    if(buf[i] & (1<<j))
        return 1;
    else
        return 0;
}

int set_bit(char *buf, int bit){
    int i = bit/8;
    int j = bit%8;
    buf[i] |= (1<<j);
}

int clr_bit(char *buf, int bit){
    buf[bit/8] &= ~(1 <<(bit%8));
}

int decFreeInodes(int dev){
    char buf[BLKSIZE];
    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev){                                // allocate an inode number from inode_bitmap
    int  i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){                    // use ninodes from SUPER block
        if (tst_bit(buf, i)==0){
            set_bit(buf, i);
            put_block(dev, imap, buf);
            decFreeInodes(dev);

            printf("allocated ino = %d\n", i+1);    // bits count from 0; ino from 1
            return i+1;
        }
    }
    printf("ERROR: ialloc failed\n");
    return 0;
}

int decFreeBlocks(int dev){
    char buf[BLKSIZE];
    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

//feel like this is not correct
int balloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);

    for(i = 0; i < nblocks; i++){
        if(tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            decFreeBlocks(dev);

            printf("allocated blk = %d\n", i+1);
            return i+1;
        }
    }
    return 0;
}

MINODE *mialloc(){                                  //allocate a FREE minode for use
    for(int i = 0; i < NMINODE; i++){
        MINODE *mp = &minode[i];
        if (mp->refCount == 0){
            mp->refCount = 1;
            return mp;
        }
    }
    printf("FS panic: out of minodes\n");
    return 0;
}


//DALLOC

int midalloc(MINODE *mip){
    mip->refCount = 0;
}

int incFreeInodes(int dev){
    char buf[BLKSIZE];

    //increment free inodes on superblock
    get_block(dev, 1, buf);
    sp = (SUPER *) buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    //increment free inodes on group desc
    get_block(dev, 2, buf);
    gp = (GD *) buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int incFreeBlocks(int dev){
    char buf[BLKSIZE];

    //increment free blocks on superblock
    get_block(dev, 1, buf);
    sp = (SUPER *) buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    //increment free blocks on group desc
    get_block(dev, 2, buf);
    gp = (GD *) buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int idalloc(int dev, int ino){
    char buf[BLKSIZE];

    if(ino > ninodes){
        printf("ERROR: inumber %d out of range\n", ino);
        return -1;
    }
    get_block(dev, imap, buf);                  //read block
    clr_bit(buf, ino-1);                        //mark inode as free
    put_block(dev, imap, buf);                  //write back to disk
    incFreeInodes(dev);
    return 1;
}

int bdalloc(int dev, int bno){
    char buf[BLKSIZE];

    if(bno > nblocks){
        printf("ERRORL bno %d out of range\n", bno);
        return -1;
    }

    get_block(dev, bmap, buf);
    clr_bit(buf, bno-1);
    put_block(dev, bmap, buf);
    incFreeBlocks(dev);

    printf("deallocated blk = %d\n", bno);
    return 0;
}