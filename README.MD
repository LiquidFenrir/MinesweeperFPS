# MinesweeperFPS

Is a recreation of regular Minesweeper, but in 3D (view, not map), and with a networked Co-op mode (works across operating systems too) !  
Uses OpenGL, GLFW, GLM, Dear ImGui, ENet, and mingw-std-threads for c++11 threads on windows when using MinGW.  

## Compilation

Requirements: Python 3 and the Pillow module for converting data files into headers

For Linux, on Linux (Tested on Debian 10):  
- get gcc (or clang, probably works too, untested)  
- get the development packages for libglm, libglfw3, libenet from your favourite package manager  
- `make nix` in the root of the repo  

For Windows 64-bit, on Linux (Tested on Debian 10):  
- get mingw targetting x86_64-w64 from your favourite package manager  
- then `make win` in the root of the repo. 

For Windows 64-bit, on Windows:  
- MSYS2's Mingw-w64 64bit x86_64-w64-gcc works. Same instructions as Windows build on Linux, but in the Mingw-w64 64bit shell  
- MSVC crashes on the generated spritesheet header.  

## License

MinesweeperFPS is licensed under the MIT license in an attempt to not break any of the included libraries' license, which are:  
https://github.com/g-truc/glm - MIT  
https://github.com/ocornut/imgui - MIT  
https://github.com/lsalzman/enet - MIT  
https://github.com/glfw/glfw - zlib  
https://github.com/meganz/mingw-std-threads - BSD-2-Clause  

## Credits

All of the contributors to the libraries mentioned above, they made good things which resulted in another good thing (this game) !  
[LiquidFenrir](https://github.com/LiquidFenrir) for the MinesweeperFPS source code.  
[The Quote-Unquote Company](https://quoteunquoteapps.com/courierprime/) for the `Courier Prime Sans` font, of which the alphanumeric characters (+ underscore and colon) are included in the spritesheet.  
https://learnopengl.com/ for teaching OpenGL, made this project extra easy.  
https://opengameart.org/ for the wall and grass textures, they're CC0 but I didn't note the specific pages where they originated, sorry :(  
https://icon-library.com/ for the clock, bomb, and flag icons used in the HUD texture. They're CC0 as well!  
