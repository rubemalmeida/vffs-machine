import os
from datetime import datetime


def read_file_content(file_path, search_str, replace_str) -> str:
    with open(file_path, "r") as file:
        file_content = file.read()
        return file_content.replace(search_str, replace_str)


def substitute_code_in_file(file_path, search_str, replace_str):
    with open(file_path, "r") as file:
        file_content = file.read()
        file_content = file_content.replace(search_str, replace_str)

    with open(file_path, "w") as file:
        file.write(file_content)


def main():
    if os.path.exists("compiled.ino"):
        os.remove("compiled.ino")

    # Leitura do conteúdo do arquivo model.js
    with open("model.js", "r") as js_file:
        js_code = js_file.read()

    # Leitura do conteúdo do arquivo model.css
    with open("model.css", "r") as css_file:
        css_code = css_file.read()

    # Leitura do conteúdo do arquivo model.html
    with open("model.html", "r", encoding="utf-8") as html_file:
        html_code = html_file.read()

    # Criação do arquivo temp-model.html
    with open("temp-model.html", "w") as temp_file:
        temp_file.write(html_code)

    # Substituição no arquivo temp-model.html
    substitute_code_in_file(
        "temp-model.html",
        '<script type="text/javascript" src="model.js"></script>',
        f"<script>{js_code}</script>",
    )
    substitute_code_in_file(
        "temp-model.html",
        '<link rel="stylesheet" href="model.css">',
        f"<style>{css_code}</style>",
    )

    # Leitura do arquivo temp-model.html
    with open("temp-model.html", "r") as html_file:
        new_html = html_file.read()

    new_ino = read_file_content(
        "vffs-webserver.ino",
        "rawliteral(%HTML%)rawliteral",
        f"rawliteral({new_html})rawliteral",
    )

    # Persistência no arquivo compiled.ino
    with open("compiled.ino", "w") as compiled_file:
        compiled_file.write(new_ino)
        compiled_file.write(f"\n// compiled at {datetime.now()}")

    # delete temp-model.html if exists
    if os.path.exists("temp-model.html"):
        os.remove("temp-model.html")


if __name__ == "__main__":
    main()
