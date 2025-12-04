from dataclasses import dataclass

from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandFWUpdateSync(Command):
    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_SYNC

    @property
    def packet(self) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Command FW Update Sync",
        )

    def handle_response(self, response_packet: Packet) -> dict:
        print(f"{'=' * 60}")
        print(f"HANDLING {self.info.nemonic}")
        print(f"{'=' * 60}")

        result = {}
        return result
