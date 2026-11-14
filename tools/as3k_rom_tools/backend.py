# SPDX-License-Identifier: BSD-3-Clause
"""Native System 3 Flash recipes and isolated emulator sessions (Python stdlib).

Evidence: Betawise os3k.h/os3k.lds and original AS3000 System 3 discovery,
004C3038..004C3158 / validator 004C3248. No applet compilation or relocation:
System 3 adds the entry offset to its slot base for the position independent image.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os
import signal
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FLASH_SIZE = 0x100000
ARENA_START, ARENA_END = 0x8000, 0xC0000
MAX_SLOTS = 31  # Slot zero is System.
MAGIC, FOOTER = b"\xc0\xff\xee\xad", b"\xca\xfe\xfe\xed"


class FormatError(ValueError):
    pass


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def u32(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


def text(data):
    return data.split(b"\0", 1)[0].decode("mac_roman", errors="replace")


@dataclass(frozen=True)
class Applet:
    data: bytes
    origin: str
    name: str
    id: int
    ram: int
    files: int
    flags: int
    version: str
    entry: int

    @classmethod
    def parse(cls, data: bytes, origin="applet"):
        if len(data) < 152 or data[:4] != MAGIC:
            raise FormatError(f"{origin}: cabecera SmartApplet inválida")
        size, ram, settings, flags = struct.unpack_from(">4I", data, 4)
        app_id, header, files = struct.unpack_from(">HBB", data, 20)
        entry = u32(data, 132)
        if size != len(data) or size & 1:
            raise FormatError(f"{origin}: tamaño declarado {size} != archivo {len(data)}, o impar")
        if header != 1 or flags >> 24 != 0xFF:
            raise FormatError(f"{origin}: versión de cabecera/estado de instalación incompatible")
        if not app_id:
            raise FormatError(f"{origin}: APPLET_ID 0 está reservado a System")
        if data[-4:] != FOOTER and data[-6:-2] != FOOTER:
            raise FormatError(f"{origin}: falta el marcador final CAFEFEED")
        if entry < 148 or entry & 1 or entry >= size - 4:
            raise FormatError(f"{origin}: entrada relativa fuera de la imagen")
        if settings and not 148 <= settings < size:
            raise FormatError(f"{origin}: configuración fuera de la imagen")
        if tuple(struct.unpack_from(">3I", data, 136)) != (0, 1, 2):
            raise FormatError(f"{origin}: extensión de cabecera no reconocida")
        if flags & 0x100:
            raise FormatError(f"{origin}: requiere hardware inalámbrico ausente en AS3000")
        name = text(data[24:60])
        if not name.strip():
            raise FormatError(f"{origin}: nombre vacío")
        version = f"{data[60]}.{data[61]}{text(data[62:63]).strip()}"
        return cls(data, str(origin), name, app_id, ram, files, flags, version, entry)

    @classmethod
    def read(cls, path):
        p = Path(path).expanduser().resolve()
        return cls.parse(p.read_bytes(), str(p))

    def metadata(self):
        return dict(name=self.name, applet_id=f"0x{self.id:04X}", version=self.version,
                    size=len(self.data), ram=self.ram, file_count=self.files,
                    flags=f"0x{self.flags:08X}", entry_offset=self.entry, sha256=sha(self.data))


@dataclass(frozen=True)
class Base:
    data: bytes
    origin: str
    applets: tuple[Applet, ...]
    ram_budget: int

    @classmethod
    def read(cls, path):
        p = Path(path).expanduser().resolve()
        return cls.parse(p.read_bytes(), str(p))

    @classmethod
    def parse(cls, data, origin="base"):
        if len(data) != FLASH_SIZE or data[ARENA_END+6:ARENA_END+14] != b"System 3":
            raise FormatError(f"{origin}: se necesita una Flash AS3000 System 3 de 1 MiB")
        # Resolve the System applet through its native System header, not a magic scan
        # (the same signature also occurs as immediate operands in System code).
        if u32(data, ARENA_END+0x50) != 0x400000 + ARENA_END:
            raise FormatError(f"{origin}: mapa Flash AS3000 incompatible")
        system = ARENA_END + u32(data, ARENA_END+0x48)
        if not ARENA_END <= system <= FLASH_SIZE - 148 or data[system:system+4] != MAGIC:
            raise FormatError(f"{origin}: índice nativo System no válido")
        if data[system+20:system+24] != b"\0\0\1\0":
            raise FormatError(f"{origin}: cabecera System no reconocida")
        # System reserves its longword-aligned BSS plus one word-sized field.
        system_ram = (u32(data, system+8) + 3) & ~3
        budget = 0x3B800 - system_ram - 4
        if budget < 0:
            raise FormatError(f"{origin}: SRAM System inválida")
        apps, pos = [], ARENA_START
        while pos < ARENA_END and data[pos:pos+4] == MAGIC:
            size = u32(data, pos+4)
            if size < 152 or size & 1 or pos + size > ARENA_END:
                raise FormatError(f"{origin}: cadena de applets dañada en {pos:#x}")
            apps.append(Applet.parse(data[pos:pos+size], f"{origin}@{pos:#x}"))
            pos += size
        if any(v != 0xFF for v in data[pos:ARENA_END]):
            raise FormatError(f"{origin}: arena no reconocida; no se borrarán datos desconocidos")
        base = cls(data, str(origin), tuple(apps), budget)
        base.compose(apps)  # Validate collisions and physical resources also for base inventory.
        return base

    def compose(self, applets):
        apps = tuple(applets)
        if len(apps) > MAX_SLOTS:
            raise FormatError("System 3 admite 31 applets además de System")
        seen = set()
        for app in apps:
            if app.id in seen:
                raise FormatError(f"APPLET_ID duplicado: 0x{app.id:04X} ({app.name})")
            seen.add(app.id)
        used = sum(len(a.data) for a in apps)
        ram = sum(a.ram for a in apps)
        file_buffers = 512 * (1 + sum(a.files for a in apps))
        if used > ARENA_END - ARENA_START:
            raise FormatError(f"No cabe en Flash: {used} bytes")
        if ram + file_buffers >= self.ram_budget:
            raise FormatError(f"No cabe en SRAM: {ram} BSS + {file_buffers} buffers")
        output = self.data[:ARENA_START] + b"".join(a.data for a in apps)
        output += b"\xff" * (ARENA_END - len(output))
        output += self.data[ARENA_END:]
        manifest = dict(schema="as3k-rom-recipe-v1", algorithm="system3-contiguous-pic-v1",
                        base_sha256=sha(self.data), rom_sha256=sha(output), size=len(output),
                        arena_start=ARENA_START, arena_end=ARENA_END,
                        flash_used=used, flash_free=ARENA_END-ARENA_START-used,
                        applet_ram=ram, file_buffers=file_buffers,
                        ram_available=self.ram_budget-ram-file_buffers,
                        protected_boot_sha256=sha(output[:ARENA_START]),
                        protected_system_sha256=sha(output[ARENA_END:]), applets=[])
        offset = ARENA_START
        for app in apps:
            manifest["applets"].append(dict(app.metadata(), flash_offset=offset,
                                            mapped_address=f"0x{0x400000+offset:08X}"))
            offset += len(app.data)
        return output, manifest


def json_bytes(value):
    return (json.dumps(value, indent=2, ensure_ascii=False, sort_keys=True) + "\n").encode()


def create_file(path, data):
    """Exclusive creation: never truncate a ROM, project, log or user's existing file."""
    p = Path(path).expanduser().absolute()
    with p.open("xb") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    return p


