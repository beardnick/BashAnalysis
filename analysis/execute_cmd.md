# execute_cmd.c 函数内部调用结构

*** execute_com.c ***
+ execute_command (command)
    * 这个函数是由内部命令来调用，函数内部又调用**execute_command_internal (command, 0, NO_PIPE, NO_PIPE, bitmap)**
+ execute_command_internal() 内部调用接口　

```C
// execute_command_internal内部流程：
// 该函数是shell源码中执行命令的实际操作函数。他需要对作为操作参数传入的具体命令结构的value成员进行分析，并针对不同的value类型，
// 再调用具体类型的命令执行函数进行具体命令的解释执行工作。

// 具体来说：如果value是simple，则直接调用execute_simple_command函数进行执行，
// execute_simple_command再根据命令是内部命令或磁盘外部命令分别调用execute_builtin和execute_disk_command来执行,
// 其中，execute_disk_command在执行外部命令的时候调用make_child函数fork子进程执行外部命令。

// 如果value是其他类型，则调用对应类型的函数进行分支控制。
// 举例来说，如果是value是for_commmand,即这是一个for循环控制结构命令，则调用execute_for_command函数。
// 在该函数中，将枚举每一个操作域中的元素，对其再次调用execute_command函数进行分析。
// 即execute_for_command这一类函数实现的是一个命令的展开以及流程控制以及递归调用execute_command的功能。
```
会根据command的类型value执行函数,如果是simple类型，调用execute_simple_command()
  - execute_builtin() 执行内部命令
  - execute_disk_command() 执行外部命令
    - 调用jobs.c / nojobs.c make_child 来fork新进程

+ builtin = find_shell_builtin (this_command_name) 找到要执行的命令在哪里

