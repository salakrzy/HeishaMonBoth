echo Check if the COM port is not used by another application and has the correct number

esptool.exe -p COM7 --before default_reset --after hard_reset --chip esp32 --baud 921600 write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware\UART_HMBoth_v3_9.bin 0x00391000 littlefs.bin