def build(base: Base, apps, output):
    raw, manifest = base.compose(apps)
    target = Path(output).expanduser().absolute()
    sidecar = target.with_name(target.name + ".manifest.json")
    if target.exists() or sidecar.exists():
        raise FileExistsError(f"Ya existe la salida o su manifiesto: {target}")
    create_file(target, raw)
    try:
        create_file(sidecar, json_bytes(manifest))
    except BaseException:
        target.unlink()
        raise
    return manifest


def save_project(path, base_path, apps):
    p = Path(path).expanduser().absolute()
    base_path = Path(base_path).expanduser().resolve()
    base = Base.read(base_path)
    entries = []
    for app in apps:
        # Applets already embedded in the base do not need a separate copied file.
        embedded = next((i for i,a in enumerate(base.applets) if a.data == app.data), None)
        if embedded is not None:
            entries.append(dict(base_slot=embedded, sha256=sha(app.data)))
        else:
            source = Path(app.origin).resolve()
            if Applet.read(source).data != app.data:
                raise FormatError(f"La applet cambió: {source}")
            entries.append(dict(path=os.path.relpath(source, p.parent), sha256=sha(app.data)))
    base.compose(apps)
    create_file(p, json_bytes(dict(schema="as3k-selection-v1",
                base=os.path.relpath(base_path, p.parent), base_sha256=sha(base.data), applets=entries)))


