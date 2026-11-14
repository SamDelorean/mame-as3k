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
#include "fileio.h"
#include "emuopts.h"
#include "cpu/m68000/m68000.h"
#include "machine/mc68328.h"
#include "machine/ram.h"
#include "machine/nvram.h"
#include "machine/intelfsh.h"
#include "bus/generic/slot.h"
#include "bus/generic/carts.h"
#include "bus/rs232/rs232.h"
#include "bus/rs232/loopback.h"
#include "bus/rs232/null_modem.h"
#include "bus/rs232/pty.h"
#include "bus/rs232/printer.h"
#include "video/hd44780.h"
#include "emupal.h"
#include "screen.h"
#include "softlist_dev.h"
#include <vector>
#include <deque>
#include <cerrno>
#include <cstdlib>
#if defined(__linux__)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{

// Keep the reduced AS3K build genuinely minimal: avoid the many CPUs
// and terminal boards pulled in by default_rs232_devices.
void as3k_rs232_devices(device_slot_interface &device)
{
	device.option_add("loopback", RS232_LOOPBACK);
	device.option_add("null_modem", NULL_MODEM);
	device.option_add("pty", PSEUDO_TERMINAL);
	// Native serial printer — reuse standard MAME output device.
	device.option_add("printer", SERIAL_PRINTER);
}

class alphasmart3k_state : public driver_device
{
public:
	alphasmart3k_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_lcdc0(*this, "ks0066_0")
		, m_lcdc1(*this, "ks0066_1")
		, m_ram(*this, RAM_TAG)
		, m_nvram(*this, "nvram")
		, m_flash(*this, "flash")
		, m_ipl(*this, "ipl")
		, m_keys(*this, "COL%u", 0U)
		, m_battery_status(*this, "BATTERY")
		, m_host_mode(*this, "HOST")
		, m_bootstrap_select(*this, "BOOT")
	{
	}

	DEVICE_IMAGE_LOAD_MEMBER(flash_image_load);
	void alphasmart3k(machine_config &config);
	void alphasmart3k_diag(machine_config &config);
	DECLARE_INPUT_CHANGED_MEMBER(keyboard_key_changed);
	DECLARE_INPUT_CHANGED_MEMBER(host_mode_changed);

protected:
	required_device<mc68ez328_device> m_maincpu;
	optional_device<ks0066_device> m_lcdc0;
	optional_device<ks0066_device> m_lcdc1;
	required_device<ram_device> m_ram;
	required_device<nvram_device> m_nvram;
	required_device<intel_28f008b3b_device> m_flash;
	required_region_ptr<u16> m_ipl;
	required_ioport_array<16> m_keys;
	required_ioport m_battery_status;
	required_ioport m_host_mode;
	required_ioport m_bootstrap_select;
	u8 m_host_connection = 0;
	bool m_host_clock_drive = false, m_host_data_drive = false;
	u16 m_host_frame = 0;
	u8 m_host_frame_bits = 0;
	// AS2K-proven Set-2 text sink adapted to original AS3K PS/2 pins.
	// Output lives in the current MAME NVRAM directory (not private ROMs).
	void send_sink_begin();
	void send_sink_byte(u8 data);
	void send_sink_end();
	std::unique_ptr<emu_file> m_send_sink;
	bool m_send_sink_active = false;
	bool m_send_sink_break = false;
	bool m_send_sink_shift = false;
	// Optical SIR link. The optional loopback socket carries only decoded
	// wire bytes; framing, FCS, LAP/LMP and application replies belong to
	// the external peer and original AlphaSmart ROM respectively.
	bool m_ir_shutdown = false, m_rs232_power = true;
	emu_timer *m_ir_sample_timer = nullptr, *m_ir_send_timer = nullptr;
	attotime m_ir_last_pulse;
	u8 m_ir_sample_phase = 0, m_ir_sample_value = 0;
	u16 m_ir_send_frame = 0;
	u8 m_ir_send_bits = 0;
	bool m_ir_send_pulse = false;
	void ir_tx_w(int state);
	TIMER_CALLBACK_MEMBER(ir_sample_tick);
	TIMER_CALLBACK_MEMBER(ir_send_tick);
#if defined(__linux__)
	int m_ir_server = -1, m_ir_peer = -1;
	emu_timer *m_ir_io_timer = nullptr;
	std::deque<u8> m_ir_rx_bytes, m_ir_tx_bytes;
	void ir_start();
	TIMER_CALLBACK_MEMBER(ir_io_tick);
#endif
	void host_clock_w(int state);
	void host_data_w(int state);
	emu_timer *m_adb_timer = nullptr;
	TIMER_CALLBACK_MEMBER(adb_tick);
	bool m_adb_low = false, m_adb_rx_started = false;
	u8 m_adb_phase = 0, m_adb_value = 0;
	u16 m_adb_cells = 0;
	u32 m_adb_poll_count = 0;
	bool m_adb_handler_identified = false;
	u8 m_adb_last_command = 0;
	u64 m_adb_fall_us = 0, m_adb_low_us = 0;
	std::vector<u8> m_adb_received;
	int host_clock_r() { return m_host_connection == 2 ? 1 : (m_host_connection ? !m_host_clock_drive : usb_cable_pb_r()); }
	int host_data_r() { return m_host_connection ? !(m_host_data_drive || m_adb_low) : usb_cable_pb_r(); }


