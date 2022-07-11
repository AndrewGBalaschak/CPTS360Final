#include "functions.h"

int mylink(char *oldfilename, char *newfilename){
	char * parent = (char *)malloc(sizeof(newfilename)), * child = (char *)malloc(sizeof(newfilename)), buf[BLKSIZE];
	int oino, pino;
	MINODE *omip, *pmip;

	//check that old file exists
	oino = getino(oldfilename);
	if(!oino){
        printf("ERROR: %s does not exist\n", oldfilename);
        return -1;
    }
    
    omip = iget(dev, oino);

    //file to link to must not be DIR
    if(S_ISDIR(omip->INODE.i_mode)){
        printf("ERROR: Cannot link DIRs\n");
        iput(omip);
        return -1;
    }
    //new file must have name
    if(strlen(newfilename) == 0){
        printf("ERROR: new file name cannot be empty");
        iput(omip);
        return -1;
    }
    //new file must not exist yet
    if(getino(newfilename) != 0){
        printf("ERROR: %s already exists\n", newfilename);
        iput(omip);
        return -1;
    }

	//divide newfilename into parent and child
    parse_tokens(newfilename, parent, child);

	pino = getino(parent);
	pmip = iget(omip->dev, pino);

	//create entry in new parent DIR with same inode number of old_file
	enter_name(pmip, oino, child);
	
	omip->INODE.i_links_count++;                //inc INODE's links_count by 1
	omip->dirty = 1;                            //for write back by iput(omip)
	memset(buf, 0, BLKSIZE);                    //clear buffer
	iput(omip);
	iput(pmip);

	return 0;
}

int myunlink(char *filename){
	char * parent = (char *)malloc(sizeof(filename)), * child = (char *)malloc(sizeof(filename)), buf[BLKSIZE];
    int ino, pino;
    MINODE *mip, *pmip;

    parse_tokens(filename, parent, child);

	ino = getino(filename);
	if(!ino){
		printf("ERROR: %s does not exist\n", filename);
		return -1;
	}
	
	mip = iget(dev, ino);

	//file to unlink to must not be DIR
    if(S_ISDIR(mip->INODE.i_mode)){
        printf("ERROR: Cannot unlink DIRs\n");
        return -1;
    }
    //check it’s a REG or symbolic LNK file
    else if(S_ISLNK(mip->INODE.i_mode)){
        printf("File is LINK type\n");
    }
    else if(S_ISREG(mip->INODE.i_mode)){
        printf("File is REG type\n");
    }

	pino = getino(parent);
	pmip = iget(dev, pino);
	rm_child(pmip, child);
	pmip->dirty = 1;
	iput(pmip);

	//decrement INODE’s link_count by 1
    mip->INODE.i_links_count--;

    if(mip->INODE.i_links_count > 0)
        mip->dirty = 1;                         //if links_count > 0: write INODE back to disk
	else{
		//deallocate all data blocks in INODE
		for (int i = 0; i < 12; i++) {
			if (mip->INODE.i_block[i] == 0)
                break;
			bdalloc(dev, mip->INODE.i_block[i]);
			mip->INODE.i_block[i] = 0;
		}
		//deallocate INODE;
		idalloc(dev, ino);
		mip->INODE.i_blocks = 0;
		mip->INODE.i_size = 0;
		mip->dirty = 1;
	}
	iput(mip);                                  //release mip
    printf("Successfully unnlinked\n");
	return 0;
}