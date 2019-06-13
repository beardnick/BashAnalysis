# Bash源码分析约定

## 格式要求
### 源码注释格式
*分析时请使⽤注释对相应代码进⾏解释 为⽅便查找和整理注释，请⼤家写注释时顺便带上标签*
+ #NOTE 20190407 注释内容 普通注释
+ #IMP 20190407 注释内容 重要笔记
+ #TODO 20190407 还没有完全搞懂，以后还要继续看的笔记 例如
```c
#include <stdio.h>
// #NOTE 2019-04-09 main函数，程序的唯⼀⼊⼝
int main(int argc, char const* argv[])
// #IMP 2019-04-09 argc 命令⾏参数的个数
// argv 命令⾏参数的值
{
// #TODO 2019-04-09 printf 怎么⽤来着？算了，今天懒得分析了，以后再分析
printf("hello, world!\n");
return 0;
}
```
### 自己写的分析格式
1. 所有函数名称均用斜体表示 *function*
2. 对一个函数的说明包括： 所在位置 location： ； 功能 function： 参数说明：args： ；
