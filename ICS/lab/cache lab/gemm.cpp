/*
请注意，你的代码不能出现任何 int/short/char/float/double/auto 等局部变量/函数传参，我们仅允许使用 reg 定义的寄存器变量。
其中 reg 等价于一个 int。

你不能自己申请额外的内存，即不能使用 new/malloc，作为补偿我们传入了一段 buffer，大小为 BUFFER_SIZE = 64，你可以视情况使用。

我们的数组按照 A, B, C, buffer 的顺序在内存上连续紧密排列，且 &A = 0x30000000（这是模拟的设定，不是 A 的真实地址）

如果你需要以更自由的方式访问内存，你可以以相对 A 的方式访问，比如 A[100]，用 *(0x30000000) 是无法访问到的。

如果你有定义常量的需求（更严谨的说法是，你想定义的是汇编层面的立即数，不应该占用寄存器），请参考下面这种方式使用宏定义来完成。
*/

#include "cachelab.h"

#define m case0_m
#define n case0_n
#define p case0_p

// 我们用这个 2*2*2 的矩阵乘法来演示寄存器是怎么被分配的
void gemm_case0(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer)
{ // allocate 0 1 2 3
  for (reg i = 0; i < m; ++i)
  { // allocate 4
    for (reg j = 0; j < p; ++j)
    {               // allocate 5
      reg tmpc = 0; // allocate 6
      for (reg k = 0; k < n; ++k)
      {                          // allocate 7
        reg tmpa = A[i * n + k]; // allocate 8
        reg tmpb = B[k * p + j]; // allocate 9
        tmpc += tmpa * tmpb;
      } // free 9 8
      // free 7
      C[i * p + j] = tmpc;
    } // free 6
    // free 5
  }
  // free 4
} // free 3 2 1 0

#undef m
#undef n
#undef p

#define m case1_m
#define n case1_n
#define p case1_p

//16×16×16
void gemm_case1(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer)
{
  for(reg i=0; i < m; i += 4)
  {
    //蛇形遍历：偶数块 j 正向，奇数块 j 反向
    if ((i/4)%2==0)
    {
      for (reg j = 0; j < p; j += 4)
      {
        // 4x4 C 子块累加缓冲（16 个寄存器）
        reg cBlk[4][4]={};

        // 沿着 k 维（n 方向）做 4 的分块累加
        for (reg t = 0; t < n; t += 4)
        {
          // 展开 k ∈ [0,4) 的微内核：
          for (reg k = 0; k < 4; ++k)
          {
            // 先加载 A 的一列（i..i+3, t+k）
            reg a0 = A[(i + 0) * n + (t + k)];
            reg a1 = A[(i + 1) * n + (t + k)];
            reg a2 = A[(i + 2) * n + (t + k)];
            reg a3 = A[(i + 3) * n + (t + k)];

            // 再连续加载 B 的一行（t+k, j..j+3)
            reg b0 = B[(t + k) * p + (j + 0)];
            reg b1 = B[(t + k) * p + (j + 1)];
            reg b2 = B[(t + k) * p + (j + 2)];
            reg b3 = B[(t + k) * p + (j + 3)];

            // 累加到 C 的 4×4 子块
            cBlk[0][0] += a0 * b0; cBlk[0][1] += a0 * b1; cBlk[0][2] += a0 * b2; cBlk[0][3] += a0 * b3;
            cBlk[1][0] += a1 * b0; cBlk[1][1] += a1 * b1; cBlk[1][2] += a1 * b2; cBlk[1][3] += a1 * b3;
            cBlk[2][0] += a2 * b0; cBlk[2][1] += a2 * b1; cBlk[2][2] += a2 * b2; cBlk[2][3] += a2 * b3;
            cBlk[3][0] += a3 * b0; cBlk[3][1] += a3 * b1; cBlk[3][2] += a3 * b2; cBlk[3][3] += a3 * b3;
          }
        }

        // 回写 4x4 C 子块
        for (reg r = 0; r < 4; ++r)
        {
          for (reg c = 0; c < 4; ++c)
          {
            C[(i + r) * p + (j + c)] = cBlk[r][c];
          }
        }
      }
    }
    else
    {
      for (reg j = 12; j >=0; j -= 4)
      {
        // 4x4 C 子块累加缓冲（16 个寄存器）
        reg cBlk[4][4]={};

        // 沿着 k 维（n 方向）做 4 的分块累加
        for (reg t = 0; t < n; t += 4)
        {
          // 展开 k ∈ [0,4) 的微内核：
          for (reg k = 0; k < 4; ++k)
          {            
            // 先加载 A 的一列（i..i+3, t+k）
            reg a0 = A[(i + 0) * n + (t + k)];
            reg a1 = A[(i + 1) * n + (t + k)];
            reg a2 = A[(i + 2) * n + (t + k)];
            reg a3 = A[(i + 3) * n + (t + k)];

            // 再连续加载 B 的一行（t+k, j..j+3)
            reg b0 = B[(t + k) * p + (j + 0)];
            reg b1 = B[(t + k) * p + (j + 1)];
            reg b2 = B[(t + k) * p + (j + 2)];
            reg b3 = B[(t + k) * p + (j + 3)];
            
            // 累加到 C 的 4×4 子块
            cBlk[0][0] += a0 * b0; cBlk[0][1] += a0 * b1; cBlk[0][2] += a0 * b2; cBlk[0][3] += a0 * b3;
            cBlk[1][0] += a1 * b0; cBlk[1][1] += a1 * b1; cBlk[1][2] += a1 * b2; cBlk[1][3] += a1 * b3;
            cBlk[2][0] += a2 * b0; cBlk[2][1] += a2 * b1; cBlk[2][2] += a2 * b2; cBlk[2][3] += a2 * b3;
            cBlk[3][0] += a3 * b0; cBlk[3][1] += a3 * b1; cBlk[3][2] += a3 * b2; cBlk[3][3] += a3 * b3;
          }
        }

        // 回写 4x4 C 子块
        for (reg r = 0; r < 4; ++r)
        {
          for (reg c = 0; c < 4; ++c)
          {
            C[(i + r) * p + (j + c)] = cBlk[r][c];
          }
        }
      }
    }
  }
}

