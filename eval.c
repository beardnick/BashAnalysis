/* eval.c -- reading and evaluating commands. */

/* Copyright (C) 1996-2011 Free Software Foundation, Inc.

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
/*eval.c读取并解释执行shell命令。主要是循环读取命令，然后语法分析器分析命令后交给execute_cmd.c执行。
主循环为reader_loop()函数，它调用read_command()，read_command()调用parse_command()，
parse_command()调用语法分析器y.tab.c中的yyparse()。
得到命令后，reader_loop()调用execute_cmd.c中的execute_command()执行命令。*/

#include "config.h"

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include "bashansi.h"
#include <stdio.h>

#include <signal.h>

#include "bashintl.h"

#include "shell.h"
#include "flags.h"
#include "trap.h"

#include "builtins/common.h"

#include "input.h"
#include "execute_cmd.h"

#if defined (HISTORY)
#  include "bashhist.h"
#endif
//#NOTE extern可以置于变量或者函数前
//#NOTE以表示变量或者函数的定义在别的文件中
//#NOTE提示编译器遇到此变量和函数时在其他模块中寻找其定义。
extern int EOF_reached;
extern int indirection_level;
extern int posixly_correct;
extern int subshell_environment, running_under_emacs;
extern int last_command_exit_value, stdin_redir;
extern int need_here_doc;
extern int current_command_number, current_command_line_count, line_number;
extern int expand_aliases;
extern char *ps0_prompt;

#if defined (HAVE_POSIX_SIGNALS)
extern sigset_t top_level_mask;
#endif

static void send_pwd_to_eterm __P((void));
static sighandler alrm_catcher __P((int));

