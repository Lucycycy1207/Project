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
void part2_output(struct dir_entry_t* de);
struct dir_entry_t* find_sub_dir(struct superblock_t* sb,char* sub_dir, void* address);
int find_file(struct superblock_t* sb,void* address,char* filename);
int find_next_block(int file_starting_block, void *address, struct superblock_t* sb);
void copy_procedure(void* address, struct dir_entry_t* de, struct superblock_t* sb, char* filename);
int find_empty_dir(struct superblock_t* sb,void * address);
void modify_dir(int free_dir_pos, void* address, uint32_t size, char* filename,int starting_block, int block_size);
int find_available_fat(struct superblock_t* sb, void * address);
int fat_value_update(int fat_pos,int new_fat_pos, void* address, struct superblock_t* sb);
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
    int free_block = FAT_info(address, htonl(sb->fat_start_block), htonl(sb->fat_block_count), htons(sb->block_size), 0);


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
    int offset;

    /*
    else{
      de = find_sub_dir(sb, sub_dir, address);
      for (int i =0; i < htonl(de->block_count);i++){
      part2_output(de);
      }

    }
    */
    /////////////////////////////////////////////////////////////

/*
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
    */
//////////////////////////////////////////////////////////////////////
    //part4
    //./diskput test.img foo.txt /foo3.txt
    //printf("part4\n");
    //printf("%d\n", argc);
    //printf("filename: %s\n", argv[2]);
    //check if it is in dir in linux
    FILE *file;
    if ((file = fopen(argv[2], "r"))){
        fclose(file);
    }
    else{
      printf("File not found\n");
      exit(-1);
    }
    //Check if the disk has enough space to store the file
    struct stat st;
    stat(argv[2], &st);
    uint32_t size = st.st_size;

    int available = free_block * htons(sb->block_size);
    //printf("available:%d\n", available);
    if (available < size){
      printf("No enough space to store this file\n");
      exit(-1);
    }

    //Create a new directory entry in the given path
    //printf("size:%d\n", size);
    char filename[31];
    strcpy(filename, argv[2]);
    //printf("filename:%s\n", filename);
    //printf("name:%s\n", filename);
    //printf("create time:%s\n", ctime(&st.st_ctime));//time of last status change
    //printf("modify time:%s\n", ctime(&st.st_mtime));
    double block_need = (double)size/htons(sb->block_size);
    int block_size = (int)ceil(block_need);
    //printf("block need:%d\n", block_size);//
    int free_dir_pos = find_empty_dir(sb, address);

    //free_dir_pos = 0x6a80;//need to delete
    //printf("this is empty dir: %x\n", free_dir_pos);



    //2 time wait ....
    //find available block
    //htonl(sb->fat_start_block)//start
    //htonl(sb->fat_block_count)// size
    //htonl(sb->fat_start_block), htonl(sb->fat_block_count), htons(sb->block_size), 0);
    int fat_pos = find_available_fat(sb,address);//count 4 byte from fat 0x400


    //printf("fat pos :%x\n", fat_pos);
    //block 80 can be the first block of file A00
    FILE * fp;
    fp = fopen(filename,"r");


    char word[size];
    //printf("content:\n");
    int word_pos = 0;
    char*show = word;
    int read_num = 0;
    int remaining_size = size;
    int new_fat_pos;
    int c = 0;
    for (int i=0; i< size;i++){
      fread(word+i,1,1,fp);
    }
    for (int i=1; i<block_size; i++){//each block

      if (htons(sb->block_size) < remaining_size){
        //printf("there\n");
        read_num = htons(sb->block_size);
        //printf("%d, %d\n", word_pos,read_num);
        c = 0;
        for (int j=word_pos; j< word_pos+read_num;j++){
          //printf("%c", *(show+j));
          offset = fat_pos * htons(sb->block_size);
          memcpy((void*) address + offset+c, (show+j), 2);
          //printf("address: %x\n", offset+c);
          c++;
        }
        word_pos += read_num+1;
        remaining_size-=read_num;
      }
      else{
        //printf("\nhere\n");
        read_num = remaining_size;
        c = 0;
        //printf("%d, %d\n", word_pos,read_num);
        for (int j=word_pos; j< word_pos+read_num;j++){
          offset = fat_pos * htons(sb->block_size);
          memcpy((void*) address + offset+c, (show+j), 2);
          c++;
          //printf("%c", *(show+j));
        }
        word_pos += read_num+1;
        continue;
      }
      fat_value_update(fat_pos,0x99,address,sb);
      new_fat_pos = find_available_fat(sb,address);
      fat_value_update(fat_pos,new_fat_pos,address,sb);

      fat_pos = new_fat_pos;
    }

   // Extract the first token
   char * token = strtok(argv[3], "/");
   strcpy(filename, token);
    //strcpy(filename, token);
    modify_dir(free_dir_pos, address, size, filename,fat_pos, block_size);

    //put in data: block pos,

    //memcpy((void*) address + free_dir_pos, size, 1);

    munmap(address,buffer.st_size);
    close(fd);

}
int fat_value_update(int fat_pos,int new_fat_pos, void* address, struct superblock_t* sb){
  int offset = htonl(sb->fat_start_block)*htons(sb->block_size)+(fat_pos*4);
  //printf("fat value update: %x;address: %x , new fat pos:%x\n", fat_pos,offset, new_fat_pos);
  //file size
  uint32_t pos1 = new_fat_pos>>4*6;
  uint32_t pos2 = new_fat_pos>>4*4 & 0xff;
  uint32_t pos3 = new_fat_pos>>4*2 & 0xff;
  uint32_t pos4 = new_fat_pos &0xff;
  //printf("values:%d,%d,%d,%d\n",pos1,pos2,pos3,pos4);
  memcpy((void*) address + offset, &pos1, 2);
  memcpy((void*) address + offset+1, &pos2, 2);
  memcpy((void*) address + offset+2, &pos3, 2);
  memcpy((void*) address + offset+3, &pos4, 2);


}
int find_available_fat(struct superblock_t* sb, void * address){
  int fat_end = (htonl(sb->fat_start_block) + htonl(sb->fat_block_count) - 1) * htons(sb->block_size);
  int block = 0;
  for (int i = 0; i < fat_end; i ++) {
    int offset = htonl(sb->fat_start_block) * htons(sb->block_size) + i*4;
    memcpy(&block, (char*) address + offset, 4);
    if ((int)htonl(block) ==0){
      //printf("fat pos:%x is available with address:%x\n", i, offset);
      return i;
    }
  }
}
void modify_dir(int free_dir_pos, void* address, uint32_t size, char* filename,int starting_block, int block_size){
  //change status to 0x03
  memcpy((void*) address + free_dir_pos, "\x3", 1);
  //starting block 4
  //num of block 4

  //printf(" dir: %x\n", free_dir_pos+9);//(unsigned char*)&(size))
  /*
  printf("size:%0x\n", size>>4*6);
  printf("size:%x\n", size>>4*4 & 0xff);
  printf("size:%x\n", size>>4*2 & 0xff);
  printf("size:%x\n", size &0xff);
  */
  //file size
  uint32_t pos1 = size>>4*6;
  uint32_t pos2 = size>>4*4 & 0xff;
  uint32_t pos3 = size>>4*2 & 0xff;
  uint32_t pos4 = size &0xff;
  memcpy((void*) address + free_dir_pos+9, &pos1, 2);
  memcpy((void*) address + free_dir_pos+9+1, &pos2, 2);
  memcpy((void*) address + free_dir_pos+9+2, &pos3, 2);
  memcpy((void*) address + free_dir_pos+9+3, &pos4, 2);
  //filename
  int finish = -1;
  for(int i=0;i<32;i++){
    if (filename[i] == '\0'){
      finish = 1;
    }
    if (finish != -1){
      memcpy((void*) address + free_dir_pos+27+i, "\x0", 1);
    }
    else{
    //printf("%c\n", filename[i]);
    memcpy((void*) address + free_dir_pos+27+i, &filename[i], 1);}
  }
  //starting blocks
  pos1 = starting_block>>4*6;
  pos2 = starting_block>>4*4 & 0xff;
  pos3 = starting_block>>4*2 & 0xff;
  pos4 = starting_block &0xff;

  memcpy((void*) address + free_dir_pos+1, &pos1, 2);
  memcpy((void*) address + free_dir_pos+1+1, &pos2, 2);
  memcpy((void*) address + free_dir_pos+1+2, &pos3, 2);
  memcpy((void*) address + free_dir_pos+1+3, &pos4, 2);
  //num of blocks
  pos1 = block_size>>4*6;
  pos2 = block_size>>4*4 & 0xff;
  pos3 = block_size>>4*2 & 0xff;
  pos4 = block_size &0xff;

  memcpy((void*) address + free_dir_pos+5, &pos1, 2);
  memcpy((void*) address + free_dir_pos+5+1, &pos2, 2);
  memcpy((void*) address + free_dir_pos+5+2, &pos3, 2);
  memcpy((void*) address + free_dir_pos+5+3, &pos4, 2);
}

