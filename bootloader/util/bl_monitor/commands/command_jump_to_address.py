from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandJumpToAddress(Command):
    def __init__(self, address: int):
        self.address = address

    @property
    def packet(self) -> Packet:
        # Address as 4 bytes little-endian
        payload = list(self.address.to_bytes(4, byteorder="little"))
        return Packet(id=CommandIDs.B_CMD_JMP_TO_ADDR.value, length=4, payload=payload)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_JMP_TO_ADDR.value,
            description=f"ID: {CommandIDs.B_CMD_JMP_TO_ADDR.value} | {CommandIDs.B_CMD_JMP_TO_ADDR.value:#04X} | Jump to Address: 0x{self.address:08X}",
            nemonic="CommandJumpToAddress",
        )
