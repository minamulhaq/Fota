from bl_monitor.command import (
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
            nemonic="Get Read Protection Level",
        )
