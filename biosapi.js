// Browser BIOS: provide host APIs and boot the virtual disk's Wasm stages.
async function boot() {
  const splashUrl = await loadSplash();
  const [bootBytes, kernelBytes, bashBytes, initrdBytes] = await Promise.all([
    loadBytes("./vdisk/vdisk01/bin/wboot/boot.wasm"),
    loadBytes("./vdisk/vdisk01/bin/wkernel/kernel.wasm"),
    loadBytes("./vdisk/vdisk01/bin/wbash/bash.wasm"),
    loadBytes("./vdisk/vdisk01/initrd/webfs1.img")
  ]);
  const vfs = createWritableVfs(initrdBytes);
  let kernelInstance;
  let bashInstance;
  const kernelModule = new WebAssembly.Module(kernelBytes);
  const bashModule = new WebAssembly.Module(bashBytes);

  const browser = createBrowserApi(() => kernelInstance, () => bashInstance, vfs);
  const launchKernel = () => {
    if (kernelInstance) return 0;
    kernelInstance = new WebAssembly.Instance(kernelModule, { browser });
    bashInstance = new WebAssembly.Instance(bashModule, {
      kernel: createBashApi(() => kernelInstance, () => bashInstance, vfs)
    });

    const capacity = kernelInstance.exports.ros_initrd_capacity();
    if (initrdBytes.byteLength > capacity) {
      throw new Error(`WebFS1 image is too large (${initrdBytes.byteLength} bytes)`);
    }
    const address = kernelInstance.exports.ros_initrd_buffer();
    const memory = new Uint8Array(kernelInstance.exports.memory.buffer);
    memory.set(new Uint8Array(initrdBytes), address);
    if (!kernelInstance.exports.ros_kernel_start(initrdBytes.byteLength)) {
      kernelInstance = undefined;
      throw new Error("Kernel could not mount the read-only WebFS1 initrd");
    }
    return 1;
  };

  const bootModule = new WebAssembly.Module(bootBytes);
  const bootInstance = new WebAssembly.Instance(bootModule, {
    bios: { launch_kernel: launchKernel }
  });
  if (!bootInstance.exports.wboot_start()) {
    throw new Error("The wboot bootloader could not start the kernel");
  }
  if (splashUrl) URL.revokeObjectURL(splashUrl);
  console.info("[biosapi] wboot launched the WebFS1 kernel");
}

async function loadBytes(path) {
  const response = await fetch(path, {
    cache: window.__ROS_DEV__ ? "no-store" : "default"
  });
  if (!response.ok) throw new Error(`Could not load ${path} (HTTP ${response.status})`);
  return response.arrayBuffer();
}

async function loadSplash() {
  const response = await fetch("./src/assets/boot.png");
  if (!response.ok) throw new Error(`Could not load boot asset (HTTP ${response.status})`);
  const url = URL.createObjectURL(await response.blob());
  document.body.replaceChildren();
  Object.assign(document.body.style, {
    margin: "0",
    minHeight: "100vh",
    display: "grid",
    placeItems: "center",
    background: "#000"
  });
  const image = document.createElement("img");
  image.src = url;
  image.alt = "rOS is booting";
  image.style.maxWidth = "min(500px, 100vw)";
  image.style.maxHeight = "100vh";
  document.body.appendChild(image);
  return url;
}

