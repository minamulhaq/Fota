from typing import Optional

from bl_monitor.command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandFWUpdateSync(Command):
    @property
    def next_command(self) -> Optional["Command"]:
        return None

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

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
        print(f"{'=' * 60}")
        print(f"HANDLING {self.info.nemonic}")
        print(f"{'=' * 60}")

        response = CommandExecutionResponse()
        if response_packet.payload:
            response.execution_status = (
                response_packet.payload[0] == ResponseType.B_ACK.value
            )

        print(f"{'=' * 70}")
        print("COMMAND EXECUTION COMPLETE")
        print(f"{'=' * 70}\n")

        return CommandExecutionResponse(execution_status=True, run_next_command=True)
