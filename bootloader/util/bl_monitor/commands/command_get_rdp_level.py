from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandGetRDPLevel(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_RDP_LVL.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_RDP_LVL.value,
            description=f"ID: {CommandIDs.B_CMD_GET_RDP_LVL.value} | {CommandIDs.B_CMD_GET_RDP_LVL.value:#04X} | Get Read Protection Level",
            nemonic="CommandGetRDPLevel",
        )
