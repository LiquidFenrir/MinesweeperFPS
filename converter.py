#!/usr/bin/env python3
import os, sys
from PIL import Image, ImageOps

def conv(out, src):
    outdep = os.path.splitext(out)[0] + ".d"
    with open(outdep, "wt") as d:
        d.write("{}: {}".format(out, src))
    
    name, ext = os.path.splitext(src)
    name = os.path.basename(name)
    if ext == ".png":
        img = ImageOps.flip(Image.open(src).convert("RGBA"))
        w, h = img.size
        pixels = list(img.getdata())
        
        with open(out, "wt") as d:
            d.write("#pragma once\n")
            d.write("\n")
            d.write("#include <tuple>\n")
            d.write("#include <array>\n")
            d.write("\n")
            d.write("namespace Images {\n")
            d.write("\n")
            d.write("inline constexpr std::tuple<int, int, std::array<unsigned char, {}>> {}{}".format(w * h * 4, name, "{"))
            d.write("{}, {}, {}".format(w, h, "{{"))
            s = ""
            for px in pixels:
                for chan in px:
                    d.write("{}{}".format(s, chan))
                    s = ", "
            d.write("}}};\n")
            d.write("\n")
            d.write("}\n")
    elif ext == ".glsl":
        with open(src) as i:
            data = i.read()

        with open(out, "wt") as d:
            d.write("#pragma once\n")
            d.write("\n")
            d.write("#include <array>\n")
            d.write("\n")
            d.write("namespace Shaders {\n")
            d.write("\n")
            d.write("inline constexpr std::array<char, {}> {}{}".format(len(data), name, "{{"))
            s = ""
            for c in data:
                d.write("{}{}".format(s, ord(c)))
                s = ", "
            d.write("}};\n")
            d.write("\n")
            d.write("}\n")

if __name__ == "__main__":
    conv(sys.argv[1], sys.argv[2])
