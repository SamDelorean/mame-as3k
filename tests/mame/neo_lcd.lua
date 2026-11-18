-- license:BSD-3-Clause
-- Neo LCD firmware regression (tested with the July 2013 system ROM).
-- Supply the asneo ROMs, then run neodiag asneo -video none -sound none
-- -nothrottle -seconds_to_run 30 -autoboot_script tests/mame/neo_lcd.lua
-- Use fresh -cfg_directory/-nvram_directory paths. Success prints PASS and
-- exits; an assertion failure prints FAIL and exits without running later steps.
local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local mem = cpu.spaces.program
local code, stack, source, capture = 0x70000, 0x78000, 0x71000, 0x72000
local width, height = 264, 64
local bytes = width * height // 8
local steps = {}
local expected = {}
local pattern = {}
for i = 0, bytes - 1 do
    -- Different values at both edges of each controller and on every page.
    pattern[i + 1] = (i * 17 + (i // width) * 37 + (i % width // 132) * 83) & 255
end

local function step(opcode, args, check)
    steps[#steps + 1] = {opcode, args, check}
end
local function expect_pixels()
    for i = 1, bytes do
        assert(mem:read_u8(capture + i - 1) == expected[i],
            string.format("pixel byte %d: got %02x, expected %02x", i,
                mem:read_u8(capture + i - 1), expected[i]))
    end
end
local function raster(operation, check)
    step(0xa04c, {0, 0, width, height, source, operation}, check)
end
local function check_capture()
    step(0xa04c, {0, 0, width, height, capture, 1}, expect_pixels)
end

step(0xa000, {})
step(0xa048, {0, 0, width, height, source}, function()
    for i = 1, bytes do expected[i] = pattern[i] end
end)
check_capture()
step(0xa028, {}) -- SaveScreen must discard the controller's dummy reads.
raster(5, function() for i = 1, bytes do expected[i] = 0 end end)
check_capture()
step(0xa02c, {}, function() for i = 1, bytes do expected[i] = pattern[i] end end)
check_capture()
-- The snapshot is reusable, and intervening LCD writes require new dummy reads.
raster(6, function() for i = 1, bytes do expected[i] = 255 end end)
check_capture()
step(0xa02c, {}, function() for i = 1, bytes do expected[i] = pattern[i] end end)
check_capture()
raster(2, function() for i = 1, bytes do expected[i] = pattern[i] ~ 255 end end)
check_capture()
raster(3, function() for i = 1, bytes do expected[i] = 255 end end)
check_capture()
raster(4, function() for i = 1, bytes do expected[i] = pattern[i] ~ 255 end end)
check_capture()
raster(7, function() for i = 1, bytes do expected[i] = 255 end end)
check_capture()

local frame, index, failed, passed = 0, 0, false, false
emu.register_frame_done(function()
    frame = frame + 1
    if frame < 120 or failed or passed then return end
    local phase = (frame - 120) % 16
    local ok, err = pcall(function()
        if phase == 0 then
            index = index + 1
            if not steps[index] then
                passed = true
                print("PASS Neo LCD snapshot, capture, drawing, and raster operations")
                machine:exit()
                return
            end
            if index == 1 then
                for i = 1, bytes do mem:write_u8(source + i - 1, pattern[i]) end
            end
            local current = steps[index]
            mem:write_u16(code, current[1])
            mem:write_u16(code + 2, 0x60fe)
            mem:write_u32(stack, code + 2)
            for i, value in ipairs(current[2]) do mem:write_u32(stack + i * 4, value) end
            cpu.state.SR.value = 0x2700
            cpu.state.SP.value = stack
            cpu.state.PC.value = code
        elseif phase == 12 then
            assert(cpu.state.PC.value == code + 2 or cpu.state.PC.value == code + 4,
                string.format("step %d has not returned (PC=%x)", index, cpu.state.PC.value))
            if steps[index][3] then steps[index][3]() end
        end
    end)
    if not ok then
        failed = true
        print("FAIL Neo LCD: " .. tostring(err))
        machine:exit()
    end
end)
