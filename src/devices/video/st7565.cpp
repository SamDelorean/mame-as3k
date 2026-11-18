// license:BSD-3-Clause
// copyright-holders:Kirk Meyer
/***************************************************************************

    Sitronix ST7565R 65 x 132 LCD controller/driver

    Reference: ST7565R v1.3, 2005-11-25 (MPU interface, RAM, commands).
    https://focuslcds.com/wp-content/uploads/Drivers/ST7565R.pdf

    One device represents one controller, not a particular LCD module.
    Page 8 has only one bit per column; start-line scrolling affects the
    64-line display RAM, not this separate icon row. /RES and the software
    reset command preserve RAM and reset different sets of registers.

    TODO: serial/pin-level bus interface, busy/reset recovery timing, static
    indicator waveform and analogue drive voltages/contrast. Power, bias,
    volume and booster registers are retained, without inventing pixel
    brightness or instruction execution times for the analogue circuitry.

***************************************************************************/

#include "emu.h"
#include "st7565.h"

#include "screen.h"

DEFINE_DEVICE_TYPE(ST7565R, st7565r_device, "st7565r", "Sitronix ST7565R LCD Controller")

st7565r_device::st7565r_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, ST7565R, tag, owner, clock)
{
}

void st7565r_device::device_start()
{
	if (m_duty != 65 && m_duty != 49 && m_duty != 33 && m_duty != 55 && m_duty != 53)
		fatalerror("%s: unsupported ST7565R duty 1/%u\n", tag(), m_duty);

	save_item(NAME(m_ddram));
	save_item(NAME(m_page));
	save_item(NAME(m_column));
	save_item(NAME(m_saved_column));
	save_item(NAME(m_start_line));
	save_item(NAME(m_read_latch));
	save_item(NAME(m_pending_command));
	save_item(NAME(m_power));
	save_item(NAME(m_resistor_ratio));
	save_item(NAME(m_volume));
	save_item(NAME(m_booster_ratio));
	save_item(NAME(m_indicator_mode));
	save_item(NAME(m_read_pending));
	save_item(NAME(m_modify_write));
	save_item(NAME(m_display_on));
	save_item(NAME(m_adc));
	save_item(NAME(m_com_reverse));
	save_item(NAME(m_reverse));
	save_item(NAME(m_all_points));
	save_item(NAME(m_bias));
	save_item(NAME(m_indicator));
	save_item(NAME(m_reset));
}

void st7565r_device::device_reset()
{
	reset_registers(true);
	m_reset = false;
}

void st7565r_device::reset_registers(bool hardware)
{
	// Datasheet reset circuit: software reset applies only items 11 through 19.
	m_page = m_column = m_saved_column = m_start_line = 0;
	m_pending_command = 0;
	m_read_pending = true;
	m_modify_write = false;
	m_com_reverse = false;
	m_resistor_ratio = 0;
	m_volume = 0x20;
	m_indicator = false;
	m_indicator_mode = 0;
	if (hardware)
	{
		m_display_on = m_adc = m_reverse = m_all_points = m_bias = false;
		m_power = m_booster_ratio = 0;
		m_read_latch = 0;
	}
}

void st7565r_device::reset_w(int state)
{
	if (!state)
		reset_registers(true);
	m_reset = !state;
}

u8 st7565r_device::read(offs_t offset)
{
	return BIT(offset, 0) ? data_r() : status_r();
}

void st7565r_device::write(offs_t offset, u8 data)
{
	if (BIT(offset, 0))
		data_w(data);
	else
		control_w(data);
}

u8 st7565r_device::status_r()
{
	// ON/OFF is active low. Only the reset pin's busy condition is modelled.
	return (m_reset ? 0x90 : 0) | (m_adc ? 0x40 : 0) | (m_display_on ? 0 : 0x20);
}

void st7565r_device::advance_column()
{
	if (m_column < COLUMNS - 1)
		++m_column;
}

