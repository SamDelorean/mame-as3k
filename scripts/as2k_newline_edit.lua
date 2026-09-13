-- license:BSD-3-Clause
-- v3.1.4: fresh private NVRAM for write, persisted NVRAM for recall.
local phase = assert(os.getenv('AS2K_NEWLINE_PHASE'))
assert(phase == 'write' or phase == 'recall'
    or phase == 'traverse' or phase == 'traverse_recall'
    or phase == 'vertical' or phase == 'vertical_recall'
    or phase == 'three' or phase == 'three_recall'
    or phase == 'boundary' or phase == 'boundary_recall')
local cpu = assert(manager.machine.devices[':maincpu'])
local keyboard = manager.machine.natkeyboard
local steps = {}
local function key(s) steps[#steps + 1] = {key = s} end
local function physical(port, mask, name)
    steps[#steps + 1] = {port = port, mask = mask, name = name}
end
local function observe(s) steps[#steps + 1] = {observe = s} end
key('{F1}')
if phase == 'three' or phase == 'boundary' then
    key('ab')
    physical(':COL.9', 0x40, 'Return')
    observe('newline1')
    key('cd')
    physical(':COL.9', 0x40, 'Return')
    observe('newline2')
    key('ef')
    observe('final')
    if phase == 'boundary' then
        -- Proposed firmware behavior across controllers; await LOCAL evidence.
        physical(':COL.5', 0x80) -- Left
        physical(':COL.5', 0x80) -- cd<CR>|ef
        physical(':COL.9', 0x01, 'Delete') -- Backspace: cd|ef
        observe('join')
        physical(':COL.9', 0x40, 'Return')
        observe('resplit')
    end
elseif phase == 'write' or phase == 'traverse' or phase == 'vertical' then
    key('abcd')
    observe('original')
    -- Use the asma2k input block, not the AlphaSmart Pro block.
    physical(':COL.5', 0x80)
    physical(':COL.5', 0x80) -- ab|cd
    physical(':COL.9', 0x40, 'Return') -- ab / |cd
    observe('split')
    if phase == 'vertical' then
        -- Proposed firmware behavior; no wrapping, scrolling or clamping.
        physical(':COL.7', 0x80) -- Up: expected |ab<CR>cd
        key('x')
        observe('up_insert')
        physical(':COL.5', 0x02) -- Down: expected xab<CR>c|d
        key('y')
        observe('down_insert')
    elseif phase == 'traverse' then
        -- Boundary expectations, not independently verified physical behavior.
        physical(':COL.5', 0x80) -- expected ab|<CR>cd
        key('x')
        observe('left_insert')
        physical(':COL.6', 0x80) -- expected abx<CR>|cd
        key('y')
        observe('right_insert')
    else
        physical(':COL.9', 0x01, 'Delete') -- Backspace joins: ab|cd
        observe('join')
        physical(':COL.9', 0x40, 'Return')
        observe('resplit')
    end
else
    observe('restart')
end
key('{F2}')
key('{F1}')
observe('switch')
local step, idle = 1, 0
local held, frames
emu.register_frame_done(function()
    if held then
        frames = frames + 1
        if frames == 5 then
            held:set_value(0)
            held:clear_value()
            held = nil
        end
        return
    end
    if cpu.state['PC'].value == 0x87d7 and keyboard.empty then
        idle = idle + 1
    else idle = 0 end
    if idle < 60 then return end
    idle = 0
    local action = steps[step]
    if not action then
        manager.machine:logerror('AS2KNEWLINE complete ' .. phase)
        print('AS2KNEWLINE complete ' .. phase)
        manager.machine:exit()
    elseif action.key then
        keyboard:post_coded(action.key)
    elseif action.port then
        held = assert(manager.machine.ioport.ports[action.port]:field(action.mask))
        assert(held.mask == action.mask and #held:keyboard_codes(0) > 0
            and (not action.name or held.name == action.name),
            'unexpected AS2000 editing key field')
        held:set_value(1)
        frames = 0
    else
        manager.machine:logerror('AS2KNEWLINE observe ' .. phase .. ' ' .. action.observe)
    end
    step = step + 1
end, 'frame')