#undef m
#undef n
#undef p

#define m case2_m
#define n case2_n
#define p case2_p

//32×32×32
void gemm_case2(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer)
{
  for (reg i = 0; i < m; i += 4)
  {
    // 蛇形遍历：偶数块 j 正向，奇数块 j 反向
    if ((i / 4) % 2 == 0)
    {
      // 正向遍历当前 i-block 的所有 j 瓦片
      for (reg j = 0; j < p; j += 4)
      {
        // 在寄存器中维护一个 4x4 的 C 瓦片累加器，避免频繁写回内存
        reg cBlk[4][4] = {};

        // 沿 k 维（用 t 表示以 4 为步长的块），对当前 4x4 瓦片做累加
        for (reg t = 0; t < n; t += 4)
        {
          // 展开 k ∈ [0,4) 的微内核
          for (reg k = 0; k < 4; ++k)
          {
            reg a0 = A[(i + 0) * n + (t + k)];
            reg a1 = A[(i + 1) * n + (t + k)];
            reg a2 = A[(i + 2) * n + (t + k)];
            reg a3 = A[(i + 3) * n + (t + k)];

            reg b0 = B[(t + k) * p + (j + 0)];
            reg b1 = B[(t + k) * p + (j + 1)];
            reg b2 = B[(t + k) * p + (j + 2)];
            reg b3 = B[(t + k) * p + (j + 3)];

            // 把 a* b 累加到寄存器中的 cBlk
            cBlk[0][0] += a0 * b0; cBlk[0][1] += a0 * b1; cBlk[0][2] += a0 * b2; cBlk[0][3] += a0 * b3;
            cBlk[1][0] += a1 * b0; cBlk[1][1] += a1 * b1; cBlk[1][2] += a1 * b2; cBlk[1][3] += a1 * b3;
            cBlk[2][0] += a2 * b0; cBlk[2][1] += a2 * b1; cBlk[2][2] += a2 * b2; cBlk[2][3] += a2 * b3;
            cBlk[3][0] += a3 * b0; cBlk[3][1] += a3 * b1; cBlk[3][2] += a3 * b2; cBlk[3][3] += a3 * b3;
          }
        }

        // 微内核完成后，将寄存器中的 4x4 子块写回到 C 的相应位置
        for (reg r = 0; r < 4; ++r)
        {
          for (reg c = 0; c < 4; ++c)
          {
            C[(i + r) * p + (j + c)] = cBlk[r][c];
          }
        }
      }
    }
    else
    {
      // 反向遍历：从右向左处理 j 瓦片，保持与正向相同的微内核逻辑
      for (reg j = p - 4; j >= 0; j -= 4)
      {
        reg cBlk[4][4] = {};
        for (reg t = 0; t < n; t += 4)
        {
          for (reg k = 0; k < 4; ++k)
          {
            reg a0 = A[(i + 0) * n + (t + k)];
            reg a1 = A[(i + 1) * n + (t + k)];
            reg a2 = A[(i + 2) * n + (t + k)];
            reg a3 = A[(i + 3) * n + (t + k)];

            reg b0 = B[(t + k) * p + (j + 0)];
            reg b1 = B[(t + k) * p + (j + 1)];
            reg b2 = B[(t + k) * p + (j + 2)];
            reg b3 = B[(t + k) * p + (j + 3)];

            cBlk[0][0] += a0 * b0; cBlk[0][1] += a0 * b1; cBlk[0][2] += a0 * b2; cBlk[0][3] += a0 * b3;
            cBlk[1][0] += a1 * b0; cBlk[1][1] += a1 * b1; cBlk[1][2] += a1 * b2; cBlk[1][3] += a1 * b3;
            cBlk[2][0] += a2 * b0; cBlk[2][1] += a2 * b1; cBlk[2][2] += a2 * b2; cBlk[2][3] += a2 * b3;
            cBlk[3][0] += a3 * b0; cBlk[3][1] += a3 * b1; cBlk[3][2] += a3 * b2; cBlk[3][3] += a3 * b3;
          }
        }
        for (reg r = 0; r < 4; ++r)
        {
          for (reg c = 0; c < 4; ++c)
          {
            C[(i + r) * p + (j + c)] = cBlk[r][c];
          }
        }
      }
    }
  }
}

