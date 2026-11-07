CLANG ?= clang
CC ?= cc
PYTHON ?= python3
WASM_FLAGS = --target=wasm32 -O2 -fno-builtin -nostdlib -Wl,--no-entry -Wl,--strip-all
VDISK = vdisk/vdisk01
MKWEBFS = $(VDISK)/bin/mkwebfs
BOOT_WASM = $(VDISK)/bin/wboot/boot.wasm
KERNEL_WASM = $(VDISK)/bin/wkernel/kernel.wasm
BASH_WASM = $(VDISK)/bin/wbash/bash.wasm
INITRD = $(VDISK)/initrd/webfs1.img
BASH_SOURCES = $(wildcard src/bash/*.c)
KERNEL_SOURCES = src/kernel.c src/boot/initrd/fsys.c src/boot/initrd/proc0.c

.PHONY: all dev clean

all: $(BOOT_WASM) $(KERNEL_WASM) $(BASH_WASM) $(INITRD)

dev: all
	$(PYTHON) dev_server.py

$(MKWEBFS): src/boot/initrd/mkwebfs.c src/boot/initrd/mkwebfs.h
	mkdir -p $(@D)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -o $@ $<

$(BOOT_WASM): src/boot/bootxR.c src/boot/bootxR.h
	mkdir -p $(@D)
	$(CLANG) $(WASM_FLAGS) -Wl,--allow-undefined -Wl,--export=wboot_start \
		src/boot/bootxR.c -o $@

$(KERNEL_WASM): $(KERNEL_SOURCES) src/api/jsapi.h \
		src/boot/initrd/fsys.h src/boot/initrd/proc0.h
	mkdir -p $(@D)
	$(CLANG) $(WASM_FLAGS) -Wl,--allow-undefined -Wl,--export-memory \
		-Wl,--export=ros_initrd_buffer -Wl,--export=ros_initrd_capacity \
		-Wl,--export=ros_kernel_start -Wl,--export=ros_terminal_submit \
		-Wl,--export=ros_bash_bridge_buffer -Wl,--export=ros_bash_bridge_capacity \
		-Wl,--export=ros_bash_write -Wl,--export=ros_bash_clear \
		-Wl,--export=ros_bash_file_count -Wl,--export=ros_bash_file_path \
		-Wl,--export=ros_bash_file_read -Wl,--export=ros_bash_disk_size \
		$(KERNEL_SOURCES) -o $@

$(BASH_WASM): $(BASH_SOURCES) src/bash/bash.h
	mkdir -p $(@D)
	$(CLANG) $(WASM_FLAGS) -Wl,--allow-undefined -Wl,--export-memory \
		-Wl,--export=bash_arguments_buffer -Wl,--export=bash_execute \
		$(BASH_SOURCES) -o $@

$(INITRD): $(MKWEBFS) $(BOOT_WASM) $(KERNEL_WASM) $(BASH_WASM)
	$(MKWEBFS) --output $@ --entry vdisk01/bin/wboot/boot.wasm $(BOOT_WASM) \
		--entry vdisk01/bin/wkernel/kernel.wasm $(KERNEL_WASM) \
		--entry vdisk01/bin/wbash/bash.wasm $(BASH_WASM)

clean:
	rm -f $(BOOT_WASM) $(KERNEL_WASM) $(BASH_WASM) $(MKWEBFS) $(INITRD)
