@cd disasm
@copy ..\*.bin
@call disasm1
@call disasm2
@call disasm3
@cd..
@copy disasm\*.txt
start cmd /C start write qcode.bin.txt
