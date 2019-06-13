/* error.c -- Functions for handling errors. */

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

#include "config.h"

#include "bashtypes.h"
#include <fcntl.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#if defined (PREFER_STDARG)
#  include <stdarg.h>
#else
#  include <varargs.h>
#endif

#include <stdio.h>

#include <errno.h>
#if !defined (errno)
extern int errno;
#endif /* !errno */

#include "bashansi.h"
#include "bashintl.h"

#include "shell.h"
#include "flags.h"
#include "input.h"

#if defined (HISTORY)
#  include "bashhist.h"
#endif

extern int executing_line_number __P((void));

extern int last_command_exit_value;
extern char *shell_name;
#if defined (JOB_CONTROL)
extern pid_t shell_pgrp;
extern int give_terminal_to __P((pid_t, int));
#endif /* JOB_CONTROL */

#if defined (ARRAY_VARS)
extern const char * const bash_badsub_errmsg;
#endif

static void error_prolog __P((int));

/* The current maintainer of the shell.  You change this in the
   Makefile. 
   TODO  shell的当前维护者。在makefile中更改此项。*/
#if !defined (MAINTAINER)
#define MAINTAINER "bash-maintainers@gnu.org"
#endif

const char * const the_current_maintainer = MAINTAINER;

int gnu_error_format = 0;

static void
error_prolog (print_lineno)
     int print_lineno;
{
  char *ename;
  int line;

  /*NOTE 函数调用，返回用于错误报告的shell或shell脚本的名称。*/
  ename = get_name_for_error ();
  /*NOTE  interactive_shell指交互式shell，print_lineno打印行编号，executing_line_number执行线号 */
  line = (print_lineno && interactive_shell == 0) ? executing_line_number () : -1;

  /* NOTE stderr -- 标准错误输出设备
  【unix】标准输出(设备)文件，对应终端的屏幕。
  进程将从标准输入文件中得到输入数据，将正常输出数据输出到标准输出文件，
  而将错误信息送到标准错误文件中。
  在C中，程序执行时，一直处于开启状态。*/
  if (line > 0)/*行错误 */
    fprintf (stderr, "%s:%s%d: ", ename, gnu_error_format ? "" : _(" line "), line);
  else
    fprintf (stderr, "%s: ", ename);
}

/* Return the name of the shell or the shell script for error reporting.
TODO 返回用于错误报告的shell或shell脚本的名称。
这是没在．ｈ里定义的错误类型 */
char *
get_name_for_error ()
{
  char *name;
#if defined (ARRAY_VARS)
  SHELL_VAR *bash_source_v;
  ARRAY *bash_source_a;/*NOTE BASH_SOURCE ,取得当前执行的 shell 文件所在的路径及文件名 */
#endif

  name = (char *)NULL;
  /*NOTE  interactive_shell指交互式shell */
  if (interactive_shell == 0)
    {
#if defined (ARRAY_VARS)
      bash_source_v = find_variable ("BASH_SOURCE");
      if (bash_source_v && array_p (bash_source_v) &&
	  (bash_source_a = array_cell (bash_source_v)))
	name = array_reference (bash_source_a, 0);
      if (name == 0 || *name == '\0')	/* XXX - was just name == 0 */
#endif
	name = dollar_vars[0];
    }
  if (name == 0 && shell_name && *shell_name)
    name = base_pathname (shell_name);/*路径文件名  */
  if (name == 0)
#if defined (PROGRAM)
    name = PROGRAM;
#else
    name = "bash";
#endif

  return (name);
}

/* Report an error having to do with FILENAME.  This does not use
   sys_error so the filename is not interpreted as a printf-style
   format string. 
   NOTE 报告与文件名有关的错误。这不使用sys_错误，
   因此文件名不会被解释为printf样式的格式字符串。*/
   
void/*ＩＭＰ　报告与文件名有关的错误。 */
file_error (filename)
     const char *filename;
{
  report_error ("%s: %s", filename, strerror (errno));
}

