// license:BSD-3-Clause
// copyright-holders:DANA-emu contributors, SamDelorean contributors
// MC68VZ328 common clock/timer/interrupt/RTC core, adapted from:
// SamDelorean/DANA-emu/src/mame/skeleton/alphasdana.cpp and alphasdana_h2.h
// The core carries no Dana ST keyboard, pen, SD, USB controller or display.
// MAME scheduling, CPU pins and NEO board GPIO remain in alphasneo.cpp.
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace neo {
class vz_core {
public:
    struct timer {
        std::uint16_t ctl=0, prer=0, cmp=0xffff, cr=0, cnt=0;
        std::uint16_t status=0, clear_seen=0;
    };
    void reset() {
        m_pllcr=0x24b3; m_pllfsr=0x0347; m_pctlr=0x1f;
        m_ivr=0; m_icr=0; m_ilcr=0x6533; m_imr=0x00ffffff; m_pending=0;
        m_clk32_high=false; m_timers={};
        m_rtc_time=0; m_rtc_alarm=0; m_rtc_day=0; m_rtc_day_alarm=0;
        m_rtc_control=0x0080; m_rtc_ien=0; m_rtc_isr=0;
        m_stopwatch=0; m_stopwatch_active=false; m_watchdog=0;
        m_rtc_prescaler=0; m_rti_prescaler=0;
        m_watchdog_trip=false;
    }
    vz_core(){reset();}
    // Direct port of Dana H2 VZ PLL equation; reset defaults are inherited
    // from Dana, not yet physically qualified on a NEO power cycle.
    static constexpr std::uint32_t xtal_hz=32768;
    std::uint32_t pll_hz() const {
        return 2U*(14U*(std::uint32_t(m_pllfsr&255)+1)+
            std::uint32_t((m_pllfsr>>8)&15)+1)*xtal_hz;
    }
    std::uint32_t dma_hz() const {
        return pll_hz()/((m_pllcr&0x80)?2:1)/((m_pllcr&0x20)?2:1);
    }
    std::uint32_t sysclk_hz() const {
        unsigned n=(m_pllcr>>8)&7;
        return dma_hz()/(n>=4?1U:(2U<<n));
    }
    std::uint32_t lcdclk_hz() const {
        unsigned n=(m_pllcr>>11)&7;
        return dma_hz()/(n>=4?1U:(2U<<n));
    }
    std::uint32_t timer_source_hz(unsigned n) const {
        if(n>1)return 0;
        switch(m_timers[n].ctl & 0xe) {
        case 2:return sysclk_hz();
        case 4:return sysclk_hz()/16;
        case 8:case 10:case 12:case 14:return xtal_hz;
        default:return 0; // STOP or unmodeled external TIN; never invent a clock
        }
    }
    const timer &timer_state(unsigned n) const {return m_timers[n];}
    timer &timer_state(unsigned n) {return m_timers[n];}
    std::uint32_t pending() const {return m_pending;}
    std::uint32_t active() const {return m_pending & ~m_imr;}
    std::uint32_t mask() const {return m_imr;}
    std::uint8_t irq_vector(unsigned level) const {return (m_ivr&0xf8) | (level&7);}
    bool irq_at_level(unsigned level) const {
        if(level<1||level>6)return false;
        std::uint32_t a=active();
        unsigned const uart2=(m_ilcr>>8)&7, spi1=(m_ilcr>>12)&7, t2=m_ilcr&7;
        bool const routed=(uart2==level && (a&0x1000))
            ||(spi1==level && (a&0x00200000))||(t2==level && (a&0x20));
        if(level==6)return routed || (a&0x00080082);
        if(level==4)return routed || (a&0x00400f5d);
        if(level==1)return routed || (a&0x00010000);
        if(level==2)return routed || (a&0x00020000);
        if(level==3)return routed || (a&0x00040000);
        if(level==5)return routed || (a&0x00100000);
        return routed;
    }
    void set_pending(std::uint32_t bits,bool raised) {
        if(raised)m_pending|=bits;else m_pending&=~bits;
    }
    void compare(unsigned n) {
        if(n>1)return;
        auto &t=m_timers[n];
        t.status|=1;
        t.cnt=(t.ctl&0x100)?t.cmp:0;
        if(t.ctl&0x10)m_pending|=(n?0x20U:0x02U);
    }
    // Port of Dana's virtual CLK32, RTI, TOD/alarm/watchdog logic.
    void seed_rtc(unsigned hour,unsigned minute,unsigned second) {
        m_rtc_time=((hour%24)<<24)|((minute%60)<<16)|(second%60);
    }
    void advance_clk32(std::uint32_t edges=1) {
        for(std::uint32_t i=0;i<edges;++i) {
            m_clk32_high=!m_clk32_high;
            if(!(m_rtc_control&0x80) && !(m_watchdog&1))continue;
            ++m_rtc_prescaler;
            ++m_rti_prescaler;
            for(unsigned n=0;n<8;++n)
                if(!(m_rti_prescaler&((8192U>>n)-1)))m_rtc_isr|=std::uint16_t(0x100U<<n);
            if(m_rtc_prescaler>=((m_rtc_control&0x20)?38400U:32768U)) {
                m_rtc_prescaler=0;rtc_second();
            }
            rtc_irq_update();
        }
    }
    bool watchdog_trip() const {return m_watchdog_trip;}
    void clear_watchdog_trip() {m_watchdog_trip=false;}
    std::optional<std::uint16_t> read_word(std::uint16_t a) {
        switch(a) {
        case 0xf200:return m_pllcr;
        case 0xf202:return (m_pllfsr&0x4fff)|(m_clk32_high?0x8000:0);
        case 0xf206:return m_pctlr; // physical byte register F207, low lane
        case 0xf300:return std::uint16_t(m_ivr)<<8; // physical byte F300
        case 0xf302:return m_icr;
        case 0xf304:return std::uint16_t(m_imr>>16);
        case 0xf306:return std::uint16_t(m_imr);
        case 0xf30c:return std::uint16_t((m_pending&~m_imr)>>16);
        case 0xf30e:return std::uint16_t(m_pending&~m_imr);
        case 0xf310:return std::uint16_t(m_pending>>16);
        case 0xf312:return std::uint16_t(m_pending);
        case 0xf314:return m_ilcr;
        case 0xfb00:return std::uint16_t(m_rtc_time>>16);
        case 0xfb02:return std::uint16_t(m_rtc_time);
        case 0xfb04:return std::uint16_t(m_rtc_alarm>>16);
        case 0xfb06:return std::uint16_t(m_rtc_alarm);
        case 0xfb0a:return m_watchdog;
        case 0xfb0c:return m_rtc_control;
        case 0xfb0e:return m_rtc_isr;
        case 0xfb10:return m_rtc_ien;
        case 0xfb12:return m_stopwatch;
        case 0xfb1a:return m_rtc_day;
        case 0xfb1c:return m_rtc_day_alarm;
        default:break;
        }
        unsigned n=0; std::uint16_t r=0;
        if(a>=0xf600 && a<=0xf60a){r=a-0xf600;n=0;}
        else if(a>=0xf610 && a<=0xf61a){r=a-0xf610;n=1;}
        else return std::nullopt;
        auto &t=m_timers[n];
        switch(r) {
        case 0:return t.ctl;
        case 2:return t.prer;
        case 4:return t.cmp;
        case 6:return t.cr;
        case 8:return t.cnt;
        case 10:t.clear_seen|=t.status;return t.status;
        default:return std::nullopt;
        }
    }
    bool write_word(std::uint16_t a,std::uint16_t value,std::uint16_t mask) {
        if(!mask)return true;
        switch(a) {
        case 0xf200: combine(m_pllcr,value,mask);m_pllcr&=0x3fbb;return true;
        case 0xf202:
            if(!(m_pllfsr&0x4000))combine(m_pllfsr,value,mask&0x0fff);
            if(value&mask&0x4000)m_pllfsr|=0x4000;
            return true;
        case 0xf206:if(mask&0x00ff)m_pctlr=value&0x9f;return true;
        case 0xf300:if(mask&0xff00)m_ivr=(value>>8)&0xf8;return true;
        case 0xf302:combine(m_icr,value,mask);return true;
        case 0xf304:case 0xf306:{
            unsigned shift=(a==0xf304)?16:0;
            std::uint32_t bits=std::uint32_t(mask)<<shift;
            m_imr=((m_imr&~bits)|(std::uint32_t(value&mask)<<shift))&0x00ffffff;return true;
        }
        case 0xf30c:case 0xf30e:{
            // INT1/2/3/6 edge acknowledge only (Dana VZ original).
            if(a==0xf30c) {
                std::uint16_t edges=((m_icr>>11)&1)|((m_icr>>9)&2)|
                    ((m_icr>>7)&4)|((m_icr>>5)&8);
                m_pending &= ~(std::uint32_t(value&mask&edges)<<16);
            }
            return true;
        }
        case 0xf314:combine(m_ilcr,value,mask);m_ilcr&=0x7777;return true;
        case 0xfb00:case 0xfb02:{
            unsigned shift=(a==0xfb00)?16:0;
            std::uint32_t bits=std::uint32_t(mask)<<shift;
            m_rtc_time=((m_rtc_time&~bits)|(std::uint32_t(value&mask)<<shift))&0x1f3f003fU;return true;
        }
        case 0xfb04:case 0xfb06:{
            unsigned shift=(a==0xfb04)?16:0;
            std::uint32_t bits=std::uint32_t(mask)<<shift;
            m_rtc_alarm=((m_rtc_alarm&~bits)|(std::uint32_t(value&mask)<<shift))&0x1f3f003fU;return true;
        }
        case 0xfb0a:{
            auto bits=mask&3;
            m_watchdog=(m_watchdog&~bits)|(value&bits);
            if(mask&0x300)m_watchdog&=~std::uint16_t(0x300);
            m_watchdog&=~(value&mask&0x80);
            rtc_irq_update();return true;
        }
        case 0xfb0c:combine(m_rtc_control,value,mask);m_rtc_control&=0xa0;rtc_irq_update();return true;
        case 0xfb0e:m_rtc_isr&=~(value&mask&0xff3f);rtc_irq_update();return true;
        case 0xfb10:combine(m_rtc_ien,value,mask);m_rtc_ien&=0xff3f;rtc_irq_update();return true;
        case 0xfb12:
            combine(m_stopwatch,value,mask);m_stopwatch&=63;
            m_stopwatch_active=m_stopwatch>0&&m_stopwatch<63;return true;
        case 0xfb1a:combine(m_rtc_day,value,mask);m_rtc_day&=511;return true;
        case 0xfb1c:combine(m_rtc_day_alarm,value,mask);m_rtc_day_alarm&=511;return true;
        default:break;
        }
        unsigned n=0; std::uint16_t r=0;
        if(a>=0xf600 && a<=0xf60a){r=a-0xf600;n=0;}
        else if(a>=0xf610 && a<=0xf61a){r=a-0xf610;n=1;}
        else return false;
        auto &t=m_timers[n];
        switch(r) {
        case 0:combine(t.ctl,value,mask);refresh_timer_irq(n);return true;
        case 2:combine(t.prer,value,mask);return true;
        case 4:combine(t.cmp,value,mask);return true;
        case 6:return true; // timer TCR read-only
        case 8:return true; // timer TCN read-only
        case 10:
            t.status&=~(t.clear_seen&mask);
            t.clear_seen&=~mask;refresh_timer_irq(n);return true;
        default:return false;
        }
    }
private:
    static void combine(std::uint16_t &slot,std::uint16_t data,std::uint16_t mask) {
        slot=(slot&~mask)|(data&mask);
    }
    void refresh_timer_irq(unsigned n) {
        auto &t=m_timers[n];
        set_pending(n?0x20U:0x02U,(t.status&1)&&(t.ctl&0x10));
    }
    void rtc_irq_update() {
        m_pending &= ~std::uint32_t(0x00400018);
        if((m_watchdog&0x80)&&(m_watchdog&2))m_pending|=8;
        if(m_rtc_isr&m_rtc_ien&0x003f)m_pending|=0x10;
        if(m_rtc_isr&m_rtc_ien&0xff00)m_pending|=0x00400000;
    }
    void rtc_second() {
        if(m_watchdog&1) {
            unsigned count=((m_watchdog>>8)&3)+1;
            if(count>=2) {
                m_watchdog&=~std::uint16_t(0x300);
                if(m_watchdog&2)m_watchdog|=0x80;
                else m_watchdog_trip=true;
            } else m_watchdog=(m_watchdog&~std::uint16_t(0x300))|std::uint16_t(count<<8);
        }
        if(!(m_rtc_control&0x80))return;
        unsigned sec=(m_rtc_time&63)+1,min=(m_rtc_time>>16)&63,hour=(m_rtc_time>>24)&31;
        m_rtc_isr|=0x10;
        if(sec>=60) {
            sec=0;++min;m_rtc_isr|=2;
            if(m_stopwatch_active) {
                m_stopwatch=(m_stopwatch-1)&63;
                if(m_stopwatch==63){m_stopwatch_active=false;m_rtc_isr|=1;}
            } else if(m_stopwatch==63 && (m_rtc_ien&1))m_rtc_isr|=1;
            if(min>=60) {
                min=0;++hour;m_rtc_isr|=0x20;
                if(hour>=24){hour=0;m_rtc_day=(m_rtc_day+1)&511;m_rtc_isr|=8;}
            }
        }
        m_rtc_time=(hour<<24)|(min<<16)|sec;
        if(m_rtc_time==m_rtc_alarm&&m_rtc_day==m_rtc_day_alarm)m_rtc_isr|=4;
    }
    std::array<timer,2> m_timers{};
    std::uint16_t m_pllcr=0x24b3,m_pllfsr=0x0347;
    std::uint8_t m_pctlr=0x1f,m_ivr=0;
    std::uint16_t m_icr=0,m_ilcr=0;
    std::uint32_t m_imr=0x00ffffff,m_pending=0;
    bool m_clk32_high=false;
    std::uint32_t m_rtc_time=0,m_rtc_alarm=0;
    std::uint16_t m_rtc_day=0,m_rtc_day_alarm=0;
    std::uint16_t m_rtc_control=0x0080,m_rtc_ien=0,m_rtc_isr=0;
    std::uint16_t m_stopwatch=0,m_watchdog=0;
    bool m_stopwatch_active=false,m_watchdog_trip=false;
    std::uint32_t m_rtc_prescaler=0,m_rti_prescaler=0;
};
} // namespace neo