#undef m
#undef n
#undef p

#define m case3_m
#define n case3_n
#define p case3_p

//29×35×29
void gemm_case3(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer)
{
  // 主体：按 4×4 分块，并沿 k 维做 4 展开，处理 k 尾部 3
  for (reg i = 0; i + 3 < m; i += 4)
  {
    // 蛇形遍历以缓解直接映射冲突
    if ((i / 4) % 2 == 0)
    {
      // 4x4 全块
      for (reg j = 0; j + 3 < p; j += 4)
      {
        if (j + 4 == p - 1)
        {
          // 专门路径：最后一个 4×4 瓦片融合尾列
          reg cBlk[4][4] = {};
          reg cTail[4] = {};
          reg t;
          for (t = 0; t + 3 < n; t += 4)
          {
            for (reg k = 0; k < 4; ++k)
            {
              reg a0 = A[(i + 0) * n + (t + k)];
              reg a1 = A[(i + 1) * n + (t + k)];
              reg a2 = A[(i + 2) * n + (t + k)];
              reg a3 = A[(i + 3) * n + (t + k)];

              reg b;
              b = B[(t + k) * p + (j + 0)];
              cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
              b = B[(t + k) * p + (j + 1)];
              cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
              b = B[(t + k) * p + (j + 2)];
              cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
              b = B[(t + k) * p + (j + 3)];
              cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;

              b = B[(t + k) * p + (p - 1)];
              cTail[0] += a0 * b; cTail[1] += a1 * b; cTail[2] += a2 * b; cTail[3] += a3 * b;
            }
          }
          for (; t < n; ++t)
          {
            reg a0 = A[(i + 0) * n + t];
            reg a1 = A[(i + 1) * n + t];
            reg a2 = A[(i + 2) * n + t];
            reg a3 = A[(i + 3) * n + t];

            reg b;
            b = B[t * p + (j + 0)];
            cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
            b = B[t * p + (j + 1)];
            cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
            b = B[t * p + (j + 2)];
            cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
            b = B[t * p + (j + 3)];
            cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;

            b = B[t * p + (p - 1)];
            cTail[0] += a0 * b; cTail[1] += a1 * b; cTail[2] += a2 * b; cTail[3] += a3 * b;
          }

          for (reg r = 0; r < 4; ++r)
          {
            for (reg c = 0; c < 4; ++c)
            {
              C[(i + r) * p + (j + c)] = cBlk[r][c];
            }
          }
          C[(i + 0) * p + (p - 1)] = cTail[0];
          C[(i + 1) * p + (p - 1)] = cTail[1];
          C[(i + 2) * p + (p - 1)] = cTail[2];
          C[(i + 3) * p + (p - 1)] = cTail[3];
        }
        else
        {
          // 常规 4×4 瓦片
          reg cBlk[4][4] = {};
          reg t;
          for (t = 0; t + 3 < n; t += 4)
          {
            for (reg k = 0; k < 4; ++k)
            {
              reg a0 = A[(i + 0) * n + (t + k)];
              reg a1 = A[(i + 1) * n + (t + k)];
              reg a2 = A[(i + 2) * n + (t + k)];
              reg a3 = A[(i + 3) * n + (t + k)];

              reg b;
              b = B[(t + k) * p + (j + 0)];
              cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
              b = B[(t + k) * p + (j + 1)];
              cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
              b = B[(t + k) * p + (j + 2)];
              cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
              b = B[(t + k) * p + (j + 3)];
              cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;
            }
          }
          for (; t < n; ++t)
          {
            reg a0 = A[(i + 0) * n + t];
            reg a1 = A[(i + 1) * n + t];
            reg a2 = A[(i + 2) * n + t];
            reg a3 = A[(i + 3) * n + t];

            reg b;
            b = B[t * p + (j + 0)];
            cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
            b = B[t * p + (j + 1)];
            cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
            b = B[t * p + (j + 2)];
            cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
            b = B[t * p + (j + 3)];
            cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;
          }

          for (reg r = 0; r < 4; ++r)
          {
            for (reg c = 0; c < 4; ++c)
            {
              C[(i + r) * p + (j + c)] = cBlk[r][c];
            }
          }
        }
      }
    }
    else
    {
      //应从 p-5 开始
      reg jStart = p - 5;
      for (reg j = jStart; j >= 0; j -= 4)
      {
        if (j == p - 5)
        {
          reg cBlk[4][4] = {};
          reg cTail[4] = {};
          reg t;
          for (t = 0; t + 3 < n; t += 4)
          {
            for (reg k = 0; k < 4; ++k)
            {
              reg a0 = A[(i + 0) * n + (t + k)];
              reg a1 = A[(i + 1) * n + (t + k)];
              reg a2 = A[(i + 2) * n + (t + k)];
              reg a3 = A[(i + 3) * n + (t + k)];

              reg b;
              b = B[(t + k) * p + (j + 0)];
              cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
              b = B[(t + k) * p + (j + 1)];
              cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
              b = B[(t + k) * p + (j + 2)];
              cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
              b = B[(t + k) * p + (j + 3)];
              cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;

              b = B[(t + k) * p + (p - 1)];
              cTail[0] += a0 * b; cTail[1] += a1 * b; cTail[2] += a2 * b; cTail[3] += a3 * b;
            }
          }
          for (; t < n; ++t)
          {
            reg a0 = A[(i + 0) * n + t];
            reg a1 = A[(i + 1) * n + t];
            reg a2 = A[(i + 2) * n + t];
            reg a3 = A[(i + 3) * n + t];

            reg b;
            b = B[t * p + (j + 0)];
            cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
            b = B[t * p + (j + 1)];
            cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
            b = B[t * p + (j + 2)];
            cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
            b = B[t * p + (j + 3)];
            cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;

            b = B[t * p + (p - 1)];
            cTail[0] += a0 * b; cTail[1] += a1 * b; cTail[2] += a2 * b; cTail[3] += a3 * b;
          }
          for (reg r = 0; r < 4; ++r)
          {
            for (reg c = 0; c < 4; ++c)
            {
              C[(i + r) * p + (j + c)] = cBlk[r][c];
            }
          }
          C[(i + 0) * p + (p - 1)] = cTail[0];
          C[(i + 1) * p + (p - 1)] = cTail[1];
          C[(i + 2) * p + (p - 1)] = cTail[2];
          C[(i + 3) * p + (p - 1)] = cTail[3];
        }
        else
        {
          reg cBlk[4][4] = {};
          reg t;
          for (t = 0; t + 3 < n; t += 4)
          {
            for (reg k = 0; k < 4; ++k)
            {
              reg a0 = A[(i + 0) * n + (t + k)];
              reg a1 = A[(i + 1) * n + (t + k)];
              reg a2 = A[(i + 2) * n + (t + k)];
              reg a3 = A[(i + 3) * n + (t + k)];

              reg b;
              b = B[(t + k) * p + (j + 0)];
              cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
              b = B[(t + k) * p + (j + 1)];
              cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
              b = B[(t + k) * p + (j + 2)];
              cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
              b = B[(t + k) * p + (j + 3)];
              cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;
            }
          }
          for (; t < n; ++t)
          {
            reg a0 = A[(i + 0) * n + t];
            reg a1 = A[(i + 1) * n + t];
            reg a2 = A[(i + 2) * n + t];
            reg a3 = A[(i + 3) * n + t];

            reg b;
            b = B[t * p + (j + 0)];
            cBlk[0][0] += a0 * b; cBlk[1][0] += a1 * b; cBlk[2][0] += a2 * b; cBlk[3][0] += a3 * b;
            b = B[t * p + (j + 1)];
            cBlk[0][1] += a0 * b; cBlk[1][1] += a1 * b; cBlk[2][1] += a2 * b; cBlk[3][1] += a3 * b;
            b = B[t * p + (j + 2)];
            cBlk[0][2] += a0 * b; cBlk[1][2] += a1 * b; cBlk[2][2] += a2 * b; cBlk[3][2] += a3 * b;
            b = B[t * p + (j + 3)];
            cBlk[0][3] += a0 * b; cBlk[1][3] += a1 * b; cBlk[2][3] += a2 * b; cBlk[3][3] += a3 * b;
          }
          for (reg r = 0; r < 4; ++r)
          {
            for (reg c = 0; c < 4; ++c)
            {
              C[(i + r) * p + (j + c)] = cBlk[r][c];
            }
          }
        }
      }
    }
  }

  // 尾部 1 行（i 剩余 = 1），需要 1x4 与 1x1 内核
  reg ii = m - 1; // 只剩最后一行
  reg sumTail = 0;
  // 处理列 24、20、16、12（同时融合最右列）
  {
    reg jH = 24; reg jL = 20;reg jK=16; reg jG=12;
    reg c16[16] = {};
    reg t; // t 为 k 维分块起点（步长 4）
    for (t = 0; t + 3 < n; t += 4)
    {
      for (reg k = 0; k < 4; ++k)
      {
        reg a0 = A[ii * n + (t + k)];
        reg b;
        b = B[(t + k) * p + (jH + 0)]; c16[0] += a0 * b;
        b = B[(t + k) * p + (jH + 1)]; c16[1] += a0 * b;
        b = B[(t + k) * p + (jH + 2)]; c16[2] += a0 * b;
        b = B[(t + k) * p + (jH + 3)]; c16[3] += a0 * b;
        b = B[(t + k) * p + (jL + 0)]; c16[4] += a0 * b;
        b = B[(t + k) * p + (jL + 1)]; c16[5] += a0 * b;
        b = B[(t + k) * p + (jL + 2)]; c16[6] += a0 * b;
        b = B[(t + k) * p + (jL + 3)]; c16[7] += a0 * b;
        b = B[(t + k) * p + (jK + 0)]; c16[8] += a0 * b;
        b = B[(t + k) * p + (jK + 1)]; c16[9] += a0 * b;
        b = B[(t + k) * p + (jK + 2)]; c16[10] += a0 * b;
        b = B[(t + k) * p + (jK + 3)]; c16[11] += a0 * b;
        b = B[( t + k) * p + (jG + 0)]; c16[12] += a0 * b;
        b = B[( t + k) * p + (jG + 1)]; c16[13] += a0 * b;
        b = B[( t + k) * p + (jG + 2)]; c16[14] += a0 * b;
        b = B[( t + k) * p + (jG + 3)]; c16[15] += a0 * b;
        b = B[(t + k) * p + (p - 1)]; sumTail += a0 * b;
      }
    }
    for (; t < n; ++t)
    {
      reg a0 = A[ii * n + t];
      reg b;
      b = B[t * p + (jH + 0)]; c16[0] += a0 * b;
      b = B[t * p + (jH + 1)]; c16[1] += a0 * b;
      b = B[t * p + (jH + 2)]; c16[2] += a0 * b;
      b = B[t * p + (jH + 3)]; c16[3] += a0 * b;
      b = B[t * p + (jL + 0)]; c16[4] += a0 * b;
      b = B[t * p + (jL + 1)]; c16[5] += a0 * b;
      b = B[t * p + (jL + 2)]; c16[6] += a0 * b;
      b = B[t * p + (jL + 3)]; c16[7] += a0 * b;
      b = B[t * p + (jK + 0)]; c16[8] += a0 * b;
      b = B[t * p + (jK + 1)]; c16[9] += a0 * b;
      b = B[t * p + (jK + 2)]; c16[10] += a0 * b;
      b = B[t * p + (jK + 3)]; c16[11] += a0 * b;
      b = B[t * p + (jG + 0)]; c16[12] += a0 * b;
      b = B[t * p + (jG + 1)]; c16[13] += a0 * b;
      b = B[t * p + (jG + 2)]; c16[14] += a0 * b;
      b = B[t * p + (jG + 3)]; c16[15] += a0 * b;
      b = B[t * p + (p - 1)]; sumTail += a0 * b;
    }
    C[ii * p + (jH + 0)] = c16[0]; C[ii * p + (jH + 1)] = c16[1];
    C[ii * p + (jH + 2)] = c16[2]; C[ii * p + (jH + 3)] = c16[3];
    C[ii * p + (jL + 0)] = c16[4]; C[ii * p + (jL + 1)] = c16[5];
    C[ii * p + (jL + 2)] = c16[6]; C[ii * p + (jL + 3)] = c16[7];
    C[ii * p + (jK + 0)] = c16[8]; C[ii * p + (jK + 1)] = c16[9];
    C[ii * p + (jK + 2)] = c16[10]; C[ii * p + (jK + 3)] = c16[11];
    C[ii * p + (jG + 0)] = c16[12]; C[ii * p + (jG + 1)] = c16[13];
    C[ii * p + (jG + 2)] = c16[14]; C[ii * p + (jG + 3)] = c16[15];
  }
  
  // 处理列 8 与 4（并同时融合 j=0 瓦片）
  {
    reg jH = 8; reg jL = 4;
    reg c8[8] = {};
    reg c0[4] = {};
    reg t; // t 为 k 维分块起点（步长 4）
    for (t = 0; t + 3 < n; t += 4)
    {
      for (reg k = 0; k < 4; ++k)
      {
        reg a0 = A[ii * n + (t + k)]; reg b;
        b = B[(t + k) * p + (jH + 0)]; c8[0] += a0 * b;
        b = B[(t + k) * p + (jH + 1)]; c8[1] += a0 * b;
        b = B[(t + k) * p + (jH + 2)]; c8[2] += a0 * b;
        b = B[(t + k) * p + (jH + 3)]; c8[3] += a0 * b;
        b = B[(t + k) * p + (jL + 0)]; c8[4] += a0 * b;
        b = B[(t + k) * p + (jL + 1)]; c8[5] += a0 * b;
        b = B[(t + k) * p + (jL + 2)]; c8[6] += a0 * b;
        b = B[(t + k) * p + (jL + 3)]; c8[7] += a0 * b;
        // 同步累加最左侧 j=0 的 4×4 瓦片
        b = B[(t + k) * p + (0 + 0)]; c0[0] += a0 * b;
        b = B[(t + k) * p + (0 + 1)]; c0[1] += a0 * b;
        b = B[(t + k) * p + (0 + 2)]; c0[2] += a0 * b;
        b = B[(t + k) * p + (0 + 3)]; c0[3] += a0 * b;
      }
    }
    for (; t < n; ++t)
    {
      reg a0 = A[ii * n + t]; reg b;
      b = B[t * p + (jH + 0)]; c8[0] += a0 * b;
      b = B[t * p + (jH + 1)]; c8[1] += a0 * b;
      b = B[t * p + (jH + 2)]; c8[2] += a0 * b;
      b = B[t * p + (jH + 3)]; c8[3] += a0 * b;
      b = B[t * p + (jL + 0)]; c8[4] += a0 * b;
      b = B[t * p + (jL + 1)]; c8[5] += a0 * b;
      b = B[t * p + (jL + 2)]; c8[6] += a0 * b;
      b = B[t * p + (jL + 3)]; c8[7] += a0 * b;
      b = B[t * p + (0 + 0)]; c0[0] += a0 * b; // 同步累加 j=0 瓦片
      b = B[t * p + (0 + 1)]; c0[1] += a0 * b;
      b = B[t * p + (0 + 2)]; c0[2] += a0 * b;
      b = B[t * p + (0 + 3)]; c0[3] += a0 * b;
    }
    C[ii * p + (jH + 0)] = c8[0]; C[ii * p + (jH + 1)] = c8[1];
    C[ii * p + (jH + 2)] = c8[2]; C[ii * p + (jH + 3)] = c8[3];
    C[ii * p + (jL + 0)] = c8[4]; C[ii * p + (jL + 1)] = c8[5];
    C[ii * p + (jL + 2)] = c8[6]; C[ii * p + (jL + 3)] = c8[7];
    // 回写融合的 j=0 瓦片
    C[ii * p + (0 + 0)] = c0[0]; C[ii * p + (0 + 1)] = c0[1];
    C[ii * p + (0 + 2)] = c0[2]; C[ii * p + (0 + 3)] = c0[3];
  }
  C[ii * p + (p - 1)] = sumTail;
}

#undef m
#undef n
#undef p