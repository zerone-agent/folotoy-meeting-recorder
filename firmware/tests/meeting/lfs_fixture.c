#define _POSIX_C_SOURCE 200809L
#include "lfs.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#define SZ 0x4f0000
static int fd;
static int rd(const struct lfs_config*c,lfs_block_t b,lfs_off_t o,void*v,lfs_size_t n){(void)c;return pread(fd,v,n,b*4096+o)==n?0:-5;}
static int wr(const struct lfs_config*c,lfs_block_t b,lfs_off_t o,const void*v,lfs_size_t n){(void)c;return pwrite(fd,v,n,b*4096+o)==n?0:-5;}
static int er(const struct lfs_config*c,lfs_block_t b){(void)c;unsigned char x[4096];memset(x,255,sizeof x);return pwrite(fd,x,sizeof x,b*4096)==sizeof x?0:-5;}
static int sy(const struct lfs_config*c){(void)c;return fsync(fd);}
int main(int argc,char **argv){if(argc!=2)return 1;fd=open(argv[1],O_RDWR|O_CREAT|O_EXCL,0600);if(fd<0)return 1;unsigned char x[4096];memset(x,255,sizeof x);for(int i=0;i<SZ/4096;i++)if(write(fd,x,sizeof x)!=sizeof x)return 1;struct lfs_config c={.read=rd,.prog=wr,.erase=er,.sync=sy,.read_size=128,.prog_size=128,.block_size=4096,.block_count=SZ/4096,.block_cycles=512,.cache_size=512,.lookahead_size=128};lfs_t l;lfs_file_t f;if(lfs_format(&l,&c)||lfs_mount(&l,&c)||lfs_mkdir(&l,"/rec")||lfs_file_open(&l,&f,"/rec/123.dat",LFS_O_CREAT|LFS_O_WRONLY)||lfs_file_write(&l,&f,"abc",3)!=3||lfs_file_close(&l,&f)||lfs_unmount(&l))return 1;return close(fd)!=0;}
