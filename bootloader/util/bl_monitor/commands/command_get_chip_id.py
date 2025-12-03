from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandGetChipID(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_CID.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_CID.value,
            description=f"ID: {CommandIDs.B_CMD_GET_CID.value} | {CommandIDs.B_CMD_GET_CID.value:#04X} | Get Chip ID",
            nemonic="CommandGetChipID",
        )
