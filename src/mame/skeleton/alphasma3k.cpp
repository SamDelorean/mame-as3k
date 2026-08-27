// license:BSD-3-Clause
// copyright-holders:
/***************************************************************************************************

AlphaSmart 3000

TODO:
- SW traps asap after writing to uninitialized A0, going off the rails.
- Snippet at PC=400148 is supposed to transfer the rest of the IPL to main RAM,
  but it fails looping at PC=40016c cmpa.w A0, A1 (DragonBall-EZ core bug?)

====================================================================================================

PCB:
            ___________                                                                _____________
           /    ::    |_______________________________________________________________/             \
   _______/ ::  ::                              _________      _________              ________       \_____________
  ||RS232|      ::   28F008B3                  |________|     |________| PCB REV 2.8 |_______|  BATT        | USB |
  ||_____|      ::                 74HC574                                                    CR2032   XTAL |_____|
  |__                  HY62U8200                       HC30M                                          A120I0E     |
   __| SP3223ECA                                                                                                  |
 _|_                     DragonBall EZ                                                                 PDIUSBD11D |
|___|<-Power             MC68EZ328PU16V                                    :)                              _______|
  |                                                                   Rise and Shout                      |
  |                            XTAL                                   the AlphaSmart's                    |
  |__   __      _____                     SW        HC132A             Out!!!                             |
     | |__|    /     | HC132A     HC74A  on/off                                       ____________________|
     |________/      |_______________________________________________________________/

The later AlphaSmart models' firmware can be updated using the Manager application (Windows / Mac) and a USB cable.
Each update comprises two files, the "os" and the "smallos". Those files do not include the full Operating System image.
Two version updaters known:
- System 3 Neo Jul 11 2013, 09:44:53 + OS 3KNeo Small ROM, included with Manager 3.93
- System 3 Neo Jan 27 2010, 13:44:00 + OS 3KNeo Small ROM, included with Manager 3.60

    TODO:
    - Everything

***************************************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/mc68328.h"
#include "machine/ram.h"
#include "video/hd44780.h"
#include "emupal.h"
#include "screen.h"
#include "softlist_dev.h"

namespace
{

class alphasmart3k_state : public driver_device
{
public:
	alphasmart3k_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_lcdc0(*this, "ks0066_0")
		, m_lcdc1(*this, "ks0066_1")
		, m_ram(*this, RAM_TAG)
		, m_ipl(*this, "ipl")
	{
	}

	void alphasmart3k(machine_config &config);
	void alphasmart3k_diag(machine_config &config);

protected:
	required_device<mc68ez328_device> m_maincpu;
	optional_device<ks0066_device> m_lcdc0;
	optional_device<ks0066_device> m_lcdc1;
	required_device<ram_device> m_ram;
	required_region_ptr<u16> m_ipl;

	std::unique_ptr<bitmap_ind16> m_tmp_bitmap;
	u8 m_lcd_port_c_pending = 0;
	u8 m_lcd_port_c_applied = 0;

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	void main_map(address_map &map) ATTR_COLD;
	void lcd_port_c_w(unsigned bit, int state);
	void lcd_port_c_commit();
	u8 lcd_data_r();
	void lcd_db4_w(int state) { lcd_port_c_w(0, state); }
	void lcd_db5_w(int state) { lcd_port_c_w(1, state); }
	void lcd_db6_w(int state) { lcd_port_c_w(2, state); }
	void lcd_db7_w(int state) { lcd_port_c_w(3, state); }
	void lcd_rw_w(int state) { lcd_port_c_w(4, state); }
	void lcd_rs_w(int state) { lcd_port_c_w(5, state); }
	void lcd_e1_w(int state) { lcd_port_c_w(6, state); }
	void lcd_e2_w(int state) { lcd_port_c_w(7, state); }
	int lcd_db4_r() { return BIT(lcd_data_r(), 0); }
	int lcd_db5_r() { return BIT(lcd_data_r(), 1); }
	int lcd_db6_r() { return BIT(lcd_data_r(), 2); }
	int lcd_db7_r() { return BIT(lcd_data_r(), 3); }
};

void alphasmart3k_state::machine_start()
{
	address_space &space = m_maincpu->space(AS_PROGRAM);
	space.install_ram(0x00000000, m_ram->size() - 1, m_ram->pointer());

	save_item(NAME(m_lcd_port_c_pending));
	save_item(NAME(m_lcd_port_c_applied));
}

void alphasmart3k_state::machine_reset()
{
	// TODO: expects specific initialized 68k values, including registers?
	// dc.w 1111 is vector $2c
	memcpy(m_ram->pointer(), memregion("ipl")->base(), 0x100);

	m_lcd_port_c_pending = 0;
	m_lcd_port_c_applied = 0;
}

void alphasmart3k_state::lcd_port_c_w(unsigned bit, int state)
{
	m_lcd_port_c_pending = (m_lcd_port_c_pending & ~(1U << bit)) | (state << bit);

	// The MC68EZ328 core publishes selected Port C bits from 0 through 7, and
	// AlphaWord selects all eight here, so PC7 completes one PCDATA write.
	if (bit == 7)
		lcd_port_c_commit();
}

void alphasmart3k_state::lcd_port_c_commit()
{
	u8 const old_data = m_lcd_port_c_applied;
	u8 const new_data = m_lcd_port_c_pending;

	// Falling enables sample the old bus and controls.
	if (m_lcdc0 && BIT(old_data, 6) && !BIT(new_data, 6))
		m_lcdc0->e_w(0);
	if (m_lcdc1 && BIT(old_data, 7) && !BIT(new_data, 7))
		m_lcdc1->e_w(0);

	if (m_lcdc0)
	{
		m_lcdc0->db_w((new_data & 0x0f) << 4);
		m_lcdc0->rw_w(BIT(new_data, 4));
		m_lcdc0->rs_w(BIT(new_data, 5));
	}
	if (m_lcdc1)
	{
		m_lcdc1->db_w((new_data & 0x0f) << 4);
		m_lcdc1->rw_w(BIT(new_data, 4));
		m_lcdc1->rs_w(BIT(new_data, 5));
	}

	// Rising enables observe the new bus and controls.
	if (m_lcdc0 && !BIT(old_data, 6) && BIT(new_data, 6))
		m_lcdc0->e_w(1);
	if (m_lcdc1 && !BIT(old_data, 7) && BIT(new_data, 7))
		m_lcdc1->e_w(1);

	m_lcd_port_c_applied = new_data;
}

u8 alphasmart3k_state::lcd_data_r()
{
	if (!m_lcdc0 && !m_lcdc1)
		return 0x00;

	if (BIT(m_lcd_port_c_applied, 6) && !BIT(m_lcd_port_c_applied, 7))
		return m_lcdc0 ? m_lcdc0->db_r() >> 4 : 0x0f;
	if (!BIT(m_lcd_port_c_applied, 6) && BIT(m_lcd_port_c_applied, 7))
		return m_lcdc1 ? m_lcdc1->db_r() >> 4 : 0x0f;
	if (!BIT(m_lcd_port_c_applied, 6) && !BIT(m_lcd_port_c_applied, 7))
		return 0x0f;

	logerror("%s: both KS0066 enable lines asserted during read\n", machine().describe_context());
	return 0x0f;
}

void alphasmart3k_state::main_map(address_map &map)
{
//  map(0x0000'0000, 0x0003'ffff).ram().share("ram");
	map(0x0040'0000, 0x004f'ffff).rom().region("ipl", 0);
}

static INPUT_PORTS_START( alphasmart3k )
INPUT_PORTS_END



void alphasmart3k_state::alphasmart3k(machine_config &config)
{
	// Basic machine hardware
	MC68EZ328(config, m_maincpu, 16'000'000); // MC68EZ328PU16V, clock unverified
	m_maincpu->set_addrmap(AS_PROGRAM, &alphasmart3k_state::main_map);
	m_maincpu->out_port_c<0>().set(FUNC(alphasmart3k_state::lcd_db4_w));
	m_maincpu->out_port_c<1>().set(FUNC(alphasmart3k_state::lcd_db5_w));
	m_maincpu->out_port_c<2>().set(FUNC(alphasmart3k_state::lcd_db6_w));
	m_maincpu->out_port_c<3>().set(FUNC(alphasmart3k_state::lcd_db7_w));
	m_maincpu->out_port_c<4>().set(FUNC(alphasmart3k_state::lcd_rw_w));
	m_maincpu->out_port_c<5>().set(FUNC(alphasmart3k_state::lcd_rs_w));
	m_maincpu->out_port_c<6>().set(FUNC(alphasmart3k_state::lcd_e1_w));
	m_maincpu->out_port_c<7>().set(FUNC(alphasmart3k_state::lcd_e2_w));
	m_maincpu->in_port_c<0>().set(FUNC(alphasmart3k_state::lcd_db4_r));
	m_maincpu->in_port_c<1>().set(FUNC(alphasmart3k_state::lcd_db5_r));
	m_maincpu->in_port_c<2>().set(FUNC(alphasmart3k_state::lcd_db6_r));
	m_maincpu->in_port_c<3>().set(FUNC(alphasmart3k_state::lcd_db7_r));

	// Values from AlphaSmart 2000, not confirmed for AlphaSmart 3000
	// AlphaSmart 3000 uses a Data Image CM4040 LCD display, LCD is 40x4 according to ref
	KS0066(config, m_lcdc0, 270'000); // TODO: Possibly wrong device type, needs confirmation; clock not measured, datasheet typical clock used
	m_lcdc0->set_default_bios_tag("f05");
	m_lcdc0->set_lcd_size(4, 40);
	KS0066(config, m_lcdc1, 270'000); // TODO: Possibly wrong device type, needs confirmation; clock not measured, datasheet typical clock used
	m_lcdc1->set_default_bios_tag("f05");
	m_lcdc1->set_lcd_size(4, 40);

	RAM(config, RAM_TAG).set_default_size("256K");

	SOFTWARE_LIST(config, "kapps_list").set_original("alphasmart_kapps");
}

void alphasmart3k_state::alphasmart3k_diag(machine_config &config)
{
	alphasmart3k(config);
	config.device_remove("ks0066_0");
	config.device_remove("ks0066_1");
}

// ROM definitions

ROM_START( asma3k )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "28f008b3.u1", 0x000000, 0x100000, CRC(73a24834) SHA1(a47e6a6d286feaba4e671a6373632222113f9276) )
ROM_END

// Local AS3000 startup diagnostics built from archived AlphaSmart development artifacts.
// These are synthetic test fixtures, not physical ROM dumps.
ROM_START( asma3kdi )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "as3k_diag_invalid_applet.u1", 0x000000, 0x100000, CRC(6432156f) SHA1(bfe849333e32887241f73d0146776001df093ccc) )
ROM_END

ROM_START( asma3kdv )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "as3k_diag_valid_alphaword.u1", 0x000000, 0x100000, CRC(08d1bf77) SHA1(f1b545de5a96f755df7f1f0302a78d8ab68bd505) )
ROM_END

ROM_START( asma3kdvl )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "as3k_diag_valid_alphaword.u1", 0x000000, 0x100000, CRC(08d1bf77) SHA1(f1b545de5a96f755df7f1f0302a78d8ab68bd505) )
ROM_END

} // anonymous namespace

//    YEAR  NAME    PARENT COMPAT MACHINE       INPUT         CLASS               INIT        COMPANY             FULLNAME           FLAGS
COMP( 2000, asma3k, 0,     0,     alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdi, 0,    0,     alphasmart3k_diag, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (invalid applet)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdv, 0,    0,     alphasmart3k_diag, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (valid AlphaWord)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdvl, asma3kdv, 0, alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (valid AlphaWord + LCD bridge)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
