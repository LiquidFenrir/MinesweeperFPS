# MinesweeperFPS

Is a recreation of regular Minesweeper, but in 3D (view, not map), and with a networked Co-op mode (works across operating systems too) !  
Uses OpenGL, GLFW, GLM, Dear ImGui, ENet, and mingw-std-threads for c++11 threads on windows.  

## Compilation

Requirements: Python 3 and the Pillow module for converting data files into headers

For Linux:
- get gcc (or clang, probably works too) and the development packages for libglm, libglfw3, libenet, then `make nix` in the root of the repo. Tested on Debian 10.  

For Windows, on Linux:  
- get mingw targetting x86_64-w64, then `make win` in the root of the repo. Tested on Debian 10.  

For Windows, on Windows:  
- MSYS2's mingw-w64-gcc might work? Untested.  
- MSVC with Visual Studio's Makefile thing? Might work if you replace the libraries in `extra-libs/libs/` to the correct ones, as the bundled ones are for MinGW ? Untested.  

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
