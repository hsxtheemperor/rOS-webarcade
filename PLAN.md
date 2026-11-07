# rOS WebAssembly Boot and Runtime Plan

## Goal

Move rOS toward a browser-hosted operating system organized around a small
WebAssembly bootloader, a C/WebAssembly kernel, an initial filesystem image,
and a separate shell and utilities layer.

The browser remains the platform and security boundary. `biosapi.js` supplies a
small, explicit host API; OS policy and behavior live in the Wasm components.
This is an OS-like runtime in a browser, not a PC BIOS or a kernel with direct
hardware access and CPU privilege levels.

## Proposed layout

Keep editable source and build configuration in the repository's source tree.
Place generated boot and kernel binaries at the paths the virtual disk exposes:

```text
biosapi.js                         Browser host API and first-stage loader
src/
  assets/
    boot.png
  boot/
    bootxR.c
    bootxR.h
    initrd/
      fsys.c
      fsys.h
      mkwebfs.c
      mkwebfs.h
  kernel.c
  api/
    jsapi.h
vdisk/
  vdisk01/
    bin/
      wboot/
        boot.wasm
      wkernel/
        kernel.wasm
      wbash/
        bash.wasm
    initrd/
      webfs1.img                    Generated read-only disk image
```

The C source should normally remain under `src/` rather than inside the
runtime disk's `bin/` directory. This keeps the virtual disk focused on files
the running OS consumes, while source, headers, and build rules remain easy to
version and rebuild. If source-in-the-disk is a requirement, treat it as a
separate development/debug payload, not as the bootable artifact.

## Boot and runtime responsibilities

1. **Browser host (`biosapi.js`)**
   - Initialize a versioned, narrow Wasm host ABI.
   - Provide only browser capabilities needed by the OS, such as loading disk
     files, basic DOM/input/output, and storage where explicitly enabled.
   - Show `src/assets/boot.png`, load the first-stage Wasm bootloader, kernel,
     and initrd from their configured `vdisk01` paths, and report failures
     visibly. The BIOS stages the disk files because browser fetch is
     asynchronous; the Wasm bootloader then decides when to launch the kernel.
   - Validate pointers, lengths, handles, and requested paths at the host
     boundary. Wasm must not receive arbitrary direct DOM or JavaScript access.

2. **Bootloader (`wboot/boot.wasm`)**
   - Ask the BIOS to instantiate the staged kernel and pass it the initrd
     through the documented host/module ABI.
   - Stop with a useful boot error if required files or versions are missing.

3. **Kernel (`wkernel/kernel.wasm`)**
   - Initialize kernel services and the WebFS1 driver.
   - Mount the configured root filesystem and establish the initial process
     environment.
   - Expose a small, versioned syscall/service interface to programs.
   - Start the shell as a separate user-space component rather than making the
     interactive shell part of kernel internals.

4. **Initrd and WebFS1**
   - Treat the initrd as an initial filesystem image/data payload, not as C
     source that the browser can execute.
   - Compile `fsys.c` into the kernel (or later load it as a documented Wasm
     driver). `fsys.h` declares the filesystem interface; neither source file
     is itself a runtime module.
   - WebFS1 currently uses an 8-byte `WEBFS1` magic, a little-endian 32-bit
     entry count, then entries containing a little-endian 16-bit path length,
     a little-endian 32-bit data length, and the path/data bytes.
   - The C driver validates bounds and exposes read/list operations only.
     Keep it read-only for the first shell milestone.
   - Add writable storage and persistence separately, preferably behind a
     storage API such as IndexedDB rather than assuming ordinary
     browser-served files can be modified.

5. **Shell and utilities**
   - Implement the shell and commands as programs using the kernel's documented
     filesystem/process/console APIs.
   - Keep command parsing and utilities out of the kernel where practical.
   - Add commands incrementally and test them against WebFS1 rather than
     depending on private kernel data structures.

## Relationship to traditional OS designs

The staged boot flow, filesystem driver, initrd, kernel services, shell, and
utilities have useful analogues in traditional OSes. The main differences are
platform constraints, not design mistakes:

- The browser starts the application; there is no firmware-to-hardware boot or
  CPU privilege transition.
- WebAssembly is sandboxed and accesses the DOM, input, network, and storage
  only through imported host APIs.
