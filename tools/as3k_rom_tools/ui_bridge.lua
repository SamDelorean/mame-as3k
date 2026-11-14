-- SPDX-License-Identifier: BSD-3-Clause
-- Physical input/UI bridge only: never writes CPU memory, PC, ROM or applet state.
-- Paths are passed by the Python frontend, and all LCD pixels come from MAME.
if AS3K_UI_BRIDGE_ACTIVE then return end
AS3K_UI_BRIDGE_ACTIVE=true
local m=manager.machine
local control=os.getenv("AS3K_UI_CONTROL_FILE")
local events=os.getenv("AS3K_UI_EVENT_FILE")
local frame=os.getenv("AS3K_UI_FRAME_FILE")
local baud=m.ioport.ports[":rs232:printer:RS232_RXBAUD"]
if baud then
 local f=baud:field(1)
 for value,label in pairs(f.settings) do if label=="57600" then f.user_value=value end end
end
local position=0
local last_frame=-1
local releases={}
local fields={}
for _,port in pairs(m.ioport.ports) do
 for name,f in pairs(port.fields) do fields[name]=f end
end
local aliases={ESC="Escape",BACKSPACE="Delete",CAPSLOCK="Caps Lock",
 LSHIFT="Left Shift",RSHIFT="Right Shift",LCTRL="Control",LALT="Left Option",
 RALT="Right Option",COMMAND="Command",GRAVE="Backtick",EQUALS="Equals",
 SEMICOLON="Semicolon",QUOTE="Quote",BACKSLASH="Backslash",PERIOD="Period",
 SPELL="Spell Check",CLEAR="Clear File",RETURN="Return",ENTER="Enter"}
local function event(line)
 local f=io.open(events,"a");if f then f:write(line.."\n");f:close() end
end
local function key(name,on)
 local label=aliases[name] or (name:sub(1,1)..name:sub(2):lower())
 if name:match("^F%d+$") or #name==1 then label=name end
 local field=fields[label]
 if field then field:set_value(on and 1 or 0)
 else event("STATUS\tTecla no disponible en AS3000: "..label) end
end
emu.register_periodic(function()
 local now=m.time:as_double()
 for name,deadline in pairs(releases) do
  if now>=deadline then key(name,false);releases[name]=nil end
 end
 local f=io.open(control,"r")
 if f then
  f:seek("set",position)
  for line in f:lines() do
   local action,arg=line:match("^(%S+)%s+(.+)$")
   if action=="KEYDOWN" then key(arg,true)
   elseif action=="KEYUP" then key(arg,false)
   elseif action=="KEY" then key(arg,true);releases[arg]=now+0.12
   elseif action=="PC" then
    m.ioport.ports[":HOST"]:field(3).user_value=(arg=="ON") and 1 or 0
    event("STATUS\tPS/2 "..arg)
   elseif line=="MACHINE RESET" then m:soft_reset()
   elseif line=="EXIT" then m:exit()
   end
  end
  position=f:seek();f:close()
 end
 if now-last_frame>=0.1 then
  last_frame=now
  m.screens[":screen"]:snapshot(frame..".part.png")
  os.rename(frame..".part.png",frame)
 end
end)
event("STATUS\tAS3K conectado: LCD nativo, teclado físico y ROM seleccionada")