	u8 m_lcd_port_c_pending = 0;
	u8 m_lcd_port_c_applied = 0;
	u8 m_keyboard_column_latch = 0xff;
	u8 m_keyboard_port_a_columns = 0x7f;
	// PDIUSBD11D I2C control bus. SDA=PE1 and SCL=PE2,
	// as driven by PEDIR in the original UsbAsmModule.s.
	bool m_usb_master_sda = true, m_usb_master_scl = true;
	bool m_usb_slave_sda_low = false;
	u8 m_usb_state = 0, m_usb_bits = 0, m_usb_shift = 0;
	u8 m_usb_route = 0, m_usb_command = 0, m_usb_ack = 0;
	u8 m_usb_read_index = 0, m_usb_tx_data = 0;
	u8 m_usb_addr = 0, m_usb_ep_enable = 0, m_usb_mode = 0;
	u8 m_usb_clkdiv = 0, m_usb_write_index = 0, m_usb_master_ack = 0;
	// PDIUSBD11: ten logical endpoint indices, each eight USB data
	// bytes plus the reserved/length I2C buffer header.
	std::array<u8, 100> m_usb_ep_fifo{};
	std::array<u8, 10> m_usb_ep_full{};
	std::array<u8, 10> m_usb_ep_stall{};
	u8 m_usb_selected_ep = 0;
	u8 m_usb_ep_ptr = 0; // D11 buffer pointer: reset ONLY by Select Endpoint
	std::array<u8, 10> m_usb_last_status{}; // last actual USB transaction, if any
	u16 m_usb_pending_irq = 0; // D11 endpoint+bus reset interrupt register
	bool m_usb_reset_n = true, m_usb_int_n = true;
	std::array<u8, 2> m_usb_setup_locked{};
	u16 m_usb_frame = 0;
#if defined(__linux__)
	int m_usbip_server = -1, m_usbip_peer = -1;
	emu_timer *m_usbip_timer = nullptr;
	struct usbip_request
	{
		std::vector<u8> payload, received;
		std::array<u8, 8> setup{};
		u32 seq = 0, devid = 0, direction = 0, ep = 0, size = 0;
		u32 phase = 0, elapsed = 0, payload_offset = 0;
	};
	usbip_request m_usbip_active;
	std::deque<usbip_request> m_usbip_requests;
	std::vector<u8> m_usbip_rx, m_usbip_tx;
	u32 m_usbip_tx_offset = 0;
	bool m_usbip_imported = false, m_usbip_host_reset_pending = false;
#endif

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;

private:
	void alphasmart3k_palette(palette_device &palette) const;
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void main_map(address_map &map) ATTR_COLD;
	u16 flash_r(offs_t offset, u16 mem_mask = ~0);
	void flash_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	void keyboard_column_latch_w(u8 data);
	void keyboard_port_a_w(unsigned bit, int state);
	int keyboard_row_r(unsigned row);
	void refresh_keyboard_lines();
	void usb_pe_line_w(unsigned pin, int state);
	bool usb_vbus_r() const
    {
#if defined(__linux__)
        // The physical D11 SDA pull-up is powered by the attached USB host.
        // Keep non-USBIP standalone diagnostic behavior unchanged.
        // An external keyboard-host session has real cable power: absent
        // until connection, present for PS/2 or ADB. Keep the historical
        // standalone powered-D11 diagnostic fixture when no host is selected.
        return m_host_connection != 0 || m_usbip_imported ||
            (m_usbip_server < 0 && !std::getenv("AS3K_KEYBOARD_HOST"));
#else
        return true;
#endif
    }
    int usb_sda_r() { return usb_vbus_r() && m_usb_master_sda && !m_usb_slave_sda_low; }
    int usb_scl_r() { return usb_vbus_r() && m_usb_master_scl; }
    int usb_cable_pb_r()
    {
#if defined(__linux__)
        // Original SystemModule: USB connector grounds PB3=CLK_IN and PB6=DATA_IN.
        return m_usbip_imported ? 0 : 1;
#else
        return 1;
#endif
    }
	void usb_receive_byte(u8 data);
	void usb_prepare_tx();
	void usb_chip_reset();
	void usb_bus_reset();
	int usb_host_out(unsigned ep, const u8 *data, unsigned len, bool setup = false);
	int usb_host_in(unsigned ep, u8 *data);
#if defined(__linux__)
	TIMER_CALLBACK_MEMBER(usbip_tick);
	void usbip_start();
	void usbip_close_peer();
	void usbip_parse();
	void usbip_progress();
	void usbip_reply(int status);
	void usbip_queue(const u8 *p, size_t len);
#endif
	void usb_command_w(u8 data);
	u8 usb_command_read();
	void usb_irq_update();
	void usb_reset_n_w(int state);
	int usb_int_n_r() { return m_usb_int_n; }
	int usb_suspend_r() { return 0; } // no host, no suspend event
	template <unsigned Pin> void usb_port_e_w(int state) { usb_pe_line_w(Pin, state); }
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

	// The 256 KiB HY62U8200 is backed up by the original AS3000's
	// battery. Persist the exact external SRAM bytes, including the
	// OS's own file allocation structures and document contents.
	// No host-side translation of AlphaWord files is performed.
	m_nvram->set_base(m_ram->pointer(), m_ram->size());
	m_adb_timer = timer_alloc(FUNC(alphasmart3k_state::adb_tick), this);
	m_ir_sample_timer = timer_alloc(FUNC(alphasmart3k_state::ir_sample_tick), this);
	m_ir_send_timer = timer_alloc(FUNC(alphasmart3k_state::ir_send_tick), this);

	save_item(NAME(m_lcd_port_c_pending));
	save_item(NAME(m_lcd_port_c_applied));
	save_item(NAME(m_keyboard_column_latch));
	save_item(NAME(m_keyboard_port_a_columns));
	save_item(NAME(m_usb_master_sda));
	save_item(NAME(m_usb_master_scl));
	save_item(NAME(m_usb_slave_sda_low));
	save_item(NAME(m_usb_state));
	save_item(NAME(m_usb_bits));
	save_item(NAME(m_usb_shift));
	save_item(NAME(m_usb_route));
	save_item(NAME(m_usb_command));
	save_item(NAME(m_usb_ack));
	save_item(NAME(m_usb_read_index));
	save_item(NAME(m_usb_tx_data));
	save_item(NAME(m_usb_addr));
	save_item(NAME(m_usb_ep_enable));
	save_item(NAME(m_usb_mode));
	save_item(NAME(m_usb_clkdiv));
	save_item(NAME(m_usb_write_index));
	save_item(NAME(m_usb_master_ack));
	save_item(NAME(m_usb_ep_fifo));
	save_item(NAME(m_usb_ep_full));
	save_item(NAME(m_usb_ep_stall));
	save_item(NAME(m_usb_selected_ep));
	save_item(NAME(m_usb_ep_ptr));
	save_item(NAME(m_usb_last_status));
	save_item(NAME(m_usb_pending_irq));
	save_item(NAME(m_usb_reset_n));
	save_item(NAME(m_usb_int_n));
	save_item(NAME(m_usb_setup_locked));
	save_item(NAME(m_usb_frame));
	save_item(NAME(m_host_connection));
	save_item(NAME(m_host_clock_drive));
	save_item(NAME(m_host_data_drive));
	save_item(NAME(m_host_frame));
	save_item(NAME(m_host_frame_bits));
	save_item(NAME(m_adb_handler_identified));
	save_item(NAME(m_adb_last_command));
#if defined(__linux__)
	usbip_start();
	ir_start();
#endif
}

void alphasmart3k_state::machine_reset()
{
	// Preserve the historical normal-boot vector copy. Bootstrap supplies
	// internal reset vectors and must not overwrite even the first SRAM page.
	if (!m_maincpu->bootstrap_mode())
		memcpy(m_ram->pointer(), memregion("ipl")->base(), 0x100);

	m_lcd_port_c_pending = 0;
	m_lcd_port_c_applied = 0;
	// The hardware power-on state is unknown; default to no active-low columns.
	m_keyboard_column_latch = 0xff;
	m_keyboard_port_a_columns = 0x7f;
	m_host_connection = m_host_mode->read();
	m_host_clock_drive = m_host_data_drive = false;
	m_host_frame = m_host_frame_bits = 0;
	m_adb_low = m_adb_rx_started = false;
	m_adb_phase = m_adb_value = 0;
	m_adb_cells = 0;
	m_adb_handler_identified = false;
	m_adb_last_command = 0;
	m_adb_received.clear();
	m_adb_timer->adjust(m_host_connection == 2 ? attotime::from_msec(1) : attotime::never);
	m_usb_master_sda = m_usb_master_scl = true;
	m_usb_slave_sda_low = false;
	m_usb_state = m_usb_bits = m_usb_shift = 0;
	m_usb_route = m_usb_command = m_usb_ack = 0;
	m_usb_read_index = m_usb_tx_data = 0;
	m_usb_addr = m_usb_ep_enable = m_usb_mode = m_usb_clkdiv = 0;
	m_usb_write_index = m_usb_master_ack = 0;
	m_usb_reset_n = true;
	usb_chip_reset();
	refresh_keyboard_lines();
}