- An initrd normally supplies an early root filesystem or early userspace.
  Making it the place where filesystem driver C source is "run" mixes source,
  executable code, and data. Compile the driver into the kernel initially, and
  use the initrd for the files needed to start the system.
- A shell is traditionally user space. Keeping it outside the kernel improves
  separation and makes shell commands depend on stable services rather than
  kernel implementation details.
- A browser virtual disk served as static files is naturally read-only. Writes
  need an explicit persistence mechanism and a defined image update strategy.

## Implementation stages

1. **Agree on initial paths and filesystem mode** — initial decisions made
   - Artifact paths and WebFS1's initial read-only mode are selected.
   - A boot manifest and stable, versioned host ABI remain future design work.

2. **Make builds reproducible** — implemented
   - Add build targets that compile bootloader and kernel sources to
     `vdisk/vdisk01/bin/wboot/boot.wasm` and
     `vdisk/vdisk01/bin/wkernel/kernel.wasm`.
   - Keep generated outputs reproducible and document compiler requirements.
   - Ensure the browser loads those exact paths rather than a stale root-level
     Wasm filename.

3. **Implement and test the browser host ABI** — initial ABI implemented
   - Specify function names, arguments, memory ownership, errors, and version
     negotiation.
   - Keep capabilities minimal and validate all cross-boundary inputs.

4. **Boot a minimal kernel** — initial boot chain implemented
   - The BIOS stages the bootloader, kernel, and initrd; the bootloader invokes
     the BIOS kernel-launch API.
   - The BIOS reports missing resources and kernel mount failures visibly.
   - Headless Chromium verifies boot and shell input over HTTP.

5. **Implement WebFS1 and initrd mounting** — initial read-only format and
   mount implemented
   - The driver performs bounded parsing and exposes read/list operations.
   - Native tests cover valid, truncated, malformed-magic, missing-path, and
     output-buffer cases.
   - The kernel mounts the generated read-only `vdisk01` payload and reads its
     staged files from the disk image.

6. **Launch the shell** — `proc0` terminal process and C command modules
   implemented
   - The kernel starts `proc0` and provides terminal and read-only filesystem
     APIs.
     - `proc0` tokenizes input and dispatches it to the separate
       `vdisk01/bin/wbash/bash.wasm` module.
     - The C command modules under `src/bash/` include `help`, `ls`, `lsblk`,
       `tree`, `cat`, `echo`, `pwd`, and `clear`.
     - The shell uses its own Wasm memory and calls the kernel through the
       browser-hosted bridge.

7. **Add persistence and broader OS features**
   - Choose a browser persistence backend and define quotas, error handling,
     migration, and export/import behavior.
   - Port additional devices, applications, and UI capabilities only after the
     underlying service contracts are stable.

## Current gaps to resolve

- `proc0` remains part of the kernel module; only the shell command runtime is
  separated into `wbash.wasm`.
- WebFS1 is read-only and currently packages the staged `vdisk01` payload. It has no
  persistence, directories as first-class entries, permissions, or metadata.
- The browser ABI is an initial implementation, not yet a version-negotiated
  or stabilized ABI for independently built user programs.
- The bootloader currently launches the staged kernel; manifest selection,
  integrity checks, and alternate boot entries are future work.

## First milestone status and next completion criteria

The shell milestone builds the bootloader, kernel, separate `wbash` module, and
WebFS1 image, mounts the image read-only, and opens a top-left monospace
terminal. The kernel starts C `proc0`, which tokenizes input and dispatches to
the shell module through a bounded bridge. `ls` shows `vdisk01` and its staged
contents, and `lsblk` reports the virtual disk. The next milestone is adding
utilities and automated browser interaction tests.

For iterative development, `make -f makefile dev` builds missing/outdated
artifacts and starts `dev_server.py`. The server watches files recursively
under `vdisk/vdisk01/bin/`, and its injected development hook reloads the page
when the bin tree changes. After editing source, run `make -f makefile` in
another terminal; the running browser will reboot when the generated Wasm
changes.

- Formalize and version the kernel-to-shell bridge before loading more
  independently built programs.
- Add shell parsing and utilities that use only the kernel/WebFS1 interfaces.
- Add automated browser interaction tests for shell input and error paths.
- Define integrity/version checks and manifest-based boot selection.
- Design persistence separately before making WebFS1 writable.
