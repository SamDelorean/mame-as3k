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
		, m_keys(*this, "COL%u", 0U)
		, m_battery_status(*this, "BATTERY")
	{
	}

	void alphasmart3k(machine_config &config);
	void alphasmart3k_diag(machine_config &config);
	DECLARE_INPUT_CHANGED_MEMBER(keyboard_key_changed);

protected:
	required_device<mc68ez328_device> m_maincpu;
	optional_device<ks0066_device> m_lcdc0;
	optional_device<ks0066_device> m_lcdc1;
	required_device<ram_device> m_ram;
	required_region_ptr<u16> m_ipl;
	required_ioport_array<16> m_keys;
	required_ioport m_battery_status;

	u8 m_lcd_port_c_pending = 0;
	u8 m_lcd_port_c_applied = 0;
	u8 m_keyboard_column_latch = 0xff;
	u8 m_keyboard_port_a_columns = 0x7f;

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	void alphasmart3k_palette(palette_device &palette) const;
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void main_map(address_map &map) ATTR_COLD;
	void keyboard_column_latch_w(u8 data);
	void keyboard_port_a_w(unsigned bit, int state);
	int keyboard_row_r(unsigned row);
	void refresh_keyboard_lines();
	int battery_good_r() { return BIT(m_battery_status->read(), 4); }
	template <unsigned Bit> void pa_column_w(int state) { keyboard_port_a_w(Bit, state); }
	template <unsigned Bit> int pd_row_r() { return keyboard_row_r(Bit); }
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
	save_item(NAME(m_keyboard_column_latch));
	save_item(NAME(m_keyboard_port_a_columns));
}

void alphasmart3k_state::machine_reset()
{
	// TODO: expects specific initialized 68k values, including registers?
	// dc.w 1111 is vector $2c
	memcpy(m_ram->pointer(), memregion("ipl")->base(), 0x100);

	m_lcd_port_c_pending = 0;
	m_lcd_port_c_applied = 0;
	// The hardware power-on state is unknown; default to no active-low columns.
	m_keyboard_column_latch = 0xff;
	m_keyboard_port_a_columns = 0x7f;
	refresh_keyboard_lines();
}

void alphasmart3k_state::keyboard_column_latch_w(u8 data)
{
	m_keyboard_column_latch = data;
	refresh_keyboard_lines();
}

void alphasmart3k_state::keyboard_port_a_w(unsigned bit, int state)
{
	m_keyboard_port_a_columns = (m_keyboard_port_a_columns & ~(1U << bit)) | (bool(state) << bit);
	refresh_keyboard_lines();
}

int alphasmart3k_state::keyboard_row_r(unsigned row)
{
	// Original KeyboardModule.c tables gpubKeyboardMatrix1to7 / 9to0:
	// logical column 0 -> latch bit 7; 1..7 -> PA bits 0..6;
	// 8 -> independent power switch (not scanned);
	// 9..15 -> latch bits 0..6. Both columns and rows are active-low.
	// A pressed key connects its selected low column to a PD row;
	// otherwise the AS3000 board's external row pull-up reads high.
	for (unsigned col = 0; col < 16; ++col)
	{
		if (col == 8)
			continue;
		bool const column_low = (col == 0)
				? !BIT(m_keyboard_column_latch, 7)
				: (col < 8)
					? !BIT(m_keyboard_port_a_columns, col - 1)
					: !BIT(m_keyboard_column_latch, col - 9);
		if (column_low && BIT(m_keys[col]->read(), row))
			return 0;
	}
	return 1;
}

void alphasmart3k_state::refresh_keyboard_lines()
{
	// Reflect the external physical row voltages into the MC68EZ328's
	// interrupt detector as well as its already wired PDDATA read callbacks.
	for (unsigned row = 0; row < 8; ++row)
		m_maincpu->port_d_in_w(keyboard_row_r(row), row);
}

