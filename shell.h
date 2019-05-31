/* shell.h -- The data structures used by the shell */

/* Copyright (C) 1993-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

// #NOTE 2019-05-28

#ifdef HAVE_CONFIG_H

/*
 * 由configure生成，决定有哪些特性要被编译进bash。如果要新增功能，可以加一个“开关”宏定义
 */
#include "config.h"
#endif
/**
 * 对setjmp.h的一层封装，定义了longjmp()的几种状态参数
 * setjmp.h头文件中定义了宏 setjmp()、函数 longjmp() 和变量类型 jmp_buf，该变量类型会绕过正常的函数调用和返回规则
 * int setjmp(jmp_buf environment) ：创建本地的jmp_buf缓冲区并且初始化，用于将来跳转回此处。
   这个子程序保存程序的调用环境于env参数所指的缓冲区，env将被longjmp使用。
   如果是从setjmp直接调用返回，setjmp返回值为0。如果是从longjmp恢复的程序调用环境返回，setjmp返回非零值。
 * void longjmp(jmp_buf environment, int value) 恢复最近一次调用 setjmp() 宏时保存的环境，jmp_buf 参数的设置是由之前调用 setjmp() 生成的。
 * jmp_buf ：这是一个用于存储宏 setjmp() 和函数 longjmp() 相关信息的数组类型。
 */
#include "bashjmp.h"

/*
 * 各类命令（控制结构、函数、算术、重定向等）的结构定义。
 */
#include "command.h"

/*
 * syntax.h定义了词法分析工作中需要的宏和标志位等
 */
#include "syntax.h"

/*
 * 很多文件可能公用的一些基础的、不便分类的函数。
 */
#include "general.h"

/*
 * 错误处理与报告函数
 */
#include "error.h"

/*
 * 处理shell变量。用不同的哈希表分别存储不同生命周期的shell变量与函数。
   变量列表是由当前环境来初始化的。bash启动时环境由main()的char **env参数传入。
   对于函数，使用栈来保存和切换局部变量的上下文。
 */
#include "variables.h"

/*
 * 字符串数组定义及相关函数，实现了数组的一些高级操作。bash程序中一些字符串数组使用了这里定义的ARRAY结构。
 */
#include "arrayfunc.h"

/*
 * 定义通用的异常退出宏，是对SIGINT信号的响应
 */
#include "quit.h"

/*
 *TODO
 */
#include "maxpath.h"

/*
 * 通用的函数执行保护和退出处理机制
 */
#include "unwind_prot.h"

/*
 * 清理COMMAND结构占用的资源，dispose_redirects()清理重定向语句
 */
#include "dispose_cmd.h"

/*
 * 构造各类命令、重定向等语法结构实例所需的函数。由语法分析器、redir.c等调用。
   其中make_redirection()填写命令结构的重定向参数。
 */
#include "make_cmd.h"

/*
 * TODO
 */
#include "ocache.h"

/*
 * 负责参数、命令、算术、路径扩展、引号等的代入、展开工作
 */
#include "subst.h"

/*
 * 信号处理相关函数
 */
#include "sig.h"

/*
 * 记录一些操作系统配置文件的路径
 */
#include "pathnames.h"

/*
 * 声明一些源文件中的函数，它们在自己的头文件中没有声明
 */
#include "externs.h"

extern int EOF_Reached;

#define NO_PIPE -1
#define REDIRECT_BOTH -2

#define NO_VARIABLE -1

/* Values that can be returned by execute_command (). */
//可由执行命令返回的值
#define EXECUTION_FAILURE 1
#define EXECUTION_SUCCESS 0

/* Usage messages by builtins result in a return status of 2. */
//内置的使用率消息导致返回状态为2
#define EX_BADUSAGE	2

#define EX_MISCERROR	2

/* Special exit statuses used by the shell, internally and externally. */
//shell内部和外部使用的特殊的终止状态
#define EX_RETRYFAIL	124
#define EX_WEXPCOMSUB	125
#define EX_BINARY_FILE	126
#define EX_NOEXEC	126
#define EX_NOINPUT	126
#define EX_NOTFOUND	127

