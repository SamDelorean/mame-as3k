// license:BSD-3-Clause
// copyright-holders:Kirk Meyer
/***************************************************************************

    Sitronix ST7565R LCD controller/driver (parallel MPU interface)

***************************************************************************/
#ifndef MAME_VIDEO_ST7565_H
#define MAME_VIDEO_ST7565_H

#pragma once

class st7565r_device : public device_t
{
public:
	st7565r_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// Duty is selected by package pins, not an MPU command.
	void set_duty(unsigned duty) { m_duty = duty; }

	u8 read(offs_t offset);
	void write(offs_t offset, u8 data);
	u8 status_r();
	u8 data_r();
	void control_w(u8 data);
	void data_w(u8 data);
	void reset_w(int state); // /RES, active low

	// Logical lit state at SEG/COM; panel wiring belongs to the host driver.
	u8 pixel_r(unsigned segment, unsigned common) const;
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
	static constexpr unsigned COLUMNS = 132;
	static constexpr unsigned PAGES = 9;
	void reset_registers(bool hardware);
	void advance_column();

	unsigned m_duty = 65;
	u8 m_ddram[PAGES][COLUMNS]{};
	u8 m_page = 0;
	u8 m_column = 0;
	u8 m_saved_column = 0;
	u8 m_start_line = 0;
	u8 m_read_latch = 0;
	u8 m_pending_command = 0;
	u8 m_power = 0;
	u8 m_resistor_ratio = 0;
	u8 m_volume = 0x20;
	u8 m_booster_ratio = 0;
	u8 m_indicator_mode = 0;
	bool m_read_pending = true;
	bool m_modify_write = false;
	bool m_display_on = false;
	bool m_adc = false;
	bool m_com_reverse = false;
	bool m_reverse = false;
	bool m_all_points = false;
	bool m_bias = false;
	bool m_indicator = false;
	bool m_reset = false;
};

DECLARE_DEVICE_TYPE(ST7565R, st7565r_device)

#endif // MAME_VIDEO_ST7565_H
