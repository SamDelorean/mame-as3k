-- license:BSD-3-Clause
-- Stock v3.1.4, fresh NVRAM. Input only; never changes CPU or memory state.
local cpu = assert(manager.machine.devices[':maincpu'])
local keyboard = assert(manager.machine.natkeyboard)
local port = assert(manager.machine.ioport.ports[':COL.7'])
local send = assert(port:field(0x10))
assert(send.mask == 0x10 and send.name == 'Send'
    and #send:keyboard_codes(0) > 0, 'unexpected Send matrix field')
local idle, stage, frames = 0, 0, 0
local host_reads, host_high = 0, 0
-- Observe actual CPU reads only; returning nil preserves the original data.
-- Stock D0 PORTA is at $0000. PA0/PA2 are documented wired-host senses.
local host_tap = cpu.spaces['program']:install_read_tap(0, 0, 'send_host_sense',
    function(offset, data, mask)
        if stage == 2 or stage == 3 then
            host_reads = host_reads + 1
            host_high = host_high | (data & 0x05)
        end
    end)
local function mark(message)
    manager.machine:logerror('AS2K_SEND_PROBE ' .. message .. '\n')
end
mark('FIELD port=:COL.7 mask=10 name=Send')
emu.register_frame_done(function()
    if stage >= 2 then
        frames = frames + 1
        if stage == 2 and frames == 5 then
            send:set_value(0)
            send:clear_value()
            mark('RELEASE')
            stage, frames = 3, 0
        elseif stage == 3 and frames == 120 then
            mark(string.format('HOST_SENSE reads=%d high_mask=%02X', host_reads, host_high))
            host_tap:remove()
            mark('COMPLETE')
            manager.machine:exit()
            stage = 4
        end
        return
    end
    if cpu.state['PC'].value == 0x87D7 and keyboard.empty then
        idle = idle + 1
    else idle = 0 end
    if idle < 60 then return end
    idle = 0
    if stage == 0 then
        mark('READY')
        keyboard:post_coded('a')
        stage = 1
    else
        mark('PRESS after_typing_idle')
        send:set_value(1)
        stage, frames = 2, 0
    end
end)
