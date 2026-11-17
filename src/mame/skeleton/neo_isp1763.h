// license:BSD-3-Clause
// copyright-holders:SamDelorean contributors
// ISP1763A register and SRAM backplane for the NEO 2013 USB HC/DC firmware.
// Philips/ST ISP1763A datasheet Rev04. No peripheral, packet, or IRQ is
// synthesized without a linked device. This is a hardware core, not a claim
// of working USB host/printer or host-side PC enumeration.
#pragma once
#include "neo_usb_packets.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace neo {
class isp1763 {
public:
    using u8=std::uint8_t;
    using u16=std::uint16_t;
    using u32=std::uint32_t;
    static constexpr u32 chip_id=0x00176320U;
    static constexpr std::size_t host_ram_size=24*1024;
    static constexpr std::size_t device_ram_size=4*1024;
    isp1763(){reset();}
    void reset() {
        m_regs.fill(0);m_host_ram.fill(0);m_dc_ram.fill(0);
        m_ep={};for(std::size_t i=0;i<m_ep.size();++i){m_ep[i].base=u16(i*64);m_ep[i].capacity=64;}
        m_hc_addr=0;m_ep_select=0x20;m_dma_ep=0;
        m_host_irq=m_host_enable=0;m_device_irq=m_device_enable=0;
        m_unlocked=true;m_regs[0x80/2]=0x001e;
        m_regs[0xD4/2]=0x0040; // HC CLK_READY after reset
        m_regs[0xb2/2]=0xffff; // ATL PTDs skipped until armed
        m_regs[0xac/2]=0xffff; // INT
        m_regs[0xa6/2]=0xffff; // ISO
        m_attached=false;m_vbus=false;m_suspended=false;
        m_atl_done=0;m_device_address=0;m_requested_address=0;
        m_address_pending=false;m_root_reset_pending=false;m_ms=0;m_root_reset_until=0;
        m_port_link=false;m_port_connect_pending=false;m_port_connect_at=0;
    }
    // ISP1763 CPU bus is 16-bit; 32-bit registers are LOW word then HIGH word.
    // Firmware explicitly exchanges word halves when constructing a BE 68k dword.
    u16 read(u16 offset) {
        if(offset&1||offset>=0x100)return 0xffff;
        switch(offset){
        case 0x70:return u16(chip_id);
        case 0x72:return u16(chip_id>>16);
        case 0x00:return m_regs[0];
        case 0xB0:{const u16 done=m_atl_done;m_atl_done=0;return done;}
        case 0xA0: {
            // ISP1763A Rev04 PORTSC1 LS[1:0] (bits 11:10) report
            // actual DP/DM levels; the attached full-speed printer asserts
            // D+ (bit 11). These read-only bits are not software latch bits.
            // The 2013 OS3K Usb1763.c:2671 check reads PORTSC1, so report
            // silicon line state rather than hardcoding its expected word.
            const u16 raw=m_regs[0xa0/2];
            const u16 physical_fs_dp=0x0800U;
            return u16((raw&~u16(0x0c00)) |
                ((m_port_link && (raw&0x1000U))?physical_fs_dp:0U));
        }
        case 0xC4:return m_hc_addr;
        case 0xC6:return host_transfer(0,false);
        case 0xD4:return m_host_irq|0x40;
        case 0xD6:return m_host_enable;
        case 0x14:return u16(m_device_enable);
        case 0x16:return u16(m_device_enable>>16);
        case 0x18:return u16(m_device_irq);
        case 0x1a:return u16(m_device_irq>>16);
        case 0x20:return dc_transfer(0,false);
        case 0x1c:return m_ep[ep_index()].length;
        case 0x1e:return m_ep[ep_index()].ready?1:0;
        case 0x2c:return m_ep_select;
        case 0x58:return m_dma_ep;
        case 0xb8:return 0; // SW_RESET bits self-clear
        default:return known(offset)?m_regs[offset/2]:0xffff;
        }
    }
    void write(u16 offset,u16 data,u16 mask=0xffff){
        if(offset&1||offset>=0x100)return;
        auto merge=[&](u16 &v){v=u16((v&~mask)|(data&mask));};
        switch(offset){
        case 0x70:case 0x72:return; // read-only silicon ID
        // Reuse Dana's status-stage ownership: CPU readback may change now,
        // but the USB wire address changes only after EP0 status IN ACK.
        case 0x00:merge(m_regs[0]);m_requested_address=u8(m_regs[0]&0x7f);
            m_address_pending=true;return;
        case 0x8c:case 0x9c:case 0xba:case 0xb2:case 0xb4: {
            if(m_unlocked)merge(m_regs[offset/2]);
            return;
        }
        case 0xa0: {
            // Reuse Dana's asynchronous root-port reset/connection timing.
            // ISP1763 PORTSC uses one external downstream port, not Dana's two.
            const u16 old=m_regs[0xa0/2];
            // ECCS/PED/ECSC are silicon-owned. W1C clears only ECSC.
            // D+/D- LS[1:0] are read-only and sampled from the powered link.
            u16 next=u16((old&0x0007U)|((data&mask)&u16(0x13f8U)));
            if((data&mask)&0x0002U)next &=~u16(0x0002); // W1C
            const bool was_powered=bool(old&0x1000U);
            const bool now_powered=bool(next&0x1000U);
            if(!now_powered){
                next&=~u16(0x0007);
                m_port_link=false;m_port_connect_pending=false;
                m_root_reset_pending=false;
            } else if(!was_powered && m_attached){
                // ISP1763A PORTSC §10.1.6: do not report the peripheral
                // until host power is stable (specified maximum 20 ms).
                next&=~u16(0x0007);
                m_port_link=false;m_port_connect_pending=true;
                m_port_connect_at=m_ms+20;
            }
            if((data&mask)&0x0100U){ // physical port RESET
                next=u16((next|u16(0x0100))&~u16(0x0004));
                m_root_reset_pending=true;m_root_reset_until=m_ms+10;
            }
            m_regs[0xa0/2]=next;
            return;
        }
        case 0x7c:if((data&mask)==0xaa37)m_unlocked=true;return;
        case 0xb8:
            if(data&1){reset();return;}
            if(data&4){reset_host();return;}
            // Reset ATX affects physical transceiver, not guest-owned files.
            return;
        case 0xc4:merge(m_hc_addr);return;
        case 0xc6:host_transfer(data,true);return;
        case 0xd4:m_host_irq &=~(data&mask);return; // W1C
        case 0xd6:merge(m_host_enable);return;
        case 0x14:write32(m_device_enable,data,mask,false);return;
        case 0x16:write32(m_device_enable,data,mask,true);return;
        case 0x18:clear32(m_device_irq,data,mask,false);return;
        case 0x1a:clear32(m_device_irq,data,mask,true);return;
        case 0x2c:m_ep_select=u8(data&0x2f);m_ep[ep_index()].cursor=0;return;
        case 0x58:m_dma_ep=u8(data&0x0f);return;
        case 0x1c:merge(m_ep[ep_index()].length);return;
        case 0x20:dc_transfer(data,true);return;
        case 0x28: {
            u16 const v=data&mask;
            auto &ep=m_ep[ep_index()];
            if(v&0x10){ep.length=0;ep.cursor=0;ep.ready=false;}
            if(v&0x08)ep.ready=true; // VENDP validates IN bank
            if(v&0x01)ep.halted=true;
            m_regs[offset/2]=v&0x1f;return;
        }
        case 0x04: {
            u16 const size=data&0x7ff;
            if(size && size<=1024)resize_endpoint(ep_index(),size);
            m_regs[offset/2]=data;return;
        }
        default:
            if(known(offset) && m_unlocked)merge(m_regs[offset/2]);
            return;
        }
    }
    // External transport adapters call these only after real USB packets.
    // In their absence endpoints remain disconnected; no fabricated ACKs.
    bool inject_out(u8 endpoint,const u8 *bytes,u16 count) {
        if(endpoint>=m_ep.size()||!bytes)return false;
        auto &ep=m_ep[endpoint];
        if(ep.halted||ep.ready||!ep.capacity||count>ep.capacity)return false;
        if(ep.base+count>m_dc_ram.size())return false;
        std::copy_n(bytes,count,m_dc_ram.begin()+ep.base);
        ep.length=count;ep.cursor=0;ep.ready=true;
        m_device_irq |= endpoint==16?u32(1)<<8:u32(1)<<(10+endpoint);
        return true;
    }
    bool take_in(u8 endpoint,u8 *bytes,u16 &count) {
        if(endpoint>=m_ep.size()||!bytes)return false;
        auto &ep=m_ep[endpoint];
        if(!ep.ready||ep.halted||ep.length>count)return false;
        std::copy_n(m_dc_ram.begin()+ep.base,ep.length,bytes);
        count=ep.length;ep.ready=false;ep.length=0;ep.cursor=0;
        m_device_irq |= endpoint==16?u32(1)<<8:u32(1)<<(10+endpoint);
        return true;
    }
    bool irq() const {
        return bool((m_device_irq&m_device_enable) ||
            (m_host_irq&m_host_enable && (m_regs[0xb6/2]&1)));
    }
    // No port reports a connected function unless the user mounted a printer.
    void set_host_attached(bool attached) {
        if(attached==m_attached)return;
        m_attached=attached;
        auto &port=m_regs[0xa0/2];
        if(!attached){
            m_port_connect_pending=false;m_root_reset_pending=false;
            if(m_port_link){
                m_port_link=false;
                port=u16((port&~u16(0x0005))|u16(0x0002));
                m_host_irq|=0x0010;
            }
        } else if(port&0x1000U) {
            // A mounted MAME printer is a connected physical device, not
            // an unconditional USB ACK. Wait for the real powered bus.
            m_port_connect_pending=true;m_port_connect_at=m_ms+20;
        }
    }
    // This is the native 1-ms USB service tick; it must be driven by MAME's
    // existing USB timer and not by host wall-clock polling or a fake IRQ.
    void tick_1ms() {
        ++m_ms;
        if(m_port_connect_pending && m_ms>=m_port_connect_at){
            m_port_connect_pending=false;
            auto &port=m_regs[0xa0/2];
            if(m_attached && (port&0x1000U)){
                m_port_link=true;
                port|=u16(0x0003); // full-speed connect + ECSC latch
                m_host_irq|=0x0010;
            }
        }
        if(m_root_reset_pending && m_ms>=m_root_reset_until) {
            m_root_reset_pending=false;
            auto &port=m_regs[0xa0/2];
            port=u16(port&~u16(0x0100));
            if(m_port_link && (port&0x1000))port|=u16(0x0004);
            else port&=~u16(0x0004);
            port|=u16(0x0002); // connection/enable change
            m_host_irq|=0x0010;
        }
    }
    // Distinguish chip INT (HC/DC mask state) from unknown NEO PCB wiring.
    // No guessed MC68VZ328 IRQ2/IRQ3 mapping is added here.
    bool controller_interrupt() const {return irq();}
    bool host_attached() const {return m_attached;}
    bool vbus() const {return m_vbus;}
    u8 device_address() const {return m_device_address;}

