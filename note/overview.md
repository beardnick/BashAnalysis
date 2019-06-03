<!-- 
约定：  
1，所有函数名称均用斜体表示 *function*
2，对一个函数的说明包括： 所在位置 location： ； 功能 function： 参数说明：args： ；
-->
# Bash分析总览

## 1.函数调用关系
main()<shell.c>
  |-shell_initialize() 初始化, run_start_files() 读取配置文件
  |-reader_loop() 读取命令,　位于　eval.c
  | |-parse_command() 分析命令,返回 current_command 形成　command
  |   |-yyparse() 返回global_command 形成current_command
  |     |-yylex()
  |       |-read_token()返回 token 形成global_command
  |         |-read_token_word()返回word形成token
  |-execute_command()<execute_cmd.c> 
    |-execute_command_internal() 根据 command->type 调用不同函数
    |  |-execute_simple_command
    |  |-execute_arith_command
    |  |-execute_cond_command
    |  |-execute_xxx_command
    |    |- 递归调用　execute_command_internal, 然后调用 execute_simple_command
    |-make_child()<job.c> 创建子shell用于执行需要传递到subshell中的函数
    |  |-fork()
    |  |-execve()
    |-expand_word(),expend_words_list_internal()<subst.c>,用于将命令中单词扩展操作
      |-find_special_bulitin()
      |-find_function()
      |-find_shell_builtin()


## 2.命令的执行分为两步：
１．解释命令： shell.c 中　
COMMAND *global_command = (COMMAND *)NULL; //#IMP 获取命令的结构体
２．执行命令：execute_cmd.c 中　execute_command (command) 根据解释的command结构体中命令类型来调用函数执行

+ 命令执行步骤：

1. shell的初始化
    + main.c 函数入口在：　shell.c 中
      - shell.c 中首先初始化：　shell_initialize ()
      - 读取需要的配置文件：　run_startup_files ()
2. 读取命令：初始化完成之后进入　eval.c reader_loop ()　循环读取命令
    + 通过　read_command ()　来调用　parse_command() 分析命令，返回命令的结构体GLOBAL_COMMAND后赋值给   current_command然后交给execute_command执行
    + **parse_command() 后面调用的函数是编译原理中语法分析文法分析等** parse_command()会调用yy.tab.c()和yyparse()
**获取命令的过程看起来是这样的：**
main()-->read_loop()-->read_command()-->parse_command()-->yyparse()-->yylex()-->read_token()-->read_token_word()
                            |                                |                      |                  |
                          current_command<-------------global_command<------------token<--------------word
3. 执行命令：　在命令解析得到　current_command 后，调用　execute_command() 来执行命令
    + execute_command(command) 调用 execute_command_internal()
    + execute_command_internal 根据命令command->type　调用不同类型的处理函数
        - if (command->type == cm_subshell && (command->flags & **CMD_NO_FORK**))
          return (execute_in_subshell (command, asynchronous, pipe_in, pipe_out, fds_to_close));
          这种
        - if (command->type == cm_subshell || (command->flags & (**CMD_WANT_SUBSHELL**|CMD_FORCE_SUBSHELL)) || (shell_control_structure (command->type) && (pipe_out != NO_PIPE || pipe_in != NO_PIPE || asynchronous)))














## 3.***main函数入口***
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
// #IMP 定义bash命令结构
/* What a command looks like. */ 
```c
typedef struct command {
  enum command_type type;	/* FOR CASE WHILE IF CONNECTION or SIMPLE. */
  int flags;			/* Flags controlling execution environment. */
  int line;			/* line number the command starts on */
  REDIRECT *redirects;		/* Special redirects for FOR CASE, etc. */
  union {
    struct for_com *For;
    struct case_com *Case;
    struct while_com *While;
    struct if_com *If;
    struct connection *Connection;
    struct simple_com *Simple;
    struct function_def *Function_def;
    struct group_com *Group;
#if defined (SELECT_COMMAND)
    struct select_com *Select;
#endif
#if defined (DPAREN_ARITHMETIC)
    struct arith_com *Arith;
#endif
#if defined (COND_COMMAND)
    struct cond_com *Cond;
#endif
#if defined (ARITH_FOR_COMMAND)
    struct arith_for_com *ArithFor;
#endif
    struct subshell_com *Subshell;
    struct coproc_com *Coproc;
  } value; 
  //#IMP 指令的类型值，shell根据不同value类型执行不同种类的指令，为simple时又会判断时候是builtin指令来调用
  // execute_command 和　execute_internal_command 
} COMMAND;
```


