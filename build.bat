windres Genshin-Launch.rc -O coff -F pe-x86-64 -o Genshin-Launch.res
gcc -o Genshin-Launch.exe Genshin-Launch.cpp Genshin-Launch.res -O3 -luuid -lntdll -lKernel32 -lgdi32 -ladvapi32 -lmsvcrt -luser32 -lole32 -ld2d1 -lwinmm -D_WIN32_WINNT=0x0601 -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables --freestanding -mcmodel=small -Wl,--subsystem,windows,--relax,-s
pause