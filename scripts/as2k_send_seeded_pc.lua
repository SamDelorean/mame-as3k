-- Send a pre-seeded document from genuine stock PC service state.
local cpu=assert(manager.machine.devices[':maincpu']); local send=assert(manager.machine.ioport.ports[':COL.7']:field(0x10))
local ready,frames,pressed,released=false,0,false,false
local function mark(s) manager.machine:logerror('AS2K_SEEDED_SEND '..s..'\n') end
emu.register_frame_done(function()
 local pc=cpu.state['PC'].value
 if not ready and pc==0x80e5 then ready=true; mark('PC_KEYBOARD_READY') end
 if not ready then return end
 frames=frames+1
 if frames==20 then send:set_value(1); pressed=true; mark('SEND_PRESS') end
 if pressed and not released and frames==25 then send:set_value(0); send:clear_value(); released=true; mark('SEND_RELEASE') end
 if released and frames>=240 then mark('COMPLETE'); manager.machine:exit() end
end)