void/*ＩＭＰ　报告程序员的错误，然后中止。通过原因，arg1…ARG5。 */
#if defined (PREFER_STDARG)
programming_error (const char *format, ...)
#else
programming_error (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  /*NOTE  va_list 是一个字符指针，可以理解为指向当前参数的一个指针，取参必须通过这个指针进行 */
  /*NOTE
  <Step 1> 在调用参数表之前，定义一个 va_list 类型的变量，(我们va_list 类型变量被定义为args)；
  <Step 2> 然后应该对args 进行初始化，让它指向可变参数表里面的第一个参数，这是通过 va_start 来实现的，
            第一个参数是 args 本身，第二个参数是在变参表前面紧挨着的一个变量,即“...”之前的那个参数；
  <Step 3> 然后是获取参数，调用va_arg，它的第一个参数是args，第二个参数是要获取的参数的指定类型，
            然后返回这个指定类型的值，并且把 args 的位置指向变参表的下一个变量位置；
  <Step 4> 获取所有的参数之后，我们有必要将这个args 指针关掉，以免发生危险，方法是调用 va_end，
          他是输入的参数 args 置为 NULL，应该养成获取完参数表之后关闭指针的习惯。
          说白了，就是让我们的程序具有健壮性。通常va_start和va_end是成对出现。
          以下多次出现，不再解释这个 */
  va_list args;
  char *h;

#if defined (JOB_CONTROL)
   give_terminal_to (shell_pgrp, 0);//终端
#endif /* JOB_CONTROL */

  /* NOTE stderr -- 标准错误输出设备
  【unix】标准输出(设备)文件，对应终端的屏幕。
  进程将从标准输入文件中得到输入数据，将正常输出数据输出到标准输出文件，
  而将错误信息送到标准错误文件中。
  在C中，程序执行时，一直处于开启状态。*/
 
  /*NOTE  start函数来获取参数列表中的参数
  args指向传入的第一个可选参数，format是最后一个确定的参数*/ 
  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");
  va_end (args);/*NOTE 使用完毕后调用va_end()结束 */

#if defined (HISTORY)
  if (remember_on_history)
    {
      h = last_history_line ();
      fprintf (stderr, _("last command: %s\n"), h ? h : "(null)");
    }
#endif

#if 0
  fprintf (stderr, "Report this to %s\n", the_current_maintainer);
#endif

  fprintf (stderr, _("Aborting..."));/*NOTE 正在中止... */
  fflush (stderr);

  abort ();
}

/* Print an error message and, if `set -e' has been executed, exit the
   shell.  Used in this file by file_error and programming_error.  Used
   outside this file mostly to report substitution and expansion errors,
   and for bad invocation options.
   
   NOTE 打印错误消息，如果已执行“set-e”，则退出shell。
   在这个文件中使用文件错误和编程错误。在该文件之外使用，
   主要是报告替换和扩展错误，以及错误的调用选项。 */

void/*IMP　常规错误报告。*/
#if defined (PREFER_STDARG)
report_error (const char *format, ...)
#else
report_error (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;

  error_prolog (1);/*TODO ？？？ */

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  if (exit_immediately_on_error)/*NOTE出现错误时立即退出 */
    {
      if (last_command_exit_value == 0)
	last_command_exit_value = 1;
      exit_shell (last_command_exit_value);/*NOTE最后一个命令返回值 */
    }
}

void  /*IMP 报告不可恢复的错误并退出。*/
#if defined (PREFER_STDARG)
fatal_error (const char *format, ...)
#else
fatal_error (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;

  error_prolog (0);

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  sh_exit (2);/*TODO ？？ */
}

void/*IMP   报告内部错误。 */
#if defined (PREFER_STDARG)
internal_error (const char *format, ...)
#else
internal_error (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;

  error_prolog (1);

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
}

void/*IMP 报告内部警告 */
#if defined (PREFER_STDARG)
internal_warning (const char *format, ...)
#else
internal_warning (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;

  error_prolog (1);
  fprintf (stderr, _("warning: "));

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
}

void
#if defined (PREFER_STDARG)
internal_inform (const char *format, ...)/*IMP 报告内部信息通知。 */
#else
internal_inform (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;

  error_prolog (1);
  /* TRANSLATORS: this is a prefix for informational messages.
  NOTE 这是信息消息的前缀。*/
  fprintf (stderr, _("INFORM: "));

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
}
 /* NOTE  函数：char * strerror(int errnum);
函数说明：strerror()用来依参数errnum 的错误代码来查询其错误原因的描述字符串, 然后将该字符串指针返回.
返回值：返回描述错误原因的字符串指针.
经常在调用linux 系统api 的时候会出现一些错误，比方说使用open() write() creat()之类的函数有些时候会返回-1，也就是调用失败，这个时候往往需要知道失败的原因。这个时候使用errno这个全局变量就相当有用了。
在程序代码中包含 #include <errno.h>,然后每次程序调用失败的时候，系统会自动用用错误代码填充errno这个全局变量，这样你只需要读errno这个全局变量就可以获得失败原因了。*/

/*IMP 报告系统错误，如BSD警告（3）。 */ 
void
#if defined (PREFER_STDARG)
sys_error (const char *format, ...)
#else
sys_error (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  int e;
  va_list args;

  e = errno;//NOTE 系统会自动用用错误代码填充errno这个全局变量
  error_prolog (0);//TODO???

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, ": %s\n", strerror (e));

  va_end (args);
}