// #IMP 2019-06-01 非常重要的循环读取指令的循环
/* 读取并执行命令，直到达到EOF。这假定输入源已初始化 */
//#IMP 主循环reader_loop()函数，调用read_command()
int reader_loop ()
{
  int our_indirection_level;
  COMMAND * volatile current_command;

// #TODO 2019-06-01 把它转换为void*的宏？
  USE_VAR(current_command);

  current_command = (COMMAND *)NULL;

  //#NOTE 命令的递归深度
  our_indirection_level = ++indirection_level;
// #NOTE 2019-06-01 没有到达EOF（end of file文件结尾）时一直读，就是没有读到指令结尾时就一直读
  //#NOTE 到达文件末尾时，这个全局变量非零。为零即当未到末尾时
  while (EOF_Reached == 0)
    {
      int code;

// #NOTE 2019-06-01 处理bash信号
      code = setjmp_nosigs (top_level);

#if defined (PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

      /* #TODO  - 为什么每次都通过循环设置这个？为什么呢
如果SIGINT被困在交互式shell中？
 */
      //#NOTE interactive_shell非零意味着shell作为交互式shell启动
      if (interactive_shell && signal_is_ignored (SIGINT) == 0)
	set_signal_handler (SIGINT, sigint_sighandler);

      if (code != NOT_JUMPED)
	{
    //#NOTE命令的递归深度
	  indirection_level = our_indirection_level;

// #NOTE 2019-06-01 根据信号来确定bash的操作
	  switch (code)
	  {
        // #NOTE 2019-06-01 退出shell
	      /* Some kind of throw to top_level has occurred.
        -发生了某种抛向top_level。- */
	    case FORCE_EOF://#NOTE停止解析，-1
	    case ERREXIT://#NOTE由于错误情况退出，-4
	    case EXITPROG://#NOTE现在无条件退出程序。-3
	      current_command = (COMMAND *)NULL;
	      if (exit_immediately_on_error)
            //#NOTE 当前的变量上下文。 这实际上是我们执行函数的深度计算。
		        variable_context = 0;	/* 不是在一个功能 */
	      EOF_Reached = EOF;//直到达到EOF
        // #NOTE 2019-06-01 跳转到exec_done的标号处
	      goto exec_done;

	    case DISCARD://#NOTE丢弃当前命令。-2
	      /* 确保退出状态重置为非零值，但是
        仅保留现有的非零值（例如，信号> 128） */
        //#NOTE如果上一个同步命令返回的值==0。
	      if (last_command_exit_value == 0)
		        last_command_exit_value = EXECUTION_FAILURE;//execute_command（）返回的值
	      //如果刚刚分叉并且当前正在子shell中运行（非零环境）。
        if (subshell_environment)
        {
          current_command = (COMMAND *)NULL;
          EOF_Reached = EOF;
          goto exec_done;
        }
    // #NOTE 2019-06-01 丢弃当前指令
	      /* 不受阻碍的命令元素等 */
	      if (current_command)
        {
          dispose_command (current_command);
          current_command = (COMMAND *)NULL;
        }
#if defined (HAVE_POSIX_SIGNALS)
	      sigprocmask (SIG_SETMASK, &top_level_mask, (sigset_t *)NULL);
#endif
	      break;

	    default:
      // #NOTE 2019-06-01 打印错误信息
	      command_error ("reader_loop", CMDERR_BADJUMP, code, 0);
	    }
	}

      executing = 0;
      //#NOTE 一组shell分配，仅在单个命令的环境中进行。
      if (temporary_env)
	        dispose_used_env_vars ();

#if (defined (ultrix) && defined (mips)) || defined (C_ALLOCA)
      /* 尝试回收使用alloca（）分配的内存。 */
      (void) alloca (0);
#endif

      if (read_command () == 0)
      {
        //#NOTE interactive_shell为零意味着非交互式，read_but_dont_execute非零表示读命令，但不执行它们
        if (interactive_shell == 0 && read_but_dont_execute)
          {
            //last_command_exit_value是上一个同步命令返回的值。
            last_command_exit_value = EXECUTION_SUCCESS;//0
            dispose_command (global_command);
            global_command = (COMMAND *)NULL;
          }
        else if (current_command = global_command)
          {
            global_command = (COMMAND *)NULL;
            //#NOTE 到目前为止执行的命令数++
            current_command_number++;

            executing = 1;//#NOTE 当我们执行顶级命令时非零
            /*如果fd 0是重定向到子shell的主题，则设置为1。 
            全局使reader_loop可以在执行命令之前将stdin_redir设置为零。 */
            stdin_redir = 0;

            /* 如果shell是交互式的，
            则在读取命令（可能是列表或管道）之后并在执行之前展开并显示$ PS0 */
     // #NOTE 2019-06-01 读取完指令后就马上展开提示(提示就是你指令前面的一些字符)
            if (interactive && ps0_prompt)
            {
              char *ps0_string;

              ps0_string = decode_prompt_string (ps0_prompt);
              if (ps0_string && *ps0_string)
                {
                  fprintf (stderr, "%s", ps0_string);
                  fflush (stderr);
                }
              free (ps0_string);
            }     
            //#IMP parse_command()函数里的yyparse将解析的命令保留在全局变量GLOBAL_COMMAND中
            //#IMP 这里得到命令后，reader_loop()调用execute_cmd.c中的execute_command()执行命令。
            execute_command (current_command);

          // #NOTE 2019-05-18 结束标号，指令执行完成
          exec_done:
          /*NOTE 宏调用很多。 SIGINT只设置interrupt_state变量。
          当它是安全的时，将QUIT放在代码中，然后“中断”地点。 
          相同的方案用于终止信号（例如，SIGHUP）和终止信号变量。 
          这会调用一个最终会退出shell的函数。*/
            QUIT;

// #NOTE 2019-06-01 释放当前指令的空间
// 每条指令的长短不一样，无法复用指令空间，只能每次重新申请
            if (current_command)
            {
              dispose_command (current_command);
              current_command = (COMMAND *)NULL;
            }
          }
      }
      else
      {
        /* 解析错误，如果不是交互式的话，可能会丢弃其余的流。
        interactive == 0指非交互式*/
        if (interactive == 0)
          EOF_Reached = EOF;
      }
      /* #IMP命令行上的-t设置变量just_one_command。 
      此变量仅在这一个地方使用：（eval.c中循环结束时的if条件中） 
      如果给出了-t标志，则在执行第一个命令之后，发出文件结束条件的信号并退出bash。
      reader_loop中的循环继续，直到它收到信号EOF_Reached。 
      -t选项的唯一作用是在循环结束时设置此标志，以确保循环仅执行一次。*/
      if (just_one_command)
	        EOF_Reached = EOF;
    }
  indirection_level--;//是命令的递归深度
  return (last_command_exit_value);//上一个同步命令返回的值。
}//#NOTE主函数结束

// #NOTE 2019-06-01 捕获警报
static sighandler
alrm_catcher(i)
     int i;
{
  printf (_("\007timed out waiting for input: auto-logout\n"));
  fflush (stdout);
  bash_logout ();	/*如果这是一个登录shell ->run ~/.bash_logout*/
  jump_to_top_level (EXITPROG);//无条件退出程序
  SIGRETURN (0);//->return(0)
}

/* Send an escape sequence to emacs term mode to tell it the
   current working directory. 
   ---将转义序列发送到emacs术语模式以告诉它当前的工作目录。---*/
