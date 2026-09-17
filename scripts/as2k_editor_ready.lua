-- Observe the established v3.1.4 editor idle criterion without CPU injection.
local cpu = manager.machine.devices[':maincpu']
assert(cpu, 'AS2000 main CPU not found')
local keyboard = manager.machine.natkeyboard
assert(keyboard, 'AS2000 natural keyboard not found')

local stable = 0
local done = false
emu.register_frame_done(function()
    if done then return end
    if cpu.state['PC'].value == 0x87D7 and keyboard.empty then
        stable = stable + 1
    else
        stable = 0
    end
    if stable >= 60 then
        done = true
        manager.machine:logerror(
            'AS2K_GATE1A EDITOR_READY PC=87D7 stable_frames=60\n')
        manager.machine:exit()
    end
end)
