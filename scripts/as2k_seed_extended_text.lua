-- Seed a deterministic mixed-case/punctuation document with PC disconnected.
local cpu=assert(manager.machine.devices[':maincpu'])
local kb=assert(manager.machine.natkeyboard)
local pc=assert(manager.machine.ioport.ports[':PC_CONNECTED']:field(0x01))
local sample=[==[AbC [];',./-=\` !@#$%^&*() {}:"<>?_+|~]==] .. "\rNext\tX"
pc.user_value=0
local stage,idle=0,0
local function mark(s) manager.machine:logerror('AS2K_EXTENDED_SEED '..s..'\n') end
emu.register_frame_done(function()
 local addr=cpu.state['PC'].value
 if addr==0x87D7 and kb.empty then idle=idle+1 else idle=0 end
 if idle<45 then return end
 idle=0
 if stage==0 then mark('EDITOR_READY'); kb:post_coded(sample); stage=1
 elseif stage==1 then mark('TEXT_ACCEPTED'); manager.machine:exit(); stage=2 end
end)
