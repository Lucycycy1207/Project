#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h> // i add this line
#include <unistd.h>// i add this line
#include <math.h>

struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

struct __attribute__((__packed__)) dir_entry_timedate_t {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

struct __attribute__((__packed__)) dir_entry_t {
  uint8_t status;
  uint32_t starting_block;
  uint32_t block_count;
  uint32_t size;
  struct dir_entry_timedate_t modify_time;
  struct dir_entry_timedate_t create_time;
  uint8_t filename[31];
  uint8_t unused[6];
};

int FAT_info(void* address, int sb, int bc, int bs, int status);

void main(int argc, char* argv[]) {

    /////////////////////////////////////////////////
    //part 1
    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    int status = fstat(fd, &buffer);
    //tamplate:   pa=mmap(addr, len, prot, flags, fildes, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb=(struct superblock_t*)address;
    printf("Super block information:\n");
    printf("Block size: %d\n", htons(sb->block_size));

    printf("Block count: %d\n", ntohl(sb->file_system_block_count));
    printf("FAT starts: %d\n", htonl(sb->fat_start_block));
    printf("FAT blocks: %d\n", htonl(sb->fat_block_count));
    printf("Root directory start: %d\n", htonl(sb->root_dir_start_block));
    printf("Root directory blocks: %d\n", htonl(sb->root_dir_block_count));

    printf("\nFAT information:\n");
    int free_block = FAT_info(address, htonl(sb->fat_start_block), htonl(sb->fat_block_count), htons(sb->block_size), 0);
    printf("Free Blocks: %d\n", free_block);
    int reversed_block = FAT_info(address, htonl(sb->fat_start_block), htonl(sb->fat_block_count), htons(sb->block_size), 1);
    printf("Reserved Blocks: %d\n", reversed_block);
    int allocated_block = ntohl(sb->file_system_block_count) -free_block - reversed_block;
    printf("Allocated Blocks: %d\n", allocated_block);

    munmap(address,buffer.st_size);
    close(fd);

}
int FAT_info(void* address, int sb, int bc, int bs, int status){
    int fat_end = (sb + bc - 1) * bs;
    int count = 0;
    for (int i = 0; i < fat_end; i += 4) {
      int block = 0;
      int offset = sb * bs + i;
      memcpy(&block, (char*) address + offset, 4);

      if ((int)htonl(block) == status) count++;
    }
    return count;

}
