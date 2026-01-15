#include "fle.hpp"
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

// 判断 str 是否以 prefix 开头
static bool starts_with(const string& str, const string& prefix)
{
    if (str.size() < prefix.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

// 计算 nm 的类型码
static char get_symbol_type_code(const Symbol& sym)
{
    if (sym.type == SymbolType::WEAK) {
        return starts_with(sym.section, ".text") ? 'W' : 'V';
    }

    char type_code = '?';
    if (starts_with(sym.section, ".text")) type_code = 't';
    else if (starts_with(sym.section, ".data")) type_code = 'd';
    else if (starts_with(sym.section, ".bss")) type_code = 'b';
    else if (starts_with(sym.section, ".rodata")) type_code = 'r';
    else type_code = 'd';

    if (sym.type == SymbolType::GLOBAL) type_code = toupper(type_code);
    return type_code;
}

void FLE_nm(const FLEObject& obj)
{
    for (const auto& sym : obj.symbols) {
        // 跳过未定义符号
        if (sym.section.empty()) continue;

        char type_code = get_symbol_type_code(sym);
        // 输出: 16 位地址 类型 名字
        cout << setw(16) << setfill('0') << hex << sym.offset
             << " " << type_code << " " << sym.name << "\n";
    }
}