INPUT_CHANGED_MEMBER(alphasmart3k_state::keyboard_key_changed)
{
	refresh_keyboard_lines();
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

void alphasmart3k_state::alphasmart3k_palette(palette_device &palette) const
{
	// Provisionally reuse the AlphaSmart palette; these are not measured AS3K colors.
	palette.set_pen_color(0, rgb_t(138, 146, 148));
	palette.set_pen_color(1, rgb_t(92, 83, 88));
}

u32 alphasmart3k_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);

	auto const draw_line = [&bitmap, &cliprect](ks0066_device &lcdc, int physical_row, int controller_line)
	{
		u8 const *const render_buffer = lcdc.render();

		for (int column = 0; column < 40; column++)
		{
			u8 const *const character = render_buffer + 16 * (controller_line * 40 + column);
			for (int glyph_y = 0; glyph_y < 8; glyph_y++)
			{
				for (int glyph_x = 0; glyph_x < 5; glyph_x++)
				{
					int const x = column * 6 + glyph_x;
					int const y = physical_row * 9 + glyph_y;
					if (cliprect.contains(x, y))
						bitmap.pix(y, x) = BIT(character[glyph_y], 4 - glyph_x);
				}
			}
		}
	};

	if (m_lcdc0)
	{
		draw_line(*m_lcdc0, 0, 0);
		draw_line(*m_lcdc0, 1, 1);
	}
	if (m_lcdc1)
	{
		draw_line(*m_lcdc1, 2, 0);
		draw_line(*m_lcdc1, 3, 1);
	}

	return 0;
}

void alphasmart3k_state::main_map(address_map &map)
{
//  map(0x0000'0000, 0x0003'ffff).ram().share("ram");
	map(0x0040'0000, 0x004f'ffff).rom().region("ipl", 0);
	// The 32 KiB chip-select window is known, but source only proves this byte address is decoded.
	map(0x0060'0000, 0x0060'0001).w(FUNC(alphasmart3k_state::keyboard_column_latch_w)).umask16(0xff00);
}

static INPUT_PORTS_START( alphasmart3k )
	PORT_START("BATTERY")
	PORT_CONFNAME(0x10, 0x10, "Battery status")
	PORT_CONFSETTING(0x10, DEF_STR(Normal))
	PORT_CONFSETTING(0x00, DEF_STR(Low))
	PORT_START("COL0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Equals") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(61) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Option") PORT_CODE(KEYCODE_LALT) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Option") PORT_CODE(KEYCODE_RALT) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Period") PORT_CODE(KEYCODE_STOP) PORT_CHAR(46) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Quote") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(39) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Semicolon") PORT_CODE(KEYCODE_COLON) PORT_CHAR(59) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Minus") PORT_CODE(KEYCODE_MINUS) PORT_CHAR(45) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Slash") PORT_CODE(KEYCODE_SLASH) PORT_CHAR(47) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Command") PORT_CODE(KEYCODE_LWIN) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Clear File") PORT_CODE(KEYCODE_DEL) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("End") PORT_CODE(KEYCODE_END) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Send") PORT_CODE(KEYCODE_F12) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Find") PORT_CODE(KEYCODE_F10) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL8")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backslash") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(92) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Print") PORT_CODE(KEYCODE_F11) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Spell Check") PORT_CODE(KEYCODE_F9) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL10")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL11")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL12")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backtick") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(96) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL13")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_START("COL14")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Intl Angle") PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL15")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N') PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::keyboard_key_changed), 0)
INPUT_PORTS_END



