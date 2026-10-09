#!/usr/bin/env python3
"""Record source-bound C++ checks and independent engine fixture results."""
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BOT = ROOT / "tyhh_test/tyhh_qos_counter_queen_v127"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    sources = [BOT / "main.cpp", BOT / "helper.hpp", HERE / "candidate_tests.cpp"]
    hashes = {str(p.relative_to(ROOT)): sha(p) for p in sources}
    with tempfile.TemporaryDirectory(prefix="queen_rules_tests_") as directory:
        executable = Path(directory) / "candidate_tests"
        command = ["g++-10", "-std=c++20", "-O2", str(HERE / "candidate_tests.cpp"), "-o", str(executable)]
        build = subprocess.run(command, text=True, capture_output=True, check=True)
        run = subprocess.run([str(executable)], text=True, capture_output=True, check=True)
    assert hashes == {str(p.relative_to(ROOT)): sha(p) for p in sources}, "Sources changed during checks"
    payload = {"passed": True, "source_sha256": hashes, "compiler": "g++-10",
               "flags": ["-std=c++20", "-O2"], "build_stderr": build.stderr,
               "test_stdout": run.stdout, "test_stderr": run.stderr}
    (HERE / "candidate_tests.json").write_text(json.dumps(payload, indent=2) + "\n")
    subprocess.run([sys.executable, str(HERE / "engine_fixtures.py"), "--output",
                    str(HERE / "engine_fixtures.json")], check=True)
    print(json.dumps({"candidate_passed": True, "candidate_main_sha256": sha(BOT / "main.cpp")}))


if __name__ == "__main__":
    main()
