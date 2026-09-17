-- Seed deterministic document text through the normal emulated keyboard.
local cpu=assert(manager.machine.devices[':maincpu']); local kb=assert(manager.machine.natkeyboard)
local stage,idle=0,0
local function mark(s) manager.machine:logerror('AS2K_SEED '..s..'\n') end
emu.register_frame_done(function()
 local pc=cpu.state['PC'].value
 if pc==0x87D7 and kb.empty then idle=idle+1 else idle=0 end
 if idle<45 then return end
 idle=0
 if stage==0 then mark('EDITOR_READY'); kb:post_coded('abc 123'); stage=1
 elseif stage==1 then mark('TEXT_ACCEPTED abc_123'); manager.machine:exit(); stage=2 end
end)