function createWritableVfs(initrdBytes) {
  const decoder = new TextDecoder();
  const files = new Map();

  function normalize(pathName) {
    if (pathName == null) return "vdisk01";
    let value = String(pathName).replace(/\\/g, "/");
    while (value.startsWith("/")) value = value.slice(1);
    value = value.replace(/\/+/g, "/");
    if (!value || value === "/") return "vdisk01";
    if (!value.startsWith("vdisk01")) value = `vdisk01/${value}`;
    if (value === "vdisk01/") return "vdisk01";
    return value.replace(/\/+$/, "");
  }

  function ensureParentDirectories(pathName) {
    const normalized = normalize(pathName);
    const parts = normalized.split("/");
    let current = "vdisk01";
    for (let i = 1; i < parts.length - 1; i++) {
      current += `/${parts[i]}`;
      if (!files.has(current)) files.set(current, new Uint8Array());
    }
    if (!files.has("vdisk01")) files.set("vdisk01", new Uint8Array());
  }

  function addEntry(pathName, bytes) {
    const normalized = normalize(pathName);
    ensureParentDirectories(normalized);
    files.set(normalized, new Uint8Array(bytes));
  }

  function readU16LE(data, offset) {
    return data[offset] | (data[offset + 1] << 8);
  }

  function readU32LE(data, offset) {
    return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
  }

  if (initrdBytes && initrdBytes.byteLength >= 12) {
    const bytes = new Uint8Array(initrdBytes);
    const magic = new TextDecoder().decode(bytes.subarray(0, 8));
    if (magic === "WEBFS1\0\0") {
      const count = readU32LE(bytes, 8);
      let offset = 12;
      for (let i = 0; i < count; i++) {
        if (offset + 6 > bytes.length) break;
        const pathLength = readU16LE(bytes, offset);
        const dataLength = readU32LE(bytes, offset + 2);
        offset += 6;
        if (offset + pathLength + dataLength > bytes.length) break;
        const path = decoder.decode(bytes.subarray(offset, offset + pathLength));
        const data = bytes.subarray(offset + pathLength, offset + pathLength + dataLength);
        addEntry(path, data);
        offset += pathLength + dataLength;
      }
    }
  }

  if (!files.has("vdisk01")) files.set("vdisk01", new Uint8Array());

  return {
    count() {
      return files.size;
    },
    list() {
      return Array.from(files.keys()).sort();
    },
    read(pathName) {
      const key = normalize(pathName);
      const bytes = files.get(key);
      return bytes ? new Uint8Array(bytes) : null;
    },
    write(pathName, bytes) {
      const key = normalize(pathName);
      if (key === "vdisk01") return 0;
      ensureParentDirectories(key);
      files.set(key, new Uint8Array(bytes));
      return 1;
    },
    remove(pathName) {
      const key = normalize(pathName);
      if (!files.has(key)) return 0;
      files.delete(key);
      return 1;
    },
    mkdir(pathName) {
      const key = normalize(pathName);
      if (key === "vdisk01") return 1;
      ensureParentDirectories(key);
      files.set(key, new Uint8Array());
      return 1;
    }
  };
}

function createBrowserApi(getKernel, getBash, vfs) {
  const elements = new Map([[1, document.body]]);
  let nextElementId = 2;
  const decoder = new TextDecoder();
  const encoder = new TextEncoder();

  function readString(pointer) {
    const kernel = getKernel();
    if (!kernel) throw new Error("Kernel is not running");
    const memory = new Uint8Array(kernel.exports.memory.buffer);
    if (!Number.isInteger(pointer) || pointer < 0 || pointer >= memory.length) {
      throw new RangeError("Invalid pointer passed from Wasm");
    }
    let end = pointer;
    while (end < memory.length && memory[end] !== 0) end++;
    if (end === memory.length) throw new RangeError("Unterminated string passed from Wasm");
    return decoder.decode(memory.subarray(pointer, end));
  }

  function getElement(id) {
    const element = elements.get(id);
    if (!element) throw new Error(`Unknown browser element handle: ${id}`);
    return element;
  }

  function readVfsPath(memory, pointer) {
    return readCStringBytes(memory, pointer);
  }

  return {
    create(tagPointer) {
      const element = document.createElement(readString(tagPointer));
      const id = nextElementId++;
      elements.set(id, element);
      return id;
    },
    append(parentId, childId) {
      getElement(parentId).appendChild(getElement(childId));
    },
    clear(id) {
      getElement(id).replaceChildren();
    },
    set_text(id, textPointer) {
      getElement(id).textContent = readString(textPointer);
    },
    set_style(id, propertyPointer, valuePointer) {
      getElement(id).style.setProperty(readString(propertyPointer), readString(valuePointer));
    },
    set_attribute(id, namePointer, valuePointer) {
      getElement(id).setAttribute(readString(namePointer), readString(valuePointer));
    },
    on_enter(id) {
      getElement(id).addEventListener("keydown", event => {
        if (event.key !== "Enter") return;
        event.preventDefault();
        const kernel = getKernel();
        if (kernel) kernel.exports.ros_terminal_submit();
      });
    },
    bash_execute(argc, argvPointer) {
      const kernel = getKernel();
      const bash = getBash();
      if (!kernel || !bash) throw new Error("The shell module is not running");
      return executeBash(kernel, bash, argc, argvPointer);
    },
    get_value(id, bufferPointer, capacity) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      if (!Number.isInteger(bufferPointer) || !Number.isInteger(capacity) ||
          bufferPointer < 0 || capacity < 0 ||
          bufferPointer + capacity > kernel.exports.memory.buffer.byteLength) {
        throw new RangeError("Invalid Wasm buffer");
      }
      const bytes = encoder.encode(getElement(id).value);
      const memory = new Uint8Array(kernel.exports.memory.buffer);
      const length = Math.min(bytes.length, Math.max(0, capacity - 1));
      memory.fill(0, bufferPointer, bufferPointer + capacity);
      memory.set(bytes.subarray(0, length), bufferPointer);
      return length;
    },
    set_value(id, valuePointer) {
      getElement(id).value = readString(valuePointer);
    },
    focus(id) {
      getElement(id).focus();
    },
    vfs_count() {
      return vfs.count();
    },
    vfs_path(index, outputPointer, outputCapacity) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const paths = vfs.list();
      if (!Number.isInteger(index) || index < 0 || index >= paths.length) return 0;
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const bytes = encoder.encode(paths[index]);
      const cap = Math.min(Math.max(0, outputCapacity), bytes.length + 1);
      const destination = checkedMemoryRange(kernelMemory, outputPointer, cap);
      destination.set(bytes.subarray(0, cap - 1));
      if (cap > 0) destination[cap - 1] = 0;
      return bytes.length;
    },
    vfs_read(pathPointer, outputPointer, outputCapacity) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(kernelMemory, pathPointer);
      const data = vfs.read(path) || new Uint8Array();
      if (data.length > outputCapacity) return 0xfffffffe;
      const destination = checkedMemoryRange(kernelMemory, outputPointer, data.length);
      destination.set(data);
      return data.length;
    },
    vfs_write(pathPointer, dataPointer, dataLength) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(kernelMemory, pathPointer);
      const data = checkedMemoryRange(kernelMemory, dataPointer, dataLength);
      return vfs.write(path, data) ? 1 : 0;
    },
    vfs_delete(pathPointer) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(kernelMemory, pathPointer);
      return vfs.remove(path) ? 1 : 0;
    },
    vfs_mkdir(pathPointer) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(kernelMemory, pathPointer);
      return vfs.mkdir(path) ? 1 : 0;
    }
  };
}

