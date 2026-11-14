# SPDX-License-Identifier: BSD-3-Clause
"""Meaningful format/integrity tests using the owner's local compiled fixtures.

Run from the canonical tree: python3 -m unittest tools.as3k_rom_tools.test_backend -v
No proprietary bytes are included in this test module or exported.
"""
from dataclasses import replace
from pathlib import Path
import json
import os
import struct
import tempfile
import unittest
from . import backend as b


class NativeRecipes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.base_path=Path(os.environ.get("AS3K_TEST_BASE",
            b.ROOT.parent/"private/roms/as3k/mame_sets/asma3ksw/os3k_min_writer.bin"))
        cls.catalog=Path(os.environ.get("AS3K_TEST_APPLETS",
            "/home/spc/GoogleDrive/ALPHASMART/SmartApplets3000"))
        cls.base=b.Base.read(cls.base_path)
        cls.writer=cls.base.applets[0]
        cls.calc=b.Applet.read(cls.catalog/"Calculator.OS3KApp")
        cls.small=b.Applet.read(cls.catalog/"SpellCheck Small USA.OS3KApp")
        cls.large=b.Applet.read(cls.catalog/"SpellCheck Large USA.OS3KApp")

    def test_version_fields_do_not_invent_zero_padding(self):
        self.assertEqual(self.writer.version,"3.0D")
        self.assertEqual(self.calc.version,"3.01")

    def test_original_writer_reconstructed_exactly(self):
        raw,manifest=self.base.compose(self.base.applets)
        self.assertEqual(raw,self.base.data)
        self.assertEqual(manifest["rom_sha256"],b.sha(self.base.data))

    def test_two_native_entries_preserve_payloads_and_protected_regions(self):
        raw,manifest=self.base.compose([self.writer,self.calc])
        loaded=b.Base.parse(raw)
        self.assertEqual([a.data for a in loaded.applets],[self.writer.data,self.calc.data])
        self.assertEqual(raw[:b.ARENA_START],self.base.data[:b.ARENA_START])
        self.assertEqual(raw[b.ARENA_END:],self.base.data[b.ARENA_END:])
        self.assertEqual(manifest["applets"][1]["flash_offset"],
                         b.ARENA_START+len(self.writer.data))

    def test_real_same_id_variants_rejected_together(self):
        self.assertEqual(self.small.id,self.large.id)
        self.assertNotEqual(self.small.data,self.large.data)
        with self.assertRaisesRegex(b.FormatError,"duplicado"):
            self.base.compose([self.small,self.large])

    def test_truncation_footer_header_and_entry_rejected(self):
        cases=[]
        cases.append(self.calc.data[:-2])
        for offset,value in [(0,0),(22,2),(132,0x7F),(len(self.calc.data)-4,0)]:
            data=bytearray(self.calc.data)
            if offset==22:
                data[offset]=value
            else:
                struct.pack_into(">I",data,offset,value)
            cases.append(bytes(data))
        for data in cases:
            with self.subTest(sha256=b.sha(data)),self.assertRaises(b.FormatError):
                b.Applet.parse(data)

    def test_unknown_arena_and_unsupported_base_rejected(self):
        data=bytearray(self.base.data)
        data[b.ARENA_END-1]=0
        with self.assertRaisesRegex(b.FormatError,"arena"):
            b.Base.parse(bytes(data))
        with self.assertRaises(b.FormatError):
            b.Base.parse(self.base.data[:-1])

    def test_flash_and_ram_limits(self):
        apps=[self.writer,self.calc,self.large,
              b.Applet.read(self.catalog/"Thesaurus Large USA.OS3KApp"),
              b.Applet.read(self.catalog/"KeyWords.OS3KApp")]
        with self.assertRaisesRegex(b.FormatError,"Flash"):
            self.base.compose(apps)
        data=bytearray(self.calc.data)
        struct.pack_into(">I",data,8,self.base.ram_budget)
        heavy=b.Applet.parse(bytes(data))
        with self.assertRaisesRegex(b.FormatError,"SRAM"):
            self.base.compose([heavy])

    def test_project_hashes_and_roundtrip(self):
        with tempfile.TemporaryDirectory() as folder:
            folder=Path(folder)
            source=folder/"calc.OS3KApp"
            source.write_bytes(self.calc.data)
            app=b.Applet.read(source)
            project=folder/"selection.json"
            b.save_project(project,self.base_path,[self.writer,app])
            path,base,apps=b.load_project(project)
            self.assertEqual(path,self.base_path.resolve())
            self.assertEqual([a.data for a in apps],[self.writer.data,self.calc.data])
            source.write_bytes(self.small.data)
            with self.assertRaisesRegex(b.FormatError,"cambió"):
                b.load_project(project)

    def test_no_overwrite_original_or_manifest(self):
        with tempfile.TemporaryDirectory() as folder:
            target=Path(folder)/"output.rom"
            target.write_bytes(b"owner data")
            with self.assertRaises(FileExistsError):
                b.build(self.base,[self.calc],target)
            self.assertEqual(target.read_bytes(),b"owner data")
            target.unlink()
            side=Path(str(target)+".manifest.json")
            side.write_bytes(b"owner manifest")
            with self.assertRaises(FileExistsError):
                b.build(self.base,[self.calc],target)
            self.assertFalse(target.exists())
            self.assertEqual(side.read_bytes(),b"owner manifest")

    def test_derived_manifest_deterministic(self):
        raw,manifest=self.base.compose([self.writer,self.calc])
        raw2,manifest2=self.base.compose([self.writer,self.calc])
        self.assertEqual(raw,raw2)
        self.assertEqual(b.json_bytes(manifest),b.json_bytes(manifest2))

    def test_inherited_transport_state_excluded(self):
        old=os.environ.get("AS3K_USBIP_PORT")
        os.environ["AS3K_USBIP_PORT"]="19999"
        try:
            env=b.runtime_environment()
            self.assertNotIn("AS3K_USBIP_PORT",env)
            self.assertEqual(env["AS3K_KEYBOARD_HOST"],"disconnected")
        finally:
            if old is None:os.environ.pop("AS3K_USBIP_PORT",None)
            else:os.environ["AS3K_USBIP_PORT"]=old


if __name__=="__main__":
    unittest.main()
