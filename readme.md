Build with arduino-cli compile --fqbn rp2040:rp2040:generic_rp2350:variantchip=RP2530B,usbstack=tinyusb_host   --build-path ./build
Flash with cp build/tbd_test.ino.uf2 /Volumes/RP2350