/* An error from the parser takes the general form

	shell_name: input file name: line number: message

   The input file name and line number are omitted if the shell is
   currently interactive.  If the shell is not currently interactive,
   the input file name is inserted only if it is different from the
   shell name.*/
   /*NOTE   分析器的错误采用常规形式
    shell名称：输入文件名：行号：消息
    如果shell当前是交互式的，则省略输入文件名和行号。
    如果shell当前不是交互式的，则仅当输入文件名与shell名不同时才插入该文件名。 */
void
#if defined (PREFER_STDARG)
/*NOTE  parser_error不调用报告语法的部分分析器的错误消息 */
parser_error (int lineno, const char *format, ...)
#else
parser_error (lineno, format, va_alist)
     int lineno;
     const char *format;
     va_dcl
#endif
{
  va_list args;
  char *ename, *iname;

  /*NOTE get_name_for_error ()：获取错误消息的shell或shell脚本的名称。 */ 
  ename = get_name_for_error ();
  iname = yy_input_name ();

  if (interactive)/*NOTE 交互 */
    fprintf (stderr, "%s: ", ename);
  else if (interactive_shell)
    fprintf (stderr, "%s: %s:%s%d: ", ename, iname, gnu_error_format ? "" : _(" line "), lineno);
  /*指((ename)[0] == (iname)[0] && strcmp(ename, iname) == 0) */
  else if (STREQ (ename, iname))
    fprintf (stderr, "%s:%s%d: ", ename, gnu_error_format ? "" : _(" line "), lineno);
  else
    fprintf (stderr, "%s: %s:%s%d: ", ename, iname, gnu_error_format ? "" : _(" line "), lineno);

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);

  if (exit_immediately_on_error)//TODO？？
    exit_shell (last_command_exit_value = 2);
}

#ifdef DEBUG
/* This assumes ASCII and is suitable only for debugging 
NOTE 这假定为ASCII，并且仅适用于调试*/
char *
strescape (str)
     const char *str;
{
  char *r, *result;
  unsigned char *s;
  
  /*给r和result字符串在堆上分配内存，
  其长度是字符串str长度的两倍，
  最后加1是因为字符串要以‘\0’结尾 */
  r = result = (char *)xmalloc (strlen (str) * 2 + 1);

  for (s = (unsigned char *)str; s && *s; s++)
    {
      if (*s < ' ')
	{
	  *r++ = '^';
	  *r++ = *s+64;
	}
      else if (*s == 127)/*NOTE 127是小三角 */
	{
	  *r++ = '^';
	  *r++ = '?';
	}
     else
	*r++ = *s;
    }

  *r = '\0';
  return result;
}

void
#if defined (PREFER_STDARG)
itrace (const char *format, ...)
#else
itrace (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  /*NOTE  va_list 是一个字符指针，可以理解为指向当前参数的一个指针，取参必须通过这个指针进行 */
  /*NOTE
  <Step 1> 在调用参数表之前，定义一个 va_list 类型的变量，(我们va_list 类型变量被定义为args)；
  <Step 2> 然后应该对args 进行初始化，让它指向可变参数表里面的第一个参数，这是通过 va_start 来实现的，
            第一个参数是 args 本身，第二个参数是在变参表前面紧挨着的一个变量,即“...”之前的那个参数；
  <Step 3> 然后是获取参数，调用va_arg，它的第一个参数是args，第二个参数是要获取的参数的指定类型，
            然后返回这个指定类型的值，并且把 args 的位置指向变参表的下一个变量位置；
  <Step 4> 获取所有的参数之后，我们有必要将这个args 指针关掉，以免发生危险，方法是调用 va_end，
          他是输入的参数 args 置为 NULL，应该养成获取完参数表之后关闭指针的习惯。
          说白了，就是让我们的程序具有健壮性。通常va_start和va_end是成对出现。 */
  va_list args;

  /*NOTE  trace追踪 
  getpid系统调用，功能是取得进程识别码,许多程序利用取到的此值来建立临时文件，以避免临时文件相同带来的问题*/
  fprintf(stderr, "TRACE: pid %ld: ", (long)getpid());

  /*NOTE  start函数来获取参数列表中的参数
  args指向传入的第一个可选参数，format是最后一个确定的参数*/  
  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);/*NOTE 使用完毕后调用va_end()结束 */
  /* NOTE stderr -- 标准错误输出设备
  【unix】标准输出(设备)文件，对应终端的屏幕。
  进程将从标准输入文件中得到输入数据，将正常输出数据输出到标准输出文件，
  而将错误信息送到标准错误文件中。
  在C中，程序执行时，一直处于开启状态。*/
  fflush(stderr);
}