def load_project(path):
    p = Path(path).expanduser().resolve()
    value = json.loads(p.read_text())
    if value.get("schema") != "as3k-selection-v1":
        raise FormatError("Versión de proyecto no reconocida")
    base_path = (p.parent / value["base"]).resolve()
    base = Base.read(base_path)
    if sha(base.data) != value["base_sha256"]:
        raise FormatError("La ROM base cambió desde que se guardó el proyecto")
    apps = []
    for entry in value["applets"]:
        if "base_slot" in entry:
            index = entry["base_slot"]
            if not isinstance(index, int) or not 0 <= index < len(base.applets):
                raise FormatError("Índice de applet del proyecto no válido")
            app = base.applets[index]
        else:
            app = Applet.read(p.parent / entry["path"])
        if sha(app.data) != entry["sha256"]:
            raise FormatError(f"La applet cambió desde que se guardó el proyecto: {app.name}")
        apps.append(app)
    base.compose(apps)
    return base_path, base, apps


def selection(base_path=None, applet_paths=(), project=None, retain_base=False):
    if project:
        if base_path or applet_paths or retain_base:
            raise FormatError("--project no se combina con --base/--applet/--retain-base")
        return load_project(project)
    if not base_path:
        raise FormatError("Seleccione una ROM base o un proyecto")
    p = Path(base_path).expanduser().resolve()
    base = Base.read(p)
    apps = list(base.applets) if retain_base else []
    apps += [Applet.read(a) for a in applet_paths]
    base.compose(apps)
    return p, base, apps


def runtime_environment():
    env = dict(os.environ)
    # Session isolation also excludes inherited transports and bootstrap straps.
    for key in list(env):
        if key.startswith("AS3K_"):
            env.pop(key)
    # Select the existing real-cable host mode: no diagnostic powered-D11 fixture.
    # Presence of this variable gates D11 cable power; HOST config still selects PS/2/ADB.
    env["AS3K_KEYBOARD_HOST"]="disconnected"
    return env


def emulator_command(image, state, machine="asma3ksb", rompath=None, headless=False,
                     observer=None, seconds=None):
    if machine not in ("asma3ksb", "asma3ksw"):
        raise FormatError("Use BOOT asma3ksb o WRITER asma3ksw")
    state = Path(state).absolute()
    state.mkdir(parents=True, exist_ok=True)
    command = [str(ROOT/"alphasma3k"), machine, "-noreadconfig",
               "-rom", str(Path(image).absolute()), "-rompath", str(rompath or default_rompath()),
               "-skip_gameinfo", "-noconfirm_quit", "-noautosave"]
    for flag, subdir in (("-cfg_directory","cfg"),("-nvram_directory","nvram"),
                         ("-state_directory","states"),("-snapshot_directory","snap"),
                         ("-diff_directory","diff"),("-input_directory","input")):
        directory = state/subdir
        directory.mkdir(exist_ok=True)
        command += [flag, str(directory)]
    if headless:
        command += ["-video","none","-sound","none","-nothrottle"]
    if observer:
        command += ["-autoboot_delay","2","-autoboot_script",str(Path(observer).resolve())]
    if seconds is not None:
        if seconds <= 0:
            raise FormatError("Duración inválida")
        command += ["-seconds_to_run", str(seconds)]
    return command


