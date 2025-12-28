from ..command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
)


class CommandGetChipID(Command):
    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=CommandIDs.B_CMD_GET_CHIP_ID.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=CommandIDs.B_CMD_GET_CHIP_ID.value,
            nemonic="Get Chip ID",
        )

    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_GET_CHIP_ID

    def handle_response(
        self, response_packet: Packet
    ) -> CommandExecutionResponse :
        response = CommandExecutionResponse()
        assert response_packet.payload
        chip_id_value = int.from_bytes(response_packet.payload, "little")
        response.data["chip_id"] = hex(chip_id_value)
        return response

    def getinput(self) -> None:
        return


    @property
    def next_command(self) -> list["Command"]:
        return []

