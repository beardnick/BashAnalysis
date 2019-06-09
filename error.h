/* error.h -- External declarations of functions appearing in error.c. */

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
/*error.c中的函数的外部声明。 */
#if !defined (_ERROR_H_)
#define _ERROR_H_

#include "stdc.h"

/* Get the name of the shell or shell script for an error message. */
/*NOTE获取错误消息的shell或shell脚本的名称。 */
extern char *get_name_for_error __P((void));

/* Report an error having to do with FILENAME. */
/*NOTE报告与文件名有关的错误。 */
extern void file_error __P((const char *));

/* Report a programmer's error, and abort.  Pass REASON, and ARG1 ... ARG5. */
/*NOTE报告程序员的错误，然后中止。通过原因，arg1…ARG5。 */
extern void programming_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* General error reporting.  Pass FORMAT and ARG1 ... ARG5. */
/*NOTE常规错误报告。传递格式和arg1…ARG5。 */
extern void report_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Error messages for parts of the parser that don't call report_syntax_error */
/*NOTE不调用报告语法的部分分析器的错误消息 */
extern void parser_error __P((int, const char *, ...))  __attribute__((__format__ (printf, 2, 3)));

/* Report an unrecoverable error and exit.  Pass FORMAT and ARG1 ... ARG5. */
/*报告不可恢复的错误并退出。传递格式和arg1…ARG5。 */
extern void fatal_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report a system error, like BSD warn(3). */
/*报告系统错误，如BSD警告（3）。 */
extern void sys_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report an internal error. */
/*NOTE报告内部错误。 */
extern void internal_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report an internal warning. */
/*NOTE报告内部警告。 */
extern void internal_warning __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report an internal informational notice. */
/*NOTE报告内部信息通知。 */
extern void internal_inform __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Debugging functions, not enabled in released version. */
/* NOTE调试函数，在发布版本中未启用。*/
extern char *strescape __P((const char *));
extern void itrace __P((const char *, ...)) __attribute__ ((__format__ (printf, 1, 2)));
extern void trace __P((const char *, ...)) __attribute__ ((__format__ (printf, 1, 2)));

/* Report an error having to do with command parsing or execution. */
/*NOTE报告与命令分析或执行有关的错误。 */
extern void command_error __P((const char *, int, int, int));

extern char *command_errstr __P((int));

/* Specific error message functions that eventually call report_error or
   internal_error. */
/*NOTE最终调用报告错误或内部错误。 */
extern void err_badarraysub __P((const char *));
extern void err_unboundvar __P((const char *));//NOTE未绑定变量
extern void err_readonly __P((const char *));//NOTE只读

#endif /* !_ERROR_H_ */
