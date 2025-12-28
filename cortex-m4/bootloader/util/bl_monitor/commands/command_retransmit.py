from serial import Serial
from ..command import Command, CommandExecutionResponse, CommandIDs, CommandInfo, Packet


class CommandRetransmit(Command):
    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_RETRANSMIT

    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    @property
    def info(self) -> CommandInfo:
        cmd = self.cmd_id.value
        return CommandInfo(
            id=cmd,
            nemonic="Retransmit command",
        )

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
        return CommandExecutionResponse()

    def getinput(self) -> None:
        return

    @property
    def next_command(self) -> list["Command"]:
        return []