#define EX_SHERRBASE	256	/* all special error values are > this. *///所有特殊错误的值大于256

#define EX_BADSYNTAX	257	/* shell syntax error */
#define EX_USAGE	258	/* syntax error in usage */
#define EX_REDIRFAIL	259	/* redirection failed */
#define EX_BADASSIGN	260	/* variable assignment error */
#define EX_EXPFAIL	261	/* word expansion failed */

/* Flag values that control parameter pattern substitution. */
//控制参数模式替换的标志值
#define MATCH_ANY	0x000
#define MATCH_BEG	0x001
#define MATCH_END	0x002

#define MATCH_TYPEMASK	0x003

#define MATCH_GLOBREP	0x010
#define MATCH_QUOTED	0x020
#define MATCH_ASSIGNRHS	0x040
#define MATCH_STARSUB	0x080

/* Some needed external declarations. */
//一些需要的外部声明
extern char **shell_environment;
extern WORD_LIST *rest_of_args;

/* Generalized global variables. */
//广义全局变量
extern int debugging_mode;
extern int executing, login_shell;
extern int interactive, interactive_shell;
extern int startup_state;
extern int subshell_environment;
extern int shell_compatibility_level;

extern int locale_mb_cur_max;

/* Structure to pass around that holds a bitmap of file descriptors
   to close, and the size of that structure.  Used in execute_cmd.c. */
//要传递的结构，其中包含要关闭的文件描述符的位图，包括结构的大小，用于execute_cmd.c
struct fd_bitmap {
  int size;
  char *bitmap;
};

#define FD_BITMAP_SIZE 32

#define CTLESC '\001'
#define CTLNUL '\177'

/* Information about the current user. */
//用户信息
struct user_info {
  uid_t uid, euid;
  gid_t gid, egid;
  char *user_name;
  char *shell;		/* shell from the password file */
  char *home_dir;
};

extern struct user_info current_user;

/* Force gcc to not clobber X on a longjmp().  Old versions of gcc mangle
   this badly. */
//强制gcc不在Longjmp上删除X，旧版本的gcc对此有严重的破坏
#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 8)
#  define USE_VAR(x)	((void) &(x))
#else
#  define USE_VAR(x)
#endif

#define HEREDOC_MAX 16

/* Structure in which to save partial parsing state when doing things like
   PROMPT_COMMAND and bash_execute_unix_command execution. */
//在执行PROMPT_COMMAND 和 bash_execute_unix_command execution操作时保存部分解析状态结构

//TODO
typedef struct _sh_parser_state_t {

  /* parsing state */
  int parser_state;
  int *token_state;

  char *token;
  int token_buffer_size;

  /* input line state -- line number saved elsewhere */
  int input_line_terminator;
  int eof_encountered;

#if defined (HANDLE_MULTIBYTE)
  /* Nothing right now for multibyte state, but might want something later. */
#endif

  char **prompt_string_pointer;

  /* history state affecting or modified by the parser */
  int current_command_line_count;
#if defined (HISTORY)
  int remember_on_history;
  int history_expansion_inhibited;
#endif

  /* execution state possibly modified by the parser */
  int last_command_exit_value;
#if defined (ARRAY_VARS)
  ARRAY *pipestatus;
#endif
  sh_builtin_func_t *last_shell_builtin, *this_shell_builtin;

  /* flags state affecting the parser */
  int expand_aliases;
  int echo_input_at_read;
  int need_here_doc;
  int here_doc_first_line;

  /* structures affecting the parser */
  REDIRECT *redir_stack[HEREDOC_MAX];
} sh_parser_state_t;

//TODO
typedef struct _sh_input_line_state_t {
  char *input_line;
  size_t input_line_index;
  size_t input_line_size;
  size_t input_line_len;
} sh_input_line_state_t;

/* Let's try declaring these here. */
//TODO
extern char *parser_remaining_input __P((void));

extern sh_parser_state_t *save_parser_state __P((sh_parser_state_t *));
extern void restore_parser_state __P((sh_parser_state_t *));

extern sh_input_line_state_t *save_input_line_state __P((sh_input_line_state_t *));
extern void restore_input_line_state __P((sh_input_line_state_t *));
