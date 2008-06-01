sort.exe build/.map > arm9_sort.map
arm-eabi-objdump.exe -a mspv21_midrcp.arm9.elf > arm9_objdump_a.txt
arm-eabi-objdump.exe -f mspv21_midrcp.arm9.elf > arm9_objdump_f.txt
arm-eabi-objdump.exe -p mspv21_midrcp.arm9.elf > arm9_objdump_p.txt
arm-eabi-objdump.exe -h mspv21_midrcp.arm9.elf > arm9_objdump_h.txt
arm-eabi-objdump.exe -x mspv21_midrcp.arm9.elf > arm9_objdump_x.txt
arm-eabi-objdump.exe -s mspv21_midrcp.arm9.elf > arm9_objdump_s.txt
arm-eabi-objdump.exe -S mspv21_midrcp.arm9.elf > arm9_objdump_src.txt
