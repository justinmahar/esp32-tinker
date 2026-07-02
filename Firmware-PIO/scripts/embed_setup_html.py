Import("env")

import os

project_dir = env.subst("$PROJECT_DIR")
html_path = os.path.join(project_dir, "include", "setup_portal.html")
out_path = os.path.join(project_dir, "include", "setup_html.generated.h")

with open(html_path, "r", encoding="utf-8") as html_file:
    html = html_file.read()

delimiter = "TinkerSetup"
while f"){delimiter}" in html:
    delimiter += "X"
    if len(delimiter) > 16:
        raise RuntimeError("Could not find a raw-string delimiter for setup_portal.html")

with open(out_path, "w", encoding="utf-8") as out_file:
    out_file.write("// Auto-generated from include/setup_portal.html — do not edit\n")
    out_file.write("#pragma once\n\n")
    out_file.write("#include <pgmspace.h>\n\n")
    out_file.write(f'const char SETUP_PORTAL_HTML[] PROGMEM = R"{delimiter}(\n')
    out_file.write(html)
    if not html.endswith("\n"):
        out_file.write("\n")
    out_file.write(f'){delimiter}";\n')
