from bl_monitor.command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandGetChipID(Command):
    @property
    def packet(self) -> Packet:
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
    ) -> CommandExecutionResponse | None:
        response = CommandExecutionResponse()
        assert response_packet.payload
        chip_id_value = int.from_bytes(response_packet.payload, "little")
        response.data["chip_id"] = hex(chip_id_value)
        return response

    def getinput(self) -> None:
        raise NotImplementedError