def default_rompath():
    value = os.environ.get("AS3K_ROMPATH")
    if value:
        return value
    return str(ROOT.parent/"private/roms/as3k/mame_sets")+";"+str(ROOT.parent/"private/roms")


def run_process(command, state):
    env = runtime_environment()
    if "-video" in command and command[command.index("-video")+1] == "none":
        env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", QT_QPA_PLATFORM="offscreen")
    log = Path(state)/"emulator.log"
    process = None
    handlers = {}
    def stop(signum, _frame):
        if process is not None and process.poll() is None:
            process.terminate()
        raise KeyboardInterrupt
    try:
        for sig in (signal.SIGINT, signal.SIGTERM):
            handlers[sig] = signal.signal(sig, stop)
        with log.open("xb") as output:
            process = subprocess.Popen(command, cwd=state, env=env, stdout=output,
                                       stderr=subprocess.STDOUT)
            return process.wait()
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        for sig, previous in handlers.items():
            signal.signal(sig, previous)


def temporary_test(base: Base, app: Applet, *, machine="asma3ksb", rompath=None,
                   headless=False, observer=None, seconds=None, report=None, session_parent=None):
    # Exact one-app image. Never writes an installed profile or its selected-ROM setting.
    raw, manifest = base.compose([app])
    report_path = Path(report).expanduser().absolute() if report else None
    if report_path and report_path.exists():
        raise FileExistsError(report_path)
    result = dict(schema="as3k-temporary-test-v1", time_utc=datetime.now(timezone.utc).isoformat(),
                  base=base.origin, applet_path=app.origin, applet=app.metadata(),
                  base_sha256=sha(base.data), rom_sha256=sha(raw), machine=machine,
                  emulator_exit=None, cleaned=False)
    session = None
    try:
        with tempfile.TemporaryDirectory(prefix="as3k-applet-", dir=session_parent) as folder:
            session = Path(folder)
            result["session_id"] = session.name
            image = create_file(session/"test.rom", raw)
            command = emulator_command(image, session, machine, rompath, headless, observer, seconds)
            result["fresh_nvram"] = not any((session/"nvram").iterdir())
            create_file(session/"recipe.json", json_bytes(manifest))
            result["emulator_exit"] = run_process(command, session)
            result["emulator_log"] = (session/"emulator.log").read_text(errors="replace")
            result["state_files"] = {str(p.relative_to(session)):dict(size=p.stat().st_size,
                                        sha256=sha(p.read_bytes()))
                                     for p in session.rglob("*") if p.is_file()
                                     and p.name not in ("test.rom","emulator.log","recipe.json")}

    finally:
        result["cleaned"] = session is not None and not session.exists()
        if report_path:
            create_file(report_path, json_bytes(result))
    return result


def persistent_run(image, *, machine="asma3ksb", rompath=None, headless=False,
                   observer=None, seconds=None, state_root=None):
    p = Path(image).expanduser().resolve()
    base = Base.read(p)
    root = Path(state_root or os.environ.get("XDG_DATA_HOME", str(Path.home()/".local/share")))
    state = root/"as3k"/"roms"/sha(base.data)/machine
    state.mkdir(parents=True, exist_ok=True)
    # Prevent simultaneous processes racing on the same persistent device state.
    import fcntl
    with (state/"session.lock").open("a") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as exc:
            raise FormatError("Esta ROM ya tiene una sesión abierta") from exc
        # Each launch keeps a unique console, without discarding its NVRAM/config.
        log = state/"emulator.log"
        if log.exists():
            archive = state/("emulator-"+str(os.stat(log).st_mtime_ns)+".log")
            log.rename(archive)
        return run_process(emulator_command(p,state,machine,rompath,headless,observer,seconds),state)
