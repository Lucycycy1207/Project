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
void part2_output(struct dir_entry_t* de);

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

    //////////////////////////////////////////////////////
    //part2
    //need add subdirectory condition
    int condition = 0;//0 for root directory, 1 for sub directory
    /*
    char* sub_dir;

    if (argc == 3){
      sub_dir = argv[2]+1;
      printf("sub dir: %s\n", sub_dir);
      condition = 1;
    }
    */

    struct dir_entry_t* de;
    //need start block, block count
    int offset = htonl(sb->root_dir_start_block) * htons(sb->block_size);
    //printf("\ndirectory entry:\n");
    //root directory
    for (int i =0; i < htonl(sb->root_dir_block_count);i++){

      //printf("offset: %d, add value: %d ",offset,64);
      de = (struct dir_entry_t*)(address + offset + 64*i);
      part2_output(de);

    }

    /*
    else{
      de = find_sub_dir(sb, sub_dir, address);
      for (int i =0; i < htonl(de->block_count);i++){
      part2_output(de);
      }

    }
    */
    munmap(address,buffer.st_size);
    close(fd);

}
void part2_output(struct dir_entry_t* de){
  int notvalid = 0;
  if (de->status >> 1 == 1){
    printf("F ");
  }
  else if (de->status >> 2 == 1){
    printf("D ");
    }
  else{
    notvalid = 1;
  }
  if (notvalid == 0){
    printf("%10d ", htons(de->size));
    printf("%30s ", de->filename);
    struct dir_entry_timedate_t* time;
    time = &de->create_time;
    printf("%d/%02d/%02d %02d:%02d:%02d\n"
            , htons(time->year), time->month, time->day
            , time->hour, time->minute, time->second);
  }
}
