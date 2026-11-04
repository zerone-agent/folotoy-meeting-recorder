/* Extract recording files from an offline LittleFS partition image.
 * Compile littlefs with LFS_READONLY: no code path can program the source. */
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE
#include "lfs.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PART_SIZE 0x4f0000u
#define BLOCK_SIZE 4096u
static int image_fd=-1;
static int read_block(const struct lfs_config *cfg,lfs_block_t block,lfs_off_t off,void *buf,lfs_size_t size)
{
    (void)cfg;
    if(block>=PART_SIZE/BLOCK_SIZE||off>BLOCK_SIZE||size>BLOCK_SIZE-off)return LFS_ERR_IO;
    off_t position=(off_t)block*BLOCK_SIZE+off;
    return pread(image_fd,buf,size,position)==(ssize_t)size?0:LFS_ERR_IO;
}
static int safe_name(const char *name)
{
    size_t n=strlen(name);if(!n||n>63||name[0]=='.')return 0;
    for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)name[i])&&name[i]!='.'&&name[i]!='_'&&name[i]!='-')return 0;
    return 1;
}
int main(int argc,char **argv)
{
    if(argc!=3){fprintf(stderr,"Usage: meeting_lfs_export PARTITION_IMAGE NEW_OUTPUT_DIRECTORY\n");return 2;}
    image_fd=open(argv[1],O_RDONLY|O_NOFOLLOW);struct stat statbuf;
    if(image_fd<0||fstat(image_fd,&statbuf)||!S_ISREG(statbuf.st_mode)||(uint64_t)statbuf.st_size!=PART_SIZE){
        fprintf(stderr,"Input must be a regular 0x4f0000-byte partition image.\n");if(image_fd>=0)close(image_fd);return 1;
    }
    if(mkdir(argv[2],0700)!=0){fprintf(stderr,"Output directory must not already exist.\n");close(image_fd);return 1;}
    int directory=open(argv[2],O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(directory<0){close(image_fd);return 1;}
    struct lfs_config cfg={.read=read_block,.read_size=128,.prog_size=128,.block_size=BLOCK_SIZE,
        .block_count=PART_SIZE/BLOCK_SIZE,.block_cycles=512,.cache_size=512,.lookahead_size=128};
    lfs_t lfs;int rc=lfs_mount(&lfs,&cfg);
    if(rc<0){fprintf(stderr,"Cannot mount image read-only (LittleFS error %d); keep the original image.\n",rc);goto done;}
    lfs_dir_t dir;rc=lfs_dir_open(&lfs,&dir,"/rec");
    if(rc<0){fprintf(stderr,"Cannot read recording directory (LittleFS error %d).\n",rc);lfs_unmount(&lfs);goto done;}
    struct lfs_info info;unsigned files=0;
    while((rc=lfs_dir_read(&lfs,&dir,&info))>0){
        if(!strcmp(info.name,".")||!strcmp(info.name,".."))continue;
        if(info.type!=LFS_TYPE_REG||!safe_name(info.name)){fprintf(stderr,"Unexpected recording entry; retained in source image.\n");rc=LFS_ERR_CORRUPT;break;}
        char path[70];int n=snprintf(path,sizeof(path),"/rec/%s",info.name);
        if(n<0||(size_t)n>=sizeof(path)){rc=LFS_ERR_INVAL;break;}
        lfs_file_t file;rc=lfs_file_open(&lfs,&file,path,LFS_O_RDONLY);
        if(rc<0)break;
        int fd=openat(directory,info.name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
        if(fd<0){lfs_file_close(&lfs,&file);rc=LFS_ERR_IO;break;}
        uint8_t buffer[4096];lfs_ssize_t got;size_t total=0;int failed=0;
        while((got=lfs_file_read(&lfs,&file,buffer,sizeof(buffer)))>0){
            size_t done=0;while(done<(size_t)got){ssize_t n=write(fd,buffer+done,(size_t)got-done);if(n<=0){failed=1;break;}done+=(size_t)n;}if(failed)break;total+=(size_t)got;
        }
        if(got<0||failed||total!=info.size||fsync(fd)!=0)failed=1;
        if(close(fd)!=0)failed=1;
        if(lfs_file_close(&lfs,&file)<0)failed=1;
        if(failed){rc=LFS_ERR_IO;break;}files++;
    }
    lfs_dir_close(&lfs,&dir);lfs_unmount(&lfs);
    if(rc<0)fprintf(stderr,"Export incomplete (error %d); retain the original image.\n",rc);
    else {printf("Exported %u files. Source image was opened read-only.\n",files);rc=0;}
 done:
    close(directory);close(image_fd);return rc<0?1:0;
}