// SystemModule + PS2Assembly: PB4/PB5 drive inverting open-collector
// transistors; PB3/PB6 read the physical clock/data lines.  Sample on the
// falling external clock (rising transistor control), as in AS2K's proven
// pc_clock_rising receiver, retaining its complete 11-bit frame validation.
void alphasmart3k_state::send_sink_begin()
{
	if (m_send_sink)
	{
		m_send_sink->close();
		m_send_sink.reset();
	}

	std::string const output_directory(machine().options().nvram_directory());
	if (output_directory.empty())
	{
		logerror("AS3K_PS2_TX TEXT_SINK_OPEN_FAILED reason=no_nvram_directory\n");
		m_send_sink_active = false;
		return;
	}

	m_send_sink = std::make_unique<emu_file>(
		output_directory,
		OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);

	std::error_condition const err = m_send_sink->open("send.txt");
	if (err)
	{
		logerror("AS3K_PS2_TX TEXT_SINK_OPEN_FAILED path=%s%s%s error=%s\n",
			output_directory,
			PATH_SEPARATOR,
			"send.txt",
			err.message());
		m_send_sink.reset();
		m_send_sink_active = false;
	}
	else
	{
		logerror("AS3K_PS2_TX TEXT_SINK_OPEN path=%s\n", m_send_sink->fullpath());
		m_send_sink_active = true;
	}

	m_send_sink_break = false;
	m_send_sink_shift = false;
}

void alphasmart3k_state::send_sink_byte(u8 data)
{
	if (!m_send_sink_active)
		return;

	if (data == 0xf0)
	{
		m_send_sink_break = true;
		return;
	}

	if (m_send_sink_break)
	{
		if (data == 0x12)
			m_send_sink_shift = false;
		m_send_sink_break = false;
		return;
	}

	if (data == 0x12)
	{
		m_send_sink_shift = true;
		return;
	}

	char out = 0;
	switch (data)
	{
	case 0x1c: out = m_send_sink_shift ? 'A' : 'a'; break;
	case 0x32: out = m_send_sink_shift ? 'B' : 'b'; break;
	case 0x21: out = m_send_sink_shift ? 'C' : 'c'; break;
	case 0x23: out = m_send_sink_shift ? 'D' : 'd'; break;
	case 0x24: out = m_send_sink_shift ? 'E' : 'e'; break;
	case 0x2b: out = m_send_sink_shift ? 'F' : 'f'; break;
	case 0x34: out = m_send_sink_shift ? 'G' : 'g'; break;
	case 0x33: out = m_send_sink_shift ? 'H' : 'h'; break;
	case 0x43: out = m_send_sink_shift ? 'I' : 'i'; break;
	case 0x3b: out = m_send_sink_shift ? 'J' : 'j'; break;
	case 0x42: out = m_send_sink_shift ? 'K' : 'k'; break;
	case 0x4b: out = m_send_sink_shift ? 'L' : 'l'; break;
	case 0x3a: out = m_send_sink_shift ? 'M' : 'm'; break;
	case 0x31: out = m_send_sink_shift ? 'N' : 'n'; break;
	case 0x44: out = m_send_sink_shift ? 'O' : 'o'; break;
	case 0x4d: out = m_send_sink_shift ? 'P' : 'p'; break;
	case 0x15: out = m_send_sink_shift ? 'Q' : 'q'; break;
	case 0x2d: out = m_send_sink_shift ? 'R' : 'r'; break;
	case 0x1b: out = m_send_sink_shift ? 'S' : 's'; break;
	case 0x2c: out = m_send_sink_shift ? 'T' : 't'; break;
	case 0x3c: out = m_send_sink_shift ? 'U' : 'u'; break;
	case 0x2a: out = m_send_sink_shift ? 'V' : 'v'; break;
	case 0x1d: out = m_send_sink_shift ? 'W' : 'w'; break;
	case 0x22: out = m_send_sink_shift ? 'X' : 'x'; break;
	case 0x35: out = m_send_sink_shift ? 'Y' : 'y'; break;
	case 0x1a: out = m_send_sink_shift ? 'Z' : 'z'; break;

	case 0x16: out = m_send_sink_shift ? '!' : '1'; break;
	case 0x1e: out = m_send_sink_shift ? '@' : '2'; break;
	case 0x26: out = m_send_sink_shift ? '#' : '3'; break;
	case 0x25: out = m_send_sink_shift ? '$' : '4'; break;
	case 0x2e: out = m_send_sink_shift ? '%' : '5'; break;
	case 0x36: out = m_send_sink_shift ? '^' : '6'; break;
	case 0x3d: out = m_send_sink_shift ? '&' : '7'; break;
	case 0x3e: out = m_send_sink_shift ? '*' : '8'; break;
	case 0x46: out = m_send_sink_shift ? '(' : '9'; break;
	case 0x45: out = m_send_sink_shift ? ')' : '0'; break;

	case 0x54: out = m_send_sink_shift ? '{' : '['; break;
	case 0x5b: out = m_send_sink_shift ? '}' : ']'; break;
	case 0x4c: out = m_send_sink_shift ? ':' : ';'; break;
	case 0x52: out = m_send_sink_shift ? '"' : '\''; break;
	case 0x41: out = m_send_sink_shift ? '<' : ','; break;
	case 0x49: out = m_send_sink_shift ? '>' : '.'; break;
	case 0x4a: out = m_send_sink_shift ? '?' : '/'; break;
	case 0x4e: out = m_send_sink_shift ? '_' : '-'; break;
	case 0x55: out = m_send_sink_shift ? '+' : '='; break;
	case 0x5d: out = m_send_sink_shift ? '|' : '\\'; break;
	case 0x0e: out = m_send_sink_shift ? '~' : '`'; break;

	case 0x29: out = ' '; break;
	case 0x5a: out = '\n'; break;
	case 0x0d: out = '\t'; break;
	default: break;
	}

	if (out)
	{
		m_send_sink->write(&out, 1);
		logerror("AS3K_PS2_TEXT: char=%02X\n", unsigned(u8(out)));
	}
}

void alphasmart3k_state::send_sink_end()
{
	if (m_send_sink)
	{
		m_send_sink->flush();
		m_send_sink->close();
		m_send_sink.reset();
	}
	m_send_sink_active = false;
	m_send_sink_break = false;
	m_send_sink_shift = false;
}