    // Original ISP1763 ATL pTD format (8 x LE32, 32 bytes), NOT the
    // ISP1161A1 8-byte PTD. 16 entries at 0x0c00; payload >=0x1000.
    // Transport is an independent, explicitly mounted USB function.
    template<class Packet>
    unsigned service_host(Packet&& packet) {
        if(!m_attached || (m_regs[0xa0/2]&0x1005)!=0x1005 ||
           !(m_regs[0x8c/2]&1) ||
           !(m_regs[0x9c/2]&1) || !(m_regs[0xba/2]&1))return 0;
        unsigned completed=0;
        for(unsigned slot=0;slot<16;++slot) {
            const u16 bit=u16(1U<<slot);
            if(m_regs[0xb2/2]&bit)continue;
            const unsigned base=0x0c00+slot*32;
            u32 w0=load32(base),w1=load32(base+4),w2=load32(base+8),w3=load32(base+12);
            if(!(w0&1)||!(w3&0x80000000U))continue;
            const unsigned total=(w0>>3)&0x7fff;
            const unsigned maxpacket=(w0>>18)&0x7ff;
            const unsigned done=w3&0x7fff;
            const unsigned payload=0x400+(((w2>>8)&0xffff)*8);
            // USB status stages are zero-length packets: DW2's payload
            // pointer is ignored by silicon when NrBytesToTransfer==0.
            // Preserve strict bounds for all nonempty PTDs.
            if(!maxpacket || maxpacket>1024 || total>1024 || done>total ||
               (total && (payload<0x1000 || payload+total>m_host_ram.size())))continue;
            const unsigned code=(w1>>10)&3;
            if(code>2 || (w1&0x4000))continue; // no unqualified split
            const unsigned requested=std::min(maxpacket,total-done);
            neo_usb::transaction tx{
                u8((w1>>3)&0x7f),u8(((w0>>31)&1)|((w1&7)<<1)),
                code==2?neo_usb::pid::setup:(code==1?neo_usb::pid::in:neo_usb::pid::out),
                false,false,bool(w3&0x02000000),u16(maxpacket)
            };
            std::array<u8,1024> scratch{};
            if(tx.direction!=neo_usb::pid::in && requested)
                std::copy_n(m_host_ram.begin()+payload+done,requested,scratch.begin());
            neo_usb::reply const answer=packet(tx,
                tx.direction==neo_usb::pid::in?nullptr:scratch.data(),
                scratch.data(),u16(requested));
            if(answer.status==neo_usb::handshake::nak ||
               answer.status==neo_usb::handshake::timeout)continue;
            bool const halted=answer.status!=neo_usb::handshake::ack;
            if(!halted && answer.size>requested)continue;
            if(!halted && tx.direction==neo_usb::pid::in && answer.size)
                std::copy_n(scratch.begin(),answer.size,m_host_ram.begin()+payload+done);
            const unsigned newdone=done+(halted?0:answer.size);
            w3=(w3&~u32(0x00007fff))|newdone;
            if(!halted)w3^=0x02000000;
            if(halted || newdone==total ||
               (tx.direction==neo_usb::pid::in && answer.size<requested)){
                w3 &=~u32(0x80000000);
                if(halted)w3|=0x40000000;
                w0 &=~u32(1);
                m_regs[0xb2/2]|=bit;
                m_atl_done|=bit;
                m_host_irq|=0x0100;
                ++completed;
            }
            store32(base,w0);store32(base+12,w3);
        }
        return completed;
    }