u8 st7565r_device::data_r()
{
	const u8 result = m_read_latch;
	if (machine().side_effects_disabled() || m_reset)
		return result;

	if (m_page < PAGES && m_column < COLUMNS)
	{
		// Dummy reads prime the bus holder at the selected column. Later
		// normal reads advance, while read/modify/write reads hold the column.
		if (!m_read_pending && !m_modify_write)
			advance_column();
		m_read_latch = m_ddram[m_page][m_column];
	}
	else
	{
		// Out-of-range addresses must not alias valid display RAM.
		m_read_latch = 0xff;
	}
	m_read_pending = false;
	return result;
}

void st7565r_device::data_w(u8 data)
{
	if (m_reset)
		return;
	m_read_latch = data;
	m_read_pending = true;
	if (m_page < PAGES && m_column < COLUMNS)
	{
		m_ddram[m_page][m_column] = m_page == 8 ? data & 1 : data;
		advance_column();
	}
}

void st7565r_device::control_w(u8 data)
{
	if (m_reset)
		return;

	// Parameter bytes are never decoded as independent commands.
	if (m_pending_command)
	{
		switch (m_pending_command)
		{
		case 0x81: m_volume = data & 0x3f; break;
		case 0xad: m_indicator_mode = data & 3; break;
		case 0xf8: m_booster_ratio = data & 3; break;
		}
		m_pending_command = 0;
	}
	else if ((data & 0xfe) == 0xae)
		m_display_on = BIT(data, 0);
	else if ((data & 0xc0) == 0x40)
		m_start_line = data & 0x3f;
	else if ((data & 0xf0) == 0xb0)
	{
		m_page = data & 0x0f;
		m_read_pending = true;
	}
	else if ((data & 0xe0) == 0x00)
	{
		// Column-address commands are not permitted in read/modify/write mode.
		if (!m_modify_write)
		{
			if (BIT(data, 4))
				m_column = (m_column & 0x0f) | ((data & 0x0f) << 4);
			else
				m_column = (m_column & 0xf0) | (data & 0x0f);
			m_read_pending = true;
		}
	}
	else if ((data & 0xfe) == 0xa0)
		m_adc = BIT(data, 0);
	else if ((data & 0xfe) == 0xa2)
		m_bias = BIT(data, 0);
	else if ((data & 0xfe) == 0xa4)
		m_all_points = BIT(data, 0);
	else if ((data & 0xfe) == 0xa6)
		m_reverse = BIT(data, 0);
	else if ((data & 0xf0) == 0xc0)
		m_com_reverse = BIT(data, 3);
	else if ((data & 0xf8) == 0x20)
		m_resistor_ratio = data & 7;
	else if ((data & 0xf8) == 0x28)
		m_power = data & 7;
	else if (data == 0x81 || data == 0xf8)
		m_pending_command = data;
	else if ((data & 0xfe) == 0xac)
	{
		m_indicator = BIT(data, 0);
		if (m_indicator)
			m_pending_command = data;
	}
	else if (data == 0xe0)
	{
		m_saved_column = m_column;
		m_modify_write = true;
	}
	else if (data == 0xee)
	{
		m_column = m_saved_column;
		m_modify_write = false;
		m_read_pending = true;
	}
	else if (data == 0xe2)
		reset_registers(false);
	else if (data != 0xe3)
		logerror("%s: unsupported ST7565R command %02x\n", machine().describe_context(), data);
}

u8 st7565r_device::pixel_r(unsigned segment, unsigned common) const
{
	if (segment >= COLUMNS || common >= m_duty || !m_display_on || m_reset)
		return 0;
	if (m_all_points)
		return 1;

	const unsigned column = m_adc ? COLUMNS - 1 - segment : segment;
	unsigned line = 64; // The last common is the separate icon row.
	if (common < m_duty - 1)
	{
		const unsigned scan = m_com_reverse ? m_duty - 2 - common : common;
		line = (scan + m_start_line) & 0x3f;
	}
	return BIT(m_ddram[line >> 3][column], line & 7) ^ m_reverse;
}

u32 st7565r_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	for (int y = cliprect.min_y; y <= cliprect.max_y; ++y)
		for (int x = cliprect.min_x; x <= cliprect.max_x; ++x)
			bitmap.pix(y, x) = pixel_r(x, y);
	return 0;
}
