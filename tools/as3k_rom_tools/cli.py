# SPDX-License-Identifier: BSD-3-Clause
"""Two standalone command-line entry points, sharing backend and GUI."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import sys
from . import backend as b


def main(kind, argv=None):
    args_list = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(
        description="Constructor de ROM AS3K" if kind == "builder" else "Prueba temporal de SmartApplet AS3K")
    parser.add_argument("--gui", action="store_true", help="Abrir interfaz GTK3")
    parser.add_argument("--base", type=Path, help="Flash AS3000 System 3 / Writer, 1 MiB, solo lectura")
    parser.add_argument("--applet", type=Path, action="append", default=[], help=".OS3KApp ya compilada")
    parser.add_argument("--report", type=Path, help="Nuevo JSON de operación con fecha y opciones")
    parser.add_argument("--machine", choices=["asma3ksb","asma3ksw"], default="asma3ksb")
    parser.add_argument("--rompath", help="Rutas MAME a los conjuntos originales")
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--seconds", type=int, help="Límite de tiempo emulado para ensayos")
    parser.add_argument("--observer", type=Path, help="Script Lua de observación/prueba")
    if kind == "builder":
        parser.add_argument("--output", type=Path, help="ROM nueva; nunca sobrescribe")
        parser.add_argument("--project", type=Path, help="Cargar selección guardada con hashes")
        parser.add_argument("--save-project", type=Path, help="Guardar selección reproducible")
        parser.add_argument("--retain-base", action="store_true", help="Incluir primero las applets ya presentes")
        parser.add_argument("--inspect", action="store_true", help="Validar y mostrar inventario")
        parser.add_argument("--open", action="store_true", help="Abrir la salida con persistencia por SHA-256")
    else:
        parser.add_argument("--session-parent", type=Path, help="Directorio padre para una sesión única")
    args = parser.parse_args(args_list)
    try:
        if not args_list or args.gui:
            from .gui import show_tool
            return show_tool(kind, args)
        if kind == "builder":
            if args.report and args.report.exists():
                raise FileExistsError(args.report)
            base_path, base, apps = b.selection(args.base,args.applet,args.project,args.retain_base)
            if args.inspect and not args.applet and not args.project:
                apps = list(base.applets)
            if args.save_project:
                b.save_project(args.save_project,base_path,apps)
            if args.output:
                manifest = b.build(base,apps,args.output)
            elif args.inspect or args.save_project:
                _, manifest = base.compose(apps)
            else:
                raise b.FormatError("Indique --output, --inspect o --save-project; sin opciones se abre la GUI")
            result = dict(time_utc=datetime.now(timezone.utc).isoformat(), operation="rom-builder",
                          base=str(base_path), options=vars(args).copy(), manifest=manifest)
            result["options"] = {k:str(v) if isinstance(v,Path) else
                                  [str(x) for x in v] if isinstance(v,list) else v
                                  for k,v in result["options"].items()}
            if args.report:
                b.create_file(args.report,b.json_bytes(result))
            print(json.dumps(manifest,indent=2,ensure_ascii=False))
            if args.open:
                if not args.output:
                    raise b.FormatError("--open requiere --output")
                return b.persistent_run(args.output,machine=args.machine,rompath=args.rompath,
                                        headless=args.headless,observer=args.observer,seconds=args.seconds)
            return 0
        if not args.base or len(args.applet) != 1:
            raise b.FormatError("La prueba temporal requiere --base y exactamente una --applet")
        base, app = b.Base.read(args.base), b.Applet.read(args.applet[0])
        result = b.temporary_test(base,app,machine=args.machine,rompath=args.rompath,
                    headless=args.headless,observer=args.observer,seconds=args.seconds,
                    report=args.report,session_parent=args.session_parent)
        result.pop("emulator_log",None)
        print(json.dumps(result,indent=2,ensure_ascii=False))
        return 0 if result["emulator_exit"] == 0 and result["cleaned"] else 1
    except KeyboardInterrupt:
        print("Sesión interrumpida; proceso y temporales cerrados.",file=sys.stderr)
        return 130
    except (OSError,ValueError,KeyError,IndexError) as exc:
        print(f"ERROR: {exc}",file=sys.stderr)
        return 2
