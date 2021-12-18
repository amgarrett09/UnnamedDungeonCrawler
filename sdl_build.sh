mkdir -p build/./src/ && \
clang  -O2 -Wall -Wvla -Wextra -Wpedantic -march=native -mavx2 -mfma \
-fno-strict-aliasing -c src/sdl_main.c -o build/./src/sdl_main.c.o && \
clang ./build/./src/sdl_main.c.o -o build/sdl_udc -lSDL2
