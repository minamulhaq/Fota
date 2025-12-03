from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandGetBootloaderVersion(Command):
    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_GET_BOOTLOADER_VERSION

    @property
    def packet(self) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            description=f"ID: {self.cmd_id.value} | {self.cmd_id.value:#04X} | Get Bootloader Version",
            nemonic="CommandGetBootloaderVersion",
        )

    def handle_response(self, response_packet: Packet) -> dict:
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
        print(f"{'=' * 60}")
        print(f"HANDLING GET_VERSION RESPONSE")
        print(f"{'=' * 60}")

        result = {}

        # Check if ACK
        if response_packet.id == ResponseType.B_ACK.value:
            print(f"[HANDLER] Received: ACK")

            # Validate payload
            if response_packet.length != 3:
                print(
                    f"[HANDLER] ERROR: Expected 3-byte payload, got {response_packet.length}"
                )
                result["success"] = False
                result["error"] = f"Invalid payload length: {response_packet.length}"
                return result

            if not response_packet.payload or len(response_packet.payload) < 3:
                print(f"[HANDLER] ERROR: Payload missing or incomplete")
                result["success"] = False
                result["error"] = "Payload missing"
                return result

            # Parse version
            major = response_packet.payload[0]
            minor = response_packet.payload[1]
            patch = response_packet.payload[2]

            version_string = f"{major}.{minor}.{patch}"

            print(f"[HANDLER] Version parsed:")
            print(f"          Major: {major}")
            print(f"          Minor: {minor}")
            print(f"          Patch: {patch}")
            print(f"          Version: {version_string}")

            result["success"] = True
            result["version"] = version_string
            result["major"] = major
            result["minor"] = minor
            result["patch"] = patch

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

            result["success"] = False
            result["error_code"] = error_code
            result["error_desc"] = error_desc

        else:
            print(
                f"[HANDLER] ERROR: Unexpected response ID: 0x{response_packet.id:02X}"
            )
            result["success"] = False
            result["error"] = f"Unexpected response ID: 0x{response_packet.id:02X}"

        print(f"{'=' * 60}\n")

        return result
