-- Validate integrated PC Connected + send.txt sink.
local cpu=assert(manager.machine.devices[':maincpu'])
local send=assert(manager.machine.ioport.ports[':COL.7']:field(0x10))
local pc=assert(manager.machine.ioport.ports[':PC_CONNECTED']:field(0x01))
local function mark(s) manager.machine:logerror('AS2K_INTEGRATED '..s..'\n') end
pc.user_value=1
mark(string.format('PC_FIELD_SET user_value=%d', pc.user_value))
local ready,frames,pressed,released=false,0,false,false
emu.register_frame_done(function()
 local addr=cpu.state['PC'].value
 if not ready and addr==0x80e5 then ready=true; frames=0; mark('PC_KEYBOARD_READY') end
 if not ready then return end
 frames=frames+1
 if frames==20 then send:set_value(1); pressed=true; mark('SEND_PRESS') end
 if pressed and not released and frames==25 then send:set_value(0); send:clear_value(); released=true; mark('SEND_RELEASE') end
 if released and frames>=120 then mark('COMPLETE'); manager.machine:exit() end
end)
