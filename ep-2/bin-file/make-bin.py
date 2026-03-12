rom = bytearray([0xea]*0x8000)

rom[0x7ffc] = 0x00
rom[0x7ffd] = 0x80

with open('./rom.bin', 'wb') as out_file:
    out_file.write(rom)

print("Success!")