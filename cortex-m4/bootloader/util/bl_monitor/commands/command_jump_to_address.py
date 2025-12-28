from ..command import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
)


class CommandJumpToAddress(Command):
    def __init__(self, address: int):
        self.address = address

    def packet(self, metadata: dict = {}) -> Packet:
        # Address as 4 bytes little-endian
        payload = list(self.address.to_bytes(4, byteorder="little"))
        return Packet(id=CommandIDs.B_CMD_JMP_TO_ADDR.value, length=4, payload=payload)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_JMP_TO_ADDR.value,
            nemonic=f"Jump to Address: 0x{self.address:08X}",
        )

    @property
    def next_command(self) -> list["Command"]:
        return []