function executeBash(kernel, bash, argc, argvPointer) {
  if (!Number.isInteger(argc) || argc < 1 || argc > 16) {
    throw new RangeError("Invalid argument count passed to bash");
  }
  const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
  const bashMemory = new Uint8Array(bash.exports.memory.buffer);
  if (!Number.isInteger(argvPointer) || argvPointer < 0 ||
      argvPointer + argc * 4 > kernelMemory.length) {
    throw new RangeError("Invalid argument vector passed to bash");
  }

  const argumentBytes = [];
  let totalLength = 0;
  for (let i = 0; i < argc; i++) {
    const pointer = new DataView(kernelMemory.buffer).getUint32(argvPointer + i * 4, true);
    const bytes = readCStringBytes(kernelMemory, pointer);
    totalLength += bytes.length + 1;
    if (totalLength > 4096) throw new RangeError("Bash arguments are too large");
    argumentBytes.push(bytes);
  }

  const argumentPointer = bash.exports.bash_arguments_buffer();
  const destination = checkedMemoryRange(bashMemory, argumentPointer, totalLength);
  let offset = 0;
  for (const bytes of argumentBytes) {
    destination.set(bytes, offset);
    offset += bytes.length;
    destination[offset++] = 0;
  }
  return bash.exports.bash_execute(argc);
}

