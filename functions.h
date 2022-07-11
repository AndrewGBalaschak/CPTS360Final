  #ifndef FUNCTIONS
#define FUNCTIONS

// C Libs:
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

// USR
#include "type.h"
#include "globals.h"


/* util.c */  
int get_block(int dev, int blk, char *buf);

int put_block(int dev, int blk, char *buf);

int tokenize(char *pathname);

int parse_tokens(char *pathname, char *dname, char *bname);

MINODE *iget(int dev, int ino);

void iput(MINODE *mip);

int search(MINODE *mip, char *name);

int getino(char *pathname);

int findmyname(MINODE *parent, u32 myino, char myname[]);

int findino(MINODE *mip, u32 *myino);

/* cd_ls_pwd.c */
int cd();

int ls_file(MINODE *mip, char *name);

int ls_dir(MINODE *mip);

int ls();

int pwd(MINODE *wd);

int get_pwd(MINODE *wd, char *output);

/* alloc-dalloc.c */
int tst_bit(char *buf, int bit);

int set_bit(char *buf, int bit);

int clr_bit(char *buf, int bit);

int decFreeInodes(int dev);

int ialloc(int dev);

int decFreeBlocks(int dev);

int midalloc(MINODE *mip);

int balloc(int dev);

int incFreeInodes(int dev);

int idalloc(int dev, int ino);

int bdalloc(int dev, int bno);

/* mkdir_creat.c */
int mymkdir(char *pathname);

int mkdirhelper(MINODE *pmip, char *basename);

int creathelper(MINODE *pmip, char *name);

int enter_name(MINODE *pip, int ino, char *name);

int mycreat();

/* rmdir.c */

int myrmdir(char *pathname);

int rm_child(MINODE *pmip, char *name);

/* link_unlink.c */
int mylink(char *oldfilename, char *newfilename);

int myunlink(char *filename);

/* symlink.c */
int mysymlink(char *old_file, char *new_file);

int myreadlink(char *file, char *buffer);

/* open_close.c */
int mytruncate(MINODE *mip);

int open_file(char *pathname, int mode);
      
int close_file(int fd);

int pfd();

/* read_cat.c*/
int myread(int fd, char *buf, int nbytes);

int mycat(char *pathname);

/* write_cp.c */
int write_file(int fd, int nbytes);

int mywrite(int fd, char buf[ ], int nbytes);

int mycp(char *src, char *dest);

int mymv(char *src, char *dest);

#endif