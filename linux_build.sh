mkdir -p build/./src/ && \
clang  -O2 -Wall -Wvla -Wextra -march=native -mavx2 -mfma -c src/linux_main.c \
-o build/./src/linux_main.c.o && \
clang ./build/./src/linux_main.c.o -o build/linux_udc -lX11 -lasound
