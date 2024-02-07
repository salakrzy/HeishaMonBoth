under windows 

run flash.bat 

or
run console CMD   and run 
esptool.exe -p COM7 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware\UART_HMBoth_v3_2_3.bin 0x00391000 littlefs.bin

or  if you prefer Python
pip install esptool
python esptool.py -p COM7 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 UART_HMBoth_v3_2_3.bin 0x00391000 littlefs.bin