static void send_pwd_to_eterm ()
{
  char *pwd, *f;

  f = 0;
  pwd = get_string_value ("PWD");
  if (pwd == 0)
    f = pwd = get_working_directory ("eterm");
  fprintf (stderr, "\032/%s\n", pwd);
  free (f);
}

// #NOTE 2019-06-01 解析bash指令语法
/* #IMP 调用YACC生成的解析器并返回解析的状态。
   从当前输入流（bash_input）读取输入。 
   yyparse将解析的命令保留在全局变量GLOBAL_COMMAND中。
   这是执行PROMPT_COMMAND的地方。 */
//#IMP read_command()调用parse_command()
int parse_command ()
{
  int r;
  char *command_to_execute;//要执行的命令

// #NOTE 2019-06-01 heredoc是指写在脚本里面的输入数据
  need_here_doc = 0;
  run_pending_traps ();

  /* 允许在打印每个主要提示之前执行随机命令。 
  如果设置了shell变量PROMPT_COMMAND，
  则它的值是要执行的命令。
  测试是来自parse.y的SHOULD_PROMPT（）
  和prompt_again（）的组合，这是实际打印提示的条件。 */
  /* The tests are a combination of SHOULD_PROMPT() and prompt_again() 
     from parse.y, which are the conditions under which the prompt is
     actually printed. */
  if (interactive && bash_input.type != st_string && parser_expanding_alias() == 0)
    {
      command_to_execute = get_string_value ("PROMPT_COMMAND");
      if (command_to_execute)
	execute_variable_command (command_to_execute, "PROMPT_COMMAND");

      if (running_under_emacs == 2)
      //#NOTE 将转义序列发送到emacs术语模式以告诉它当前的工作目录。
	send_pwd_to_eterm ();	/* Yuck */
      // #TODO 2019-06-01 Yuck是呸的意思，这是什么鬼，程序员爆粗口？
    }

  current_command_line_count = 0;
  //#IMP parse_command()调用语法分析器y.tab.c中的yyparse()是语法分析的开始
  //#NOTE bash也是利用yacc生成的代码来完成语法分析的，y.tab.c的yyparse()是yacc自动生成的
  r = yyparse ();

  if (need_here_doc)
    gather_here_documents ();

  return (r);//得到命令
}

/*  #IMP read_command ()被主函数调用，读取并解析命令，返回解析的状态。
解析结果的命令字符串保留在全局变量GLOBAL_COMMAND中以供reader_loop使用。
这是执行shell超时代码的地方。*/
int read_command ()
{
  //#NOTE bash中的变量不强调类型，可以认为都是字符串。
  //#NOTE typedef struct variable SHELL_VAR，变量
  SHELL_VAR *tmout_var;
  int tmout_len, result;
  //#NOTE typedef <error-type> SigHandler(int)，错误类型
  SigHandler *old_alrm;

  //#NOTE 配置左侧命令提示符的内容，显示当前目录名字
  set_current_prompt_level (1);
  //#NOTE typedef struct command COMMAND,命令保留变量
  global_command = (COMMAND *)NULL;

  /* #NOTE 只有交互时才会超时 */
  tmout_var = (SHELL_VAR *)NULL;
  tmout_len = 0;
  old_alrm = (SigHandler *)NULL;
  //#IMP 非零意味着此时shell是交互式的。
  //#IMP 一般来说，这意味着shell正在读取输入从键盘。
  if (interactive)
    {
      //#NOTE linux设置的空闲等待时间TMOUT
      tmout_var = find_variable ("TMOUT");
      //#NOTE ((tmout_var)->value != 0)，已经连接
      if (tmout_var && var_isset (tmout_var))
      {
        //#NOTE atoi字符串转换函数，((tmout_var)->value)，value_cell访问变量值
        tmout_len = atoi (value_cell (tmout_var));
        if (tmout_len > 0)
          {
            //#NOTE =>>(SigHandler *)signal (SIGALRM, alrm_catcher)
            old_alrm = set_signal_handler (SIGALRM, alrm_catcher);
            //#NOTE 设置时限
            alarm (tmout_len);
          }
      }
    }

  QUIT;

  current_command_line_count = 0;
  //#IMP 解析命令
  result = parse_command ();

  //#NOTE 此时shell是交互式的，shell正在读取输入从键盘
  if (interactive && tmout_var && (tmout_len > 0))
    {
      //#NOTE alarm()用来设置信号SIGALRM 
      //在经过参数指定的秒数后传送给目前的进程. 
      //如果参数为0, 则之前设置的闹钟会被取消, 并将剩下的时间返回.
      alarm(0);
      set_signal_handler (SIGALRM, old_alrm);
    }
  //#IMP 解析的命令
  return (result);
}
