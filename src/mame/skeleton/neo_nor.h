// license:BSD-3-Clause
// copyright-holders:SamDelorean contributors
// AlphaSmart NEO external NOR: AMD-compatible ST M29W320EB candidate.
// Based on ST M29W320ET/EB Rev 5 (Jan 2007), tables 4, 21, 23-25.
// NEVER import Intel M28W640HCB commands from Dana as NEO commands.
//
// Scope: x16 word bus, read array, ID, CFI, word-program, block/chip erase,
// bypass program. NOR timing, DQ toggles, protection pins/blocks and full
// x8 program/erase remain unqualified. One byte-lane CFI query/exit was
// proved in NEO 2013 native traces (R8), not generalized to all byte writes.
// Command completion is immediate; raw storage is owned by MAME NVRAM.
#pragma once
#include <cstddef>
#include <cstdint>
#include <array>

namespace neo {
class nor_flash {
public:
    static constexpr std::size_t size_bytes=4*1024*1024;
    enum class profile : std::uint8_t { unknown_readonly=0, m29w320eb=1 };
    enum class state : std::uint8_t {
        array, unlock2, command, program, erase_unlock1,
        erase_unlock2, erase_command, autoselect, query,
        bypass, bypass_program, bypass_reset
    };
    void attach(std::uint8_t *bytes,std::size_t size) {
        m_bytes=bytes;m_size=size;reset();
    }
    void select_profile(profile p) { m_profile=p;reset(); }
    void reset() { m_state=state::array;m_rejected=0; }
    bool ready() const { return m_bytes && m_size==size_bytes; }
    state current_state() const { return m_state; }
    std::uint32_t rejected_commands() const { return m_rejected; }
    bool read_only() const {return m_profile!=profile::m29w320eb;}
    std::uint16_t read_word(std::size_t byte_offset) const {
        if(!ready() || (byte_offset&1) || byte_offset+1>=m_size)return 0xffff;
        if(m_state==state::autoselect) {
            const std::size_t word=byte_offset/2;
            if(word==0)return 0x0020; // ST manufacturer
            if(word==1)return 0x2257; // M29W320EB bottom boot
            // Protection status is not known for this actual board.
            return 0xffff;
        }
        if(m_state==state::query) {
            return cfi_word(byte_offset/2);
        }
        return (std::uint16_t(m_bytes[byte_offset])<<8)|m_bytes[byte_offset+1];
    }
    // For x16 organization only the complete 16-bit word accesses are
    // qualified. Reject byte lane commands rather than invent board wiring.
    bool write_word(std::size_t byte_offset,std::uint16_t value,
                    std::uint16_t mem_mask=0xffff) {
        if(!ready() || (byte_offset&1) || byte_offset+1>=m_size)return reject();
        // Primary NEO 2013 firmware evidence: at PC 0x000004D4, the RAM-
        // relocated geometry probe issues MOVE.B #0x98,$004000AA with a
        // high-byte bus strobe (mem_mask=0xff00). At 0x000004DC it reads
        // QRY byte 0 from $00400021. Allow only this evidenced byte-lane
        // CFI enter and Reset/Read Array exit for the *selected* ST candidate.
        // Byte-lane program, erase and unlock remain unqualified/rejected.
        if(mem_mask!=0xffff) {
            if(read_only())return reject();
            const std::uint8_t v=(mem_mask==0xff00)?
                std::uint8_t(value>>8):std::uint8_t(value);
            if(m_state==state::array && byte_offset==0x00aa &&
               mem_mask==0xff00 && v==0x98) {
                m_state=state::query;return true;
            }
            if(m_state==state::query &&
               (mem_mask==0xff00 || mem_mask==0x00ff) &&
               v==0xf0) {
                m_state=state::array;return true;
            }
            return reject();
        }
        if(read_only())return reject();
        const std::size_t wa=byte_offset/2;
        const std::size_t cmd_addr=wa&0x7ff; // A0–A10 for unlock decode
        const bool unlock1=cmd_addr==0x555,unlock2=cmd_addr==0x2aa;
        // In a pending program cycle 0x00F0 is data, not a reset command.
        if(value==0x00f0 && m_state!=state::program &&
           m_state!=state::bypass_program){m_state=state::array;return true;}
        switch(m_state) {
        case state::array:
            if(unlock1 && value==0x00aa){m_state=state::unlock2;return true;}
            if(cmd_addr==0x55 && value==0x0098){m_state=state::query;return true;}
            return reject();
        case state::unlock2:
            if(unlock2 && value==0x0055){m_state=state::command;return true;}
            m_state=state::array;return reject();
        case state::command:
            if(!unlock1){m_state=state::array;return reject();}
            switch(value) {
            case 0x0090:m_state=state::autoselect;return true;
            case 0x00a0:m_state=state::program;return true;
            case 0x0080:m_state=state::erase_unlock1;return true;
            case 0x0020:m_state=state::bypass;return true;
            default:m_state=state::array;return reject();
            }
        case state::program:{
            // Erased bits are 1. Programming may only clear bits (1 -> 0).
            const auto old=read_array(byte_offset);
            write_array(byte_offset,old & value);
            m_state=state::array;
            return ((value & ~old)==0);
        }
        case state::erase_unlock1:
            if(unlock1 && value==0x00aa){m_state=state::erase_unlock2;return true;}
            m_state=state::array;return reject();
        case state::erase_unlock2:
            if(unlock2 && value==0x0055){m_state=state::erase_command;return true;}
            m_state=state::array;return reject();
        case state::erase_command:
            m_state=state::array;
            if(unlock1 && value==0x0010){fill_range(0,m_size);return true;}
            if(value==0x0030) {
                std::size_t start=0,count=0;
                block_range(byte_offset,start,count);
                if(count){fill_range(start,count);return true;}
            }
            return reject();
        case state::autoselect:case state::query:
            // Only Read/Reset exits query/autoselect; already caught above.
            return reject();
        case state::bypass:
            if(value==0x00a0){m_state=state::bypass_program;return true;}
            if(value==0x0090){m_state=state::bypass_reset;return true;}
            return reject();
        case state::bypass_program:{
            const auto old=read_array(byte_offset);
            write_array(byte_offset,old & value);
            m_state=state::bypass;
            return ((value & ~old)==0);
        }
        case state::bypass_reset:
            if(value==0x0000){m_state=state::array;return true;}
            return reject();
        }
        return reject();
    }
    static void block_range(std::size_t byte_offset,std::size_t &start,std::size_t &length) {
        start=length=0;
        if(byte_offset>=size_bytes)return;
        // Eight 8-KiB parameter blocks at the BOTTOM, then 63 64-KiB blocks.
        const std::size_t block=byte_offset<0x10000?0x2000:0x10000;
        start=(byte_offset/block)*block;
        length=block;
    }
    static std::uint16_t cfi_word(std::size_t word_addr) {
        // Only fields independently documented in M29W320EB Rev 5.
        switch(word_addr) {
        case 0x10:return 0x51;case 0x11:return 0x52;case 0x12:return 0x59;
        case 0x13:return 0x02;case 0x14:return 0x00; // AMD command set
        case 0x15:return 0x40;case 0x16:return 0x00; // primary extended table
        case 0x17:case 0x18:case 0x19:case 0x1a:return 0;
        case 0x1b:return 0x27;case 0x1c:return 0x36;
        case 0x1d:return 0xb5;case 0x1e:return 0xc5;
        case 0x1f:return 0x04;case 0x20:return 0x00;
        case 0x21:return 0x0a;case 0x22:return 0x00;
        case 0x23:return 0x04;case 0x24:return 0x00;
        case 0x25:return 0x03;case 0x26:return 0x00;
        case 0x27:return 0x16; // 2^22 bytes
        case 0x28:return 0x02;case 0x29:return 0;
        case 0x2a:case 0x2b:return 0;
        case 0x2c:return 2; // 2 erase block regions
        case 0x2d:return 7;case 0x2e:return 0; // 8 parameter blocks
        case 0x2f:return 0x20;case 0x30:return 0;
        case 0x31:return 0x3e;case 0x32:return 0; // 63 main blocks
        case 0x33:return 0;case 0x34:return 1; // 0x100 x 256=64K
        default:return 0xffff; // no invented extended block/security data
        }
    }
private:
    std::uint16_t read_array(std::size_t o) const {
        return (std::uint16_t(m_bytes[o])<<8)|m_bytes[o+1];
    }
    void write_array(std::size_t o,std::uint16_t w) {
        m_bytes[o]=w>>8;m_bytes[o+1]=std::uint8_t(w);
    }
    void fill_range(std::size_t start,std::size_t count) {
        for(std::size_t i=0;i<count;++i)m_bytes[start+i]=0xff;
    }
    bool reject(){++m_rejected;return false;}
    std::uint8_t *m_bytes=nullptr;
    std::size_t m_size=0;
    profile m_profile=profile::unknown_readonly;
    state m_state=state::array;
    std::uint32_t m_rejected=0;
};
} // namespace neo
