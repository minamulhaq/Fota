from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
)


class CommandGetHelp(Command):
    @property
    def packet(self) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_HELP.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_HELP.value,
            description=f"ID: {CommandIDs.B_CMD_GET_HELP.value} | {CommandIDs.B_CMD_GET_HELP.value:#04X} | Get Supported Commands",
            nemonic="CommandGetHelp",
        )
