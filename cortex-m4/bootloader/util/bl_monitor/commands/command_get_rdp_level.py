from ..command import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
)


class CommandGetRDPLevel(Command):
    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_RDP_LVL.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_RDP_LVL.value,
            nemonic="Get Read Protection Level",
        )

    @property
    def next_command(self) -> list["Command"]:
        return []