INPUT_CHANGED_MEMBER(alphasmart3k_state::host_mode_changed)
{
	// Input callbacks run before the port's current-frame value is committed.
	// Set electrical connector state before asserting its interrupt or timer.
	m_host_connection = newval;
	// Close the previous real Send stream when the physical host mode changes.
	send_sink_end();
	m_host_frame = 0;
	m_host_frame_bits = 0;
	m_adb_phase = 0;
	m_adb_poll_count = 0;
	m_adb_handler_identified = false;
	m_adb_last_command = 0;
	m_adb_low = false;
	m_adb_timer->adjust(newval == 2 ? attotime::from_msec(1) : attotime::never);
	// Preserve the previously accredited host-presence wake behavior.
	// This is the original AS3K-DIAG host-mode signal, not a printer change.
	m_maincpu->irq5_w(newval != 0);
	logerror("AS3K HOST: mode=%u\n", unsigned(newval));
}

// Minimal ADB host: standard attention, sync, MSB-first Talk keyboard
// register 0 and stop; the original ROM supplies all response keycodes.
// Its DATA_IN sees the wired AND of host and peripheral open collectors.
TIMER_CALLBACK_MEMBER(alphasmart3k_state::adb_tick)
{
	if (m_host_connection != 2)
		return;
	unsigned delay = 0;
	if (m_adb_phase == 0)
	{
        // The original AS3000 ROM initializes its ADB address to 7
        // and handler ID to 0x5E. Enumerate once, then poll keyboard 2.
        if (!m_adb_handler_identified && m_adb_last_command == 0x7f &&
            m_adb_received.size() >= 2 && m_adb_received.back() == 0x5e)
        {
            m_adb_handler_identified = true;
            logerror("AS3K ADB: original handler identified id=5E address=7\n");
        }
		++m_adb_poll_count;
		if (std::getenv("AS3K_ADB_TRACE") && m_adb_poll_count <= 8)
			logerror("AS3K ADB HOST: poll=%u us=%llu command=2C\n", m_adb_poll_count, (unsigned long long)machine().time().as_ticks(1'000'000));
		if (!m_adb_received.empty())
		{
			for (u8 byte : m_adb_received)
				logerror("AS3K ADB: byte=%02X cells=%u\n", byte, m_adb_cells);
		}
		m_adb_received.clear();
		m_adb_rx_started = false;
		m_adb_cells = m_adb_value = 0;
		m_adb_low = true;
		delay = 800;
	}
	else if (m_adb_phase == 1)
	{
		m_adb_low = false;
		delay = 70;
	}
	else if (m_adb_phase < 18)
	{
		unsigned const index = (m_adb_phase - 2) / 2;
		// Original ROM AlphaSmart default device address 7 (0x70).
		// Probe its Register 3, then poll Mac keyboard Reg 0 at 0x2C.
		u8 const command = (!m_adb_handler_identified && (m_adb_poll_count % 16) == 1) ? 0x7f : 0x2c;
		m_adb_last_command = command;
		bool const bit = BIT(command, 7 - index);
		m_adb_low = !(m_adb_phase & 1);
		delay = m_adb_low ? (bit ? 35 : 65) : (bit ? 65 : 35);
	}
	else if (m_adb_phase == 18)
	{
		m_adb_low = true;
		delay = 70;
	}
	else
	{
		m_adb_low = false;
		delay = 6'000;
		m_adb_phase = 255; // next callback starts the next poll
	}
	++m_adb_phase;
	m_adb_timer->adjust(attotime::from_usec(delay));
}

void alphasmart3k_state::host_data_w(int state)
{
	bool const old = m_host_data_drive;
	m_host_data_drive = bool(state);
	if (m_host_connection == 2 && old != bool(state) && std::getenv("AS3K_ADB_TRACE"))
		logerror("AS3K ADB EDGE: us=%llu low=%u phase=%u pc=%08X\n", (unsigned long long)machine().time().as_ticks(1'000'000), unsigned(bool(state)), m_adb_phase, m_maincpu->pc());
	if (m_host_connection != 2 || m_adb_phase != 0 || old == bool(state))
		return;
	u64 const now = machine().time().as_ticks(1'000'000);
	if (state) // falling external data edge
	{
		if (m_adb_rx_started)
		{
			u64 const cell = now - m_adb_fall_us;
			if (cell >= 50 && cell <= 160 && m_adb_low_us >= 15 && m_adb_low_us < cell)
			{
				bool const bit = m_adb_low_us < cell - m_adb_low_us;
				// First cell is the ADB start bit; subsequent bytes are MSB-first.
				if (m_adb_cells)
				{
					m_adb_value = (m_adb_value << 1) | bit;
					if (!(m_adb_cells & 7))
						m_adb_received.push_back(m_adb_value);
				}
				++m_adb_cells;
			}
			else
				logerror("AS3K ADB: invalid low=%llu cell=%llu\n", (unsigned long long)m_adb_low_us, (unsigned long long)cell);
		}
		m_adb_fall_us = now;
		m_adb_rx_started = true;
	}
	else
		m_adb_low_us = now - m_adb_fall_us;
}

// Decode the received optical pulses at UART data-bit centers. No pulse
// represents one; a pulse in the current cell represents zero.
void alphasmart3k_state::ir_tx_w(int state)
{
#if defined(__linux__)
	if (m_ir_server < 0 || !state || m_ir_shutdown || m_rs232_power)
		return;
	m_ir_last_pulse = machine().time();
	if (!m_ir_sample_phase)
	{
		m_ir_sample_phase = 1;
		m_ir_sample_value = 0;
		m_ir_sample_timer->adjust(m_maincpu->uart_bit_period() * 3 / 2);
	}
#endif
}

TIMER_CALLBACK_MEMBER(alphasmart3k_state::ir_sample_tick)
{
#if defined(__linux__)
	bool const bit = (machine().time() - m_ir_last_pulse) > m_maincpu->uart_bit_period();
	if (m_ir_sample_phase <= 8)
	{
		if (bit) m_ir_sample_value |= 1 << (m_ir_sample_phase - 1);
		++m_ir_sample_phase;
		m_ir_sample_timer->adjust(m_maincpu->uart_bit_period());
	}
	else
	{
		if (bit)
		{
			if (m_ir_tx_bytes.size() < 65536) m_ir_tx_bytes.push_back(m_ir_sample_value);
			logerror("AS3K SIR TX: byte=%02X\n", m_ir_sample_value);
		}
		else
			logerror("AS3K SIR TX: framing error\n");
		m_ir_sample_phase = 0;
	}
#endif
}

TIMER_CALLBACK_MEMBER(alphasmart3k_state::ir_send_tick)
{
#if defined(__linux__)
	attotime const bit = m_maincpu->uart_bit_period();
	attotime const pulse = attotime::from_nsec(1600);
	if (m_ir_send_pulse)
	{
		m_maincpu->uart_ir_rx_w(0);
		m_ir_send_pulse = false;
		m_ir_send_timer->adjust(bit - pulse);
		return;
	}
	if (!m_ir_send_bits)
	{
		if (m_ir_rx_bytes.empty() || !m_maincpu->uart_irda_enabled() || m_ir_shutdown || m_rs232_power)
			return;
		u8 const value = m_ir_rx_bytes.front();
		m_ir_rx_bytes.pop_front();
		m_ir_send_frame = (u16(value) << 1) | 0x200;
		m_ir_send_bits = 10;
		logerror("AS3K SIR RX: byte=%02X\n", value);
	}
	bool const zero = !(m_ir_send_frame & 1);
	m_ir_send_frame >>= 1;
	--m_ir_send_bits;
	if (zero)
	{
		m_maincpu->uart_ir_rx_w(1);
		m_ir_send_pulse = true;
		m_ir_send_timer->adjust(pulse);
	}
	else
		m_ir_send_timer->adjust(bit);
#endif
}

