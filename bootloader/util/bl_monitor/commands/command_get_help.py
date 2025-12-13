from ..command import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
)


class CommandGetHelp(Command):
    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_HELP.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_HELP.value,
            nemonic="Get Supported Commands",
        )

    @property
    def next_command(self) -> list["Command"]:
        return []

