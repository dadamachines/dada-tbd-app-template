Import("env")

# Override the linker script from memmap_default.ld to memmap_copy_to_ram.ld
# This makes the binary live in flash (Picoboot WebUSB can write it) while
# the CRT0 copies everything to SRAM before main() — safe to erase flash.
#
# PlatformIO's picosdk.py hardcodes memmap_default.ld into LINKFLAGS.
# env.Replace(LDSCRIPT_PATH=...) does NOT propagate, so we must patch
# LINKFLAGS directly. This script runs as post: (after framework setup).

found = False
for i, flag in enumerate(env["LINKFLAGS"]):
    s = str(flag)
    if "memmap_default.ld" in s:
        env["LINKFLAGS"][i] = s.replace("memmap_default.ld", "memmap_copy_to_ram.ld")
        print(">> LINKFLAGS[%d]: patched to memmap_copy_to_ram.ld" % i)
        found = True

if not found:
    print("!! WARNING: memmap_default.ld not found in LINKFLAGS")
