# Bash源码分析

### 格式要求
分析时请使用注释对相应代码进行解释
为方便查找和整理注释，请大家写注释时顺便带上标签
+ #NOTE 2019-04-07 注释内容 
  普通笔记

+ #IMP 2019-04-07 注释内容
  重要笔记

+ #TODO 2019-04-07 注释内容
  还没有完全搞懂，以后还要继续看的笔记
  例如

```c
  #include <stdio.h>
  
  // #NOTE 2019-04-09 main函数，程序的唯一入口
  int main(int argc, char const* argv[])
  // #IMP 2019-04-09 argc 命令行参数的个数
  // argv 命令行参数的值
  {
  // #TODO 2019-04-09 printf 怎么用来着？算了，今天懒得分析了，以后再分析
      printf("hello, world!\n");
      return 0;
  }
```

### 项目组织

1. note/ 存放笔记的目录
2. res/ 资源文件夹，存放收集的学习资源

### Git使用

1. 每个人在自己的分支上进行学习，所有笔记都由程合并到主分支当中
2. 拉取个人的分支
    `git clone -b 你的分支名 git@github.com:beardnick/BashAnalysis.git`
3. 更新修改的文件
    `git add -u`
4. 提交更新到本地代码库并书写提交日志（注意写好git提交日志，表明自己做了什么）
    `git commit -m ‘添加笔记‘`
5. 提交更新到github上, 如果出现错误可以联系程,禁止使用`git -f` 选项
    `git push origin 你的分支名`
6. 每次提交前最好先执行
    `git pull`