#if defined(__linux__)
void alphasmart3k_state::ir_start()
{
	char const *const setting = std::getenv("AS3K_IR_PORT");
	if (!setting || !*setting) return;
	char *end = nullptr;
	long const port = std::strtol(setting, &end, 10);
	if (*end || port < 1024 || port > 65535) fatalerror("AS3K_IR_PORT must be 1024..65535");
	m_ir_server = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	int yes = 1;
	::setsockopt(m_ir_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	if (m_ir_server < 0 || ::bind(m_ir_server, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0 || ::listen(m_ir_server, 1) < 0)
		fatalerror("AS3K SIR socket failed: %s", std::strerror(errno));
	m_ir_io_timer = timer_alloc(FUNC(alphasmart3k_state::ir_io_tick), this);
	m_ir_io_timer->adjust(attotime::from_msec(1), 0, attotime::from_msec(1));
	logerror("AS3K SIR peer: 127.0.0.1:%ld\n", port);
}

TIMER_CALLBACK_MEMBER(alphasmart3k_state::ir_io_tick)
{
	if (m_ir_peer < 0)
	{
		m_ir_peer = ::accept4(m_ir_server, nullptr, nullptr, SOCK_NONBLOCK);
		return;
	}
	u8 buffer[1024];
	ssize_t const got = ::recv(m_ir_peer, buffer, sizeof(buffer), 0);
	if (got == 0 || (got < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
	{
		::close(m_ir_peer);
		m_ir_peer = -1;
		m_ir_rx_bytes.clear();
		m_ir_tx_bytes.clear();
		return;
	}
	if (got > 0)
	{
		if (m_ir_rx_bytes.size() + got > 65536) fatalerror("AS3K SIR input queue overflow");
		m_ir_rx_bytes.insert(m_ir_rx_bytes.end(), buffer, buffer + got);
	}
	if (!m_ir_send_bits && !m_ir_send_pulse && !m_ir_rx_bytes.empty() && !m_ir_send_timer->enabled())
		m_ir_send_timer->adjust(attotime::zero);
	if (!m_ir_tx_bytes.empty())
	{
		unsigned const count = std::min<unsigned>(sizeof(buffer), m_ir_tx_bytes.size());
		std::copy_n(m_ir_tx_bytes.begin(), count, buffer);
		ssize_t const sent = ::send(m_ir_peer, buffer, count, MSG_NOSIGNAL);
		if (sent > 0) m_ir_tx_bytes.erase(m_ir_tx_bytes.begin(), m_ir_tx_bytes.begin() + sent);
	}
}
#endif

void alphasmart3k_state::host_clock_w(int state)
{
	bool const old = m_host_clock_drive;
	m_host_clock_drive = bool(state);
	if (m_host_connection != 1 || old || !state)
		return;
	bool const data = !m_host_data_drive;
	if (!m_host_frame_bits && data)
		return;
	if (data)
		m_host_frame |= u16(1) << m_host_frame_bits;
	if (++m_host_frame_bits != 11)
		return;
	u8 const value = (m_host_frame >> 1) & 0xff;
	bool odd = BIT(m_host_frame, 9);
	for (unsigned bit = 0; bit < 8; ++bit)
		odd ^= BIT(value, bit);
	bool const valid = !BIT(m_host_frame, 0) && BIT(m_host_frame, 10) && odd;
	logerror("AS3K PS2: byte=%02X valid=%u frame=%03X us=%llu\n", value, valid, m_host_frame, (unsigned long long)machine().time().as_ticks(1'000'000));
	if (valid)
	{
		if (!m_send_sink_active)
			send_sink_begin();
		send_sink_byte(value);
	}
	bool const resync = !BIT(m_host_frame, 10);
	m_host_frame = 0;
	m_host_frame_bits = valid || !resync ? 0 : 1;
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

// PDIUSBD11D: I2C serial interface at command address 0x36,
// data-write 0x34 and data-read 0x35 (Philips original datasheet).
// The original AlphaSmart UsbAsmModule.s toggles PEDIR rather than
// the latched low value in PEDATA. No USB packets are invented:
// without a USB host, the controller has no endpoint transactions.
void alphasmart3k_state::usb_irq_update()
{
	// D11 INT_N is open-drain active low on EZ IRQ5/PF1.
	// Without a connected USB host there can be no SIE transactions
	// to assert the interrupt source. Programming/validating an IN
	// FIFO is NOT an interrupt until the host consumes it.
	m_usb_int_n = !m_usb_pending_irq;
	m_maincpu->irq5_w(!m_usb_int_n);
}

void alphasmart3k_state::usb_chip_reset()
{
	m_usb_addr = m_usb_ep_enable = m_usb_mode = 0;
	// Philips PDIUSBD11 default CLKOUT divider is 11 (48 MHz / 12).
	m_usb_clkdiv = 11;
	m_usb_selected_ep = m_usb_ep_ptr = 0;
	m_usb_last_status.fill(0);
	m_usb_setup_locked.fill(0);
	m_usb_frame = 0;
	m_usb_pending_irq = 0;
	m_usb_ep_fifo.fill(0);
	m_usb_ep_full.fill(0);
	m_usb_ep_stall.fill(0);
	m_usb_route = m_usb_command = m_usb_ack = 0;
	m_usb_write_index = m_usb_read_index = m_usb_master_ack = 0;
	m_usb_tx_data = m_usb_bits = m_usb_shift = 0;
	m_usb_state = 0;
	m_usb_slave_sda_low = false;
	usb_irq_update();
}

void alphasmart3k_state::usb_reset_n_w(int state)
{
	bool const next = bool(state);
	if (m_usb_reset_n && !next)
		usb_chip_reset(); // hardware RESET_N, not simulated USB bus reset
	m_usb_reset_n = next;
	// The D11 releases SDA for the entire time RESET_N is asserted.
	// It cannot ACK I2C addresses or accept commands while held in reset.
	if (!next)
		m_usb_slave_sda_low = false;
}

void alphasmart3k_state::usb_command_w(u8 command)
{
	m_usb_command = command;
	m_usb_read_index = m_usb_write_index = 0;
	if (command <= 9)
	{
		m_usb_selected_ep = command;
		m_usb_ep_ptr = 0;  // Select Endpoint repositions the on-chip RAM pointer
	}
	else if (command == 0xf1)
	{
		// Philips Acknowledge Setup must be issued separately to EP0 OUT/IN.
		if (m_usb_selected_ep == 2 || m_usb_selected_ep == 3)
			m_usb_setup_locked[m_usb_selected_ep - 2] = 0;
	}
	else if (command == 0xf2)
	{
		// Clear Buffer: consumed OUT packet, freeing its FIFO.
		// This is harmless without host traffic, and is distinct
		// from a USB receive interrupt.
		if ((m_usb_selected_ep == 2 || m_usb_selected_ep == 5 ||
			m_usb_selected_ep == 6 || m_usb_selected_ep == 8) &&
			!(m_usb_selected_ep == 2 && m_usb_setup_locked[0]))
			m_usb_ep_full[m_usb_selected_ep] = 0;
	}
	else if (command == 0xfa)
	{
		// Validate Buffer: expose prepared IN data to a USB host.
		// Actual delivery will be possible after SIE implementation.
		if ((m_usb_selected_ep == 3 || m_usb_selected_ep == 4 ||
			m_usb_selected_ep == 7 || m_usb_selected_ep == 9) &&
			!(m_usb_selected_ep == 3 && m_usb_setup_locked[1]))
			m_usb_ep_full[m_usb_selected_ep] = 1;
	}
}

u8 alphasmart3k_state::usb_command_read()
{
	u8 const ep = m_usb_selected_ep;
	if (m_usb_command == 0xf4)
	{
		u8 const result = m_usb_read_index == 0
			? (m_usb_pending_irq & 0xff) : (m_usb_pending_irq >> 8);
		if (m_usb_read_index == 1)
		{
			// The high byte bus-reset flag is cleared by the F4 read;
			// endpoint IRQ bits are cleared by Last Transaction Status.
			m_usb_pending_irq &= ~0x4000;
			usb_irq_update();
		}
		return result;
	}
	if (m_usb_command == 0xf5)
		return m_usb_read_index == 0 ? (m_usb_frame & 0xff) :
			((m_usb_frame >> 8) & 7);
	if (m_usb_command <= 9)
		return m_usb_ep_full[ep] ? 1 : 0; // Select Endpoint
	if (m_usb_command >= 0x80 && m_usb_command <= 0x89)
	{
		u8 const index = m_usb_command & 0x0f;
		// Philips PDIUSBD11: BUFFER FULL is bit 5, STALL bit 3.
		return (m_usb_ep_full[index] ? 0x20 : 0) |
			(m_usb_ep_stall[index] ? 0x08 : 0);
	}
	if (m_usb_command >= 0x40 && m_usb_command <= 0x49)
	{
		u8 const index = m_usb_command & 0x0f;
		u8 const status = m_usb_last_status[index];
		m_usb_last_status[index] = 0;
		// Philips interrupt bits match endpoint indices 2..9.
		if (index >= 2)
			m_usb_pending_irq &= ~(1U << index);
		usb_irq_update();
		return status;
	}
	if (m_usb_command == 0xf0)
	{
		return m_usb_ep_ptr < 10
			? m_usb_ep_fifo[ep * 10 + m_usb_ep_ptr++] : 0;
	}
	return 0;
}

// Philips USB SIE packet side: host transactions only, never CPU injection.
// Original OS consumes the FIFO and INT_N through PE1/PE2/PF1.
void alphasmart3k_state::usb_bus_reset()
{
    if (!m_usb_reset_n) return;
    u8 const mode = m_usb_mode, clock = m_usb_clkdiv;
    usb_chip_reset();
    m_usb_mode = mode;
    m_usb_clkdiv = clock;
    m_usb_pending_irq = 0x4000;
    usb_irq_update();
}
int alphasmart3k_state::usb_host_out(unsigned ep, const u8 *data, unsigned len, bool setup)
{
    if (!m_usb_reset_n || ep < 2 || ep >= 10 || len > 8 || (setup && ep != 2)) return -1;
    if (ep >= 4 && !(m_usb_ep_enable & 0x02)) return -1;
    if (m_usb_ep_stall[ep] && !setup) return -2;
    if (m_usb_ep_full[ep] && !setup) return -1;
    if (setup)
    {
        m_usb_ep_full[3] = 0;
        m_usb_ep_stall[2] = m_usb_ep_stall[3] = 0;
        m_usb_setup_locked[0] = m_usb_setup_locked[1] = 1;
    }
    unsigned const at = ep * 10;
    m_usb_ep_fifo[at] = 0;
    m_usb_ep_fifo[at + 1] = u8(len);
    for (unsigned i = 0; i < len; ++i) m_usb_ep_fifo[at + i + 2] = data[i];
    m_usb_ep_full[ep] = 1;
    m_usb_last_status[ep] = setup ? 0x21 : 0x01;
    m_usb_pending_irq |= (1U << ep);
    usb_irq_update();
    return int(len);
}
int alphasmart3k_state::usb_host_in(unsigned ep, u8 *data)
{
    if (!m_usb_reset_n || ep < 2 || ep >= 10) return -1;
    if (ep >= 4 && !(m_usb_ep_enable & 0x02)) return -1;
    if (m_usb_ep_stall[ep]) return -2;
    if (!m_usb_ep_full[ep] || (ep == 3 && m_usb_setup_locked[1])) return -1;
    unsigned const at = ep * 10;
    unsigned const len = std::min<unsigned>(m_usb_ep_fifo[at + 1], 8);
    for (unsigned i = 0; i < len; ++i) data[i] = m_usb_ep_fifo[at + i + 2];
    m_usb_ep_full[ep] = 0;
    m_usb_last_status[ep] = 0x01;
    m_usb_pending_irq |= (1U << ep);
    usb_irq_update();
    return int(len);
}

void alphasmart3k_state::usb_receive_byte(u8 data)
{
	if (!m_usb_route)
	{
		m_usb_route = data;
		m_usb_ack = (data == 0x36 || data == 0x34 || data == 0x35);
		m_usb_read_index = 0;
		m_usb_write_index = 0;
	}
	else if (m_usb_route == 0x36)
	{
		// Every byte on command address is independently a command.
		usb_command_w(data);
		m_usb_ack = 1;
	}
	else if (m_usb_route == 0x34)
	{
		u8 const ep = m_usb_selected_ep;
		switch (m_usb_command)
		{
		case 0xd1: m_usb_addr = data; break;
		case 0xd8: m_usb_ep_enable = data; break;
		case 0xf3:
			if (m_usb_write_index == 0)
            {
#if defined(__linux__)
                // SoftConnect is bit 4 (UsbChip.h / Philips D11). Each
                // genuine firmware reconnection requires a fresh host reset.
                if (m_usbip_imported && (data & 0x10) && !(m_usb_mode & 0x10))
                    m_usbip_host_reset_pending = true;
#endif
                m_usb_mode = data;
                if (std::getenv("AS3K_USBIP_PORT"))
                    logerror("AS3K USBIP: original firmware SetMode F3=%02X\n", data);
            }
			else if (m_usb_write_index == 1) m_usb_clkdiv = data;
			break;
		case 0xf0: // host writes USB IN buffer via I2C
			if (m_usb_ep_ptr < 10)
				m_usb_ep_fifo[ep * 10 + m_usb_ep_ptr++] = data;
			break;
		default:
			// Same 0x40..0x49 commands are used for *write* of
			// endpoint stall status, unlike read last status.
			if (m_usb_command >= 0x40 && m_usb_command <= 0x49)
			{
				u8 const i = m_usb_command - 0x40;
				m_usb_ep_stall[i] = BIT(data, 0);
				m_usb_ep_ptr = 0;
				m_usb_ep_full[i] = 0; // endpoint reset on status write
			}
			break;
		}
		++m_usb_write_index;
		m_usb_ack = 1;
	}
}

void alphasmart3k_state::usb_prepare_tx()
{
	m_usb_tx_data = usb_command_read();
	++m_usb_read_index;
	m_usb_bits = 0;
	m_usb_slave_sda_low = !BIT(m_usb_tx_data, 7);
}

void alphasmart3k_state::usb_pe_line_w(unsigned pin, int state)
{
	bool const old_sda = m_usb_master_sda;
	bool const old_scl = m_usb_master_scl;
	if (pin == 1) m_usb_master_sda = bool(state);
	else if (pin == 2) m_usb_master_scl = bool(state);
	else return;

	// RESET_N is a level-sensitive physical reset, not an I2C command.
	// Track the MCU's wire levels but keep the D11 slave inactive.
	if (!m_usb_reset_n)
	{
		m_usb_state = 0;
		m_usb_slave_sda_low = false;
		return;
	}

	// Start and Stop are SDA transitions while SCL is high.
	if (old_scl && m_usb_master_scl && old_sda && !m_usb_master_sda)
	{
		m_usb_state = 1; // Receive address
		m_usb_route = m_usb_bits = m_usb_shift = 0;
		m_usb_slave_sda_low = false;
		return;
	}
	if (old_scl && m_usb_master_scl && !old_sda && m_usb_master_sda)
	{
		m_usb_state = 0; // STOP, idle
		m_usb_slave_sda_low = false;
		return;
	}
	if (old_scl == m_usb_master_scl)
		return;

	bool const rising = m_usb_master_scl;
	switch (m_usb_state)
	{
	case 1: // Receive 8 bits, MSB first
		if (rising)
		{
			m_usb_shift = (m_usb_shift << 1) | u8(m_usb_master_sda);
			if (++m_usb_bits == 8)
			{
				usb_receive_byte(m_usb_shift);
				m_usb_state = 2; // Address/data ACK preparation
				m_usb_bits = m_usb_shift = 0;
			}
		}
		break;
	case 2: // SCL low after bit 8, slave drives the ACK
		if (!rising)
		{
			m_usb_slave_sda_low = bool(m_usb_ack);
			m_usb_state = 3;
		}
		break;
	case 3: // ACK clock
		if (!rising)
		{
			m_usb_slave_sda_low = false;
			if (m_usb_route == 0x35 && !m_usb_read_index && !m_usb_write_index)
			{
				m_usb_state = 4; // transmit after initial address
				usb_prepare_tx();
			}
			else
			{
				m_usb_state = 1;
			}
		}
		break;
	case 4: // Slave data out: bit available on SCL low
		if (rising)
		{
			if (++m_usb_bits == 8)
				m_usb_state = 5;
		}
		else
		{
			m_usb_slave_sda_low = !BIT(m_usb_tx_data, 7 - m_usb_bits);
		}
		break;
	case 5: // After eighth bit: release SDA for master's ACK
		if (!rising)
		{
			m_usb_slave_sda_low = false;
			m_usb_state = 6;
		}
		break;
	case 6: // Master ACK on ninth clock
		if (rising)
			m_usb_master_ack = !m_usb_master_sda;
		else if (m_usb_master_ack)
		{
			m_usb_state = 4;
			usb_prepare_tx();
		}
		else
			m_usb_state = 7; // Wait for STOP/repeated START
		break;
	default:
		break;
	}
}

#if defined(__linux__)
#include "as3k_usbip.inc"
#endif

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

u16 alphasmart3k_state::flash_r(offs_t offset, u16 mem_mask)
{
	u16 result = 0xffff;
	if (ACCESSING_BITS_8_15)
		result = (result & 0x00ff) | (u16(m_flash->read(offset * 2)) << 8);
	if (ACCESSING_BITS_0_7)
		result = (result & 0xff00) | m_flash->read(offset * 2 + 1);
	return result;
}

void alphasmart3k_state::flash_w(offs_t offset, u16 data, u16 mem_mask)
{
	if (ACCESSING_BITS_8_15)
		m_flash->write(offset * 2, data >> 8);
	if (ACCESSING_BITS_0_7)
		m_flash->write(offset * 2 + 1, data & 0xff);
}

void alphasmart3k_state::main_map(address_map &map)
{
//  map(0x0000'0000, 0x0003'ffff).ram().share("ram");
	map(0x0040'0000, 0x004f'ffff).rw(FUNC(alphasmart3k_state::flash_r), FUNC(alphasmart3k_state::flash_w));
	// The 32 KiB chip-select window is known, but source only proves this byte address is decoded.
	map(0x0060'0000, 0x0060'0001).w(FUNC(alphasmart3k_state::keyboard_column_latch_w)).umask16(0xff00);
}

static INPUT_PORTS_START( alphasmart3k )
	PORT_START("BOOT")
	PORT_CONFNAME(0x01, 0x01, "EZ328 EMUBRK/PG5 during reset")
	PORT_CONFSETTING(0x01, "High - normal boot")
	PORT_CONFSETTING(0x00, "Low - UART bootstrap (documented service model)")

	PORT_START("HOST")
	PORT_CONFNAME(0x03, 0x00, "External keyboard host") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart3k_state::host_mode_changed), 0)
	PORT_CONFSETTING(0x00, "Disconnected")
	PORT_CONFSETTING(0x01, "PC PS/2")
	PORT_CONFSETTING(0x02, "Macintosh ADB")

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



// Read-only user-selected Flash seed. image_manager loads this before nvram_load;
// an existing per-image NVRAM Flash therefore retains normal persistence. Tools
// isolate NVRAM by image digest (or by unique disposable session for applet tests).
DEVICE_IMAGE_LOAD_MEMBER(alphasmart3k_state::flash_image_load)
{
	if (image.length() != 0x100000)
		return { image_error::INVALIDLENGTH, "AS3K Flash images must contain exactly 1048576 bytes" };
	std::vector<u8> raw(0x100000);
	if (image.fread(raw.data(), raw.size()) != raw.size())
		return { image_error::UNSPECIFIED, "Could not read complete AS3K Flash image" };
	std::copy(raw.begin(), raw.end(), memregion("flash")->base());
	// IPL is a host-endian u16 region, while the Intel Flash region stores wire bytes.
	for (unsigned i = 0; i < raw.size() / 2; ++i)
		m_ipl[i] = (u16(raw[2 * i]) << 8) | raw[2 * i + 1];
	return { std::error_condition(), std::string() };
}

void alphasmart3k_state::alphasmart3k(machine_config &config)
{
	// Standard read-only image input; it seeds the existing Intel Flash, not a second device.
	generic_socket_device &rom(GENERIC_SOCKET(config, "flash_image", generic_plain_slot, "as3k_flash", "bin,rom"));
	rom.set_fixed(true);
	rom.set_device_load(FUNC(alphasmart3k_state::flash_image_load));
	// Basic machine hardware
	MC68EZ328(config, m_maincpu, 16'000'000); // MC68EZ328PU16V, clock unverified
	m_maincpu->set_addrmap(AS_PROGRAM, &alphasmart3k_state::main_map);
	m_maincpu->in_emubrk().set([this]() { return BIT(m_bootstrap_select->read(), 0); });
	m_maincpu->in_port_g<5>().set([this]() { return BIT(m_bootstrap_select->read(), 0); });
	// Native EZ UART bit stream. In bootstrap the diagnostic adapter connects
	// directly to MCU UART pins; normal sessions retain the SP3223 PG2 gate.
	m_maincpu->out_uart_txd().set([this](int state) { subdevice<rs232_port_device>("rs232")->write_txd((m_maincpu->bootstrap_mode() || m_rs232_power) ? state : 1); });
	m_maincpu->out_uart_ir().set(FUNC(alphasmart3k_state::ir_tx_w));
	m_maincpu->out_port_a<7>().set([this](int state) { m_ir_shutdown = bool(state); });
	m_maincpu->out_port_g<2>().set([this](int state) { m_rs232_power = bool(state); });
	rs232_port_device &rs232(RS232_PORT(config, "rs232", as3k_rs232_devices, nullptr));
	rs232.rxd_handler().set([this](int state) { if (m_maincpu->bootstrap_mode() || m_rs232_power) m_maincpu->uart_rxd_w(state); });
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
	// Original USB cable discrimination uses PB3 and PB6 held low.
    // Both are physical external inputs; no firmware or CPU-address hook.
    m_maincpu->out_port_b<4>().set(FUNC(alphasmart3k_state::host_clock_w));
    m_maincpu->out_port_b<5>().set(FUNC(alphasmart3k_state::host_data_w));
    // The optional IR flex assembly grounds PB1. An absent assembly
    // leaves the enabled pull-up high (IrdaModule original presence test).
    m_maincpu->in_port_b<1>().set([this]() {
#if defined(__linux__)
        return m_ir_server < 0 ? 1 : 0;
#else
        return 1;
#endif
    });
    m_maincpu->in_port_b<3>().set(FUNC(alphasmart3k_state::host_clock_r));
    m_maincpu->in_port_b<6>().set(FUNC(alphasmart3k_state::host_data_r));
    // Actual USB control interface is wired to EZ Port E's two
	// open-drain pins (UsbAsmModule.s / UsbModule.c).
	m_maincpu->out_port_e<1>().set(FUNC(alphasmart3k_state::usb_port_e_w<1>));
	m_maincpu->out_port_e<2>().set(FUNC(alphasmart3k_state::usb_port_e_w<2>));
	m_maincpu->in_port_e<1>().set(FUNC(alphasmart3k_state::usb_sda_r));
	m_maincpu->in_port_e<2>().set(FUNC(alphasmart3k_state::usb_scl_r));
	// USB suspend PE0; INT_N is EZ IRQ5 / PF1; RESET_N is PF2.
	// Hardware-source UsbModule.c sets PF2 high to release reset.
	m_maincpu->in_port_e<0>().set(FUNC(alphasmart3k_state::usb_suspend_r));
	m_maincpu->in_port_f<1>().set(FUNC(alphasmart3k_state::usb_int_n_r));
	m_maincpu->out_port_f<2>().set(FUNC(alphasmart3k_state::usb_reset_n_w));
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
	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);
	INTEL_28F008B3B(config, m_flash);

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
	ROM_REGION( 0x100000, "flash", 0 )
	ROM_COPY( "ipl", 0x000000, 0x000000, 0x100000 )
ROM_END

// Private AS3000 System 3 (Nov 2005) Flash builds. The binaries are held
// outside the repository; neither variant replaces the physical 28f008b3.u1 dump.
// Small ROM, native OS segments and (Writer only) A000 are at manufacturer
// addresses. ROM presence does not establish completed firmware boot.
ROM_START( asma3ksb )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "os3k_min_boot.bin", 0x000000, 0x100000, CRC(ac3b2a2c) SHA1(c2936532f37514d0eaa74e2cf97bf96bb17e883d) )
	ROM_REGION( 0x100000, "flash", 0 )
	ROM_COPY( "ipl", 0x000000, 0x000000, 0x100000 )
ROM_END

ROM_START( asma3ksw )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "os3k_min_writer.bin", 0x000000, 0x100000, CRC(c79df93f) SHA1(02ae5d564b48476abf7067b71a7e0166e471bdfa) )
	ROM_REGION( 0x100000, "flash", 0 )
	ROM_COPY( "ipl", 0x000000, 0x000000, 0x100000 )
ROM_END

// Local AS3000 startup diagnostics built from archived AlphaSmart development artifacts.
// These are synthetic test fixtures, not physical ROM dumps.
ROM_START( asma3kdi )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "as3k_diag_invalid_applet.u1", 0x000000, 0x100000, CRC(6432156f) SHA1(bfe849333e32887241f73d0146776001df093ccc) )
	ROM_REGION( 0x100000, "flash", 0 )
	ROM_COPY( "ipl", 0x000000, 0x000000, 0x100000 )
