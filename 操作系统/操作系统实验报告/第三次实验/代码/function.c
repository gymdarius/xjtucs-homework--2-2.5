#include "EX2.h"

int getch()
{
    int ch;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);       //获得终端文件的描述信息
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);     //设置终端非规范模式，不为行编辑，输入即刻生效
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void clearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
/********格式化文件系统**********/
/*
 * 初始化组描述符
 * 初始化数据块位图
 * 初始化索引节点位图
 * 初始化索引节点表 -添加一个索引节点
 * 第一个数据块中写入当前目录和上一目录
 */
int format(ext2_inode* cu)
{
    FILE *fp = NULL;
    int i;
    unsigned int zero[blocksiz / 4]; //零数组，用来初始化块为 0，除4是因为无符号整数占了4个字节.
    time_t now;
    time(&now);
    while (fp == NULL)fp = fopen(PATH, "w+");
    for (i = 0; i < blocksiz / 4; i++)zero[i] = 0;
    for (i = 0; i < blocks; i++) //初始化所有 4611 块为 0
    {
        fseek(fp, i * blocksiz, SEEK_SET);   //文件开头开始偏移块的字节数，实现文件指针重定位
        fwrite(&zero, blocksiz, 1, fp);     //只能一个字节一个字节写入，按照一个块大小写入块,满足要求
    }
    //初始化组描述符
    strcpy(group_desc.bg_volume_name, "my_file"); //初始化卷名为my_file
    group_desc.bg_block_bitmap = 1;            //保存块位图的块号
    group_desc.bg_inode_bitmap = 2;            //保存索引节点位图的块号
    group_desc.bg_inode_table = 3;             //索引节点表的起始块号
    group_desc.bg_free_blocks_count = 4095;    //除去一个初始化目录
    group_desc.bg_free_inodes_count = 4095;
    group_desc.bg_used_dirs_count = 1;
    strcpy(group_desc.psw, "123");
    fseek(fp, 0, SEEK_SET);
    fwrite(&group_desc, sizeof(ext2_group_desc), 1, fp); //第一块为组描述符

    //初始化数据块位图和索引节点位图，第一位置为 1,表示被占用
    zero[0] = 0x80000000;
    fseek(fp, 1 * blocksiz, SEEK_SET);
    fwrite(&zero, blocksiz, 1, fp); //第二块为块位图，块位图的第一位为 1
    fseek(fp, 2 * blocksiz, SEEK_SET);
    fwrite(&zero, blocksiz, 1, fp); //第三块为索引位图，索引节点位图的第一位为 1

    //初始化索引节点表，添加一个索引节点
    inode.i_mode = 2;
    inode.i_blocks = 1;
    inode.i_size = 64;
    inode.i_ctime = now;
    inode.i_atime = now;
    inode.i_mtime = now;
    inode.i_dtime = 0;
    inode.i_block[0] = 0;
    fseek(fp, 3 * blocksiz, SEEK_SET);
    fwrite(&inode, sizeof(ext2_inode), 1, fp); //第四块开始为索引节点表

    //向第一个数据块写 当前目录
    dir.inode = 0;
    dir.rec_len = 32;
    dir.name_len = 1;
    dir.file_type = 2;
    strcpy(dir.name, "."); //当前目录
    fseek(fp, data_begin_block * blocksiz, SEEK_SET);
    fwrite(&dir, sizeof(ext2_dir_entry), 1, fp);

    //当前目录之后写 上一目录
    dir.inode = 0;
    dir.rec_len = 32;
    dir.name_len = 2;
    dir.file_type = 2;
    strcpy(dir.name, ".."); //上一目录
    fseek(fp, data_begin_block * blocksiz + dirsiz, SEEK_SET);
    fwrite(&dir, sizeof(ext2_dir_entry), 1, fp); //第data_begin_block+1 =516 块开始为数
    initialize(cu);
    init_open_table();
    fclose(fp);
    return 0;
}

int init_open_table(){
    int i = 0;
    ext2_inode node;
    for(i = 0; i < 16; i++){
        if(test_fd(i) == 1){        //关闭文件系统中已经打开的文件，并释放相应的文件描述符。
            FILE *fp = NULL;
            while (fp == NULL)fp = fopen(PATH, "r+");
            fseek(fp, 3 * blocksiz + (fopen_table[i]) * sizeof(ext2_inode), SEEK_SET);
            fread(&node, sizeof(ext2_inode), 1, fp);
            node.fd = 0;
            fseek(fp, 3 * blocksiz + (fopen_table[i]) * sizeof(ext2_inode), SEEK_SET);
            fwrite(&node, sizeof(ext2_inode), 1, fp);
            fopen_table[i] = 0;
            fclose(fp);
            return 0;
        }
        else fopen_table[i] = 0;
    }
    return 0;
}
/*计算目录项在文件系统中的绝对字节位置，考虑了直接索引、一级间接索引和二级间接索引的情况。*/
int dir_entry_position(int dir_entry_begin, short i_block[8]) // dir_entry_begin目录体的相对开始字节
{
    int dir_blocks = dir_entry_begin / 512;   // 存储目录需要的块数
    int block_offset = dir_entry_begin % 512; // 块内偏移字节数
    int a;
    FILE *fp = NULL;
    if (dir_blocks <= 5) //前六个直接索引
        return data_begin_block * blocksiz + i_block[dir_blocks] * blocksiz + block_offset;
    else //间接索引
    {   
        while (fp == NULL)
        fp = fopen(PATH, "r+");
        dir_blocks = dir_blocks - 6;
        if (dir_blocks < 128) //一个块 512 字节，一个int为 4 个字节 一级索引有 512/4= 128 个
            {
                int a;
                fseek(fp, data_begin_block * blocksiz + i_block[6] * blocksiz + dir_blocks * 4, SEEK_SET);
                fread(&a, sizeof(int), 1, fp);
                return data_begin_block * blocksiz + a * blocksiz + block_offset;
            }
        else //二级索引
        {
            dir_blocks = dir_blocks - 128;
            fseek(fp, data_begin_block * blocksiz + i_block[7] * blocksiz + dir_blocks / 128 * 4, SEEK_SET);
            fread(&a, sizeof(int), 1, fp);
            fseek(fp, data_begin_block * blocksiz + a * blocksiz + dir_blocks % 128 * 4, SEEK_SET);
            fread(&a, sizeof(int), 1, fp);
            return data_begin_block * blocksiz + a * blocksiz + block_offset;
        }
        fclose(fp);
    }
}

