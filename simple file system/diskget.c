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

int find_file(struct superblock_t* sb,void* address,char* filename);
void copy_procedure(void* address, struct dir_entry_t* de, struct superblock_t* sb, char* filename);
int find_next_block(int file_starting_block, void *address, struct superblock_t* sb);


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


    struct dir_entry_t* de;

    //part3
    //first find file in sub_dir
    //./diskget test.img /foo2.txt foo.txt

    //printf("\npart3\n");
    //printf("routine: %s\n", argv[2]);
    char argument[50];
    strcpy(argument,argv[2]);
    // Extract the first token
    char * token = strtok(argument, "/");

    char filename[50];
    strcpy(filename, token);
    //printf( "file name: %s\n", filename);
    //printf("new file name: %s\n", argv[3]);

    //find file in root
    //find_file(address,htonl(sb->root_dir_start_block),htons(sb->block_size),htonl(sb->root_dir_block_count),filename);
    //find_file in sub_dir
    //de = find_sub_dir(sb,sub_dir, address);

    //find_file(address,htonl(sb->root_dir_start_block), htons(de->block_count),htonl(de->block_count), filename);
    int file_addr = find_file(sb,address,filename);
    de = (struct dir_entry_t*)(address + file_addr);

    copy_procedure(address, de, sb, argv[3]);

    munmap(address,buffer.st_size);
    close(fd);

}
int find_file(struct superblock_t* sb, void* address, char* filename){//, struct dir_entry_t* de, ){
  struct dir_entry_t* de;
  int offset = htonl(sb->root_dir_start_block) * htons(sb->block_size);

  for (int i =0; i < htonl(sb->root_dir_block_count);i++){

    //printf("offset: %d, add value: %d ",offset,64);
    de = (struct dir_entry_t*)(address + offset + 64*i);
    if (de->status >> 1 == 1){
      if (strcmp(de->filename,filename) == 0){
        //printf("it has file\n");
        //printf("pos: %x\n", offset + 64*i);

        int block = 0;
        void * file_addr = address + offset + 64*i;
        //memcpy(&block, (void*) file_addr, 4);
        //printf("%x\n", block);
        //printf("file starting block: %x\n", htonl(de->starting_block));
        //printf("file block num: %x\n", htonl(de->block_count));
        //printf("file size: %d\n", htonl(de->size));
        //function: put in the number of block, get that block informatio
        //function: put in the number of block, get the next number of block by fat
        return offset + 64*i;
      }
    }
  }
}
void copy_procedure(void* address, struct dir_entry_t* de, struct superblock_t* sb, char * filename){
  int first_block = htonl(de->starting_block)* htons(sb->block_size);
  //printf("file size: %d\n", htonl(de->size));
  //printf("first_block: 0x%x\n", first_block);
  //printf("block count: %d\n", htonl(de->block_count));

  FILE *fp = fopen(filename, "w");

  char content[htonl(de->size)];
  int add_size;
  int remaining_size = htonl(de->size);
  int curr_block = first_block;
  int next_block;
  int content_pos = 0;
  for (int i=0; i< htonl(de->block_count); i++){
    if (htons(sb->block_size) < remaining_size){//remaining file > block size
        add_size = htons(sb->block_size);
        remaining_size -= add_size;
    }
    else{//remaining file < block size
      add_size = remaining_size;
      remaining_size = 0;
    }

    strcpy(content,memcpy(&content, (char*) address + curr_block, add_size));
    fwrite(content ,1,add_size, fp);
    //content_pos += add_size;
    if (i+1 != htonl(de->block_count)){
      next_block = find_next_block(curr_block/htons(sb->block_size), address,(struct superblock_t*) sb);
      //printf("next_block: %x\n", next_block);
      curr_block = next_block * htons(sb->block_size);
    }
    if (remaining_size == 0){
      break;
    }
  }

  fclose(fp);


}
int find_next_block(int file_starting_block, void *address, struct superblock_t* sb){
  int block = 0;
  int fat_start = htonl(sb->fat_start_block) * htons(sb->block_size);
  memcpy(&block, (void*) address + fat_start + 4*file_starting_block, 4);
  return (int)htonl(block);
}
