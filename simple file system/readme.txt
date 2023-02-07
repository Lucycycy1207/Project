##IMPORTANT##
#need to use test.dmg neither test.img to work the four files
##IMPORTANT##


##classes use for all four files

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
