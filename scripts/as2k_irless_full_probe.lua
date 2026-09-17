-- license:BSD-3-Clause
-- Derived Gate 1A ROM full Send/Print stimulus. Input only.
local cpu = assert(manager.machine.devices[':maincpu'])
local keyboard = assert(manager.machine.natkeyboard)
local send = assert(manager.machine.ioport.ports[':COL.7']:field(0x10))
local print_key = assert(manager.machine.ioport.ports[':COL.9']:field(0x10))
assert(send.name == 'Send' and print_key.name == 'Print')
local idle, stage, frames = 0, 0, 0
local function mark(s) manager.machine:logerror('AS2K_IRLESS_FULL ' .. s .. '\n') end
local function press(field, name)
    mark('PRESS ' .. name); field:set_value(1); frames = 0
end
mark('START')
emu.register_frame_done(function()
    if stage == 2 or stage == 4 then
        frames = frames + 1
        if frames == 5 then
            local field = stage == 2 and send or print_key
            local name = stage == 2 and 'Send' or 'Print'
            field:set_value(0); field:clear_value(); mark('RELEASE ' .. name)
            stage = stage + 1; frames = 0
        end
        return
    end
    if stage == 3 or stage == 5 then
        frames = frames + 1
        if frames >= 120 then
            if stage == 3 then press(print_key, 'Print'); stage = 4
            else mark('COMPLETE'); manager.machine:exit(); stage = 6 end
        end
        return
    end
    if cpu.state['PC'].value == 0x87D7 and keyboard.empty then idle = idle + 1 else idle = 0 end
    if idle < 60 then return end
    idle = 0
    if stage == 0 then mark('EDITOR_READY'); keyboard:post_coded('a'); stage = 1
    elseif stage == 1 then press(send, 'Send'); stage = 2 end
end)