int find_empty_dir(struct superblock_t* sb,void * address){
  struct dir_entry_t* de;
  //need start block, block count
  int offset = htonl(sb->root_dir_start_block) * htons(sb->block_size);
  //root directory
  for (int i =0; i < htonl(sb->root_dir_block_count);i++){
    //printf("offset: %d, add value: %d ",offset,64);
    de = (struct dir_entry_t*)(address + offset + 64*i);
    if (de->status == 0){

      return offset + 64*i;
    }
  }
}

int find_next_block(int file_starting_block, void *address, struct superblock_t* sb){
  int block = 0;
  int fat_start = htonl(sb->fat_start_block) * htons(sb->block_size);
  memcpy(&block, (void*) address + fat_start + 4*file_starting_block, 4);
  return (int)htonl(block);
}

struct dir_entry_t* find_sub_dir(struct superblock_t* sb,char* sub_dir, void* address){//return address of sub
  struct dir_entry_t* de;
  //need start block, block count
  int offset = htonl(sb->root_dir_start_block) * htons(sb->block_size);

  for (int i =0; i < htonl(sb->root_dir_block_count);i++){
    de = (struct dir_entry_t*)(address + offset + 64*i);
    if (strcmp(de->filename, sub_dir) == 0 && de->status >> 2 == 1)
      return de;
    }
  printf("No sub directory\n");
  exit(-1);
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
