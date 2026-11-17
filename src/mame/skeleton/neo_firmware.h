// license:BSD-3-Clause
// Firmware package layout based on the segmented 2013 NEO updater documented
// in ioma8/neo-re/alpha-emu/src/firmware.rs. New C++ implementation.
// Input images are private, never embedded in the public repository.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace neo {
namespace firmware {
inline std::uint32_t be32(const std::uint8_t *p) {
    return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)
        |(std::uint32_t(p[2])<<8)|p[3];
}

struct segment {std::uint32_t address, size, offset, transfer_size;};

inline bool parse_system(const std::uint8_t *rom,std::size_t size,
                         std::vector<segment> &segments,std::string &err) {
    segments.clear();
    if(size<0x70 || std::memcmp(rom+6,"System 3 Neo      ",18)) {
        err="Unexpected NEO System package header";return false;
    }
    // Update stream includes its 0x70-byte header in the FIRST transfer.
    // Descriptor lengths are logical; transfer chunks are rounded to 1 KiB.
    // This is the firmware-confirmed NEO SmallROM segment mapper contract:
    // source [0x00000..0x5ffff] -> 0x00410000;
    // source [0x60000..0x603ff] -> 0x00406000;
    // source [0x60400..0x607ff] -> 0x005ffc00.
    std::size_t source=0;
    bool terminator=false;
    for(std::size_t pos=0x50;pos+8<=0x70;pos+=8) {
        const std::uint32_t addr=be32(rom+pos), len=be32(rom+pos+4);
        if(addr==0 && len==0){terminator=true;break;}
        if(!len || len>0xffffffffU-0x3ffU) {
            err="Invalid NEO segment length";return false;
        }
        const std::uint32_t transfer=(len+0x3ffU)&~0x3ffU;
        if(source+transfer>size || source+transfer<source) {
            err="NEO segmented transfer exceeds package";return false;
        }
        segments.push_back({addr,len,static_cast<std::uint32_t>(source),transfer});
        source+=transfer;
    }
    if(segments.empty() || !terminator) {
        err="Missing NEO segments or table terminator";return false;
    }
    return true;
}

inline bool install_small(const std::uint8_t *src,std::size_t size,
                          std::vector<std::uint8_t> &flash,std::string &err) {
    if(size<8 || size>flash.size()) {err="Invalid SmallROM size";return false;}
    const auto ssp=be32(src),pc=be32(src+4);
    if(ssp&1 || pc&1 || pc<0x00400000 || pc>=0x00400000+size) {
        err="Invalid NEO SmallROM reset vectors";return false;
    }
    for(std::size_t i=0;i<size;++i)flash[i]=src[i];
    return true;
}

inline bool install_system(const std::uint8_t *src,std::size_t size,
                           std::vector<std::uint8_t> &flash,std::string &err) {
    std::vector<segment> segments;
    if(!parse_system(src,size,segments,err))return false;
    // Validate all destinations BEFORE modifying Flash to avoid partial imports.
    for(const auto &s:segments) {
        if(s.address<0x00400000 || s.address>=0x00600000
           || s.transfer_size>flash.size()-(s.address-0x00400000)) {
            err="NEO segment outside modeled Flash aperture";return false;
        }
    }
    for(const auto &s:segments) {
        const auto destination=s.address-0x00400000;
        std::memcpy(flash.data()+destination,src+s.offset,s.transfer_size);
    }
    return true;
}
} // namespace firmware
} // namespace neo
