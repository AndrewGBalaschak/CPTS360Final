#include "functions.h"

//creates node and data blocks
int mkdirhelper(MINODE *pmip, char *basename){
    //allocate INODE and disk block
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //create INODE
    MINODE *mip = iget(dev, ino);                   //load INODE into memory
    INODE *ip = &(mip->INODE);

    ip->i_mode = 0x41ED;                            //init INODE as DIR
    ip->i_uid = running->uid;                       //owner uid
    ip->i_gid = running->gid;                       //group id
    ip->i_size = BLKSIZE;                           //size in bytes
    ip->i_links_count = 2;                          //2 links for . and ..
    mip->refCount = 0;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;                               //blocks are 512-byte chunks
    ip->i_block[0] = blk;                           //DIR has one data block
    for(int i = 1; i <= 14; i++)                    //init rest of blocks
        ip->i_block[i] = 0;
    mip->dirty = 1;                                 //mip is DIRTY
    iput(mip);                                      //write INODE to disk

    //create data block for . and ..
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE);
    DIR *dp = (DIR *) buf;
    char *cp = buf;

    //make .
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    //move dp to next entry
    cp += dp->rec_len;
    dp = (DIR *) cp;

    //make ..
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12;                       //rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, blk, buf);                       //write to block on disk
    enter_name(pmip, ino, basename);                //register new dir to parent
}

int creathelper(MINODE *pmip, char *name){
    //allocate INODE
    int ino = ialloc(dev);

    //create INODE
    MINODE *mip = iget(dev, ino);                   //load INODE into memory
    INODE *ip = &mip->INODE;

    ip->i_mode = 0100644;                           //init INODE as DIR
    ip->i_uid = running->uid;                       //owner uid
    ip->i_gid = running->gid;                       //group id
    ip->i_size = 0;                                 //size in bytes
    ip->i_links_count = 1;                          //only one link, parent
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 0;                               //blocks are 512-byte chunks
    for(int i = 0; i <= 14; i++)                    //init rest of blocks
        ip->i_block[i] = 0;
    mip->dirty = 1;                                 //mip is DIRTY
    iput(mip);                                      //write INODE to disk

    enter_name(pmip, ino, name);                    //register new dir to parent
}

//registers the node at ino to the parent pip
int enter_name(MINODE *pip, int ino, char *name){
    int i, ideal_length, need_length, remain, blk;
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;

    need_length  = 4*((8 + strlen(name) + 3) / 4);

    //trying to find space in existing data blocks
    for(i = 0; i < 12; i++){
        if(pip->INODE.i_block[i] == 0)          //break on null i_block
            break;  //to step 5
        blk = pip->INODE.i_block[i];

        //step to last entry
        get_block(dev, blk, buf);
        dp = (DIR *) buf;
        cp = buf;

        while((cp + dp->rec_len) < (buf + BLKSIZE)){
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }

        ideal_length = 4*((8 + dp->name_len + 3) / 4);
        remain = dp->rec_len - ideal_length;

        //if space in block
        if(remain >= need_length){
            //trim previous entry rec_len to ideal_length
            dp->rec_len = ideal_length;

            //insert new entry
            cp += ideal_length;
            dp = (DIR *) cp;
            dp->inode = ino;
            dp->rec_len = remain;
            dp->name_len = strlen(name);
            strncpy(dp->name, name, dp->name_len);

            put_block(pip->dev, blk, buf);
            return 1;
        }
    }
    //no space in existing blocks
    blk = balloc(dev);                          //allocate new data block
    pip->INODE.i_block[i] = blk;
    pip->INODE.i_size = BLKSIZE;                //increment parent size
    pip->dirty = 1;                             //parent is DIRTY
    get_block(pip->dev, blk, buf);              //read block
    dp = (DIR *) buf;

    //insert new entry
    dp->inode = ino;
    dp->rec_len = BLKSIZE;
    dp->name_len = strlen(name);
    strncpy(dp->name, name, dp->name_len);
    put_block(pip->dev, blk, buf);              //write back to disk
    return 1;
}

int mymkdir(char *pathname){
    //divide pathname into dirname and basename
    char dname[128] = "";
    char bname[128] = "";
    parse_tokens(pathname, dname, bname);

    if(strlen(bname) < 1){
        printf("ERROR: cannot have empty dirname\n");
        return -1;
    }

    int pino = getino(dname);                   //dirname exists and is DIR
    MINODE *pmip = iget(dev, pino);
    if(S_ISDIR(pmip->INODE.i_mode)){
        if(!search(pmip, bname)){               //basename not exist in parent DIR
            mkdirhelper(pmip, bname);
            pmip->INODE.i_links_count++;
            pmip->dirty = 1;
            iput(pmip);
            printf("mkdir successfuly executed\n");
            return 0;
        }
        else{
            printf("ERROR: %s already exists in %s\n", bname, dname);
            return -1;
        }
    }
    else{
        printf("ERROR: %s is not a DIR\n", dname);
        return -1;
    }
    printf("ERROR: mkdir failed\n");
    return -1;
}

int mycreat(){
    tokenize(pathname);

    char path[128] = "";                            //stores dirname
    if(n > 1){
        for(int i = 0; i < n-1; i++){
            strcat(path, "/");
            strcat(path, name[i]);
        }
    }else
        strcat(path, "/");

    printf("path: %s\n", path);
    printf("base: %s\n", name[n-1]);

    int pino = getino(path);                        //dirname exists and is DIR
    MINODE *pmip = iget(dev, pino);
    if(S_ISDIR(pmip->INODE.i_mode)){
        if(!search(pmip, name[n-1])){               //basename not exist in parent DIR
            creathelper(pmip, name[n-1]);
            pmip->INODE.i_links_count++;
            pmip->dirty = 1;
            iput(pmip);
        }
        else{
            printf("ERROR: %s already exists in %s\n", name[n-1], path);
        }
    }
    else{
        printf("ERROR: %s is not a DIR\n", path);
    }
}