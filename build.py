import subprocess
import sys

def run(cmd):
    result = subprocess.run(cmd, shell=True)
    if result.returncode != 0:
        sys.exit(result.returncode)

run("cmake -B build")
run("cmake --build build --parallel")
print("Build exitoso.")