ROM_END

ROM_START( asma3kdv )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "as3k_diag_valid_alphaword.u1", 0x000000, 0x100000, CRC(08d1bf77) SHA1(f1b545de5a96f755df7f1f0302a78d8ab68bd505) )
	ROM_REGION( 0x100000, "flash", 0 )
	ROM_COPY( "ipl", 0x000000, 0x000000, 0x100000 )
ROM_END

ROM_START( asma3kdvl )
	ROM_REGION16_BE( 0x100000, "ipl", 0 )
	ROM_LOAD16_WORD( "as3k_diag_valid_alphaword.u1", 0x000000, 0x100000, CRC(08d1bf77) SHA1(f1b545de5a96f755df7f1f0302a78d8ab68bd505) )
	ROM_REGION( 0x100000, "flash", 0 )
	ROM_COPY( "ipl", 0x000000, 0x000000, 0x100000 )
ROM_END

} // anonymous namespace

//    YEAR  NAME    PARENT COMPAT MACHINE       INPUT         CLASS               INIT        COMPANY             FULLNAME           FLAGS
COMP( 2000, asma3k, 0,     0,     alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2005, asma3ksb, 0,     0,     alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 System 3 (minimal boot Flash, private)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2005, asma3ksw, 0,     0,     alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 System 3 (AlphaWord Plus Flash, private)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdi, 0,    0,     alphasmart3k_diag, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (invalid applet)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdv, 0,    0,     alphasmart3k_diag, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (valid AlphaWord)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 2000, asma3kdvl, asma3kdv, 0, alphasmart3k, alphasmart3k, alphasmart3k_state, empty_init, "AlphaSmart, Inc.", "AlphaSmart 3000 startup diagnostic (valid AlphaWord + LCD bridge)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
