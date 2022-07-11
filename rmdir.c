#include "functions.h"

int myrmdir(char *pathname){
    char *cp, buf[BLKSIZE], parent[128], child[128];

    //get in-memory INODE of pathname
    int ino = getino(pathname);
    if(ino == -1){
        printf("ERROR: %s does not exist", pathname);
        return -1;
    }
    MINODE *mip = iget(dev, ino);

    if(!S_ISDIR(mip->INODE.i_mode)){        //check that mip is DIR
        printf("ERROR: %s is not a DIR\n", pathname);
        iput(mip);
        return -1;
    }         
    if(!(mip->refCount = 1)){               //check that mip is NOT BUSY
        printf("ERROR: %s is BUSY\n", pathname);
        return -1;
    }
    if(mip->INODE.i_links_count > 2){       //preliminary check that mip is EMPTY
        printf("ERROR: %s is not empty\n", pathname);
        return -1;
    }
    
    //step through dir_entries to make sure it only has . and ..
    get_block(dev, mip->INODE.i_block[0], buf);
    dp = (DIR *) buf;
    cp = buf;
    int count = 0;
    while(cp < buf + BLKSIZE){
        count++;
        cp += dp->rec_len;
        dp = (DIR *) cp;
    }
    if(count > 2){
        printf("ERROR: %s is not empty\n", pathname);
        return -1;
    }

    //get parent's ino and INODE
    int pino = search(mip, "..");
    MINODE *pmip = iget(dev, pino);

    parse_tokens(pathname, parent, child);

    if(rm_child(pmip, child) == -1){
        iput(pmip);
        iput(mip);
        return -1;
    }
    
    //dec parent links_count by 1, mark parent pmip dirty
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    //deallocate data blocks and inode
    for(int i = 0; i < 12; i++){
        if(mip->INODE.i_block[i] == 0)
            break;
        bdalloc(dev, mip->INODE.i_block[i]);
    }
    idalloc(dev, mip->ino);
    iput(mip);

    return 0;
}

int rm_child(MINODE *pmip, char *name){  
    char buf[BLKSIZE];
    DIR *dp, *prev_dp;
    char *cp;

    //search parent's INODE data blocks for name
    for(int i = 0; i < 12; i++){
        if(pmip->INODE.i_block[i] == 0)             //break on null i_block
            break;
        
        get_block(dev, pmip->INODE.i_block[i], buf);
        dp = (DIR *) buf;
        cp = buf;
        char searchname[64];

        while(cp < buf + BLKSIZE){
            //delete name from parent directory
            memset(searchname, 0, 64);
            strncpy(searchname, dp->name, dp->name_len);
            printf("TO REMOVE: %s\n", name);
            printf("CURRENT AT: %s\n", searchname);

            if(strcmp(name, searchname) == 0){                          //if name is in block
                printf("found child\n");
                //if first and only entry in data block
                if(cp == buf && (cp + dp->rec_len) == (buf + BLKSIZE)){
                    printf("Child is first and only entry\n");
                    //deallocate & reduce parent's size by BLKSIZE
                    bdalloc(dev, pmip->INODE.i_block[i]);
                    pmip->INODE.i_size -= BLKSIZE;

                    //compact parent's i_block[] array if between nonzero entries
                    for(int j = i; j < 12; j++){
                        if(dev, pmip->INODE.i_block[j+1] == 0)          //break on last i_block
                            break;
                        get_block(dev, pmip->INODE.i_block[j], buf);
                        put_block(dev, pmip->INODE.i_block[j-1], buf);
                    }
                }
                //else if last entry in block
                else if((cp + dp->rec_len) == (buf + BLKSIZE)){
                    printf("Child is last entry\n");
                    //absorb rec_len into predecessor
                    prev_dp->rec_len += dp->rec_len;
                    put_block(dev, pmip->INODE.i_block[i], buf);
                }

                //else entry is first but not only entry or is in middle
                else{
                    printf("Child is first but not only entry or in middle\n");
                    DIR *temp_dp = (DIR *) buf;
                    char *temp_cp = buf;

                    //move to last entry
                    while((temp_cp + temp_dp->rec_len) < (buf + BLKSIZE)){
                        temp_cp += temp_dp->rec_len;
                        temp_dp = (DIR *) temp_cp;
                    }

                    //absorb rec_len into last entry
                    temp_dp->rec_len += dp->rec_len;

                    //move all trailing entries LEFT
                    cp += dp->rec_len;
                    int size = (buf + BLKSIZE) - cp;
                    memcpy(dp, cp, size);

                    //add deleted rec_len to last entry, no change in parent's file size
                    put_block(dev, pmip->INODE.i_block[i], buf);
                }
                //successfully unlinked
                printf("Unlink success\n");
                return 0;
            }

            //increment pointers
            prev_dp = dp;
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }
    }

    printf("ERROR: Did not find child %s, rm_child failed\n", name);
    return -1;
}