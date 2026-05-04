import os
import subprocess
import shutil

# Archivos c++
programa = ["main.cpp", "scanner.cpp", "token.cpp", "parser.cpp", "ast.cpp", "visitor.cpp","SequentialFile.cpp","BPTree.cpp"]

# Compilar
compile = ["g++"] + programa
print("Compilando:", " ".join(compile))
result = subprocess.run(compile, capture_output=True, text=True)

if result.returncode != 0:
    print("Error en compilación:\n", result.stderr)
    exit(1)

print("Compilación exitosa")

# Ejecutar
input_dir = "inputs"
output_dir = "outputs"
os.makedirs(output_dir, exist_ok=True)

for i in range(1, 6):
    filename = f"input{i}.txt"
    filepath = os.path.join(input_dir, filename)

    if os.path.isfile(filepath):
        print(f"Ejecutando {filename}")
        run_cmd = ["a.exe", filepath]
        result = subprocess.run(run_cmd, capture_output=True, text=True)

        # Guardar stdout y stderr
        output_file = os.path.join(output_dir, f"output{i}.txt")
        with open(output_file, "w", encoding="utf-8") as f:
            f.write("=== STDOUT ===\n")
            f.write(result.stdout)
            f.write("\n=== STDERR ===\n")
            f.write(result.stderr)

        # Archivos generados
        tokens_file = os.path.join(input_dir, f"input{i}_tokens.txt")  # se crea en inputs/
        ast_file = "ast.dot"  # se crea en raíz del proyecto

        # Mover archivo de tokens si existe
        if os.path.isfile(tokens_file):
            dest_tokens = os.path.join(output_dir, f"tokens_{i}.txt")
            shutil.move(tokens_file, dest_tokens)


    else:
        print(filename, "no encontrado en", input_dir)
