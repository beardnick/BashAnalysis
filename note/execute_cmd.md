# execute_cmd.c 函数内部调用结构

*** execute_com.c ***
+ 1. execute_command (command)
    * 这个函数是由内部命令来调用，函数内部又调用**execute_command_internal (command, 0, NO_PIPE, NO_PIPE, bitmap)**
+ 2. execute_command_internal() 内部调用接口　
    + execute_command_internal内部流程：
该函数是shell源码中执行命令的实际操作函数。他需要对作为操作参数传入的具体命令结构的value成员进行分析，并针对不同的value类型，再调用具体类型的命令执行函数进行具体命令的解释执行工作。
具体来说：

  + 2.1. **如果value是其他类型，则调用对应类型的函数进行分支控制**
  举例来说，如果是value是for_commmand,即这是一个for循环控制结构命令，则调用execute_for_command函数。
  在该函数中，将枚举每一个操作域中的元素，对其再次调用execute_command函数进行分析。  即execute_for_command这一类函数实现的是一个命令的展开以及流程控制以及递归调用execute_command的功能。

  + 2.2 **如果value是***simple***，则直接调用execute_simple_command函数进行执行**
  execute_simple_command再根据命令是内部命令或磁盘外部命令分别调用execute_builtin和execute_disk_command来执行。 其中,execute_disk_command在执行外部命令的时候调用make_child函数fork子进程执行外部命令。
    - 2.2.1 如果命令需要在*subshell*中执行，则先调用<job.c>中 make_child 来执行系统调用 fork, execve()或者shell_execve()
    - 2.2.2 如果不需要在*subshell*中执行，只需要对命令进行简单单词扩展操作，调用<subst.c>中expand_words(),以及expand_word_list_internal() 
  
  **Execute a simple command** that is hopefully defined in a disk file
   somewhere.

    + 1) fork ()
    + 2) connect pipes
    + 3) look up the command
    + 4) do redirections
    + 5) exetcve ()
    + 6) If the execve failed, see if the file has executable mode set.
   If so, and it isn't a directory, then execute its contents as
   a shell script.
+ 3. 搜索命令来执行　
    - 3.1 先执行　find_special_builtin()来搜索内建特殊命令
      ```C
      if (posixly_correct)
	      builtin = find_special_builtin (words->word->word);
      ```

    - 3.2 没有找到特殊内建命令后，执行　find_function()　搜索当前环境中函数
      ```C
      if (builtin == 0) 
	      func = find_function (words->word->word);
      ```
    - 3.3 还没有找到目标命令，执行　find_shell_builtin()　搜索当前shell内建命令
      ```C
      if (func == 0 && builtin == 0)
        builtin = find_shell_builtin (this_command_name);
      ```
+ 4. 根据搜索命令结果来选择分支执行
  + 4.1 如果搜索命令得到结果，执行　execute_builtin_or_function
    ```C
    execute_builtin_or_function (words, builtin, var, redirects,
			     fds_to_close, flags)
     WORD_LIST *words;
     sh_builtin_func_t *builtin;
     SHELL_VAR *var;
     REDIRECT *redirects;
     struct fd_bitmap *fds_to_close;
     int flags;
     ```
    execute_builtin_or_function中又有两个分支
    - 4.1.1 执行内建命令　execute_builtin
    ```C
    if (builtin)
      result = execute_builtin (builtin, words, flags, 0);
    ```
    - 4.1.2 执行函数　execute_function
    ```C
    else
      result = execute_function (var, words, flags, fds_to_close, 0, 0);
    ```
  + 4.2 如果搜索命令没有在上述三种命令中找到目标命令，则执行　execute_disk_command
  ```C
    execute_disk_command (words, redirects, command_line, pipe_in, pipe_out,
		      async, fds_to_close, cmdflags)
     WORD_LIST *words;
     REDIRECT *redirects;
     char *command_line;
     int pipe_in, pipe_out, async;
     struct fd_bitmap *fds_to_close;
     int cmdflags;
    ```
    - 4.2.1 在execute_disk_command中，先调用findcmd.c 中 search_for_command找到目标函数
      char *
      ```C
      search_for_command (pathname)
          const char *pathname;
      {
          ....
          hashed_file = phash_search (pathname);
          ....
          if (hashed_file)
            command = hashed_file;
          else if (absolute_program (pathname))
            command = savestring (pathname);
          else
          {
            ....
            command = find_user_command (pathname);
            ....
          }
          return (command);
      }
      ```
      - 命令首先在hash缓存中找
        - 如果命令包括'/'，则直接返回命令； 否则在hash中搜， 找到则返回命令索引， 没找到则

```C
      main()
        |
   reader_loop()       解析
        |--------------------------->read_command()-->parse_command()-->yyparse()-->yylex()-->read_token()-->read_token_word()
        |                                 |                               |                       |                 |
 execute_command() <-------------- current_command <--------------- global_command <------------token------------word
        |
execute_command_internal()
        |
 execute_xxxx_command()
        |
execute_simple_command()
        |
        |--->expand_words()-->expand_word_list_internal()
        |                                                                  子进程
        |------------------------------------->execute_disk_command()------------->shell_execve()-->execve()                
        |                  磁盘命令                       |                |                       |
        |函数及内置命令                              make_child()          |                       |FAILED
        |                                                |                |                       |
execute_builtin_or_function()                          fork()----------->pid                      ->execute_shell_script()
                                                                          |
                                                                          --------->return(result)
                                                                            父进程
```            