/*打开一个指定名称的目录，将当前目录切换为该目录，并更新当前目录的 ext2_inode 结构体。*/
int Open(ext2_inode *current, char *name)   
{
    FILE *fp = NULL;
    int i;
    while (fp == NULL)fp = fopen(PATH, "r+");
    for (i = 0; i < (current->i_size / 32); i++)
    {
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET); //定位目录的偏移位置
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);
        if (!strcmp(dir.name, name))
        {
            if (dir.file_type == 2) //目录
            {
                fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                fread(current, sizeof(ext2_inode), 1, fp);
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return 1;
}
/********关闭当前目录**********/
/*
关闭时仅修改最后访问时间
返回时 打开上一目录 作为当前目录
*/
int Close(ext2_inode *current)
{
    time_t now;
    ext2_dir_entry bentry;
    FILE *fout;
    fout = fopen(PATH, "r+");
    time(&now);
    current->i_atime = now; //修改最后访问时间
    fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET);
    fread(&bentry, sizeof(ext2_dir_entry), 1, fout); // current's dir_entry
    fseek(fout, 3 * blocksiz + (bentry.inode) * sizeof(ext2_inode), SEEK_SET);
    fwrite(current, sizeof(ext2_inode), 1, fout);        //将修改后的时间重新写入索引节点
    fclose(fout);
    return Open(current, "..");
}
/*
从目录中读取文件内容的功能，包括了权限检查和更新文件的最后访问时间。
*/
int Read(ext2_inode *current, char *name)
{
    FILE *fp = NULL;
    int i;
    while (fp == NULL)fp = fopen(PATH, "r+");
    for (i = 0; i < (current->i_size / 32); i++)
    {
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);
        if (!strcmp(dir.name, name))
        {   
            if(!(dir.authority == 1||dir.authority == 0)){
                    fclose(fp);
                    return 1;
            }
            if (dir.file_type == 1)
            {
                time_t now;
                ext2_inode node;
                char content_char;
                fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                fread(&node, sizeof(ext2_inode), 1, fp); // 原本的索引节点
                i = 0;
                for (i = 0; i < node.i_size; i++)
                {
                    fseek(fp, dir_entry_position(i, node.i_block), SEEK_SET);
                    fread(&content_char, sizeof(char), 1, fp);
                    if (content_char == 0xD)printf("\n");
                    else printf("%c", content_char);           //按字节流读出
                }
                printf("\n");
                time(&now);
                node.i_atime = now;
                fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                fwrite(&node, sizeof(ext2_inode), 1, fp); // 更新索引节点
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return 1;
}
//寻找空索引。在给定的索引节点位图中找到一个空闲的索引节点，占用该索引节点，并返回其索引节点号。
int FindInode()
{
    FILE *fp = NULL;
    unsigned int zero[blocksiz / 4];
    int i;
    while (fp == NULL)fp = fopen(PATH, "r+");
    fseek(fp, 2 * blocksiz, SEEK_SET); // inode 位图
    fread(zero, blocksiz, 1, fp);      // zero保存索引节点位图
    for (i = last_allco_inode; i < (last_allco_inode + blocksiz / 4); i++)
    {
        if (zero[i % (blocksiz / 4)] != 0xffffffff)                  //使用模运算，遍历整个位图。防止出现溢出
        {
            unsigned int j = 0x80000000, k = zero[i % (blocksiz / 4)], l = i;
            for (i = 0; i < 32; i++)
            {
                if (!(k & j))                   //代表该位空闲
                {
                    zero[l % (blocksiz / 4)] = zero[l % (blocksiz / 4)] | j;   //占用该空闲节点
                    group_desc.bg_free_inodes_count -= 1; //索引节点数减 1
                    fseek(fp, 0, 0);
                    fwrite(&group_desc, sizeof(ext2_group_desc), 1, fp); //更新组描述符
                    fseek(fp, 2 * blocksiz, SEEK_SET);
                    fwrite(zero, blocksiz, 1, fp); //更新inode位图
                    last_allco_inode = l % (blocksiz / 4);
                    fclose(fp);
                    return l % (blocksiz / 4) * 32 + i;            //返回索引节点号
                }
                else
                    j = j / 2;                //右移一位
            }
        }
    }
    fclose(fp);
    return -1;
}
//寻找空block，在给定的块位图中找到一个空闲的块，占用该块，并返回其块号
int FindBlock()
{
    FILE *fp = NULL;
    unsigned int zero[blocksiz / 4];
    int i;
    while (fp == NULL)fp = fopen(PATH, "r+");
    fseek(fp, 1 * blocksiz, SEEK_SET);
    fread(zero, blocksiz, 1, fp); // zero保存块位图
    for (i = last_allco_block; i < (last_allco_block + blocksiz / 4); i++)
    {
        if (zero[i % (blocksiz / 4)] != 0xffffffff)
        {
            unsigned int j = 0X80000000, k = zero[i % (blocksiz / 4)], l = i;
            for (i = 0; i < 32; i++)
            {
                if (!(k & j))
                {
                    zero[l % (blocksiz / 4)] = zero[l % (blocksiz / 4)] | j;
                    group_desc.bg_free_blocks_count -= 1; //块数减 1
                    fseek(fp, 0, 0);
                    fwrite(&group_desc, sizeof(ext2_group_desc), 1, fp);
                    fseek(fp, 1 * blocksiz, SEEK_SET);
                    fwrite(zero, blocksiz, 1, fp);           //更新空闲块位图
                    last_allco_block = l % (blocksiz / 4);
                    fclose(fp);
                    return l % (blocksiz / 4) * 32 + i;
                }
                else
                    j = j / 2;
            }
        }
    }
    fclose(fp);
    return -1;
}
//删除inode，更新inode节点位图
void DelInode(int len)
{
    unsigned int zero[blocksiz / 4], i;
    int j;
    f = fopen(PATH, "r+");
    fseek(f, 2 * blocksiz, SEEK_SET);
    fread(zero, blocksiz, 1, f);
    i = 0x80000000;
    for (j = 0; j < len % 32; j++)i = i / 2;
    zero[len / 32] = zero[len / 32] ^ i;        //异或运算，通过异或来清零
    fseek(f, 2 * blocksiz, SEEK_SET);
    fwrite(zero, blocksiz, 1, f);              //更新空闲索引表
    fclose(f);
}
//删除block块，更新块位图
void DelBlock(int len)
{
    unsigned int zero[blocksiz / 4], i;
    int j;
    f = fopen(PATH, "r+");
    fseek(f, 1 * blocksiz, SEEK_SET);
    fread(zero, blocksiz, 1, f);
    i = 0x80000000;
    for (j = 0; j < len % 32; j++)i = i / 2;
    zero[len / 32] = zero[len / 32] ^ i;          //异或运算，通过异或来清零
    fseek(f, 1 * blocksiz, SEEK_SET);
    fwrite(zero, blocksiz, 1, f);            //更新空闲块表
    fclose(f);
}
/*为一个文件的 inode 添加数据块
    写目录和写文件时会用到
*/
void add_block(ext2_inode *current, int i, int j)             // j为加的数据块号,i为需要的块数
{
    FILE *fp = NULL;
    while (fp == NULL)fp = fopen(PATH, "r+");
    if (i < 6) //直接索引
    {
        current->i_block[i] = j;
    }
    else
    {
        i = i - 6;
        if (i == 0)
        {
            current->i_block[6] = FindBlock();               //第一次使用一级索引，申请一级索引空间
            fseek(fp, data_begin_block * blocksiz + current->i_block[6] * blocksiz, SEEK_SET);
            fwrite(&j, sizeof(int), 1, fp);
        }
        else if (i < 128) //一级索引
        {
            fseek(fp, data_begin_block * blocksiz + current->i_block[6] * blocksiz + i * 4, SEEK_SET);        //*4是为了跳过记录号的数字
            fwrite(&j, sizeof(int), 1, fp);
        }
        else //二级索引
        {
            i = i - 128;
            if (i == 0)
            {
                current->i_block[7] = FindBlock();
                fseek(fp, data_begin_block * blocksiz + current->i_block[7] * blocksiz, SEEK_SET);  //第一次使用二级索引，更新
                i = FindBlock();                       //第二层初始化
                fwrite(&i, sizeof(int), 1, fp);
                fseek(fp, data_begin_block * blocksiz + i * blocksiz, SEEK_SET);
                fwrite(&j, sizeof(int), 1, fp);
            }
            if (i % 128 == 0)
            {
                fseek(fp, data_begin_block * blocksiz + current->i_block[7] * blocksiz + i / 128 * 4, SEEK_SET);
                i = FindBlock();
                fwrite(&i, sizeof(int), 1, fp);                   //需要初始化第二层
                fseek(fp, data_begin_block * blocksiz + i * blocksiz, SEEK_SET);
                fwrite(&j, sizeof(int), 1, fp);                //加入二级索引表
            }
            else
            {
                fseek(fp, data_begin_block * blocksiz + current->i_block[7] * blocksiz + i / 128 * 4, SEEK_SET);
                fread(&i, sizeof(int), 1, fp);
                fseek(fp, data_begin_block * blocksiz + i * blocksiz + i % 128 * 4, SEEK_SET);   //增加了偏移量
                fwrite(&j, sizeof(int), 1, fp);
            }
        }
    }
}
// 为当前目录寻找一个空目录体
int FindEntry(ext2_inode *current)
{
    FILE *fout = NULL;
    int location;       //条目的绝对地址
    int block_location; //块号
    int temp;           //每个block 可以存放的INT 数量
    int remain_block;   //剩余块数
    location = data_begin_block * blocksiz;
    temp = blocksiz / sizeof(int);
    fout = fopen(PATH, "r+");
    if (current->i_size % blocksiz == 0) //一个BLOCK 使用完后增加一个块
    {
        add_block(current, current->i_blocks, FindBlock());
        current->i_blocks++;
    }
    if (current->i_blocks < 7) //前 6 个块直接索引                  
    {
        location += current->i_block[current->i_blocks - 1] * blocksiz;
        location += current->i_size % blocksiz;
    }
    else if (current->i_blocks < temp + 7) //一级索引                       
    {                                                                                 
        block_location = current->i_block[6];
        fseek(fout, (data_begin_block + block_location) * blocksiz + (current->i_blocks - 7) * sizeof(int), SEEK_SET);
        fread(&block_location, sizeof(int), 1, fout);
        location += block_location * blocksiz;
        location += current->i_size % blocksiz;
    }
    else //二级索引
    {
        block_location = current->i_block[7];
        remain_block = current->i_blocks - 6 - temp;
        fseek(fout, (data_begin_block + block_location) * blocksiz + (int)(remain_block / temp) * sizeof(int), SEEK_SET);
        fread(&block_location, sizeof(int), 1, fout);
        remain_block = remain_block % temp;
        fseek(fout, (data_begin_block + block_location) * blocksiz + remain_block * sizeof(int), SEEK_SET);
        fread(&block_location, sizeof(int), 1, fout);
        location += block_location * blocksiz;
        location += current->i_size % blocksiz;                                  
    }
    current->i_size += dirsiz;
    fclose(fout);
    return location;
}
/*********创建文件或者目录*********/
/*
 * type=1 创建文件 type=2 创建目录
 * current 当前目录索引节点
 * name 文件名或目录名
 * 创建成功返回0 失败返回1
 */
int Create(int type, ext2_inode *current, char *name)
{
    FILE *fout = NULL;
    int i;
    int block_location;     // block location
    int node_location;      // node location
    int dir_entry_location; // dir entry location
    time_t now;
    ext2_inode ainode;
    ext2_dir_entry aentry, bentry; // bentry保存当前系统的目录体信息
    time(&now);
    fout = fopen(PATH, "r+");
    node_location = FindInode();
    /*检查当前目录中是否已经存在同名的文件或目录。如果存在，
    则说明创建操作不可行，函数返回 1；如果不存在同名的文件
    或目录，则继续执行后续的创建操作。
    */
    for (i = 0; i < current->i_size / dirsiz; i++)
    {
        fseek(fout, dir_entry_position(i * sizeof(ext2_dir_entry), current->i_block), SEEK_SET);
        fread(&aentry, sizeof(ext2_dir_entry), 1, fout);
        if (aentry.file_type == type && !strcmp(aentry.name, name))return 1;
    }
    fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET);
    fread(&bentry, sizeof(ext2_dir_entry), 1, fout); // 读取当前目录的目录项
    if (type == 1)                                   //文件
    {
        ainode.i_mode = 1;
        ainode.i_blocks = 0; //文件暂无内容
        ainode.i_size = 0;   //初始文件大小为 0
        ainode.i_atime = now;
        ainode.i_ctime = now;
        ainode.i_mtime = now;
        ainode.i_dtime = 0;
        for (i = 0; i < 8; i++)
        {
            ainode.i_block[i] = 0;
        }
    }
    else //目录
    {
        ainode.i_mode = 2;   //目录
        ainode.i_blocks = 1; //目录 当前和上一目录
        ainode.i_size = 64;  //初始大小 32*2=64
        ainode.i_atime = now;
        ainode.i_ctime = now;
        ainode.i_mtime = now;
        ainode.i_dtime = 0;
        block_location = FindBlock();
        ainode.i_block[0] = block_location;
        for (i = 1; i < 8; i++)
        {
            ainode.i_block[i] = 0;
        }
        //当前目录
        aentry.inode = node_location;
        aentry.rec_len = sizeof(ext2_dir_entry);
        aentry.name_len = 1;
        aentry.file_type = 2;
        strcpy(aentry.name, ".");
        aentry.authority = 0;
        fseek(fout, (data_begin_block + block_location) * blocksiz, SEEK_SET);
        fwrite(&aentry, sizeof(ext2_dir_entry), 1, fout);
        //上一级目录
        aentry.inode = bentry.inode;
        aentry.rec_len = sizeof(ext2_dir_entry);
        aentry.name_len = 2;
        aentry.file_type = 2;
        strcpy(aentry.name, "..");
        aentry.authority = 0;
        fwrite(&aentry, sizeof(ext2_dir_entry), 1, fout);
        //一个空条目
        aentry.inode = 0;
        aentry.rec_len = sizeof(ext2_dir_entry);
        aentry.name_len = 0;
        aentry.file_type = 0;
        aentry.name[EXT2_NAME_LEN] = 0;
        aentry.authority = 0;
        fwrite(&aentry, sizeof(ext2_dir_entry), 14, fout); //清空数据块
    }                                                      // end else
    //保存新建inode
    fseek(fout, 3 * blocksiz + (node_location) * sizeof(ext2_inode), SEEK_SET);
    fwrite(&ainode, sizeof(ext2_inode), 1, fout);
    // 将新建inode 的信息写入current 指向的数据块
    aentry.inode = node_location;                                  //保存信息到现在的目录
    aentry.rec_len = dirsiz;                                                      
    aentry.name_len = strlen(name);
    if (type == 1)
    {
        aentry.file_type = 1;
    } //文件
    else
    {
        aentry.file_type = 2;
    } //目录
    strcpy(aentry.name, name);
    aentry.authority = 0;
    dir_entry_location = FindEntry(current);
    fseek(fout, dir_entry_location, SEEK_SET); //定位条目位置
    fwrite(&aentry, sizeof(ext2_dir_entry), 1, fout);
    //保存current 的信息,bentry 是current 指向的block 中的第一条
    fseek(fout, 3 * blocksiz + (bentry.inode) * sizeof(ext2_inode), SEEK_SET);
    fwrite(current, sizeof(ext2_inode), 1, fout);
    fclose(fout);
    return 0;
}
/******************/
/*
 * 在当前目录下找到名为name的非目录文件进行写操作
 * 'ESC'终止输入
 * 如果当前目录下没有该文件，则提醒创建一个
 */
int Write(ext2_inode *current, char *name)
{
    FILE *fp = NULL;
    ext2_dir_entry dir;
    ext2_inode node;
    time_t now;
    char str;
    int i;
    while (fp == NULL)fp = fopen(PATH, "r+");
    while (1)
    {
        for (i = 0; i < (current->i_size / 32); i++)
        {
            fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
            fread(&dir, sizeof(ext2_dir_entry), 1, fp);
            if (!strcmp(dir.name, name))
            {
                if(!(dir.authority == 2||dir.authority == 0)){
                    fclose(fp);
                    return 1;
                }
                if (dir.file_type == 1)
                {
                    fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                    fread(&node, sizeof(ext2_inode), 1, fp);
                    break;
                }
            }
        }
        if (i < current->i_size / 32)break;                  //现在存在该文件
        // Create(1,current,name); //现在不存在该文件
        printf("There isn't this file, please create it first\n");
        return 0;
    }
    str = getch();
    while (str != 27)
    {
        printf("%c", str);
        if (!(node.i_size % 512))
        {
            add_block(&node, node.i_size / 512, FindBlock());
            node.i_blocks += 1;                                                                       
        }
        fseek(fp, dir_entry_position(node.i_size, node.i_block), SEEK_SET);
        fwrite(&str, sizeof(char), 1, fp);
        node.i_size += sizeof(char);
        if (str == 0x0d)printf("%c", 0x0a);                  //回车键，输出换行符
        str = getch();
        if (str == 27)break;                                 //ESC键代表结束写
    }
    time(&now);
    node.i_mtime = now;
    node.i_atime = now;
    fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
    fwrite(&node, sizeof(ext2_inode), 1, fp);
    fclose(fp);
    printf("\n");
    return 0;
}
/**********ls命令********/
/*
 * 列出当前目录的文件和目录
 * 分为详细模式和普通模式
 */
void lsdir(ext2_inode *current, int mode)
{
    ext2_dir_entry dir;
    int i, j;
    char timestr[150];
    ext2_inode node;
    f = fopen(PATH, "r+");
    if(mode == 0)
    {
    printf("Type\t\tFileName\tCreateTime\t\t\tLastAccessTime\t\t\tModifyTime");
    for (i = 0; i < current->i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, f);
        fseek(f, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
        fread(&node, sizeof(ext2_inode), 1, f);
        strcpy(timestr, "");
        strcat(timestr, asctime(localtime(&node.i_ctime)));
        strcat(timestr, asctime(localtime(&node.i_atime)));
        strcat(timestr, asctime(localtime(&node.i_mtime)));
        for (j = 0; j < strlen(timestr); j++)
            if (timestr[j] == '\n')
            {
                timestr[j] = '\t';
            }
        if (dir.file_type == 1)
            printf("\nFile     \t%s\t\t%s", dir.name, timestr);
        else
            printf("\nDirectory\t%s\t\t%s", dir.name, timestr);
    }
    }
    else if(mode == 1)
    {
    char au[3][10] = {"rw","r","w"};
    printf("Type\t\tFileName\tCreateTime\t\t\tLastAccessTime\t\t\tModifyTime\t\t\tAuthority");
    for (i = 0; i < current->i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, f);
        fseek(f, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
        fread(&node, sizeof(ext2_inode), 1, f);
        strcpy(timestr, "");
        strcat(timestr, asctime(localtime(&node.i_ctime)));
        strcat(timestr, asctime(localtime(&node.i_atime)));
        strcat(timestr, asctime(localtime(&node.i_mtime)));
        for (j = 0; j < strlen(timestr); j++)
            if (timestr[j] == '\n')
            {
                timestr[j] = '\t';
            }
        if (dir.file_type == 1)
        {
            printf("\nFile \t\t%s\t\t%s%s", dir.name, timestr, au[(int)dir.authority]); 
        }
        else printf("\nDirectory\t%s\t\t%sNULL", dir.name, timestr);
    }    
    }
    fclose(f);
}
int initialize(ext2_inode *cu)
{
    f = fopen(PATH, "r+");
    fseek(f, 3 * blocksiz, 0);
    fread(cu, sizeof(ext2_inode), 1, f);
    fclose(f);
    return 0;
}
/*********修改文件系统中指定文件或目录的权限*********/
/*
 * 修改成功返回 0
 * 修改不成功返回 1
 */
int attrib(ext2_inode *current, char *name, char authority){
    FILE *fp = NULL;
    int i;
    while (fp == NULL)fp = fopen(PATH, "r+");
    for (i = 0; i < (current->i_size / 32); i++)
    {
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);
        if (!strcmp(dir.name, name))
        {   
            dir.authority = authority;
            fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
            fwrite(&dir, sizeof(ext2_dir_entry), 1, fp);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return 1;
}
int Password()          //修改密码
{
    char psw[16], ch[10],newPsw1[16],newPsw2[16];
    printf("Please input the old password\n");
    int i=0;
    clearBuffer();
    while (i < sizeof(psw) - 1) {
        system("stty -echo");
        char c =getchar();
        system("stty echo");

        if (c == '\n' ) {
            break;  // 当用户按下 Enter 键时结束输入
        } else if (c=='\b'&&i>0)
        {
            printf("\b \b");
            i--;
        }
         if (isprint(c)) {
            psw[i++] = c;
            printf("*");  // 显示一个星号代替输入的字符
        }
    }
    printf("\n");
    psw[i] = '\0';  // 在密码末尾添加字符串终止符
    if (strcmp(psw, group_desc.psw) != 0)
    {
        printf("Password error!\n");
        return 1;
    }
    while (1)
    {
        printf("Please input the new password:");
        i = 0;
        char passwordChar;

        while (i < sizeof(psw) - 1) {
            system("stty -echo");
            passwordChar =getchar();
            system("stty echo");
            if (passwordChar == '\n') {
                newPsw1[i] = '\0';
                break;
            } 
            if (passwordChar == '\b' && i > 0) {
                printf("\b \b");
                i--;
            } else if (isprint(passwordChar)) {
                newPsw1[i] = passwordChar;
                printf("*");
                i++;
            }
        }
        printf("\nPlease input the new password again: ");
        i = 0;
        
        while (i < sizeof(psw) - 1) {
            system("stty -echo");
            passwordChar =getchar();
            system("stty echo");

            if (passwordChar == '\n') {
                newPsw2[i] = '\0';
                break;
            } 
            if (passwordChar == '\b' && i > 0) {
                printf("\b \b");
                i--;
            } else if (isprint(passwordChar)) {
                newPsw2[i] = passwordChar;
                printf("*");
                i++;
            }
        }
        if (strcmp(newPsw1, newPsw2) != 0) {
            printf("\nPasswords do not match. Please try again.\n");
        } else{
        while (1)
        {
            printf("\nModify the password?[Y/N]");
            scanf("%s", ch);
            if (ch[0] == 'N' || ch[0] == 'n')
            {
                printf("You canceled the modify of your password\n");
                return 1;
            }
            else if (ch[0] == 'Y' || ch[0] == 'y')
            {
                strcpy(group_desc.psw, newPsw1);
                f = fopen(PATH, "r+");
                fseek(f, 0, 0);
                fwrite(&group_desc, sizeof(ext2_group_desc), 1, f);
                fclose(f);
                return 0;
            }
            else
                printf("Meaningless command\n");
        }
        }
    }
}
/******************/
int login()                  //登录文件系统
{   
    int i = 0;
    init_open_table();
    char psw[16];
    printf("please input the password(init:123):");

    //scanf("%s", psw);
    while (i < sizeof(psw) - 1) {
        system("stty -echo");
        char c =getchar();
        system("stty echo");

        if (c == '\n' ) {
            break;  // 当用户按下 Enter 键时结束输入
        } else if (c=='\b'&&i>0)
        {
            printf("\b \b");
            i--;
        }else if (isprint(c)) {
            psw[i++] = c;
            printf("*");  // 显示一个星号代替输入的字符
        }
    }
    printf("\n");
    psw[i] = '\0';  // 在密码末尾添加字符串终止符
    return strcmp(group_desc.psw, psw);
}
/******************/
void exitdisplay()         //退出展示
{   
    init_open_table();
    printf("Thank you for using!\n");
    return;
}
/**************初始化文件系统******************/
/*返回 1 初始化失败，返回 0 初始化成功*/
int initfs(ext2_inode *cu)
{
    f = fopen(PATH, "r+");
    if (f == NULL)
    {
        // char ch[20];/***********/
        char ch;
        int i;
        printf("File system couldn't be found. Do you want to create one?\n[Y/N]"); 
        i=1; 
        while(i) 
        {
            scanf("%c", &ch); /*******/
            clearBuffer();
            switch (ch)
            {
                case 'Y':
                case 'y': /********/
                    if (format(cu) != 0)return 1;
                    f = fopen(PATH, "r");
                    i = 0;
                    break;
                case 'N':
                case 'n': /*******/
                    exitdisplay();
                return 1;
                default:
                    printf("Sorry, meaningless command\n");
                    break;
            } 
        }
    }
    fseek(f, 0, SEEK_SET);
    fread(&group_desc, sizeof(ext2_group_desc), 1, f);
    fseek(f, 3 * blocksiz, SEEK_SET);       //组描述符、数据块位图、索引结点位图的空间
    fread(&inode, sizeof(ext2_inode), 1, f);    //索引结点表
    fclose(f);
    initialize(cu);
    int i = 0;
    init_open_table();
    return 0;
}
/*********获取当前目录的目录名*********/
void getstring(char *cs, ext2_inode node)
{
    ext2_inode current = node;
    int i, j;
    ext2_dir_entry dir;
    f = fopen(PATH, "r+");
    Open(&current, ".."); // current指向上一目录
    for (i = 0; i < node.i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, node.i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, f);
        if (!strcmp(dir.name, "."))                                       //通过该目录来获得索引节点号
        {
            j = dir.inode;
            break;
        }
    }
    for (i = 0; i < current.i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, current.i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, f);                           //通过上一目录来获得名称
        if (dir.inode == j)
        {
            strcpy(cs, dir.name);
            return;
        }
    }
}
/*******在当前目录删除目录或者文件***********/
int Delet(int type, ext2_inode *current, char *name)
{
    FILE *fout = NULL;
    int i, j, t, k, flag;
    // int Nlocation,Elocation,Blocation,
    int Blocation2, Blocation3;
    int node_location, dir_entry_location, block_location;
    int block_location2, block_location3;
    ext2_inode cinode;
    ext2_dir_entry bentry, centry, dentry;
    //一个空条目
    dentry.inode = 0;
    dentry.rec_len = sizeof(ext2_dir_entry);
    dentry.name_len = 0;
    dentry.file_type = 0;
    strcpy(dentry.name, "");
    dentry.authority = 0;
    fout = fopen(PATH, "r+");
    t = (int)(current->i_size / dirsiz); //总条目数
    flag = 0;                            //是否找到文件或目录
    for (i = 0; i < t; i++)
    {
        dir_entry_location = dir_entry_position(i * dirsiz, current->i_block);
        fseek(fout, dir_entry_location, SEEK_SET);
        fread(&centry, sizeof(ext2_dir_entry), 1, fout);
        if ((strcmp(centry.name, name) == 0) && (centry.file_type == type)) 
        {
            flag = 1;
            j = i;
            break;
        }
    }
    if (flag)   //找到了
    {
        node_location = centry.inode;
        fseek(fout, 3 * blocksiz + node_location * sizeof(ext2_inode), SEEK_SET); //定位INODE 位置
        fread(&cinode, sizeof(ext2_inode), 1, fout);
        block_location = cinode.i_block[0];
        //删文件夹，检查文件夹是否为空，如果不为空则无法删除。
        //否则，删除文件夹的数据块和inode。
        if (type == 2)
        {
            if (cinode.i_size > 2 * dirsiz) 
            {
                printf("The folder is not empty!\n");
                fclose(fout);
                return 1;
            }
            else
            { 
                DelBlock(block_location);
                DelInode(node_location);
                dir_entry_location = dir_entry_position(current->i_size - dirsiz, current->i_block); //找到current 指向条目的最后一条
                fseek(fout, dir_entry_location, SEEK_SET);
                fread(&centry, dirsiz, 1, fout); //将最后一条条目存入centry
                fseek(fout, dir_entry_location, SEEK_SET);
                fwrite(&dentry, dirsiz, 1, fout);                  //清空该位置                    //为了将目录顺序排列
                dir_entry_location -= data_begin_block * blocksiz; //在数据中的位置
                if ((dir_entry_location % blocksiz) == 0)               //如果这个位置刚好是一个块的起始位置，则删掉这个块
                {
                    DelBlock((int)(dir_entry_location / blocksiz));
                    current->i_blocks--;
                    if (current->i_blocks == 6)DelBlock(current->i_block[6]);
                    else if (current->i_blocks == (blocksiz / sizeof(int) + 6))
                    {
                        int a;
                        fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz, SEEK_SET);
                        fread(&a, sizeof(int), 1, fout);
                        DelBlock(a);
                        DelBlock(current->i_block[7]);
                    }
                    else if (!((current->i_blocks - 6 - blocksiz / sizeof(int)) % (blocksiz / sizeof(int))))       
                    {
                        int a;
                        fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz + ((current->i_blocks - 6 - blocksiz / sizeof(int)) / (blocksiz / sizeof(int))), SEEK_SET);
                        fread(&a, sizeof(int), 1, fout);
                        DelBlock(a);
                    }
                }
                current->i_size -= dirsiz;
                if (j * dirsiz < current->i_size) //删除的条目如果不是最后一条，用centry覆盖
                {
                    dir_entry_location = dir_entry_position(j * dirsiz, current->i_block);
                    fseek(fout, dir_entry_location, SEEK_SET);
                    fwrite(&centry, dirsiz, 1, fout);
                }
            }
        }
        //删文件
        //删除文件直接指向的块。
        //如果有一级索引存在，删除一级索引中的块。
        //如果有二级索引存在，删除二级索引中的块。
        else
        {
            //删直接指向的块
            for (i = 0; i < 6; i++)
            {
                if (cinode.i_blocks == 0)
                {
                    break;
                }
                block_location = cinode.i_block[i];
                DelBlock(block_location);
                cinode.i_blocks--;
            }
            //删一级索引中的块
            if (cinode.i_blocks > 0)
            {
                block_location = cinode.i_block[6];
                fseek(fout, (data_begin_block + block_location) * blocksiz, SEEK_SET);
                for (i = 0; i < blocksiz / sizeof(int); i++)
                {
                    if (cinode.i_blocks == 0)
                    {
                        break;
                    }
                    fread(&Blocation2, sizeof(int), 1, fout);
                    DelBlock(Blocation2);
                    cinode.i_blocks--;
                }
                DelBlock(block_location); // 删除一级索引
            }
            if (cinode.i_blocks > 0) //有二级索引存在
            {
                block_location = cinode.i_block[7];
                for (i = 0; i < blocksiz / sizeof(int); i++)
                {
                    fseek(fout, (data_begin_block + block_location) * blocksiz + i * sizeof(int), SEEK_SET);
                    fread(&Blocation2, sizeof(int), 1, fout);
                    fseek(fout, (data_begin_block + Blocation2) * blocksiz, SEEK_SET);
                    for (k = 0; k < blocksiz / sizeof(int); k++)
                    {
                        if (cinode.i_blocks == 0)
                        {
                            break;
                        }
                        fread(&Blocation3, sizeof(int), 1, fout);
                        DelBlock(Blocation3);
                        cinode.i_blocks--;
                    }
                    DelBlock(Blocation2); //删除二级索引
                }
                DelBlock(block_location); // 删除一级索引
            }
            DelInode(node_location); //删除文件的inode
            /*删除目录项的逻辑：
            找到当前目录下的最后一条目录项，将其内容覆盖到删除的目录项上。
            如果删除的目录项不是最后一条，清空最后一条的内容。
            如果最后一条目录项所在的块是一个块的起始位置，删除这个块。
            */
            dir_entry_location = dir_entry_position(current->i_size - dirsiz, current->i_block); //找到current 指向条目的最后一条
            fseek(fout, dir_entry_location, SEEK_SET);
            fread(&centry, dirsiz, 1, fout); //将最后一条条目存入centry
            fseek(fout, dir_entry_location, SEEK_SET);
            fwrite(&dentry, dirsiz, 1, fout);                  //清空该位置
            dir_entry_location -= data_begin_block * blocksiz; //在数据中的位置
            //如果这个位置刚好是一个块的起始位置，则删掉这个块
            if (dir_entry_location % blocksiz == 0)
            {
                DelBlock((int)(dir_entry_location / blocksiz));
                current->i_blocks--;
                if (current->i_blocks == 6)DelBlock(current->i_block[6]);
                else if (current->i_blocks == (blocksiz / sizeof(int) + 6))
                {
                    int a;
                    fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz, SEEK_SET);
                    fread(&a, sizeof(int), 1, fout);
                    DelBlock(a);
                    DelBlock(current->i_block[7]);
                }
                else if (!((current->i_blocks - 6 - blocksiz / sizeof(int)) % (blocksiz / sizeof(int))))    
                {
                    int a;
                    fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz + ((current->i_blocks - 6 - blocksiz / sizeof(int)) / (blocksiz / sizeof(int))), SEEK_SET);
                    fread(&a, sizeof(int), 1, fout);
                    DelBlock(a);
                }
            }
            current->i_size -= dirsiz;
            if (j * dirsiz < current->i_size) // 删除的条目如果不是最后一条，用centry覆盖
            {
                dir_entry_location = dir_entry_position(j * dirsiz, current->i_block);
                fseek(fout, dir_entry_location, SEEK_SET);
                fwrite(&centry, dirsiz, 1, fout);
            }    
        }
        fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET);
        fread(&bentry, sizeof(ext2_dir_entry), 1, fout); // current's dir_entry
        time_t now;
        time(&now);
        current->i_mtime = now;
        fseek(fout, 3 * blocksiz + (bentry.inode) * sizeof(ext2_inode), SEEK_SET);
        fwrite(current, sizeof(ext2_inode), 1, fout); //将current修改的数据写回文件
    }
    else
    {
        fclose(fout);
        return 1;
    }
    fclose(fout);
    return 0;
}
/*文件打开表是否有空位，为0则表示有空位*/ 
int test_fd(int number){
    if(fopen_table[number] == 0){                  // 0不会分配给文件
        return 0;
    }
    else if(fopen_table[number] > 0)return 1;
    else return 2;
}
int openfile(ext2_inode *current, char *name){
    FILE *fp = NULL;
    int i,pos,full = 1;
    ext2_inode node;
    while (fp == NULL)fp = fopen(PATH, "r+");
    for (i = 0; i < (current->i_size / 32); i++)
    {
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);
        if (!strcmp(dir.name, name)&&dir.file_type == 1)
        {   
            fseek(fp, 3 * blocksiz + (dir.inode) * sizeof(ext2_inode), SEEK_SET);
            fread(&node, sizeof(ext2_inode), 1, fp);
            if(test_fd(node.fd - 1) == 1){
                fclose(fp);
                return 2; 
            }
            for(i = 0; i < 16; i++){
                if(!test_fd(i)){
                    full = 0;
                    pos = i;           //打开表有空位
                    break;
                }
            }
            if(!full){
                fopen_table[pos] = dir.inode;         //装入打开表
                node.fd = pos + 1;
                time_t now;
                time(&now);
                node.i_atime = now;
                fseek(fp, 3 * blocksiz + (dir.inode) * sizeof(ext2_inode), SEEK_SET);
                fwrite(&node, sizeof(ext2_inode), 1, fp);
                fclose(fp);
                return 0;    
            }
            else{
                fclose(fp);
                return 3;             //打开表已经满了
            }
        }
    }
    fclose(fp);
    return 1;
}
/*从打开表中移除文件，并更新文件的相关信息。*/
int closefile(ext2_inode *current, char *name){
    FILE *fp = NULL;
    int i;
    ext2_inode node;
    while (fp == NULL)fp = fopen(PATH, "r+");
    for (i = 0; i < (current->i_size / 32); i++)
    {
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);
        if (!strcmp(dir.name, name)&&dir.file_type == 1)
        {   
            fseek(fp, 3 * blocksiz + (dir.inode) * sizeof(ext2_inode), SEEK_SET);
            fread(&node, sizeof(ext2_inode), 1, fp);
            if(test_fd(node.fd - 1) == 1){
                fopen_table[node.fd - 1] = 0;
                node.fd = 0;
                fseek(fp, 3 * blocksiz + (dir.inode) * sizeof(ext2_inode), SEEK_SET);
                fwrite(&node, sizeof(ext2_inode), 1, fp);
                fclose(fp);
                return 0;
            }
            else{
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 1;
}
/*main shell*/
void shellloop(ext2_inode currentdir)
{
    char command[10], var1[10], var2[128], path[10];
    ext2_inode temp;
    int i, j;
    char currentstring[20];
    char ctable[16][10] = { "create", "delete", "cd", "close", "read", "write", "password", "format","exit","login","logout","dir","mkdir","rmdir","attrib","open"}; 
    while (1){getstring(currentstring, currentdir); //获取当前目录的目录名
    if(currentdir.i_block[0] == 0)printf("\n[root@ext2]# ");
    else printf("\n[root@ext2 %s]# ", currentstring);
    scanf("%s", command);
    for (i = 0; i < 16; i++)if (!strcmp(command, ctable[i]))break;
    if (i == 0 || i == 1) //create delete
    {
        scanf("%s", var2);  //读入文件名
        if (i == 0)
        {
            if (Create(1, &currentdir, var2) == 1)printf("Failed! %s can't be created\n", var2);
            else printf("Congratulations! %s is created\n", var2);
        }
        else
        {
            if (Delet(1, &currentdir, var2) == 1)printf("Failed! %s can't be deleted!\n", var2);
            else printf("Congratulations! %s is deleted!\n", var2);
        }
    }
    else if (i == 14){             //modify the authority
        scanf("%s", var1);
        scanf("%s", var2);
        if(!strcmp(var2, "rw")){
            if (attrib(&currentdir, var1, 0) == 1)printf("Failed! The file'authority can't be modified\n");
            else printf("Modify successfully!\n");
        }
        else if(!strcmp(var2, "r")){
            if (attrib(&currentdir, var1, 1) == 1)printf("Failed! The file'authority can't be modified\n");
            else printf("Modify successfully!\n");
        }
        else if(!strcmp(var2, "w")){
            if (attrib(&currentdir, var1, 2) == 1)printf("Failed! The file'authority can't be modified\n");
            else printf("Modify successfully!\n");
        }
        else printf("please input a right kind of authority!\n");
        
    }
    else if (i == 12||i == 13){       //mkdir rmdir
        scanf("%s", var2);
        if (i == 12)
        {
            if (Create(2, &currentdir, var2) == 1)printf("Failed! %s can't be created\n", var2);
            else printf("Congratulations! %s is created\n", var2);
        }
        else if(i == 13)
        {
            if (Delet(2, &currentdir, var2) == 1)printf("Failed! %s can't be deleted!\n", var2);
            else printf("Congratulations! %s is deleted!\n", var2);
        }
    }
    else if(i == 15){             //open file
        scanf("%s", var2);
        int judge = openfile(&currentdir, var2);
        if (judge == 1)printf("Failed! The file can't be opened\n");    //没有或者为目录
        else if(judge == 2)printf("The file has already been opened!\n");//已经在文件打开表中
        else if(judge == 3)printf("Failed! The file_open_table is full\n");//文件打开表已满
        else if(judge == 0)printf("Open successfully!\n");
    }
    else if (i == 2) // open == cd change dir
    {
        scanf("%s", var2);
        i = 0;
        j = 0;
        temp = currentdir;
        while (1)
        {
            path[i] = var2[j];
            if (path[i] == '/')
            {
                if (j == 0)initialize(&currentdir);
                else if (i == 0)
                {
                    printf("path input error!\n");
                    break;
                }
                else
                {
                    path[i] = '\0';
                    if (Open(&currentdir, path) == 1)
                    {
                        printf("path input error!\n");
                        currentdir = temp;
                    }
                }
                i = 0;
            }
            else if (path[i] == '\0')
            {
                if (i == 0)break;
                if (Open(&currentdir, path) == 1)
                {
                    printf("path input error!\n");
                    currentdir = temp;
                }
                break;
            }
            else i++;
            j++;
        }
    }
    else if (i == 3) // close
    {
        scanf("%s", var2);
        int judge = closefile(&currentdir, var2);
        if (judge == 1)printf("Failed! The file can't be closed\n");
        else if(judge == 0)printf("Close successfully!\n");
    }
    else if (i == 4) // read
    {
        scanf("%s", var2);
        int judge = openfile(&currentdir, var2);
        if (judge == 1)printf("Failed! The file can't be opened\n");
        else if(judge == 3)printf("Failed! The file_open_table is full\n");
        else{
            if (Read(&currentdir, var2) == 1)printf("Failed! The file can't be read\n");
        }
    }
    else if (i == 5) // write
    {
        scanf("%s", var2);
        int judge = openfile(&currentdir, var2);
        if (judge == 1)printf("Failed! The file can't be opened\n");
        else if(judge == 3)printf("Failed! The file_open_table is full\n");
        else {
            if (Write(&currentdir, var2) == 1)printf("Failed! The file can't be written\n");
        }
    }
    else if (i == 6) // password
        Password();
    else if (i == 7) // format
    {
        while (1)
        {
            printf("Do you want to format the filesystem?\n It will be dangerous to your data.\n"); 
            printf("[Y/N]"); 
            scanf("%s",var1); 
            if(var1[0]=='N'||var1[0]=='n')break; 
            else if(var1[0]=='Y'|| var1[0]=='y') 
            {
                format(&currentdir);
                break; 
            } 
            else printf("please input [Y/N]");
        }
    }
    else if (i == 8) // exit
    {
        while (1)
        {
            printf("Do you want to exit from filesystem?[Y/N]");
            scanf("%s", &var2);
            if (var2[0] == 'N' || var2[0] == 'n')
                break;
            else if (var2[0] == 'Y' || var2[0] == 'y')
                return;
            else
                printf("please input [Y/N]");
        }
    }
    else if (i == 9) // login
        printf("Failed! You havn't logged out yet\n");
    else if (i == 10) // logout
    {
        while (i)
        {
            printf("Do you want to logout from filesystem?[Y/N]");
            scanf("%s", var1);
            if (var1[0] == 'N' || var1[0] == 'n')
                break;
            else if (var1[0] == 'Y' || var1[0] == 'y')
            {
                initialize(&currentdir);
                while (1)
                {
                    printf("[no user]# ");
                    scanf("%s", var2);
                    clearBuffer();
                    if (strcmp(var2, "login") == 0)
                    {
                        if (login() == 0)
                        {
                            i = 0;
                            break;
                        }else {
                            printf("Wrong Password!\n");
                        }
                    }
                    else if (strcmp(var2, "exit") == 0)return;
                    else printf("please login!\n");
                }
            }
            else
                printf("please input [Y/N]");
        }
    }
    else if (i == 11) // dir
    {
        char v;
        while(1){
            v = getchar();
            if(v == '-'){
                v = getchar();
                break;
            }
            else if( v == '\n'){
                break;
            }
        }
        if(v == 'm'){
            lsdir(&currentdir,1);
        }
        else if(v == '\n'||v == 'n'){
            lsdir(&currentdir,0);
        }
    }                   
    else
        printf("Failed! Command not available\n");
}
}