/* A trace function for silent debugging -- doesn't require a control
   terminal. */
   /* NOTE 用于静默调试的跟踪函数——不需要控制终端. */
void
#if defined (PREFER_STDARG)
trace (const char *format, ...)
#else
trace (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  /*NOTE  va_list 是一个字符指针，可以理解为指向当前参数的一个指针，取参必须通过这个指针进行 */
  /*NOTE
  <Step 1> 在调用参数表之前，定义一个 va_list 类型的变量，(我们va_list 类型变量被定义为args)；
  <Step 2> 然后应该对args 进行初始化，让它指向可变参数表里面的第一个参数，这是通过 va_start 来实现的，
            第一个参数是 args 本身，第二个参数是在变参表前面紧挨着的一个变量,即“...”之前的那个参数；
  <Step 3> 然后是获取参数，调用va_arg，它的第一个参数是args，第二个参数是要获取的参数的指定类型，
            然后返回这个指定类型的值，并且把 args 的位置指向变参表的下一个变量位置；
  <Step 4> 获取所有的参数之后，我们有必要将这个args 指针关掉，以免发生危险，方法是调用 va_end，
          他是输入的参数 args 置为 NULL，应该养成获取完参数表之后关闭指针的习惯。
          说白了，就是让我们的程序具有健壮性。通常va_start和va_end是成对出现。 */
  va_list args;
  static FILE *tracefp = (FILE *)NULL;/*NOTE 让文件指针指向空 */

  if (tracefp == NULL)
    tracefp = fopen("/tmp/bash-trace.log", "a+");

  if (tracefp == NULL)
    tracefp = stderr;
  else
  /*NOTE  fcntl可实现对指定文件描述符的各种操作,F_SETFD：设置文件描述标识 
  操作类型由cmd决定。F_SETFD是cmd可取的值。
  fileno()函数：把文件流指针转换成文件描述符*/
    fcntl (fileno (tracefp), F_SETFD, 1);     /* close-on-exec */
/*NOTE  trace追踪 
  getpid系统调用，功能是取得进程识别码,许多程序利用取到的此值来建立临时文件，以避免临时文件相同带来的问题*/
  fprintf(tracefp, "TRACE: pid %ld: ", (long)getpid());

/*NOTE 可以通过探测到任意一个变量的地址，并且知道其他变量的类型，
通过指针移位运算，则顺藤摸瓜找到其他的输入变量。 */
  SH_VA_START (args, format);

  vfprintf (tracefp, format, args);
  fprintf (tracefp, "\n");

  va_end (args);

  fflush(tracefp);
}

#endif /* DEBUG */

/* **************************************************************** */
/*								    */
/*  		    Common error reporting    --->常见错误报告			    */
/*								    */
/* **************************************************************** */


static const char * const cmd_error_table[] = {
	N_("unknown command error"),	/* CMDERR_DEFAULT NOTE 未知的命令错误*/
	N_("bad command type"),		/* CMDERR_BADTYPE  NOTE错误的命令类型 */
	N_("bad connector"),		/* CMDERR_BADCONN  NOTE连接器损坏*/
	N_("bad jump"),			/* CMDERR_BADJUMP  NOTE错误跳转*/
	0
};
/*IMP　报告与命令分析或执行有关的错误。 */
void
command_error (func, code, e, flags)
     const char *func;
     int code, e, flags;	/* NOTE  flags currently unused（未完成的标志） */
{
  if (code > CMDERR_LAST)
  /*NOTE code可能的命令错误 */
    code = CMDERR_DEFAULT;

  /*NOTE programming_er报告程序员的错误，然后中止。 */
  programming_error ("%s: %s: %d", func, _(cmd_error_table[code]), e);
}

char *
command_errstr (code)
     int code;
{
  if (code > CMDERR_LAST)
    code = CMDERR_DEFAULT;

  return (_(cmd_error_table[code]));
}

#ifdef ARRAY_VARS
void/*NOTE最终调用报告错误或内部错误。 */
err_badarraysub (s)
     const char *s;
{
  /*TODO   report_error报告错误或内部错误，什么错误？ */
  report_error ("%s: %s", s, _(bash_badsub_errmsg));
}
#endif

void/*NOTE最终调用报告错误或内部错误。 */
err_unboundvar (s)
     const char *s;
{
   /*NOTE   report_error报告错误或内部错误,报告--为未绑定变量 */
  report_error (_("%s: unbound variable"), s);
}

void/*NOTE最终调用报告错误或内部错误。 */
err_readonly (s)
     const char *s;
{
  /*NOTE   report_error报告错误或内部错误,报告--为只读变量 */
  report_error (_("%s: readonly variable"), s);
}
