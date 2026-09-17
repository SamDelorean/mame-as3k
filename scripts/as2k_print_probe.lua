-- license:BSD-3-Clause
-- Stock v3.1.4 Print routing probe. Input only; never changes CPU or memory state.
local cpu = assert(manager.machine.devices[':maincpu'])
local keyboard = assert(manager.machine.natkeyboard)
local port = assert(manager.machine.ioport.ports[':COL.9'])
local print_key = assert(port:field(0x10))
assert(print_key.mask == 0x10 and print_key.name == 'Print'
    and #print_key:keyboard_codes(0) > 0, 'unexpected Print matrix field')
local idle, stage, frames = 0, 0, 0
local function mark(message)
    manager.machine:logerror('AS2K_PRINT_PROBE ' .. message .. '\n')
end
mark('FIELD port=:COL.9 mask=10 name=Print')
emu.register_frame_done(function()
    if stage >= 2 then
        frames = frames + 1
        if stage == 2 and frames == 5 then
            print_key:set_value(0)
            print_key:clear_value()
            mark('RELEASE')
            stage, frames = 3, 0
        elseif stage == 3 and frames == 120 then
            mark('COMPLETE')
            manager.machine:exit()
            stage = 4
        end
        return
    end
    if cpu.state['PC'].value == 0x87D7 and keyboard.empty then idle = idle + 1 else idle = 0 end
    if idle < 60 then return end
    idle = 0
    if stage == 0 then
        mark('READY')
        keyboard:post_coded('a')
        stage = 1
    else
        mark('PRESS after_typing_idle')
        print_key:set_value(1)
        stage, frames = 2, 0
    end
end)