function createBashApi(getKernel, getBash, vfs) {
  return {
    write(pointer, length) {
      const kernel = getKernel();
      const bash = getBash();
      if (!kernel || !bash) throw new Error("The shell module is not running");
      const source = checkedMemoryRange(
        new Uint8Array(bash.exports.memory.buffer), pointer, length
      );
      const bridgePointer = kernel.exports.ros_bash_bridge_buffer();
      const capacity = kernel.exports.ros_bash_bridge_capacity();
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      for (let offset = 0; offset < source.length; offset += capacity) {
        const chunk = source.subarray(offset, Math.min(offset + capacity, source.length));
        checkedMemoryRange(kernelMemory, bridgePointer, chunk.length).set(chunk);
        kernel.exports.ros_bash_write(chunk.length);
      }
    },
    clear() {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      kernel.exports.ros_bash_clear();
    },
    file_write(pathPointer, dataPointer, dataLength) {
      const bash = getBash();
      if (!bash) throw new Error("The shell module is not running");
      const bashMemory = new Uint8Array(bash.exports.memory.buffer);
      const path = readCStringBytes(bashMemory, pathPointer);
      const data = checkedMemoryRange(bashMemory, dataPointer, dataLength);
      return vfs.write(path, data) ? 1 : 0;
    },
    file_remove(pathPointer) {
      const bash = getBash();
      if (!bash) throw new Error("The shell module is not running");
      const bashMemory = new Uint8Array(bash.exports.memory.buffer);
      return vfs.remove(readCStringBytes(bashMemory, pathPointer)) ? 1 : 0;
    },
    dir_make(pathPointer) {
      const bash = getBash();
      if (!bash) throw new Error("The shell module is not running");
      const bashMemory = new Uint8Array(bash.exports.memory.buffer);
      return vfs.mkdir(readCStringBytes(bashMemory, pathPointer)) ? 1 : 0;
    },
    file_count() {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      return kernel.exports.ros_bash_file_count();
    },
    file_path(index, outputPointer, outputCapacity) {
      const kernel = getKernel();
      const bash = getBash();
      if (!kernel || !bash) throw new Error("The shell module is not running");
      const length = kernel.exports.ros_bash_file_path(index);
      if (!length || length + 1 > outputCapacity) return 0;
      const bridgePointer = kernel.exports.ros_bash_bridge_buffer();
      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const bashMemory = new Uint8Array(bash.exports.memory.buffer);
      const source = checkedMemoryRange(kernelMemory, bridgePointer, length);
      const destination = checkedMemoryRange(bashMemory, outputPointer, length + 1);
      destination.set(source);
      destination[length] = 0;
      return length;
    },
    file_read(pathPointer, outputPointer, outputCapacity) {
      const kernel = getKernel();
      const bash = getBash();
      if (!kernel || !bash) throw new Error("The shell module is not running");
      const bashMemory = new Uint8Array(bash.exports.memory.buffer);
      const path = readCStringBytes(bashMemory, pathPointer);
      const bridgePointer = kernel.exports.ros_bash_bridge_buffer();
      const bridgeCapacity = kernel.exports.ros_bash_bridge_capacity();
      if (path.length >= bridgeCapacity) return 0xffffffff;

      const kernelMemory = new Uint8Array(kernel.exports.memory.buffer);
      const bridge = checkedMemoryRange(kernelMemory, bridgePointer, path.length + 1);
      bridge.set(path);
      bridge[path.length] = 0;
      const size = kernel.exports.ros_bash_file_read(path.length);
      if (size === 0xffffffff || size === 0xfffffffe) return size;
      if (size > outputCapacity) return 0xfffffffe;

      const contents = checkedMemoryRange(kernelMemory, bridgePointer, size);
      checkedMemoryRange(bashMemory, outputPointer, size).set(contents);
      return size;
    },
    vfs_count() {
      return vfs.count();
    },
    vfs_path(index, outputPointer, outputCapacity) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const paths = vfs.list();
      if (!Number.isInteger(index) || index < 0 || index >= paths.length) return 0;
      const bytes = encoder.encode(paths[index]);
      const memory = new Uint8Array(kernel.exports.memory.buffer);
      const cap = Math.min(Math.max(0, outputCapacity), bytes.length + 1);
      const destination = checkedMemoryRange(memory, outputPointer, cap);
      destination.set(bytes.subarray(0, cap - 1));
      if (cap > 0) destination[cap - 1] = 0;
      return bytes.length;
    },
    vfs_read(pathPointer, outputPointer, outputCapacity) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const memory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(memory, pathPointer);
      const data = vfs.read(path) || new Uint8Array();
      if (data.length > outputCapacity) return 0xfffffffe;
      checkedMemoryRange(memory, outputPointer, data.length).set(data);
      return data.length;
    },
    vfs_write(pathPointer, dataPointer, dataLength) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const memory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(memory, pathPointer);
      const data = checkedMemoryRange(memory, dataPointer, dataLength);
      return vfs.write(path, data) ? 1 : 0;
    },
    vfs_delete(pathPointer) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const memory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(memory, pathPointer);
      return vfs.remove(path) ? 1 : 0;
    },
    vfs_mkdir(pathPointer) {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      const memory = new Uint8Array(kernel.exports.memory.buffer);
      const path = readCStringBytes(memory, pathPointer);
      return vfs.mkdir(path) ? 1 : 0;
    },
    disk_size() {
      const kernel = getKernel();
      if (!kernel) throw new Error("Kernel is not running");
      return kernel.exports.ros_bash_disk_size();
    }
  };
}

function checkedMemoryRange(memory, pointer, length) {
  if (!Number.isInteger(pointer) || !Number.isInteger(length) ||
      pointer < 0 || length < 0 || pointer + length > memory.length) {
    throw new RangeError("Invalid Wasm memory range");
  }
  return memory.subarray(pointer, pointer + length);
}

function readCStringBytes(memory, pointer) {
  if (!Number.isInteger(pointer) || pointer < 0 || pointer >= memory.length) {
    throw new RangeError("Invalid string pointer passed from Wasm");
  }
  let end = pointer;
  while (end < memory.length && memory[end] !== 0) end++;
  if (end === memory.length) throw new RangeError("Unterminated string passed from Wasm");
  return memory.subarray(pointer, end);
}

function showBootError(error) {
  document.body.replaceChildren();
  Object.assign(document.body.style, {
    margin: "0",
    minHeight: "100vh",
    padding: "32px",
    boxSizing: "border-box",
    background: "#10131a",
    color: "#ffb4b4",
    font: "16px ui-monospace, monospace"
  });
  const heading = document.createElement("h1");
  heading.textContent = "rOS boot failed";
  const detail = document.createElement("pre");
  detail.textContent = error instanceof Error ? error.message : String(error);
  document.body.append(heading, detail);
  console.error("[biosapi] Boot failed:", error);
}

boot().catch(showBootError);
