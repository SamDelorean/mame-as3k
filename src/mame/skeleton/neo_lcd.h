// license:BSD-3-Clause
// NEO graphical LCD port, behavior cross-checked against ioma8/neo-re
// alpha-emu/src/lcd.rs. Implemented afresh for C++/MAME; no Rust code copied.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace neo {
class lcd {
public:
    static constexpr unsigned width=264, height=64;
    static constexpr unsigned columns=160, pages=16, page_height=8;
    static constexpr unsigned right_origin=132;

    struct controller {
        std::array<std::array<std::uint8_t,columns>,pages> ram{};
        unsigned page=0, column=0, start_line=0;
        unsigned saved_column=0;
        bool rmw=false, on=true, reverse=false;
    };
    void reset() { m_c={}; }
    void command(unsigned n, std::uint8_t value) {
        if(n>1) return;
        auto &c=m_c[n];
        if(value==0xae) c.on=false;
        else if(value==0xaf) c.on=true;
        else if(value>=0xb0 && value<=0xbf) c.page=(value&15);
        else if(value<=0x0f) c.column=(c.column&0xf0)|value;
        else if(value>=0x10 && value<=0x1f) c.column=(c.column&0x0f)|((value&15)<<4);
        else if(value>=0x40 && value<=0x7f) c.start_line=value&0x3f;
        else if(value==0xa6) c.reverse=false;
        else if(value==0xa7) c.reverse=true;
        else if(value==0xe0) {c.saved_column=c.column; c.rmw=true;}
        else if(value==0xee && c.rmw) {c.column=c.saved_column;c.rmw=false;}
        if(c.column>=columns)c.column=columns-1;
    }
    void write(unsigned n, std::uint8_t value) {
        if(n>1)return;
        auto &c=m_c[n];
        c.ram[c.page][c.column]=value;
        if(c.column+1<columns)++c.column;
    }
    std::uint8_t status(unsigned n) const {
        return n>1?0xff:(m_c[n].on?0x00:0x20);
    }
    std::uint8_t read(unsigned n) {
        if(n>1)return 0xff;
        auto &c=m_c[n];
        auto value=c.ram[c.page][c.column];
        if(!c.rmw && c.column+1<columns)++c.column;
        return value;
    }
    bool pixel(unsigned x, unsigned y) const {
        if(x>=width || y>=height)return false;
        bool value=false;
        // Same overwrite priority as neo-re: the right controller starts at x=132.
        for(unsigned n=0;n<2;++n) {
            const unsigned base=n?right_origin:0;
            if(x<base || x>=base+columns)continue;
            const auto &c=m_c[n];
            const unsigned ram_y=(y+c.start_line)%128;
            const auto bits=c.ram[ram_y/8][x-base];
            value=c.on && (bool((bits>>(ram_y%8))&1) != c.reverse);
        }
        return value;
    }
    const controller &state(unsigned n) const {return m_c[n];}
private:
    std::array<controller,2> m_c{};
};
} // namespace neo
