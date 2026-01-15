#include "fle.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include <set>
using namespace std;

// 程序的起始加载地址
constexpr uint64_t BASE_ADDR = 0x400000;
static inline uint64_t align_up(uint64_t x, uint64_t a) { return (x + a - 1) / a * a; }

// 记录每个输入节在最终内存中的映射信息
struct SectionMapping {
    uint64_t vaddr;                 // 该节的虚拟地址
    const FLESection* original_section; // 指向原节
    const FLEObject* parent_obj;    // 所属目标文件
    string name;                    // 节名
};

FLEObject FLE_ld(const vector<FLEObject>& objects, const LinkerOptions& options)
{
    // 0) 解析输入：普通对象、静态库、共享库依赖
    vector<const FLEObject*> base_inputs;
    vector<const FLEObject*> archives;
    vector<const FLEObject*> shared_deps;
    for (const auto& obj : objects) {
        if (obj.type == ".ar") archives.push_back(&obj);
        else if (obj.type == ".so") shared_deps.push_back(&obj);
        else base_inputs.push_back(&obj);
    }

    auto collect_unresolved = [&](const vector<const FLEObject*>& inputs,
                                  const map<string, uint64_t>& globals_now,
                                  const map<const FLEObject*, map<string, uint64_t>>& locals_now) {
        set<string> unresolved;
        for (auto* o : inputs) {
            for (const auto& [secname, sec] : o->sections) {
                for (const auto& r : sec.relocs) {
                    // 局部优先
                    auto lit = locals_now.find(o);
                    bool ok = false;
                    if (lit != locals_now.end()) {
                        ok = lit->second.count(r.symbol) > 0;
                    }
                    if (!ok) ok = globals_now.count(r.symbol) > 0;
                    if (!ok) unresolved.insert(r.symbol);
                }
            }
        }
        return unresolved;
    };

    // 初始全局/局部用于选择成员（粗略先取各对象自身的定义位置）
    map<string, uint64_t> globals_seed;
    map<const FLEObject*, map<string, uint64_t>> locals_seed;
    auto seed_sections = [&](const FLEObject* obj, uint64_t dummy_base) {
        for (const auto& sym : obj->symbols) {
            if (sym.section.empty()) continue;
            if (sym.type == SymbolType::LOCAL) locals_seed[obj][sym.name] = dummy_base + sym.offset;
            else globals_seed[sym.name] = dummy_base + sym.offset;
        }
    };
    for (auto* o : base_inputs) seed_sections(o, 0);

    vector<const FLEObject*> active = base_inputs;
    set<const FLEObject*> included_members;
    bool changed = true;
    while (changed) {
        changed = false;
        auto unresolved = collect_unresolved(active, globals_seed, locals_seed);
        if (unresolved.empty()) break;
        for (auto* ar : archives) {
            for (const auto& mem : ar->members) {
                if (included_members.count(&mem)) continue;
                bool useful = false;
                for (const auto& s : mem.symbols) {
                    if (!s.section.empty() && s.type != SymbolType::LOCAL && unresolved.count(s.name)) {
                        useful = true; break;
                    }
                }
                if (useful) {
                    // 选择该成员
                    active.push_back(&mem);
                    included_members.insert(&mem);
                    seed_sections(&mem, 0);
                    changed = true;
                }
            }
        }
    }

    // 1) 分类并合并节到多段：text/rodata/data/bss
    vector<uint8_t> text_data, rodata_data, data_data;
    uint64_t bss_size = 0;
    struct PendingMap { const FLEObject* obj; const FLESection* sec; string name; size_t size; string cat; size_t seg_offset; };
    vector<PendingMap> pending;
    auto cat_of = [](const string& n) {
        if (n.rfind(".text", 0) == 0) return string("text");
        if (n.rfind(".rodata", 0) == 0) return string("rodata");
        if (n.rfind(".data", 0) == 0) return string("data");
        if (n.rfind(".bss", 0) == 0) return string("bss");
        return string("data");
    };

    for (auto* objp : active) {
        const auto& obj = *objp;
        for (const auto& shdr : obj.shdrs) {
            auto it = obj.sections.find(shdr.name);
            if (it == obj.sections.end()) continue;
            const FLESection& section = it->second;
            string cat = cat_of(shdr.name);
            size_t seg_off = 0;
            if (cat == "text") { seg_off = text_data.size(); text_data.insert(text_data.end(), section.data.begin(), section.data.end()); }
            else if (cat == "rodata") { seg_off = rodata_data.size(); rodata_data.insert(rodata_data.end(), section.data.begin(), section.data.end()); }
            else if (cat == "data") { seg_off = data_data.size(); data_data.insert(data_data.end(), section.data.begin(), section.data.end()); }
            else { seg_off = bss_size; bss_size += shdr.size; }
            pending.push_back({ objp, &section, shdr.name, (size_t)shdr.size, cat, seg_off });
        }
    }

    // 收集共享库中已定义的全局符号名（用于强制走 PLT）
    set<string> so_defined_globals;
    for (auto* so : shared_deps) {
        for (const auto& sym : so->symbols) {
            if (!sym.section.empty() && (sym.type == SymbolType::GLOBAL || sym.type == SymbolType::WEAK)) {
                so_defined_globals.insert(sym.name);
            }
        }
    }

    // 预扫描：外部引用（用于 EXE 的 PLT/GOT）——按重定位类型收集
    set<string> extern_funcs, extern_datas;
    if (!options.shared) {
        for (const auto& pm : pending) {
            for (const auto& r : pm.sec->relocs) {
                if (r.symbol.size() && r.symbol[0] == '.') continue; // 跳过节名等伪符号
                if (r.type == RelocationType::R_X86_64_PC32) {
                    extern_funcs.insert(r.symbol);
                    if (so_defined_globals.count(r.symbol)) extern_funcs.insert(r.symbol);
                } else if (r.type == RelocationType::R_X86_64_GOTPCREL) {
                    extern_datas.insert(r.symbol);
                }
            }
        }
    }

    size_t plt_size = options.shared ? 0 : extern_funcs.size() * 6;

    // 预构建 GOT 索引，后续用于确定各段基址
    map<string, size_t> got_index; // 符号 -> 槽位
    if (!options.shared) {
        size_t idx = 0;
        for (const auto& s : extern_funcs) got_index.emplace(s, idx++);
        for (const auto& s : extern_datas) if (!got_index.count(s)) got_index.emplace(s, idx++);
    }
    size_t original_data_size = data_data.size();
    size_t got_bytes = options.shared ? 0 : got_index.size() * 8;

    // 段地址与权限（考虑 .plt 紧随 .text，.got 独立对齐，最终 bss 基址基于最终布局）
    uint64_t text_base = BASE_ADDR;
    uint64_t rodata_base = align_up(text_base + text_data.size() + plt_size, 4096);
    uint64_t data_base = align_up(rodata_base + rodata_data.size(), 4096);
    uint64_t got_base = align_up(data_base + original_data_size, 4096);
    uint64_t bss_base = align_up(got_base + got_bytes, 4096);

    // 构造映射（节 -> 虚拟地址），使用最终计算的 bss 基址
    vector<SectionMapping> mappings;
    for (const auto& pm : pending) {
        uint64_t base = 0;
        if (pm.cat == "text") base = text_base;
        else if (pm.cat == "rodata") base = rodata_base;
        else if (pm.cat == "data") base = data_base;
        else base = bss_base;
        mappings.push_back({ base + pm.seg_offset, pm.sec, pm.obj, pm.name });
    }

    // 2) 解析符号 -> 绝对地址（全局/弱 与 局部分离）
    struct GlobalSym { SymbolType type; uint64_t addr; };
    map<string, GlobalSym> globals;
    map<const FLEObject*, map<string, uint64_t>> locals;

    auto find_base = [&](const FLEObject* obj, const string& secname) -> uint64_t {
        for (const auto& mp : mappings) {
            if (mp.parent_obj == obj && mp.name == secname) return mp.vaddr;
        }
        return 0;
    };

    for (auto* objp : active) {
        const auto& obj = *objp;
        for (const auto& sym : obj.symbols) {
            if (sym.section.empty()) continue;
            uint64_t base = find_base(objp, sym.section);
            if (base == 0) continue;
            uint64_t addr = base + sym.offset;
            if (sym.type == SymbolType::LOCAL) {
                locals[objp][sym.name] = addr;
            } else {
                auto it = globals.find(sym.name);
                if (it == globals.end()) globals.emplace(sym.name, GlobalSym{ sym.type, addr });
                else {
                    if (it->second.type == SymbolType::GLOBAL && sym.type == SymbolType::GLOBAL)
                        throw runtime_error("Multiple definition of strong symbol: " + sym.name);
                    else if (it->second.type == SymbolType::WEAK && sym.type == SymbolType::GLOBAL)
                        it->second = GlobalSym{ SymbolType::GLOBAL, addr };
                }
            }
        }
    }

    // 3) 重定位：内部立即解析；EXE 的外部通过 PLT/GOT，SO 的外部记录为动态重定位
    // 先构建输出缓冲区骨架：.text | .plt | .rodata | .data（.got 追加到 .data 尾部）
    uint64_t plt_base = text_base + text_data.size();

    vector<uint8_t> output_data;
    output_data.reserve(text_data.size() + plt_size + rodata_data.size() + data_data.size());
    // text
    output_data.insert(output_data.end(), text_data.begin(), text_data.end());
    // plt（先占位，稍后回填）
    if (plt_size) output_data.insert(output_data.end(), plt_size, 0);
    // rodata
    output_data.insert(output_data.end(), rodata_data.begin(), rodata_data.end());
    // data
    output_data.insert(output_data.end(), data_data.begin(), data_data.end());
    // got（单独节，便于判定）
    vector<uint8_t> got_data;
    if (got_bytes) got_data.resize(got_bytes, 0);
    if (got_bytes) output_data.insert(output_data.end(), got_data.begin(), got_data.end());

    auto lookup_addr = [&](const FLEObject* obj, const string& name) -> uint64_t {
        auto lit = locals.find(obj);
        if (lit != locals.end()) {
            auto fit = lit->second.find(name);
            if (fit != lit->second.end()) return fit->second;
        }
        auto git = globals.find(name);
        if (git != globals.end()) return git->second.addr;
        throw runtime_error("Undefined symbol: " + name);
    };

    auto write32 = [&](size_t off, uint32_t v) {
        if (off + 4 > output_data.size()) return;
        output_data[off + 0] = static_cast<uint8_t>(v & 0xff);
        output_data[off + 1] = static_cast<uint8_t>((v >> 8) & 0xff);
        output_data[off + 2] = static_cast<uint8_t>((v >> 16) & 0xff);
        output_data[off + 3] = static_cast<uint8_t>((v >> 24) & 0xff);
    };
    auto write64 = [&](size_t off, uint64_t v) {
        if (off + 8 > output_data.size()) return;
        for (int i = 0; i < 8; ++i) output_data[off + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xff);
    };

    auto is_internal = [&](const FLEObject* obj, const string& name) -> bool {
        auto lit = locals.find(obj);
        if (lit != locals.end() && lit->second.count(name)) return true;
        return globals.count(name) > 0;
    };

    vector<Relocation> dyn_relocs_out;

    for (const auto& mp : mappings) {
        const FLEObject* obj = mp.parent_obj;
        for (const auto& reloc : mp.original_section->relocs) {
            int64_t A = reloc.addend;
            // 计算 P 与补丁偏移：按段拼接顺序
            uint64_t P = mp.vaddr + reloc.offset;
            size_t patch = 0;
            if (mp.vaddr >= text_base && mp.vaddr < rodata_base) patch = (mp.vaddr - text_base) + reloc.offset;
            else if (mp.vaddr >= rodata_base && mp.vaddr < data_base) patch = text_data.size() + plt_size + (mp.vaddr - rodata_base) + reloc.offset;
            else if (mp.vaddr >= data_base && mp.vaddr < bss_base) patch = text_data.size() + plt_size + rodata_data.size() + (mp.vaddr - data_base) + reloc.offset;
            else patch = SIZE_MAX; // bss 无文件内容
            bool internal = is_internal(obj, reloc.symbol);
            if (options.shared) {
                if (internal && !so_defined_globals.count(reloc.symbol)) {
                    uint64_t S = lookup_addr(obj, reloc.symbol);
                    switch (reloc.type) {
                        case RelocationType::R_X86_64_32:
                        case RelocationType::R_X86_64_32S: {
                            uint64_t V = S + A;
                            if (patch != SIZE_MAX) write32(patch, static_cast<uint32_t>(V));
                            break;
                        }
                        case RelocationType::R_X86_64_PC32: {
                            int64_t V = static_cast<int64_t>(S) + A - static_cast<int64_t>(P);
                            if (patch != SIZE_MAX) write32(patch, static_cast<uint32_t>(static_cast<int32_t>(V)));
                            break;
                        }
                        case RelocationType::R_X86_64_64: {
                            uint64_t V = S + A;
                            if (patch != SIZE_MAX) write64(patch, V);
                            break;
                        }
                        default: break;
                    }
                } else {
                    // 外部：留给加载器
                    dyn_relocs_out.push_back(Relocation{ reloc.type, (size_t)P, reloc.symbol, A });
                }
            } else {
                if (internal) {
                    uint64_t S = lookup_addr(obj, reloc.symbol);
                    switch (reloc.type) {
                        case RelocationType::R_X86_64_32:
                        case RelocationType::R_X86_64_32S: {
                            uint64_t V = S + A;
                            if (patch != SIZE_MAX) write32(patch, static_cast<uint32_t>(V));
                            break;
                        }
                        case RelocationType::R_X86_64_PC32: {
                            int64_t V = static_cast<int64_t>(S) + A - static_cast<int64_t>(P);
                            if (patch != SIZE_MAX) write32(patch, static_cast<uint32_t>(static_cast<int32_t>(V)));
                            break;
                        }
                        case RelocationType::R_X86_64_64: {
                            uint64_t V = S + A;
                            if (patch != SIZE_MAX) write64(patch, V);
                            break;
                        }
                        default: break;
                    }
                } else {
                    bool provided_by_shared = so_defined_globals.count(reloc.symbol) > 0;
                    if (!provided_by_shared) {
                        throw runtime_error("Undefined symbol: " + reloc.symbol);
                    }
                    // EXE 的外部：PC32 -> PLT；GOTPCREL -> GOT 槽
                    if (reloc.type == RelocationType::R_X86_64_PC32) {
                        auto it = got_index.find(reloc.symbol);
                        if (it == got_index.end()) continue;
                        size_t idx = it->second;
                        uint64_t stub_addr = plt_base + idx * 6;
                        int32_t V = (int32_t)((int64_t)stub_addr + A - (int64_t)P);
                        if (patch != SIZE_MAX) write32(patch, (uint32_t)V);
                    } else if (reloc.type == RelocationType::R_X86_64_GOTPCREL) {
                        auto it = got_index.find(reloc.symbol);
                        if (it == got_index.end()) continue;
                        size_t idx = it->second;
                        uint64_t got_slot = got_base + idx * 8;
                        int32_t V = (int32_t)((int64_t)got_slot + A - (int64_t)P);
                        if (patch != SIZE_MAX) write32(patch, (uint32_t)V);
                    } else {
                        throw runtime_error("Undefined symbol: " + reloc.symbol);
                    }
                }
            }
        }
    }

    // 4) 生成输出文件（多段 + 权限 + 对齐 + BSS）
    // 使用已重定位后的数据切片
    vector<uint8_t> text_patched;
    vector<uint8_t> plt_patched;
    vector<uint8_t> rodata_patched;
    vector<uint8_t> data_patched;
    text_patched.insert(text_patched.end(), output_data.begin(), output_data.begin() + text_data.size());
    if (plt_size) plt_patched.insert(plt_patched.end(), output_data.begin() + text_data.size(), output_data.begin() + text_data.size() + plt_size);
    rodata_patched.insert(rodata_patched.end(), output_data.begin() + text_data.size() + plt_size, output_data.begin() + text_data.size() + plt_size + rodata_data.size());
    data_patched.insert(data_patched.end(), output_data.begin() + text_data.size() + plt_size + rodata_data.size(), output_data.end());

    // 构建 PLT stub：写入 GOT 相对偏移
    if (plt_size) {
        for (const auto& kv : got_index) {
            size_t idx = kv.second;
            uint64_t stub_addr = plt_base + idx * 6;
            uint64_t got_slot = got_base + idx * 8;
            int32_t rel = (int32_t)((int64_t)got_slot - (int64_t)(stub_addr + 6));
            auto stub = generate_plt_stub(rel);
            size_t off = idx * 6;
            if (off + 6 <= plt_patched.size()) {
                for (int i = 0; i < 6; ++i) plt_patched[off + i] = stub[i];
            }
        }
    }

    FLEObject output;
    output.name = options.outputFile.empty() ? (options.shared ? "lib.so" : "a.out") : options.outputFile;
    output.type = options.shared ? ".so" : ".exe";

    // 将 .plt 直接并入 .text 的数据末尾，避免非页对齐映射
    vector<uint8_t> text_with_plt = text_patched;
    if (plt_size) text_with_plt.insert(text_with_plt.end(), plt_patched.begin(), plt_patched.end());
    FLESection s_text; s_text.name = ".text"; s_text.data = text_with_plt; s_text.has_symbols = false; output.sections[".text"] = s_text;
    FLESection s_rodata; s_rodata.name = ".rodata"; s_rodata.data = rodata_patched; s_rodata.has_symbols = false; output.sections[".rodata"] = s_rodata;
    FLESection s_data; s_data.name = ".data"; s_data.data = data_patched; s_data.has_symbols = false; output.sections[".data"] = s_data;
    if (got_bytes) { FLESection s_got; s_got.name = ".got"; s_got.data = got_data; s_got.has_symbols = false; output.sections[".got"] = s_got; }
    FLESection s_bss; s_bss.name = ".bss"; s_bss.data.assign(static_cast<size_t>(bss_size), 0); s_bss.has_symbols = false; output.sections[".bss"] = s_bss;

    ProgramHeader ph_text; ph_text.name = ".text"; ph_text.vaddr = text_base; ph_text.size = text_data.size() + plt_size; ph_text.flags = PHF::R | PHF::X;
    ProgramHeader ph_rodata; ph_rodata.name = ".rodata"; ph_rodata.vaddr = rodata_base; ph_rodata.size = rodata_data.size(); ph_rodata.flags = static_cast<uint32_t>(PHF::R);
    ProgramHeader ph_data; ph_data.name = ".data"; ph_data.vaddr = data_base; ph_data.size = data_data.size(); ph_data.flags = PHF::R | PHF::W;
    ProgramHeader ph_got; if (got_bytes) { ph_got.name = ".got"; ph_got.vaddr = got_base; ph_got.size = got_bytes; ph_got.flags = PHF::R | PHF::W; }
    ProgramHeader ph_bss; ph_bss.name = ".bss"; ph_bss.vaddr = bss_base; ph_bss.size = bss_size; ph_bss.flags = PHF::R | PHF::W;
    output.phdrs.push_back(ph_text);
    output.phdrs.push_back(ph_rodata);
    output.phdrs.push_back(ph_data);
    if (got_bytes) output.phdrs.push_back(ph_got);
    output.phdrs.push_back(ph_bss);

    // 导出符号（共享库）与动态重定位/依赖（可执行）
    if (options.shared) {
        // 导出已定义的全局/弱
        auto find_base = [&](const FLEObject* obj, const string& secname) -> uint64_t {
            for (const auto& mp : mappings) if (mp.parent_obj == obj && mp.name == secname) return mp.vaddr;
            return 0;
        };
        for (auto* objp : active) {
            for (const auto& sym : objp->symbols) {
                if (sym.section.empty()) continue;
                if (sym.type != SymbolType::GLOBAL && sym.type != SymbolType::WEAK) continue;
                uint64_t base = find_base(objp, sym.section);
                if (base == 0) continue;
                string cat = (sym.section.rfind(".text",0)==0?"text":(sym.section.rfind(".rodata",0)==0?"rodata":(sym.section.rfind(".data",0)==0?"data":"bss")));
                string out_sec = cat=="text"?".text":(cat=="rodata"?".rodata":(cat=="data"?".data":".bss"));
                uint64_t out_base = (cat=="text"?text_base:(cat=="rodata"?rodata_base:(cat=="data"?data_base:bss_base)));
                size_t off = (size_t)((base + sym.offset) - out_base);
                output.symbols.push_back(Symbol{ sym.type, out_sec, off, sym.size, sym.name });
            }
        }
        output.dyn_relocs = dyn_relocs_out;
        // 记录共享库依赖
        for (auto* so : shared_deps) if (!so->name.empty()) output.needed.push_back(so->name);
    } else {
        // 为每个 GOT 槽生成动态重定位（在加载时填地址）
        for (const auto& kv : got_index) {
            size_t idx = kv.second;
            uint64_t slot_vaddr = got_base + idx * 8;
            output.dyn_relocs.push_back(Relocation{ RelocationType::R_X86_64_64, (size_t)slot_vaddr, kv.first, 0 });
        }
        // 导出 EXE 中已定义的全局/弱符号，供 SO 解析使用
        auto find_base = [&](const FLEObject* obj, const string& secname) -> uint64_t {
            for (const auto& mp : mappings) if (mp.parent_obj == obj && mp.name == secname) return mp.vaddr;
            return 0;
        };
        for (auto* objp : active) {
            for (const auto& sym : objp->symbols) {
                if (sym.section.empty()) continue;
                if (sym.type != SymbolType::GLOBAL && sym.type != SymbolType::WEAK) continue;
                uint64_t base = find_base(objp, sym.section);
                if (base == 0) continue;
                string cat = (sym.section.rfind(".text",0)==0?"text":(sym.section.rfind(".rodata",0)==0?"rodata":(sym.section.rfind(".data",0)==0?"data":"bss")));
                string out_sec = cat=="text"?".text":(cat=="rodata"?".rodata":(cat=="data"?".data":".bss"));
                uint64_t out_base = (cat=="text"?text_base:(cat=="rodata"?rodata_base:(cat=="data"?data_base:bss_base)));
                size_t off = (size_t)((base + sym.offset) - out_base);
                output.symbols.push_back(Symbol{ sym.type, out_sec, off, sym.size, sym.name });
            }
        }
        // 记录依赖的共享库
        for (auto* so : shared_deps) if (!so->name.empty()) output.needed.push_back(so->name);
        // 入口点
        string entry = options.entryPoint.empty() ? string("_start") : options.entryPoint;
        auto ge = globals.find(entry);
        output.entry = (ge != globals.end()) ? ge->second.addr : 0;
    }

    return output;
}
