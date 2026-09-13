-- license:BSD-3-Clause
-- v3.1.4 diagnostic bus regression; fresh NVRAM for write, same NVRAM for recall.
local phase = assert(os.getenv('AS2K_FILE_PHASE'))
assert(phase == 'write' or phase == 'recall')
local tokens = {'file1aa', 'file2bb', 'file3cc', 'file4dd',
                'file5ee', 'file6ff', 'file7gg', 'file8hh'}
local cpu = assert(manager.machine.devices[':maincpu'])
local keyboard = manager.machine.natkeyboard
local steps = {}
local function key(s) steps[#steps + 1] = {key = s} end
if phase == 'write' then
    for i = 1, 8 do
        key(string.format('{F%d}', i))
        key(tokens[i])
    end
end
-- Every observation follows a switch away and back, including F8 after typing.
for i = 1, 8 do
    key(string.format('{F%d}', i % 8 + 1))
    key(string.format('{F%d}', i))
    steps[#steps + 1] = {file = i}
end
local step, idle = 1, 0
emu.register_frame_done(function()
    if cpu.state['PC'].value == 0x87d7 and keyboard.empty then
        idle = idle + 1
    else idle = 0 end
    if idle < 60 then return end
    idle = 0
    local action = steps[step]
    if not action then
        manager.machine:logerror('AS2KFILES complete ' .. phase)
        print('AS2KFILES complete ' .. phase)
        manager.machine:exit()
    elseif action.key then
        keyboard:post_coded(action.key)
    else
        -- Same ordered error-log stream as the existing LCD bus trace.
        manager.machine:logerror(string.format('AS2KFILES observe %s F%d', phase, action.file))
    end
    step = step + 1
end, 'frame')