    // Explicit packet-level upstream transport. No synthetic USB descriptors:
    // all setup requests go into the NEO firmware's EP0 SETUP FIFO.
    neo_usb::reply upstream(u8 op,u8 addr,u8 endpoint,u8 flags,
                           const u8 *out,u8 *in,u16 length) {
        using namespace neo_usb;
        (void)flags; // framing flag interpreted by mounted USB peer
        if(op==1) {m_vbus=true;m_suspended=false;m_device_irq|=0x80;return {handshake::ack,0};}
        if(op==2) {m_vbus=false;m_suspended=false;m_address_pending=false;
            return {handshake::ack,0};}
        if(op==0x10) {
            if(!in)return {handshake::pid_error,0};
            const std::array<u8,8> status{
                u8(m_vbus),u8(m_regs[0x0c/2]),m_device_address,
                u8(0),u8(0),u8(irq()),u8(m_suspended),u8(0)};
            std::copy(status.begin(),status.end(),in);
            return {handshake::ack,8};
        }
        if(!m_vbus || !(m_regs[0x0c/2]&1))return {handshake::timeout,0};
        if(op==3){m_device_address=m_requested_address=0;
            m_address_pending=false;m_regs[0]=0;
            for(auto &ep:m_ep){ep.length=ep.cursor=0;ep.ready=false;ep.halted=false;}
            m_device_irq|=1;return {handshake::ack,0};}
        if(op==4){m_regs[0x74/2]=u16((m_regs[0x74/2]+1)&0x7ff);
            return {handshake::ack,0};}
        if(op==5){m_suspended=true;return {handshake::ack,0};}
        if(op==6){m_suspended=false;return {handshake::ack,0};}
        if(op<7||op>9)return {handshake::pid_error,0};
        if(m_suspended || addr!=m_device_address)return {handshake::timeout,0};
        if(endpoint>7)return {handshake::stall,0};
        if(op==7){
            if(endpoint||length!=8||!out)return {handshake::stall,0};
            return inject_out(16,out,length)?reply{handshake::ack,length}:reply{handshake::nak,0};
        }
        if(op==8){
            if(!out)return {handshake::pid_error,0};
            return inject_out(u8(endpoint*2),out,length)?reply{handshake::ack,length}:reply{handshake::nak,0};
        }
        u16 received=length;
        if(!in || !take_in(u8(endpoint*2+1),in,received))
            return {handshake::nak,0};
        // USB SET_ADDRESS takes effect after the acknowledged ZLP status
        // packet, exactly as in Dana's qualified DC implementation.
        if(endpoint==0 && received==0 && m_address_pending) {
            m_device_address=m_requested_address;m_address_pending=false;
        }
        return {handshake::ack,received};
    }
    u16 read_host_word(unsigned address) const {
        if(address+1>=m_host_ram.size())return 0xffff;
        return u16(m_host_ram[address])|u16(m_host_ram[address+1])<<8;
    }
    u16 host_memory_address() const {return m_hc_addr;}
    std::size_t host_size() const {return m_host_ram.size();}
private:
    struct endpoint_state { u16 base=0,capacity=64,length=0,cursor=0;bool ready=false,halted=false; };
    std::array<u16,0x80>m_regs{};
    std::array<u8,host_ram_size>m_host_ram{};
    std::array<u8,device_ram_size>m_dc_ram{};
    std::array<endpoint_state,17>m_ep{};
    u16 m_hc_addr=0,m_host_irq=0,m_host_enable=0;
    u32 m_device_irq=0,m_device_enable=0;
    u8 m_ep_select=0x20,m_dma_ep=0;
    bool m_unlocked=true;
    bool m_attached=false,m_vbus=false,m_suspended=false;
    bool m_port_link=false,m_port_connect_pending=false;
    u32 m_port_connect_at=0;
    u8 m_device_address=0,m_requested_address=0;
    bool m_address_pending=false,m_root_reset_pending=false;
    u32 m_ms=0,m_root_reset_until=0;
    u16 m_atl_done=0;
    std::size_t ep_index() const {return (m_ep_select&0x20)?16:std::size_t(m_ep_select&0x0f);}
    bool resize_endpoint(std::size_t which,u16 size){
        std::size_t total=0;
        for(std::size_t i=0;i<m_ep.size();++i)total+=(i==which)?size:m_ep[i].capacity;
        if(total>device_ram_size)return false;
        std::array<u8,device_ram_size> next{};
        std::size_t base=0;
        for(std::size_t i=0;i<m_ep.size();++i){
            auto &e=m_ep[i];
            const u16 new_capacity=i==which?size:e.capacity;
            const u16 copy=u16(std::min<u16>(e.length,new_capacity));
            if(copy)std::copy_n(m_dc_ram.begin()+e.base,copy,next.begin()+base);
            e.base=u16(base);e.capacity=new_capacity;e.length=copy;
            e.cursor=std::min<u16>(e.cursor,copy);
            base+=new_capacity;
        }
        m_dc_ram=next;return true;
    }
    u32 load32(std::size_t offset) const {
        if(offset+3>=m_host_ram.size())return 0;
        return u32(m_host_ram[offset])|(u32(m_host_ram[offset+1])<<8)|
            (u32(m_host_ram[offset+2])<<16)|(u32(m_host_ram[offset+3])<<24);
    }
    void store32(std::size_t offset,u32 value) {
        if(offset+3>=m_host_ram.size())return;
        for(unsigned n=0;n<4;++n)m_host_ram[offset+n]=u8(value>>(n*8));
    }
    void reset_host(){
        m_host_ram.fill(0);m_hc_addr=m_host_irq=m_host_enable=m_atl_done=0;
        m_root_reset_pending=false;m_port_link=false;m_port_connect_pending=false;
        m_regs[0xa0/2]=0;
        m_regs[0xb2/2]=m_regs[0xac/2]=m_regs[0xa6/2]=0xffff;
        m_regs[0xd4/2]=0x40;
    }
    static void write32(u32 &v,u16 data,u16 mask,bool high) {
        const u32 shift=high?16:0;
        const u32 m=u32(mask)<<shift;
        v=(v&~m)|((u32(data&mask))<<shift);
    }
    static void clear32(u32 &v,u16 data,u16 mask,bool high) {
        v &= ~(u32(data&mask)<<(high?16:0));
    }
    u16 host_transfer(u16 value,bool wr){
        if(m_hc_addr>=m_host_ram.size()-1)return 0xffff;
        const std::size_t p=m_hc_addr;
        u16 result=u16(m_host_ram[p])|(u16(m_host_ram[p+1])<<8);
        if(wr){m_host_ram[p]=u8(value);m_host_ram[p+1]=u8(value>>8);}
        m_hc_addr=u16(m_hc_addr+2);
        return result;
    }
    u16 dc_transfer(u16 value,bool wr){
        auto &ep=m_ep[ep_index()];
        if(ep.halted||ep.cursor>=ep.capacity||ep.base+ep.cursor>=m_dc_ram.size())return 0xffff;
        const std::size_t p=ep.base+ep.cursor;
        if(p+1>=m_dc_ram.size())return 0xffff;
        const u16 result=u16(m_dc_ram[p])|(u16(m_dc_ram[p+1])<<8);
        if(wr){m_dc_ram[p]=u8(value);m_dc_ram[p+1]=u8(value>>8);ep.length=std::max<u16>(ep.length,u16(ep.cursor+2));}
        ep.cursor=u16(ep.cursor+2);
        return result;
    }
    static bool known(u16 offset){
        switch(offset){
            case 0x00:case 0x04:case 0x08:case 0x0c:case 0x10:
            case 0x24:case 0x28:case 0x2c:case 0x30:case 0x34:case 0x38:
            case 0x58:case 0x74:case 0x78:case 0x7c:case 0x80:
            case 0x88:case 0x8c:case 0x90:case 0x94:case 0x98:
            case 0x9c:case 0xa0:case 0xa2:case 0xa4:case 0xa6:
            case 0xb0:case 0xb2:case 0xb4:case 0xb6:case 0xba:
            case 0xbc:case 0xc0:case 0xc2:case 0xc8:case 0xca:
            case 0xcc:case 0xce:case 0xd0:case 0xd2:case 0xd8:
            case 0xda:case 0xdc:case 0xde:case 0xe0:case 0xe2:
                return true;
            default:return false;
        }
    }
};
} // namespace neo
