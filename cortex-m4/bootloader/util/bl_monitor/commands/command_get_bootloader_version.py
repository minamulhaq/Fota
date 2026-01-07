from typing import Optional

from ..command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandGetBootloaderVersion(Command):
    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_GET_BOOTLOADER_VERSION

    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    @property
    def timeout(self) -> int:
        return 1


    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Get Bootloader Version",
        )

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
        """
        Handle GET_VERSION response.

        Expected response:
        - ACK (0xE0) with 3-byte payload: [major, minor, patch]
        - NACK (0xE1) with error code

        Args:
            response_packet: Validated packet from bootloader

        Returns:
            dict: {
                'success': bool,
                'version': str (if success),
                'major': int (if success),
                'minor': int (if success),
                'patch': int (if success),
                'error_code': int (if NACK)
            }
        """
        print(f"HANDLING {self.info.nemonic}")
        print(f"{'=' * 60}")

        response = CommandExecutionResponse()

        # Check if ACK
        response = CommandExecutionResponse()
        if response_packet.id == ResponseType.B_ACK.value:
            print(f"[HANDLER] Received: ACK")

            # Validate payload
            if response_packet.length != 3:
                print(
                    f"[HANDLER] ERROR: Expected 3-byte payload, got {response_packet.length}"
                )
                response.data["error"] = (
                    f"Invalid payload length: {response_packet.length}"
                )
                response.execution_success = False
                return response

            if not response_packet.payload or len(response_packet.payload) < 3:
                print(f"[HANDLER] ERROR: Payload missing or incomplete")
                response.execution_success = False
                response.data["error"] = "Payload missing"
                return response

            # Parse version
            major = response_packet.payload[0]
            minor = response_packet.payload[1]
            patch = response_packet.payload[2]

            version_string = f"{major}.{minor}.{patch}"

            response.execution_success = True
            response.data["version"] = version_string
            response.data["major"] = major
            response.data["minor"] = minor
            response.data["patch"] = patch

        # Check if NACK
        elif response_packet.id == ResponseType.B_NACK.value:
            print(f"[HANDLER] Received: NACK")

            error_code = response_packet.payload[0] if response_packet.payload else 0xFF

            error_names = {
                0x00: "SUCCESS (unexpected in NACK)",
                0x01: "INVALID_CMD",
                0x02: "INVALID_PARAMS",
                0x03: "EXECUTION_FAILED",
                0x04: "FLASH_ERROR",
                0x05: "ADDRESS_ERROR",
            }

            error_desc = error_names.get(error_code, "UNKNOWN_ERROR")

            print(f"[HANDLER] Error code: 0x{error_code:02X} ({error_desc})")

            response.data["success"] = False
            response.data["error_code"] = error_code
            response.data["error_desc"] = error_desc

        else:
            print(
                f"[HANDLER] ERROR: Unexpected response ID: 0x{response_packet.id:02X}"
            )
            response.data["success"] = False
            response.data["error"] = (
                f"Unexpected response ID: 0x{response_packet.id:02X}"
            )

        return response

    def getinput(self) -> None:
        return

    @property
    def next_command(self) -> list["Command"]:
        return []

