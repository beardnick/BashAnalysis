<!-- 
约定：  
1，所有函数名称均用斜体表示 *function*
2，对一个函数的说明包括： 所在位置 location： ； 功能 function： 参数说明：args： ；
-->
# Bash分析总览

+ ***main函数入口***
  1. location: *shell.c*

-------------------------------
+  ***Shell Command Structs***
  1. **location**： *command.h*
  2. **function**: 
      +  这个函数定义的是**bash**所使用命令中所有结构定义，包括内部结构和显示出来的命令部分
      +  定义了常量， 字word_dedc，字的链接表word_list， 重定向redirect
      +  **数据结构**
         +  *command*：定义了**bash**所能执行的命令的形式结构，包括： for, case, while, if, connection, simple, function_def, group, select, arith, cond, arith_for, subshell, coproc， 这些结构在command中是以共用体union组织的; *command.h*后面部分就是对这些命令结构的定义, 
        - 例如对 **for** 的定义
        ```c
        /* FOR command. */
        typedef struct for_com {
          int flags;		/* See description of CMD flags. */
          int line;		/* line number the `for' keyword appears on */
          WORD_DESC *name;	/* The variable name to get mapped over. */
          WORD_LIST *map_list;	/* The things to map over.  This is never NULL. */
          COMMAND *action;	/* The action to execute.During execution, NAME is bound to successive members of MAP_LIST. */
        } FOR_COM; 
        ```
        - 对重定向 **redirect**的定义：
        ```c
        typedef union {
          int dest;			/* Place to redirect REDIRECTOR to, or ... */
          WORD_DESC *filename;		/* filename to redirect to. */
        } REDIRECTEE;

        typedef struct redirect {
          struct redirect *next;	/* Next element, or NULL. */
          REDIRECTEE redirector;	/* Descriptor or varname to be redirected. */
          int rflags;			/* Private flags for this redirection */
          int flags;			/* Flag value for `open'. */
          enum r_instruction  instruction; /* What to do with the information. */
          REDIRECTEE redirectee;	/* File descriptor or filename */
          char *here_doc_eof;		/* The word that appeared in <<foo. */
        } REDIRECT;
        ```