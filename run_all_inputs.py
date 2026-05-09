import os
import subprocess
import shutil

# Modificar PATH para usar msys2 primero
env = os.environ.copy()
env["PATH"] = "C:/msys64/mingw64/bin;" + env["PATH"]

# Archivos c++
programa = [
    "main.cpp",
    "Parser/scanner.cpp",
    "Parser/token.cpp",
    "Parser/parser.cpp",
    "Parser/ast.cpp",
    "Parser/visitor.cpp",
    "Files/SequentialFile.cpp",
    "Index/BPTree.cpp",
    "rtree/Rtree_helpers.cpp"
]

compile = ["C:/msys64/mingw64/bin/g++.exe"] + programa + [
    "-std=c++17",
    "-IC:/msys64/mingw64/include",
    "-LC:/msys64/mingw64/lib",
    "-lspatialindex",
    "-o", "a.exe"
]
print("Compilando:", " ".join(compile))
result = subprocess.run(
    compile,
    capture_output=True,
    text=True,
    encoding='utf-8',
    errors='replace',
    env=env
)

if result.returncode != 0:
    print("Error en compilación:")
    print(result.stderr)
    exit(1)

print("Compilación exitosa")

# Ejecutar
input_dir = "inputs"
output_dir = "outputs"
os.makedirs(output_dir, exist_ok=True)

for i in range(0, 5):
    filename = f"input{i}.txt"
    filepath = os.path.join(input_dir, filename)

    if os.path.isfile(filepath):
        print(f"Ejecutando {filename}")
        run_cmd = ["a.exe", filepath]
        result = subprocess.run(
            run_cmd,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace',
            env=env
        )

        output_file = os.path.join(output_dir, f"output{i}.txt")
        with open(output_file, "w", encoding="utf-8") as f:
            f.write("=== STDOUT ===\n")
            f.write(result.stdout)
            f.write("\n=== STDERR ===\n")
            f.write(result.stderr)

        tokens_file = os.path.join(input_dir, f"input{i}_tokens.txt")
        if os.path.isfile(tokens_file):
            dest_tokens = os.path.join(output_dir, f"tokens_{i}.txt")
            shutil.move(tokens_file, dest_tokens)

    else:
        print(filename, "no encontrado en", input_dir)