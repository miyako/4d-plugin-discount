"""Actual native sources + full unchanged bundled SDK; no project build phases."""

import argparse
import hashlib
import os
from pathlib import Path
import platform
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT
SDK_SHA256 = "5bac571c5c4cafd280fa7f23d775458a23701287e8735be27d4a2190002f4e3f"


@unittest.skipUnless(platform.system() == "Darwin", "macOS native frameworks required")
class OwnershipTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="discount-ownership-")
        cls.addClassCleanup(cls.temp.cleanup)
        build = Path(cls.temp.name)
        sdk = SOURCE / "4D Plugin API"
        assert hashlib.sha256((sdk / "4DPluginAPI.c").read_bytes()).hexdigest() == SDK_SHA256
        common = ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                  "-Wno-multichar", "-Wno-deprecated-declarations", "-I", str(sdk),
                  "-I", str(sdk / "Classes"), "-I", str(SOURCE / "support"),
                  "-I", str(SOURCE / "discount"), "-I", str(SOURCE)]
        sdk_object = build / "sdk.o"
        result = subprocess.run(["clang", *common, "-Wno-int-conversion",
                                 "-c", str(sdk / "4DPluginAPI.c"),
                                 "-o", str(sdk_object)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        ns = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}
        project = ET.parse(SOURCE / "Discount.vcxproj")
        native = [SOURCE / node.attrib["Include"].replace("\\", "/")
                  for node in project.findall(".//m:ClCompile", ns) if "Include" in node.attrib]
        native.remove(sdk / "4DPluginAPI.c")
        native.extend([SOURCE / "support/4DPlugin-JSON.cpp",
                       sdk / "Classes/ARRAY_TEXT.cpp", sdk / "Classes/C_BLOB.cpp"])
        cls.executable = build / "ownership"
        result = subprocess.run(
            ["clang++", *common, "-std=c++11", "-x", "objective-c++",
             *map(str, native), str(ROOT / ".github/tests/ownership.cpp"),
             "-x", "none", str(sdk_object), "-framework", "Cocoa",
             "-framework", "CoreGraphics", "-o", str(cls.executable)],
            text=True, capture_output=True,
        )
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    def run_case(self, case):
        for representation in ("allocated", "null"):
            with self.subTest(case=case, empty=representation):
                result = subprocess.run(
                    [str(self.executable), case, representation],
                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1",
                         "UBSAN_OPTIONS": "halt_on_error=1"},
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_string_setter_initial_owner(self):
        self.run_case("setters")

    def test_picture_setter_initial_owner(self):
        self.run_case("picture")

    def test_property_results_and_collection_handoff(self):
        self.run_case("getters")

    def test_failed_property_output_not_inspected(self):
        self.run_case("failures")

    def test_sdk_conditional_array_transfer(self):
        self.run_case("arrays")

    def test_dormant_array_wrapper_copies_and_transfers(self):
        self.run_case("array-wrapper")

    def test_real_dispatcher_scalar_text(self):
        self.run_case("live")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", help="Compile immutable git revision with this same harness")
    args, remaining = parser.parse_known_args()
    if args.baseline:
        with tempfile.TemporaryDirectory(prefix="discount-baseline-") as directory:
            SOURCE = Path(directory)
            paths = ["4D Plugin API", "support", "discount", "4DPlugin-Discount.cpp",
                     "4DPlugin-Discount.h", "Discount.vcxproj"]
            archive = subprocess.run(["git", "archive", args.baseline, *paths],
                                     cwd=ROOT, capture_output=True, check=True)
            subprocess.run(["tar", "-x", "-C", directory], input=archive.stdout, check=True)
            unittest.main(argv=[__file__, *remaining])
    else:
        unittest.main(argv=[__file__, *remaining])