void alphasmart3k_state::alphasmart3k(machine_config &config)
{
	// Basic machine hardware
	MC68EZ328(config, m_maincpu, 16'000'000); // MC68EZ328PU16V, clock unverified
	m_maincpu->set_addrmap(AS_PROGRAM, &alphasmart3k_state::main_map);
	// Full physical X1-X15 / Y1-Y8 matrix; idle rows remain pulled high.
	m_maincpu->out_port_a<0>().set(FUNC(alphasmart3k_state::pa_column_w<0>));
	m_maincpu->out_port_a<1>().set(FUNC(alphasmart3k_state::pa_column_w<1>));
	m_maincpu->out_port_a<2>().set(FUNC(alphasmart3k_state::pa_column_w<2>));
	m_maincpu->out_port_a<3>().set(FUNC(alphasmart3k_state::pa_column_w<3>));
	m_maincpu->out_port_a<4>().set(FUNC(alphasmart3k_state::pa_column_w<4>));
	m_maincpu->out_port_a<5>().set(FUNC(alphasmart3k_state::pa_column_w<5>));
	m_maincpu->out_port_a<6>().set(FUNC(alphasmart3k_state::pa_column_w<6>));
	m_maincpu->in_port_d<0>().set(FUNC(alphasmart3k_state::pd_row_r<0>));
	m_maincpu->in_port_d<1>().set(FUNC(alphasmart3k_state::pd_row_r<1>));
	m_maincpu->in_port_d<2>().set(FUNC(alphasmart3k_state::pd_row_r<2>));
	m_maincpu->in_port_d<3>().set(FUNC(alphasmart3k_state::pd_row_r<3>));
	m_maincpu->in_port_d<4>().set(FUNC(alphasmart3k_state::pd_row_r<4>));
	m_maincpu->in_port_d<5>().set(FUNC(alphasmart3k_state::pd_row_r<5>));
	m_maincpu->in_port_d<6>().set(FUNC(alphasmart3k_state::pd_row_r<6>));
	m_maincpu->in_port_d<7>().set(FUNC(alphasmart3k_state::pd_row_r<7>));
	// AS3000 original PowerModule.c: PG4 high means batteries normal,
	// low triggers PowerIsBatteryLow. Model the real board input pin.
	m_maincpu->in_port_g<4>().set(FUNC(alphasmart3k_state::battery_good_r));
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
	m_lcdc0->set_lcd_size(2, 40);
	KS0066(config, m_lcdc1, 270'000); // TODO: Possibly wrong device type, needs confirmation; clock not measured, datasheet typical clock used
	m_lcdc1->set_default_bios_tag("f05");
	m_lcdc1->set_lcd_size(2, 40);

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_LCD));
	screen.set_refresh_hz(50);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500)); // Provisional value reused from alphasma.cpp
	screen.set_screen_update(FUNC(alphasmart3k_state::screen_update));
	screen.set_size(6 * 40, 9 * 4);
	screen.set_visarea_full();
	screen.set_palette("palette");

	PALETTE(config, "palette", FUNC(alphasmart3k_state::alphasmart3k_palette), 2);

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

// Private AS3000 System 3 (Nov 2005) Flash builds. The binaries are held
// outside the repository; neither variant replaces the physical 28f008b3.u1 dump.
// Small ROM, native OS segments and (Writer only) A000 are at manufacturer
// addresses. ROM presence does not establish completed firmware boot.
ROM_START( asma3ksb )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "os3k_min_boot.bin", 0x000000, 0x100000, CRC(ac3b2a2c) SHA1(c2936532f37514d0eaa74e2cf97bf96bb17e883d) )
ROM_END

ROM_START( asma3ksw )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "os3k_min_writer.bin", 0x000000, 0x100000, CRC(c79df93f) SHA1(02ae5d564b48476abf7067b71a7e0166e471bdfa) )
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
COMP( 2005, asma3ksb, 0,     0,     alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 System 3 (minimal boot Flash, private)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2005, asma3ksw, 0,     0,     alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 System 3 (AlphaWord Plus Flash, private)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdi, 0,    0,     alphasmart3k_diag, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (invalid applet)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdv, 0,    0,     alphasmart3k_diag, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (valid AlphaWord)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdvl, asma3kdv, 0, alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (valid AlphaWord + LCD bridge)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
