import subprocess
import sys

def run(cmd):
    result = subprocess.run(cmd, shell=True)
    if result.returncode != 0:
        sys.exit(result.returncode)

run("./build/tests/cpp/run_tests")
run("python -m pytest tests/python/ -v")
