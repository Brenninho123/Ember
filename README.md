# Ember

A beginner operating system written in C and x86 Assembly.

Ember is a hobby OS project built from scratch: custom bootloader integration via GRUB Multiboot, a freestanding C kernel, a VGA text-mode UI layer, and an in-memory content/storage system. The goal is to learn low-level systems programming by building every layer by hand.

## Features

- Multiboot-compliant kernel, boots via GRUB
- Freestanding C kernel with no libc dependency
- VGA text-mode terminal driver with hardware cursor support
- Custom `printf`-style formatted output (`%d %u %x %X %c %s`)
- Basic UI primitives (boxes, cursor positioning, color control)
- Path utility library (join, basename, dirname, extension, normalize)
- In-memory content/storage layer (create, read, write, append, delete, rename, list)
- Low-level assembly support routines (GDT/IDT loading, paging control)
- Android/Linux setup tool for preparing the build environment
- Automated CI build via GitHub Actions, producing a bootable ISO

## Project Structure

```
Ember/
├── source/
│   ├── boot.asm              Multiboot header and kernel entry point
│   ├── linker.ld              Linker script, loads kernel at 1M
│   ├── Main.c                  VGA terminal driver and formatted output
│   └── ember/
│       ├── Paths.c            Path manipulation utilities
│       ├── Content.c         In-memory content storage layer
│       └── ui/
│           └── Kernel.c      Kernel entry point and UI layer
├── build/
│   └── Compile.asm            Low-level support routines (GDT, IDT, paging)
├── app/
│   └── Setup.c                  Android/Linux build environment setup tool
├── .github/
│   └── workflows/
│       └── main.yml            CI build pipeline
├── LICENSE
└── README.md
```

## Requirements

To build Ember, you need:

- `gcc` (with 32-bit support, targeting `i686-elf` or using `-m32`)
- `nasm`
- `ld` (GNU binutils)
- `grub-mkrescue` and `xorriso`
- `qemu-system-i386` (for testing)

On Debian/Ubuntu:

```bash
sudo apt install gcc nasm binutils grub-pc-bin grub-common xorriso qemu-system-x86
```

On Android (Termux):

```bash
pkg install gcc nasm binutils grub-efi qemu-system-x86
```

You can also run the included setup tool to check your environment automatically:

```bash
gcc app/Setup.c -o setup
./setup
```

## Building

```bash
nasm -f elf32 source/boot.asm -o build/boot.o
nasm -f elf32 build/Compile.asm -o build/compile.o
gcc -m32 -c source/Main.c -o build/main.o -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c source/ember/ui/Kernel.c -o build/kernel.o -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c source/ember/Paths.c -o build/paths.o -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c source/ember/Content.c -o build/content.o -ffreestanding -O2 -Wall -Wextra

ld -m elf_i386 -T source/linker.ld -o build/ember.bin \
  build/boot.o build/compile.o build/main.o build/kernel.o build/paths.o build/content.o \
  -nostdlib

mkdir -p isodir/boot/grub
cp build/ember.bin isodir/boot/ember.bin
cat > isodir/boot/grub/grub.cfg << 'EOF'
menuentry "Ember OS" {
    multiboot /boot/ember.bin
}
EOF

grub-mkrescue -o build/ember.iso isodir
```

Or simply push to `main`, or trigger the **Build Ember OS** workflow manually from the Actions tab. The finished `ember.iso` is uploaded as a build artifact.

## Running

Test the ISO with QEMU:

```bash
qemu-system-i386 -cdrom build/ember.iso
```

## Roadmap

- [x] Multiboot bootloader integration
- [x] VGA text-mode terminal driver
- [x] Formatted output
- [x] Path utilities
- [x] In-memory content storage
- [ ] GDT and IDT setup in C
- [ ] Interrupt handling (keyboard input)
- [ ] Paging and memory management
- [ ] Basic disk-backed filesystem
- [ ] Simple shell/command interpreter
- [ ] Multitasking

## License

Unlicense — see [LICENSE](LICENSE).
