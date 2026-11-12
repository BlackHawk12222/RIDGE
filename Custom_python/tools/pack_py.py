import os
import glob

PY_DIR = "py"
OUT_HEADER = os.path.join("include", "frozen_py.h")

def main():
    os.makedirs(os.path.dirname(OUT_HEADER), exist_ok=True)
    py_files = sorted(glob.glob(os.path.join(PY_DIR, "*.py")))

    with open(OUT_HEADER, "w", encoding="utf-8") as out:
        out.write("#pragma once\n")
        out.write("#include <cstddef>\n\n")
        out.write("namespace frozen_py {\n\n")

        entries = []
        for filepath in py_files:
            filename = os.path.basename(filepath)
            var_name = filename.replace(".", "_").replace("-", "_")
            
            with open(filepath, "r", encoding="utf-8") as f:
                code = f.read()

            out.write(f'constexpr const char* {var_name} = R"PY_EOF(\n{code}\n)PY_EOF";\n\n')
            entries.append((filename, var_name))

        # Optional lookup table if your port needs to import other files by name
        out.write("struct PyModule {\n    const char* name;\n    const char* source;\n};\n\n")
        out.write(f"constexpr size_t MODULE_COUNT = {len(entries)};\n")
        out.write("constexpr PyModule MODULES[] = {\n")
        for filename, var_name in entries:
            out.write(f'    {{"{filename}", {var_name}}},\n')
        out.write("};\n\n")
        out.write("} // namespace frozen_py\n")

    print(f"[pack_py] Packed {len(py_files)} file(s) into {OUT_HEADER}")

if __name__ == "__main__":
    main()
