# peripheral registers by name in gdb, read from the chip's svd
#
#   svd              every peripheral
#   svd SPI1         every register in SPI1, with its fields
#   svd SPI1 SR      one register
#
# the svd calls some of them SPI or I2C, so a name it doesn't know goes
# through gdb instead: SPI1 is a macro in st's header, and Debug builds keep
# the macros
import os
import xml.etree.ElementTree as ET

import gdb

SVD = os.path.join(os.path.dirname(__file__), "..", ".pyocd", "STM32H533.svd")


def num(text):
    return int(text, 0)


def fields(reg):
    for f in reg.iter("field"):
        if f.find("bitOffset") is not None:
            lsb = num(f.findtext("bitOffset"))
            width = num(f.findtext("bitWidth"))
        else:
            # bitRange is [msb:lsb]
            msb, lsb = map(int, f.findtext("bitRange").strip("[]").split(":"))
            width = msb - lsb + 1
        yield f.findtext("name"), lsb, width


class Svd(gdb.Command):
    def __init__(self):
        super().__init__("svd", gdb.COMMAND_DATA)
        self.peripherals = None

    def load(self):
        # 7 MB of xml, so only on first use
        root = ET.parse(SVD).getroot()
        self.peripherals = {p.findtext("name"): p for p in root.iter("peripheral")}

    def at(self, expr):
        try:
            out = gdb.execute(f"macro expand {expr}", to_string=True)
            expanded = out.split("expands to: ", 1)[1]
            addr = int(gdb.parse_and_eval(f"(unsigned long)({expanded})"))
        except (gdb.error, IndexError):
            raise gdb.GdbError(f"no peripheral {expr}, `svd` lists them")
        for p in self.peripherals.values():
            if num(p.findtext("baseAddress")) == addr:
                return p
        raise gdb.GdbError(f"no peripheral at {addr:#x}")

    def registers(self, p):
        base = p.get("derivedFrom")
        return (p if base is None else self.peripherals[base]).iter("register")

    def show(self, base, reg):
        addr = base + num(reg.findtext("addressOffset"))
        raw = gdb.selected_inferior().read_memory(addr, 4)
        value = int.from_bytes(bytes(raw), "little")
        print(f"{reg.findtext('name'):<16} {addr:#010x} = {value:#010x}")
        for name, lsb, width in sorted(fields(reg), key=lambda f: -f[1]):
            bits = f"{lsb + width - 1}:{lsb}" if width > 1 else f"{lsb}"
            v = value >> lsb & ((1 << width) - 1)
            print(f"    {name:<14} [{bits:>5}] {v:#x}")

    def invoke(self, arg, from_tty):
        if self.peripherals is None:
            self.load()
        args = arg.upper().split()
        if not args:
            print(" ".join(sorted(self.peripherals)))
            return
        p = self.peripherals.get(args[0])
        if p is None:
            p = self.at(arg.split()[0])
        base = num(p.findtext("baseAddress"))
        for reg in self.registers(p):
            name = reg.findtext("name")
            # st names them SPI_SR, so SR finds it too
            if len(args) == 1 or args[1] in (name, name.split("_", 1)[-1]):
                self.show(base, reg)


Svd()
