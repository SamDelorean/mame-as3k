# SPDX-License-Identifier: BSD-3-Clause
"""Independent GTK3 applications; AS2K file-dialog and menu conventions."""
from __future__ import annotations
from datetime import datetime, timezone
from pathlib import Path
import json
import os
import signal
import sys
import uuid
import gi
gi.require_version("Gtk","3.0")
from gi.repository import Gtk, GLib
from . import backend as b


def choose(parent, title, *, save=False, folder=False, name=None):
    action = Gtk.FileChooserAction.SAVE if save else (
        Gtk.FileChooserAction.SELECT_FOLDER if folder else Gtk.FileChooserAction.OPEN)
    dialog = Gtk.FileChooserDialog(title=title, transient_for=parent, action=action)
    dialog.add_buttons(Gtk.STOCK_CANCEL,Gtk.ResponseType.CANCEL,
                       Gtk.STOCK_SAVE if save else Gtk.STOCK_OPEN,Gtk.ResponseType.OK)
    if name:
        dialog.set_current_name(name)
    path = Path(dialog.get_filename()) if dialog.run() == Gtk.ResponseType.OK else None
    dialog.destroy()
    return path


def error(parent, detail):
    dialog = Gtk.MessageDialog(transient_for=parent, modal=True,
                message_type=Gtk.MessageType.ERROR, buttons=Gtk.ButtonsType.CLOSE,
                text="No se pudo completar la operación")
    dialog.format_secondary_text(str(detail))
    dialog.run()
    dialog.destroy()


def guarded(method):
    def call(self, *args):
        try:
            return method(self, *args)
        except (OSError,ValueError,KeyError,IndexError) as exc:
            error(self,exc)
    return call


