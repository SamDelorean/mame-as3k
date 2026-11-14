# SPDX-License-Identifier: BSD-3-Clause
"""AS3K Stable/DIAG share the unmodified AS2K GTK shell and one MAME executable."""
from __future__ import annotations
import argparse
import fcntl
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import uuid
from . import backend as b
from .as2k_ui_reference import AS2KWindow, Gtk, GLib, GdkPixbuf
from gi.repository import Gio


class AS3KWindow(AS2KWindow):
    def __init__(self,app,args):
        self.args=args
        self.core=None
        self.core_watch=None
        self.lock=None
        self.console=None
        self.builder_children={}
        self.test_pid=None
        self.test_watch=None
        self.closing=False
        self.bridge=tempfile.TemporaryDirectory(prefix="as3k-ui-")
        runtime=Path(self.bridge.name)
        self.settings=Path(os.environ.get("XDG_CONFIG_HOME",str(Path.home()/".config")))/"as3k"/"ui.json"
        settings=json.loads(self.settings.read_text()) if self.settings.exists() else {}
        self.active_rom=Path(args.rom or settings.get("rom",b.ROOT.parent/"private/roms/as3k/mame_sets/asma3ksw/os3k_min_writer.bin")).resolve()
        self.active_machine=args.machine
        for key,file in [("CONTROL","control.queue"),("EVENT","events.queue"),("FRAME","lcd.png")]:
            os.environ["AS2K_UI_"+key+"_FILE"]=str(runtime/file)
        os.environ["AS2K_WORKBENCH"]="1" if args.profile=="diag" else "0"
        super().__init__(app)
        self.set_title("AS3K-DIAG" if self.workbench else "AS3K Stable")
        self.set_default_size(1000,300)
        self.set_size_request(700,250)
        self.firmware_path=self.active_rom
        self.print_position=0
        self.print_stream=None
        self.capture_file=None
        self.capture_size=0
        self._update_header()
        if not args.no_start:
            try:
                self.start_core()
            except (OSError,ValueError) as exc:
                self.status.set_text(str(exc))

    def _build_ui(self):
        super()._build_ui()
        self.get_child().get_style_context().add_class("background")
        frame=self.lcd_area.get_parent()
        frame.remove(self.lcd_area)
        self.lcd_area=Gtk.Image.new_from_pixbuf(self.lcd_pixbuf)
        frame.add(self.lcd_area)
        self._set_scale(1.0)

    def _set_scale(self,scale):
        self.ui_scale=scale
        if hasattr(self.lcd_area,"set_from_pixbuf"):
            self.lcd_area.set_from_pixbuf(self.lcd_pixbuf.scale_simple(
                int(240*scale),int(36*scale),GdkPixbuf.InterpType.NEAREST))
        self.lcd_area.set_size_request(int(240*scale),int(36*scale))
        self.queue_resize()

    def _build_menu_bar(self):
        bar=super()._build_menu_bar()
        # Keep AS2K menu order, entries, screenshots, recording, view and dialogs.
        root=Gtk.MenuItem(label="Herramientas")
        menu=Gtk.Menu()
        root.set_submenu(menu)
        self.tool_items={}
        for label,kind in [("Constructor de ROM…","builder"),("Probar applet…","test")]:
            self.tool_items[kind]=self._menu_item(menu,label,lambda _i,k=kind:self.launch_tool(k))
        bar.insert(root,3)
        file_menu=bar.get_children()[0].get_submenu()
        for item in file_menu.get_children():
            if isinstance(item,Gtk.MenuItem) and item.get_label()=="Abrir DictROM…":
                item.set_sensitive(False)
                item.set_tooltip_text("AS3000 integra diccionarios como SmartApplets; no tiene DictROM externa.")
        self.ir_item.set_sensitive(False)
        self.ir_item.set_tooltip_text("IrDA AS3K permanece OPEN; esta intervención no lo acredita.")
        return bar

    def _build_payload_menu(self):
        # The AS2K payload binaries target another CPU. Retain its menu entries but
        # explicitly prevent loading them into the DragonBall via a misleading selector.
        menu=super()._build_payload_menu()
        for item in menu.get_submenu().get_children():
            if isinstance(item,Gtk.MenuItem) and item.get_label()!="STOCK":
                item.set_sensitive(False)
                item.set_tooltip_text("Payload AS2K no portable. Bootstrap AS3K: scripts/as3k_bootstrap_test.py.")
        return menu

    def _update_header(self):
        if not hasattr(self,"header"):
            return
        profile="AS3K-DIAG" if self.workbench else "AS3K Stable"
        self.header.set_text(profile+" — "+self.active_rom.name+
                            (" | REC" if self.recording else ""))

    def _poll_events(self):
        if self.closing:
            return False
        return super()._poll_events()

    def _poll_frame_file(self):
        if self.closing:
            return False
        try:
            st=self.frame_file.stat()
            if st.st_mtime_ns!=self.frame_mtime_ns:
                pixbuf=GdkPixbuf.Pixbuf.new_from_file(str(self.frame_file))
                if (pixbuf.get_width(),pixbuf.get_height()) != (240,36):
                    raise ValueError("Tamaño LCD nativo inesperado")
                self.lcd_pixbuf=pixbuf
                self.frame_mtime_ns=st.st_mtime_ns
                self._set_scale(self.ui_scale)
            if self.capture_file and self.pc_connected and self.capture_file.exists():
                data=self.capture_file.read_bytes()
                if len(data)!=self.capture_size:
                    self.send_capture=bytearray(data)
                    self.capture_size=len(data)
            self.read_print_jobs()
        except (OSError,GLib.Error,ValueError) as exc:
            if not isinstance(exc,FileNotFoundError):
                self.status.set_text(str(exc))
        return True

    def _rgb_bytes(self):
        raw=bytes(self.lcd_pixbuf.get_pixels())
        stride=self.lcd_pixbuf.get_rowstride()
        channels=self.lcd_pixbuf.get_n_channels()
        return b"".join(raw[y*stride+x*channels:y*stride+x*channels+3]
                        for y in range(36) for x in range(240))

    def _choose_firmware(self,*_):
        path=self._choose_file("Abrir ROM AS3K")
        if path:
            try:
                b.Base.read(path)
                self.stop_core()
                self.active_rom=path.resolve()
                self.firmware_path=self.active_rom
                self.settings.parent.mkdir(parents=True,exist_ok=True)
                tmp=self.settings.with_name("ui-"+uuid.uuid4().hex+".json")
                tmp.write_bytes(b.json_bytes(dict(rom=str(self.active_rom))))
                os.replace(tmp,self.settings)
                self.start_core()
                self._update_header()
            except (OSError,ValueError) as exc:
                self._error("No se pudo abrir la ROM",str(exc))

    def _toggle_pc(self,item):
        connected=item.get_active()
        if not connected and self.send_capture:
            self._save_send_capture()
        super()._toggle_pc(item)

    def _toggle_printer(self,item):
        super()._toggle_printer(item)
        if self.core:
            try:
                self.stop_core()
                self.start_core()
                self.status.set_text("Impresora serial "+("conectada" if self.printer_connected else "desconectada")+"; sesión reiniciada.")
            except (OSError,ValueError) as exc:
                self._error("Puerto serial",str(exc))

    def start_core(self):
        if self.core:
            return
        base=b.Base.read(self.active_rom)
        data=Path(os.environ.get("XDG_DATA_HOME",str(Path.home()/".local/share")))
        state=data/"as3k"/"roms"/b.sha(base.data)/self.active_machine
        state.mkdir(parents=True,exist_ok=True)
        lock=(state/"session.lock").open("a")
        try:
            fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except BlockingIOError:
            lock.close()
            raise b.FormatError("Esta ROM ya tiene una sesión abierta")
        self.lock=lock
        self.state=state
        self.capture_file=state/"nvram"/"send.txt"
        self.capture_size=self.capture_file.stat().st_size if self.capture_file.exists() else 0
        self.control_file.write_text("")
        command=b.emulator_command(self.active_rom,state,self.active_machine,self.args.rompath,
                    headless=True,observer=Path(__file__).with_name("ui_bridge.lua"))
        command.remove("-nothrottle")
        command.append("-throttle")
        env=b.runtime_environment()
        env.update(SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy",
                   AS3K_UI_CONTROL_FILE=str(self.control_file),
                   AS3K_UI_EVENT_FILE=str(self.event_file),AS3K_UI_FRAME_FILE=str(self.frame_file))
        if self.printer_connected:
            folder=state/"prints"/uuid.uuid4().hex
            folder.mkdir(parents=True)
            self.print_stream=folder/"serial.pcl"
            self.print_position=0
            command.extend(["-rs232","printer","-printout",str(self.print_stream)])
        else:
            self.print_stream=None
        self.console=(state/("ui-"+uuid.uuid4().hex+".log")).open("xb")
        try:
            self.core=subprocess.Popen(command,cwd=state,env=env,stdout=self.console,stderr=subprocess.STDOUT)
        except BaseException:
            self.console.close()
            self.lock.close()
            self.lock=None
            raise
        self.core_watch=GLib.child_watch_add(self.core.pid,self.core_finished)
        if self.pc_connected:
            self._send_control("PC ON")

    def core_finished(self,pid,status):
        if self.core:
            self.core.returncode=os.waitstatus_to_exitcode(status)
        self.core=None
        self.core_watch=None
        if self.console:
            self.console.close()
            self.console=None
        if self.lock:
            self.lock.close()
            self.lock=None
        self.status.set_text("Emulador cerrado; exit "+str(os.waitstatus_to_exitcode(status)))

    def stop_core(self):
        if not self.core:
            return
        process=self.core
        if self.core_watch:
            GLib.source_remove(self.core_watch)
            self.core_watch=None
        self._send_control("EXIT")
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.terminate()
            process.wait(timeout=5)
        self.core_finished(process.pid,process.returncode<<8)

    def read_print_jobs(self):
        if not self.print_stream or not self.print_stream.exists():
            return
        data=self.print_stream.read_bytes()
        end=data.find(b"\x1b&l0H\x1bE",self.print_position)
        if end<0:
            return
        end+=7
        raw=data[self.print_position:end]
        self.print_position=end
        job=self.print_stream.with_name("job-"+uuid.uuid4().hex+".pcl")
        b.create_file(job,raw)
        output=job.with_suffix(".cups.log")
        # Reuse the installed AS3K adapter already validated against CUPS.
        with output.open("xb") as stream:
            proc=subprocess.Popen([sys.executable,str(b.ROOT/"scripts/as3k_print_to_host.py"),str(job)],
                        stdout=stream,stderr=subprocess.STDOUT)
        GLib.child_watch_add(proc.pid,lambda pid,status:self.status.set_text(
            "CUPS: "+("trabajo enviado" if os.waitstatus_to_exitcode(status)==0 else "error; revise "+str(output))))

    def tool_command(self,kind):
        exe="as3k-rom-builder" if kind=="builder" else "as3k-applet-test"
        return [sys.executable,str(b.ROOT/"scripts"/exe),"--gui","--base",str(self.active_rom),
                "--machine",self.active_machine]

    def launch_tool(self,kind):
        try:
            command=self.tool_command(kind)
            if kind=="test":
                if self.test_pid:
                    return
                self.stop_core()
            pid,_,_,_=GLib.spawn_async(command,flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD)
            if kind=="test":
                self.test_pid=pid
                self.test_watch=GLib.child_watch_add(pid,self.test_finished)
            else:
                self.builder_children[pid]=GLib.child_watch_add(pid,self.builder_finished)
        except (OSError,ValueError,GLib.Error) as exc:
            self._error("No se pudo abrir la herramienta",str(exc))

    def builder_finished(self,pid,status):
        GLib.spawn_close_pid(pid)
        self.builder_children.pop(pid,None)

    def test_finished(self,pid,status):
        GLib.spawn_close_pid(pid)
        self.test_pid=None
        self.test_watch=None
        if self.closing:
            return
        try:
            self.start_core()
            self.status.set_text("Prueba cerrada; restaurada la ROM y persistencia originales.")
        except (OSError,ValueError) as exc:
            self._error("Restaurar sesión",str(exc))

    def _show_paths(self,*_):
        self._error("Rutas AS3K",f"ROM: {self.active_rom}\nEstado: {getattr(self,'state','sin sesión')}\n"
                    f"Aplicaciones: {b.ROOT/'scripts'}\nLCD nativo PNG: {self.frame_file}")

    def _show_shortcuts(self,*_):
        self._error("Atajos AS3K","F1–F8: archivos; F9: Print; F10: Spell; F11: Find; F12: Send.\n"
                    "Teclas virtuales y teclado anfitrión actúan sobre la matriz AS3000.\n"
                    "Command+Esc: menú de applets (firmware original).")

    def _show_about(self,*_):
        dialog=Gtk.AboutDialog(transient_for=self,modal=True)
        dialog.set_program_name("AS3K-DIAG" if self.workbench else "AS3K Stable")
        dialog.set_comments("Interfaz GTK de AS2K reutilizada; único núcleo MAME alphasma3k.\n"
                            "Constructor de ROM y prueba temporal comparten el backend System 3.")
        dialog.run()
        dialog.destroy()

    def _on_delete_event(self,*args):
        if super()._on_delete_event(*args):
            return True
        self.closing=True
        if self.test_pid:
            # The testing GUI handles SIGTERM and removes its CLI/MAME session.
            if self.test_watch:
                GLib.source_remove(self.test_watch)
            os.kill(self.test_pid,signal.SIGTERM)
            os.waitpid(self.test_pid,0)
            GLib.spawn_close_pid(self.test_pid)
            self.test_pid=None
        self.stop_core()
        self.bridge.cleanup()
        return False


def main(argv=None):
    parser=argparse.ArgumentParser(description="Interfaz AS3K basada en AS2K; mismo alphasma3k")
    parser.add_argument("--profile",choices=["stable","diag"],default="stable")
    parser.add_argument("--rom",type=Path)
    parser.add_argument("--machine",choices=["asma3ksb","asma3ksw"],default="asma3ksb")
    parser.add_argument("--rompath")
    parser.add_argument("--no-start",action="store_true",help=argparse.SUPPRESS)
    args=parser.parse_args(argv)
    app=Gtk.Application(application_id="org.alphasmart.AS3K."+args.profile,
                        flags=Gio.ApplicationFlags.NON_UNIQUE)
    app.connect("activate",lambda app: AS3KWindow(app,args).show_all())
    return app.run([sys.argv[0]])
