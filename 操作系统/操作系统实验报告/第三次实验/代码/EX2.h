#ifndef EXT_2_h
#define EXT_2_h

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<sys/ioctl.h>
#include<termios.h>
#include<unistd.h>
#define blocks 4611            // 1+1+1+512+4096,总块数
#define blocksiz 512           //每块字节数
#define data_begin_block 515   //数据开始块 1+1+1+512=515
#define dirsiz 32              //目录体长度
#define EXT2_NAME_LEN 15       //文件名长度
#define PATH "virtualdisk"           //文件系统
struct ext2_group_desc //组描述符 80 字节
{
    char bg_volume_name[16];  //卷名                1
    int bg_block_bitmap;      //保存块位图的块号     4
    int bg_inode_bitmap;      //保存索引结点位图的块号
    int bg_inode_table;       //索引结点表的起始块号
    int bg_free_blocks_count; //本组空闲块的个数
    int bg_free_inodes_count; //本组空闲索引结点的个数
    int bg_used_dirs_count;   //本组目录的个数
    char psw[16];             //密码
    char bg_pad[24];            //填充(0xff)
};
struct ext2_inode //索引节点 64 字节
{
    int i_mode;     //文件类型及访问权限                4
    int i_blocks;   //文件的数据块个数            
    int i_size;     //大小(字节) 
    int fd;         //文件描述符 
    time_t i_atime; //访问时间                        8
    time_t i_ctime; //创建时间                       
    time_t i_mtime; //修改时间
    time_t i_dtime; //删除时间
    short i_block[8]; //指向数据块的指针              2                  
};
struct ext2_dir_entry //目录体 32 字节
{
    int inode;                //索引节点号
    int rec_len;              //目录项长度
    int name_len;             //文件名长度
    int file_type;            //文件类型(1:普通文件，2:目录)
    char name[EXT2_NAME_LEN]; //文件名
    char authority;             //权限 0代表读写 1代表只读 2代表只写
};
typedef struct ext2_group_desc ext2_group_desc;
typedef struct ext2_inode ext2_inode;
typedef struct ext2_dir_entry ext2_dir_entry;
/*定义全局变量*/
extern ext2_group_desc group_desc; //组描述符
extern ext2_inode inode;
extern ext2_dir_entry dir;                //目录体
extern FILE *f;                           /*文件指针*/
extern unsigned int last_allco_inode; //上次分配的索引节点号
extern unsigned int last_allco_block; //上次分配的数据块号
extern unsigned int fopen_table[16]; //文件打开表 
/******************/
int getch();
int format(ext2_inode* cu);
int init_open_table();
int dir_entry_position(int dir_entry_begin, short i_block[8]);
int Open(ext2_inode *current, char *name);
int Close(ext2_inode *current);
int Read(ext2_inode *current, char *name);
int FindInode();
int FindBlock();
void DelInode(int len);
void DelBlock(int len);
int attrib(ext2_inode *current, char *name, char authority);
void add_block(ext2_inode *current, int i, int j);
int FindEntry(ext2_inode *current);
int Create(int type, ext2_inode *current, char *name);
int Write(ext2_inode *current, char *name);
void lsdir(ext2_inode *current,int mode);
int initialize(ext2_inode *cu);
int openfile(ext2_inode *current, char *name);
int closefile(ext2_inode *current, char *name);
int Password();
int login();
void exitdisplay();
int initfs(ext2_inode *cu);
void getstring(char *cs, ext2_inode node);
int Delet(int type, ext2_inode *current, char *name);
void shellloop(ext2_inode currentdir);
#endif
