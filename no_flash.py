Import("env")

# Patch LINKFLAGS to swap memmap_default.ld -> memmap_no_flash.ld
# PlatformIO's picosdk.py bakes the linker script path directly into LINKFLAGS,
# so env.Replace(LDSCRIPT_PATH=...) doesn't work. We patch the array directly.
linkflags = env.get("LINKFLAGS", [])
for i, flag in enumerate(linkflags):
    if isinstance(flag, str) and "memmap_default.ld" in flag:
        linkflags[i] = flag.replace("memmap_default.ld", "memmap_no_flash.ld")
        print(f"no_flash.py: patched linker script -> {linkflags[i]}")
        break
else:
    print("no_flash.py: WARNING - could not find memmap_default.ld in LINKFLAGS")
