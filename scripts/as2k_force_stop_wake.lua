-- license:BSD-3-Clause
--
-- AS2000 firmware-level auto-off diagnostic for BIOS v3.1.4.
--
-- This deliberately does NOT emulate the physical low-power wake source.
-- The stock MC68HC11 core currently fails to resume STOP for a masked XIRQ.
-- To validate the firmware path independently, this script observes the STOP
-- instruction at $87D7 and resumes execution at $87D8, which is the behaviour
-- expected for a masked XIRQ wake according to the HC11 documentation.
--
-- A successful run reaches the final power-off loop at $87ED after the idle
-- counter at $004A reaches $94 (148 wake cycles).  Timing of register_periodic
-- is intentionally not interpreted as a hardware wake period.

local cpu = manager.machine.devices[':maincpu']
assert(cpu, 'AS2000 main CPU not found')
local program = cpu.spaces['program']
assert(program, 'AS2000 CPU program space not found')

local STOP_PC = 0x87D7
local RESUME_PC = 0x87D8
local POWEROFF_LOOP_PC = 0x87ED
local IDLE_COUNTER = 0x004A
local TARGET_COUNT = 0x94

local forced_wakes = 0
local started = false

emu.register_periodic(function()
    local t = manager.machine.time.seconds
    local pc = cpu.state['PC'].value
    local count = program:read_u8(IDLE_COUNTER)

    -- Give normal boot/initialisation time to finish before intervening.
    if not started and t >= 8.0 then
        started = true
        print(string.format('AS2KAUTOFF begin t=%.3f PC=%04X count=%02X', t, pc, count))
    end

    if started and pc == STOP_PC then
        cpu.state['PC'].value = RESUME_PC
        forced_wakes = forced_wakes + 1
        if forced_wakes <= 5 or (forced_wakes % 25) == 0 or count >= 0x90 then
            print(string.format('AS2KAUTOFF wake=%d count_before=%02X', forced_wakes, count))
        end
    end

    if pc == POWEROFF_LOOP_PC then
        print(string.format(
            'AS2KAUTOFF PASS t=%.3f wakes=%d count=%02X target=%02X PC=%04X',
            t, forced_wakes, count, TARGET_COUNT, pc))
        manager.machine:exit()
    end

    if t >= 240.0 then
        print(string.format(
            'AS2KAUTOFF TIMEOUT t=%.3f wakes=%d count=%02X PC=%04X',
            t, forced_wakes, count, pc))
        manager.machine:exit()
    end
end)
