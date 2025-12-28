from typing import Optional

from ..command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)
from .command_fw_verify_device_id import CommandFWVerifyDeviceID


class CommandFWUpdateSync(Command):
    @property
    def next_command(self)  -> list[Command]:
        return [CommandFWVerifyDeviceID()]

    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_SYNC

    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    def getinput(self) -> None:
        return

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Command FW Update Sync",
        )

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:

        response = CommandExecutionResponse()
        if response_packet.payload:
            response.execution_success = (
                response_packet.payload[0] == ResponseType.B_ACK.value
            )

        return CommandExecutionResponse(execution_success=True)
