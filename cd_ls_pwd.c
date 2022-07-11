/************* cd_ls_pwd.c file **************/

#include "functions.h"

int cd()
{
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    if(!S_ISDIR(mip->INODE.i_mode)){
        printf("Error: %s is not a dir!\n", pathname);
        iput(running->cwd);
        return 0;
    }
    iput(running->cwd);
    running->cwd = mip;
}

int ls_file(MINODE *mip, char *name)
{
    char buf[BLKSIZE], temp[256];
    struct stat filestats;
    INODE node = mip->INODE;
    char time_holder[50]="";
    
    //print out permission data
    (S_ISDIR(node.i_mode)) ? printf("d") : printf("-");
    (node.i_mode & S_IRUSR) ? printf("r") : printf("-");
    (node.i_mode & S_IWUSR) ? printf("w") : printf("-");
    (node.i_mode & S_IXUSR) ? printf("x") : printf("-");
    (node.i_mode & S_IRGRP) ? printf("r") : printf("-");
    (node.i_mode & S_IWGRP) ? printf("w") : printf("-");
    (node.i_mode & S_IXGRP) ? printf("x") : printf("-");
    (node.i_mode & S_IROTH) ? printf("r") : printf("-");
    (node.i_mode & S_IWOTH) ? printf("w") : printf("-");
    (node.i_mode & S_IXOTH) ? printf("x") : printf("-");
    printf("  %5li ", node.i_links_count);

    //print out group and user
    struct passwd *uid;
    struct group *gid;
    
    // read in uid as a formatted type and print it out.
    uid = getpwuid(node.i_uid);
    printf("%s ",uid->pw_name);
    gid = getgrgid(node.i_gid);
    printf("%s ",gid->gr_name);
    
    //print file size in bytes
    printf("%8li ", node.i_size);
    
    //update timeinfo and print
    struct tm* time_information = localtime(&(node.i_mtime));
    strftime(time_holder,49,"%b %d %R",time_information);
    printf("%s ",time_holder);
}

int ls_dir(MINODE *mip)
{
    char buf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;
    MINODE *temp_mip;

    get_block(dev, mip->INODE.i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;
    
    while (cp < buf + BLKSIZE){
        temp_mip = iget(dev, dp->inode);
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        
        ls_file(temp_mip,temp);
        iput(temp_mip);
        printf("%s  \n", temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    printf("\n");
}

int ls()
{
    int ino = getino(pathname);
    if(ino != 0){
        MINODE *mip = iget(dev, ino);
        ls_dir(mip);
    }
    else{
        ls_dir(running->cwd);
    }
}

int rpwd(MINODE *wd)
{
  if (wd == root){
    printf("CWD = ");
    return;
  }
  
  int my_ino;
  int parent_ino = findino(wd, &my_ino);
  MINODE *pip = iget(dev, parent_ino);
  char my_name[128] = "";
  findmyname(pip, my_ino, my_name);
  rpwd(pip);
  printf("/%s", my_name);
}

int pwd(MINODE *wd)
{
  if (wd == root){
    printf("CWD = /\n");
    return;
  }
  
  rpwd(wd);
  printf("\n");
}

int get_pwd(MINODE *wd, char *output)         //puts pwd in output
{
    if (wd == root){
        strcpy(output, "/");
        return 0;
    }
    
    int my_ino;
    int parent_ino = findino(wd, &my_ino);
    MINODE *pip = iget(dev, parent_ino);
    char my_name[128] = "";
    findmyname(pip, my_ino, my_name);
    rpwd(pip);
    strcat(output, my_name);
    strcat(output, "/");
    return 0;
}