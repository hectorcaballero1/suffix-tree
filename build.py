import subprocess
import sys
from pathlib import Path

# Python del venv del proyecto (donde está pybind11, gestionado por uv)
VENV_PYTHON = Path(__file__).parent / ".venv" / "bin" / "python"

def run(cmd):
    result = subprocess.run(cmd, shell=True)
    if result.returncode != 0:
        sys.exit(result.returncode)

run(f'cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPython3_EXECUTABLE="{VENV_PYTHON}"')
run("cmake --build build --parallel")
print("Build exitoso.")
