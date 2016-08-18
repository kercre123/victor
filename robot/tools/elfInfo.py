from elftools.elf.elffile import ELFFile

HEADER_LENGTH = 128

def rom_info(file):
    with open(file, "rb") as fo:
        elffile = ELFFile(fo)

        segments = {}

        # Build rom segments
        for segment in elffile.iter_segments():
            fo.seek(segment['p_offset'])
            segments[segment['p_vaddr']] = fo.read(segment['p_filesz'])
            
        magic_location = None

        # Locate vector table + magic header
        for section in elffile.iter_sections():
            if type(section.name) is bytes and str != bytes: # Deal with API changes between versions of elftools
                section.name = section.name.decode()
            if section.name == "ER_IROM1":
                temp = fo.tell()
                fo.seek(section.header.sh_offset + 4)
                if fo.read(4) != b"CZM0":
                    raise Exception("Could not locate cozmo header")
                fo.seek(temp)

                magic_location = section.header.sh_offset + 12

        if magic_location is None:
            raise Exception("Could not locate cozmo header. Sections = {}".format(repr([s.name for s in elffile.iter_sections()])))

        # flatten rom image
        first = min(segments.keys())
        last = max([addr+len(data) for addr, data in segments.items() if addr < 0x40000])

        base_addr = first
        rom_data = b"\xFF" * (last-first)

        for addr, data in segments.items():
            addr = addr - first
            rom_data = rom_data[:addr] + data + rom_data[addr+len(data):]
    return rom_data, base_addr, magic_location
