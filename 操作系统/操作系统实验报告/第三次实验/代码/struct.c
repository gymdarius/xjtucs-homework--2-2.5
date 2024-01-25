#include "EX2.h"
ext2_group_desc group_desc; //组描述符
ext2_inode inode;
ext2_dir_entry dir;                //目录体
FILE *f;                           /*文件指针*/
unsigned int last_allco_inode = 0; //上次分配的索引节点号
unsigned int last_allco_block = 0; //上次分配的数据块号
unsigned int fopen_table[16] = {0};



