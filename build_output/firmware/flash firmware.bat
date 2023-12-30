rem install esptool usin command pip3 install esptool
rem connect USB to UART TTL converter to the UART0 connector on the HeishaMOnBoth board
rem close jumper PROGRAM on the HeishaMOnBoth board
rem connect UART conwerter do computer
rem run below command

python %EspTool%\esptool.py --port COM7 -b 921600 write_flash 0x10000 UART_HMBoth_v3.2.3.bin