class ToolWindow(Gtk.Window):
    def __init__(self, kind, args):
        super().__init__(title="Constructor de ROM — AS3K" if kind=="builder" else "Probar applet — AS3K")
        self.kind, self.args = kind, args
        self.base = None
        self.base_path = None
        self.apps = []
        self.output = None
        self.pid, self.watch = None, None
        self.connect("delete-event",self.on_close)
        GLib.unix_signal_add(GLib.PRIORITY_DEFAULT,signal.SIGTERM,self.on_close)
        self.set_default_size(900,500 if kind=="builder" else 330)
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,spacing=10)
        box.set_border_width(12)
        box.get_style_context().add_class("background")
        self.add(box)
        bar = Gtk.MenuBar()
        root, menu = Gtk.MenuItem(label="Archivo"), Gtk.Menu()
        root.set_submenu(menu)
        bar.append(root)
        self.item(menu,"Abrir ROM base…",lambda *_: self.choose_base())
        if kind=="builder":
            self.item(menu,"Abrir proyecto…",self.open_project)
            self.item(menu,"Guardar proyecto…",self.save_project)
        self.item(menu,"Salir",lambda *_: self.close())
        box.pack_start(bar,False,False,0)
        self.base_label = Gtk.Label(label="ROM base: seleccione System 3 / Writer (1 MiB)",xalign=0)
        self.base_label.set_ellipsize(3)
        box.pack_start(self.base_label,False,False,0)
        controls = Gtk.Box(spacing=8)
        box.pack_start(controls,False,False,0)
        self.button(controls,"Abrir ROM base…",lambda *_: self.choose_base())
        if kind=="builder":
            self.button(controls,"Carpeta de applets…",self.choose_catalog)
            self.button(controls,"Añadir .OS3KApp…",self.choose_applet)
            self.button(controls,"Seleccionar todas",lambda *_: self.select_all(True))
            self.button(controls,"Deseleccionar todas",lambda *_: self.select_all(False))
            self.model = Gtk.ListStore(bool,str,str,str,int,str,int)
            self.tree = Gtk.TreeView(model=self.model)
            toggle = Gtk.CellRendererToggle()
            toggle.connect("toggled",self.toggled)
            self.tree.append_column(Gtk.TreeViewColumn("Incluir",toggle,active=0))
            for label,index in [("SmartApplet",1),("APPLET_ID",2),("Versión",3),("Bytes",4),("SHA-256",5)]:
                renderer=Gtk.CellRendererText()
                if index==5:
                    renderer.set_property("ellipsize",3)
                self.tree.append_column(Gtk.TreeViewColumn(label,renderer,text=index))
            scroll=Gtk.ScrolledWindow()
            scroll.add(self.tree)
            box.pack_start(scroll,True,True,0)
            order=Gtk.Box(spacing=8)
            self.button(order,"Subir",lambda *_: self.move(-1))
            self.button(order,"Bajar",lambda *_: self.move(1))
            self.button(order,"Guardar proyecto…",self.save_project)
            self.button(order,"Abrir proyecto…",self.open_project)
            box.pack_start(order,False,False,0)
        else:
            self.button(controls,"Seleccionar .OS3KApp…",self.choose_applet)
            self.detail=Gtk.Label(label="Una applet compilada por prueba.",xalign=0)
            self.detail.set_line_wrap(True)
            box.pack_start(self.detail,True,True,0)
        self.status=Gtk.Label(label="Los archivos originales se abren en solo lectura.",xalign=0)
        self.status.set_line_wrap(True)
        box.pack_start(self.status,False,False,0)
        bottom=Gtk.Box(spacing=8)
        box.pack_end(bottom,False,False,0)
        self.action=self.button(bottom,"Construir ROM…" if kind=="builder" else "Iniciar prueba temporal",self.perform)
        if kind=="builder":
            self.open_button=self.button(bottom,"Abrir ROM generada",self.open_output)
            self.open_button.set_sensitive(False)
        else:
            self.stop_button=self.button(bottom,"Cerrar prueba y descartar",self.stop)
            self.stop_button.set_sensitive(False)
        if args.base:
            self.set_base(args.base)
        for path in args.applet:
            self.add_applet(path)
        if kind=="builder" and getattr(args,"project",None):
            self.apply_project(args.project)

    @staticmethod
    def item(menu,label,callback):
        item=Gtk.MenuItem(label=label)
        item.connect("activate",callback)
        menu.append(item)
        return item

    @staticmethod
    def button(box,label,callback):
        button=Gtk.Button(label=label)
        button.connect("clicked",callback)
        box.pack_start(button,False,False,0)
        return button

    def selected(self):
        return [self.apps[row[6]] for row in self.model if row[0]] if self.kind=="builder" else list(self.apps)

    def set_base(self,path):
        base=b.Base.read(path)
        self.base,self.base_path=base,Path(path).resolve()
        self.base_label.set_text("ROM base: "+self.base_path.name)
        self.base_label.set_tooltip_text(str(self.base_path))
        if self.kind=="builder":
            self.apps=list(base.applets)
            self.refresh([True]*len(self.apps))
        self.status.set_text("Base validada: "+b.sha(base.data))

    @guarded
    def choose_base(self):
        path=choose(self,"Abrir ROM base AS3K")
        if path:
            self.set_base(path)

    def refresh(self,selected):
        self.model.clear()
        for index,(app,checked) in enumerate(zip(self.apps,selected)):
            self.model.append([checked,app.name,f"0x{app.id:04X}",app.version,
                               len(app.data),b.sha(app.data),index])

    def add_applet(self,path):
        app=b.Applet.read(path)
        if self.kind=="builder":
            if any(a.data==app.data for a in self.apps):
                return
            self.apps.append(app)
            self.model.append([True,app.name,f"0x{app.id:04X}",app.version,
                               len(app.data),b.sha(app.data),len(self.apps)-1])
        else:
            self.apps=[app]
            self.detail.set_text(f"{app.name} — ID 0x{app.id:04X} — {app.version}\n"
                                 f"{len(app.data)} bytes — SHA-256 {b.sha(app.data)}")
        self.status.set_text("Applet validada. Revise selección y recursos antes de ejecutar.")

    @guarded
    def choose_applet(self,*_):
        path=choose(self,"Seleccionar SmartApplet ya compilada")
        if path:
            self.add_applet(path)

    @guarded
    def choose_catalog(self,*_):
        path=choose(self,"Carpeta principal de SmartApplets",folder=True)
        if path:
            self.load_catalog(path)

    def load_catalog(self,path):
        failures=[]
        paths=sorted(Path(path).rglob("*.OS3KApp"))
        for p in paths:
            try:
                before=len(self.apps)
                self.add_applet(p)
                # Preserve the current selection when a catalog contains an embedded image.
                if len(self.apps)>before:
                    self.model[-1][0]=False
            except (OSError,ValueError) as exc:
                failures.append(str(exc))
        self.status.set_text(f"Catálogo: {len(paths)} archivos, {len(failures)} rechazados."
                             + ("\n"+"\n".join(failures[:3]) if failures else ""))

    def toggled(self,_cell,path):
        self.model[path][0]=not self.model[path][0]

    def select_all(self,value):
        for row in self.model:
            row[0]=value

    def move(self,step):
        model,it=self.tree.get_selection().get_selected()
        if it is None:
            return
        index=model.get_path(it).get_indices()[0]
        other=index+step
        if 0<=other<len(model):
            model.swap(it,model.get_iter(Gtk.TreePath.new_from_indices([other])))

    def apply_project(self,path):
        base_path,base,apps=b.load_project(path)
        self.base_path,self.base=base_path,base
        self.apps=apps
        self.base_label.set_text("ROM base: "+base_path.name)
        self.base_label.set_tooltip_text(str(base_path))
        self.refresh([True]*len(apps))
        self.status.set_text("Proyecto cargado; todas las huellas verificadas.")

    @guarded
    def open_project(self,*_):
        path=choose(self,"Abrir selección de applets")
        if path:
            self.apply_project(path)

    @guarded
    def save_project(self,*_):
        if self.base is None:
            raise b.FormatError("Seleccione primero la ROM base")
        path=choose(self,"Guardar proyecto de selección",save=True,name="seleccion.as3k.json")
        if path:
            b.save_project(path,self.base_path,self.selected())
            self.status.set_text("Proyecto guardado: "+str(path))

    def generate(self,path):
        if self.base is None:
            raise b.FormatError("Seleccione primero la ROM base")
        apps=self.selected()
        manifest=b.build(self.base,apps,path)
        b.create_file(str(path)+".operation.json",b.json_bytes(dict(
            time_utc=datetime.now(timezone.utc).isoformat(),operation="rom-builder-gui",
            base=str(self.base_path),applets=[a.origin for a in apps],manifest=manifest)))
        self.output=Path(path)
        self.open_button.set_sensitive(True)
        self.status.set_text("ROM construida: "+str(path)+"\nSHA-256 "+manifest["rom_sha256"])
        return manifest

    @guarded
    def perform(self,*_):
        if self.kind=="builder":
            if self.base is None:
                raise b.FormatError("Seleccione primero la ROM base")
            self.base.compose(self.selected())
            path=choose(self,"Guardar nueva ROM Flash",save=True,name="as3k-personal.rom")
            if path:
                self.generate(path)
        else:
            self.start_test()

    def start_test(self):
        if self.pid:
            raise b.FormatError("Cierre primero la prueba actual")
        if self.base is None or len(self.apps)!=1:
            raise b.FormatError("Seleccione ROM base y una applet")
        self.base.compose(self.apps)
        if b.Base.read(self.base_path).data != self.base.data or b.Applet.read(self.apps[0].origin).data != self.apps[0].data:
            raise b.FormatError("Un archivo de entrada cambió; vuelva a seleccionarlo")
        reports=Path(os.environ.get("XDG_STATE_HOME",str(Path.home()/".local/state")))/"as3k"/"applet-tests"
        reports.mkdir(parents=True,exist_ok=True)
        report=reports/(uuid.uuid4().hex+".json")
        command=[sys.executable,str(b.ROOT/"scripts/as3k-applet-test"),
                 "--base",str(self.base_path),"--applet",self.apps[0].origin,
                 "--machine",self.args.machine,"--report",str(report)]
        for name in ["rompath","observer","seconds"]:
            value=getattr(self.args,name,None)
            if value is not None:
                command.extend(["--"+name,str(value)])
        if self.args.headless:
            command.append("--headless")
        self.pid,_,_,_=GLib.spawn_async(command,flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD)
        self.watch=GLib.child_watch_add(self.pid,self.finished,report)
        self.action.set_sensitive(False)
        self.stop_button.set_sensitive(True)
        self.status.set_text("Prueba aislada en ejecución. Al cerrar se descarta ROM y memoria.")

    def finished(self,pid,status,report):
        GLib.spawn_close_pid(pid)
        self.pid,self.watch=None,None
        self.action.set_sensitive(True)
        self.stop_button.set_sensitive(False)
        result=json.loads(report.read_text()) if report.exists() else {}
        self.status.set_text(f"Prueba finalizada: exit {os.waitstatus_to_exitcode(status)}; "
                             f"temporales eliminados: {result.get('cleaned',False)}.\nEvidencia: {report}")

    def stop(self,*_):
        if self.pid:
            os.kill(self.pid,signal.SIGTERM)

    @guarded
    def open_output(self,*_):
        if self.output:
            # Independent launcher reads this image only; current profile stays selected.
            subprocess_command=[sys.executable,str(b.ROOT/"scripts/as3k"),
                                "--rom",str(self.output),"--machine",self.args.machine]
            # scripts/as3k is an existing directory, so the actual launcher has a distinct filename.
            subprocess_command[1]=str(b.ROOT/"scripts/as3k-ui")
            GLib.spawn_async(subprocess_command,flags=GLib.SpawnFlags.SEARCH_PATH)

    def on_close(self,*_):
        if self.pid:
            os.kill(self.pid,signal.SIGTERM)
            if self.watch:
                GLib.source_remove(self.watch)
            os.waitpid(self.pid,0)
            GLib.spawn_close_pid(self.pid)
            self.pid=None
        if Gtk.main_level():
            Gtk.main_quit()
        return False


def show_tool(kind,args):
    window=ToolWindow(kind,args)
    window.show_all()
    Gtk.main()
    return 0
