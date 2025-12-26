
#include "bootloader_cmds.h"
#include "usart.h"
#include "packet_controller.h"

static packet_controller_t pcontroller = { 0 };
static bool
cmd_get_bootloader_version_process(comms_packet_t *const last_received_packet,
				   comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	response_packet->command_id = B_ACK;
	response_packet->length = sizeof(bootloader_version);
	memcpy(response_packet->payload, &bootloader_version,
	       sizeof(bootloader_version));
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bool
cmd_get_app_version_process(comms_packet_t *const last_received_packet,
			    comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	fw_version_t v = { 0 };
	bootloader_read_app_version(&v);
	response_packet->command_id = B_ACK;
	response_packet->length = sizeof(fw_version_t);
	memcpy(response_packet->payload, &v, sizeof(fw_version_t));
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bool
cmd_synced_nack_invalid_command(comms_packet_t *const last_received_packet,
				comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	response_packet->command_id = B_NACK;
	response_packet->length = 1;
	response_packet->payload[0] = ERROR_INVALID_COMMAND;
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bool cmd_fw_synced_process(comms_packet_t *const last_received_packet,
				  comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	response_packet->command_id = B_ACK;
	response_packet->length = 0;
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bool
cmd_fw_verify_device_id_process(comms_packet_t *const last_received_packet,
				comms_packet_t *const response_packet)
{
	uint16_t chip_id = DBGMCU->IDCODE & 0xFFFF;
	bool is_valid = *(uint16_t *)(&last_received_packet->payload) ==
			chip_id;
	response_packet->command_id = is_valid ? B_ACK : B_NACK;
	response_packet->length = 0;
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bool
cmd_fw_send_bin_size_process(comms_packet_t *const last_received_packet,
			     comms_packet_t *const response_packet)
{
	uint32_t fwsize = *(uint32_t *)&last_received_packet->payload;

	packet_controller_init(&pcontroller, fwsize);

	uint32_t *pl = (uint32_t *)&response_packet->payload;

	pl[0] = pcontroller.current_flash_address;
	pl[1] = pcontroller.total_packets;

	response_packet->command_id = B_ACK;
	response_packet->length = 2 * sizeof(uint32_t);
	response_packet->crc = bootloader_compute_crc(response_packet);
	bootloader_erase_shared_plus_app();
	HAL_FLASH_Unlock();
	return true;
}

static bool
cmd_fw_send_bin_in_packets(comms_packet_t *const last_received_packet,
			   comms_packet_t *const response_packet)
{
	uint8_t buffer[MAX_PAYLOAD_SIZE];
	memset(buffer, 0xFF, MAX_PAYLOAD_SIZE);
	memcpy(buffer, last_received_packet->payload,
	       last_received_packet->length);

	uint64_t dw1, dw2;
	memcpy(&dw1, &buffer[0], 8);
	memcpy(&dw2, &buffer[8], 8);

	if ((pcontroller.current_packet_number < pcontroller.total_packets) &&
	    !(pcontroller.error_occured)) {
		bool status1 = bootloader_flash_double_word(
			pcontroller.current_flash_address, dw1);
		pcontroller.current_flash_address += 8;

		bool status2 = bootloader_flash_double_word(
			pcontroller.current_flash_address, dw2);
		pcontroller.current_flash_address += 8;

		if (status1 && status2) {
			pcontroller.current_packet_number += 1;
			uint32_t *pl = (uint32_t *)&response_packet->payload;
			pl[0] = pcontroller.current_flash_address;
			pl[1] = pcontroller.current_packet_number;

			response_packet->length = 2 * sizeof(uint32_t);
			response_packet->command_id = B_ACK;
			pcontroller.error_occured = false;
		} else {
			pcontroller.error_occured = true;
			response_packet->command_id = B_NACK;
			response_packet->length = 0;
		}
	}
	response_packet->crc = bootloader_compute_crc(response_packet);
	if (pcontroller.current_packet_number >= (pcontroller.total_packets)) {
		HAL_FLASH_Lock();
		packet_controller_reset(&pcontroller);
		return false;
	}
	return true;
}

bool bootloader_is_app_flash_finished(void)
{
	return (pcontroller.current_packet_number >=
		(pcontroller.total_packets));
}

static bool cmd_get_chip_id_process(comms_packet_t *const last_received_packet,
				    comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	response_packet->command_id = B_ACK;
	uint16_t chip_id = DBGMCU->IDCODE & 0xFFFF;
	response_packet->payload[0] = (uint8_t)(chip_id & 0xFF);
	response_packet->payload[1] = (uint8_t)((chip_id >> 8) & 0xFF);
	response_packet->length = 2;
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bool
cmd_retransmit_last_packet(comms_packet_t *const last_received_packet,
			   comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	bootlader_get_last_transmitted_packet(response_packet);
	return true;
}

static bool
retransmit_response_handle(comms_packet_t *const last_received_packet,
			   comms_packet_t *const response_packet)
{
	(void)last_received_packet;
	response_packet->command_id = B_RETRANSMIT;
	response_packet->length = 0U;
	uint32_t crc = bootloader_compute_crc(response_packet);
	response_packet->crc = crc;
	return true;
}

static bootloader_cmd_t RESPONSE_GET_BOOTLOADER_VERSION = {
	.send_response = true,
	.command_id = B_CMD_GET_BOOTLOADER_VERSION,
	.process = cmd_get_bootloader_version_process
};

static bootloader_cmd_t RESPONSE_GET_APP_VERSION = {
	.send_response = true,
	.command_id = B_CMD_GET_APP_VERSION,
	.process = cmd_get_app_version_process
};

static bootloader_cmd_t RESPONSE_FW_SEND_SYNCED = {
	.send_response = true,
	.command_id = B_CMD_FW_SYNC,
	.process = cmd_fw_synced_process
};

static bootloader_cmd_t RESPONSE_FW_SEND_VERIFY_DEVICE_ID = {
	.send_response = true,
	.command_id = B_CMD_FW_VERIFY_DEVICE_ID,
	.process = cmd_fw_verify_device_id_process
};

static bootloader_cmd_t RESPONSE_FW_SEND_BIN_SIZE = {
	.send_response = true,
	.command_id = B_CMD_FW_SEND_BIN_SIZE,
	.process = cmd_fw_send_bin_size_process
};

static bootloader_cmd_t RESPONSE_FW_SEND_BIN_IN_PACKETS = {
	.send_response = true,
	.command_id = B_CMD_FW_SEND_BIN_IN_PACKETS,
	.process = cmd_fw_send_bin_in_packets,

};

static bootloader_cmd_t RESPONSE_SEND_CHIP_ID = {
	.send_response = true,
	.command_id = B_CMD_GET_CHIP_ID,
	.process = cmd_get_chip_id_process
};

static bootloader_cmd_t RESPONSE_SEND_NACK_INVALID_COMMAND = {
	.send_response = true,
	.process = cmd_synced_nack_invalid_command
};

static bootloader_cmd_t RESPONSE_REQUEST_CLIENT_RETRANSMIT_REQUEST = {
	.command_id = B_RETRANSMIT,
	.process = retransmit_response_handle
};

static bootloader_cmd_t CMD_RETRANSMIT_LAST_PACKET_TO_CLIENT = {
	.command_id = B_ACK,
	.process = cmd_retransmit_last_packet
};

bootloader_cmd_t *cmd_send_retransmit_last_cmd(void)
{
	bootloader_cmd_t *cmd = &RESPONSE_REQUEST_CLIENT_RETRANSMIT_REQUEST;
	return cmd;
}
bootloader_cmd_t *get_command_handle(comms_packet_t const *const packet)
{
	bootloader_cmd_t *cmd;
	switch (packet->command_id) {
	case B_CMD_RETRANSMIT_LAST_PACKET_TO_CLIENT: {
		cmd = &CMD_RETRANSMIT_LAST_PACKET_TO_CLIENT;
		break;
	}

	case B_CMD_GET_BOOTLOADER_VERSION: {
		cmd = &RESPONSE_GET_BOOTLOADER_VERSION;
		break;
	}

	case B_CMD_GET_APP_VERSION: {
		cmd = &RESPONSE_GET_APP_VERSION;
		break;
	}

	case B_CMD_GET_CHIP_ID: {
		cmd = &RESPONSE_SEND_CHIP_ID;
		break;
	}

	case B_CMD_FW_SYNC: {
		cmd = &RESPONSE_FW_SEND_SYNCED;
		break;
	}

	case B_CMD_FW_VERIFY_DEVICE_ID: {
		cmd = &RESPONSE_FW_SEND_VERIFY_DEVICE_ID;
		break;
	}

	case B_CMD_FW_SEND_BIN_SIZE: {
		cmd = &RESPONSE_FW_SEND_BIN_SIZE;
		break;
	}

	case B_CMD_FW_SEND_BIN_IN_PACKETS: {
		cmd = &RESPONSE_FW_SEND_BIN_IN_PACKETS;
		break;
	}

	default:
		cmd = &RESPONSE_SEND_NACK_INVALID_COMMAND;
		break;
	}

	return cmd;
}

/*
void bootloader_send_command_response(CRC_VERIFICATION v, bootloader_cmd *cmd)
{
	BootloaderResponseCode code = v == VERIFY_CRC_SUCCESS ? B_ACK : B_NACK;
	uint16_t length = 2 + cmd->response_length;
	// uint8_t* response = (uint8_t*)malloc(sizeof(uint8_t) * length);
	uint8_t *response = (uint8_t *)calloc(length, sizeof(uint8_t));
	response[0] = code;
	if (code == B_ACK) {
		response[1] = length - 2;
		cmd->response_buffer(response);
	}
	HAL_UART_Transmit(&huart2, response, length, HAL_MAX_DELAY);
	free(response);
}

bootloader_cmd BootloaderCommandGetVersion = {
	B_CMD_GET_BOOTLOADER_VERSION, 0x03, bcmd_default_handle,
	bcmd_get_version_response_buffer
};
bootloader_cmd BootloaderCommandGetHelp = { B_CMD_GET_HELP, 0x00,
					    bcmd_get_help_handle,
					    bcmd_get_help_response_buffer };
bootloader_cmd BootloaderCommandGetCID = { B_CMD_GET_CID, 0x02,
					   bcmd_default_handle,
					   bcmd_get_cid_response_buffer };
bootloader_cmd BootloaderCommandGetRDPLevel = {
	B_CMD_GET_RDP_LVL, 0x01, bcmd_default_handle,
	bcmd_get_rdp_level_response_buffer
};
bootloader_cmd BootloaderCommandJumpToAddress = {
	B_CMD_JMP_TO_ADDR, 0x00, bcmd_jump_to_addr_handle,
	bcmd_jump_to_addr_response_buffer
};
bootloader_cmd BootloaderCommandEraseFlash = {
	B_CMD_ERASE_FLASH, 0x00, bcmd_erase_flash_handle,
	bcmd_erase_flash_response_buffer
};

bootloader_cmd *cmd_table[] = {
	&BootloaderCommandGetVersion,	 &BootloaderCommandGetHelp,
	&BootloaderCommandGetCID,	 &BootloaderCommandGetRDPLevel,
	&BootloaderCommandJumpToAddress, &BootloaderCommandEraseFlash,
};

uint8_t cmd_table_size = sizeof(cmd_table) / sizeof(cmd_table[0]);

void bcmd_default_handle(bootloader_cmd *cmd, uint8_t *data)
{
	uint32_t command_packet_len = data[0] + 1;
	uint32_t host_crc = *((uint32_t *)(data + command_packet_len - 4));
	CRC_VERIFICATION v =
		bootloader_verify_crc(data, command_packet_len - 4, host_crc);
	bootloader_send_command_response(v, cmd);
}

void bcmd_get_version_response_buffer(uint8_t *buffer)
{
	uint8_t offset = 2;
	*(buffer + offset) = MAJOR_VERSION;
	*(buffer + offset + 1) = MINOR_VERSION;
	*(buffer + offset + 2) = PATCH_VERSION;
}

void bcmd_get_help_handle(bootloader_cmd *cmd, uint8_t *data)
{
	uint32_t command_packet_len = data[0] + 1;
	// char* version = get_bootloader_version();
	uint32_t host_crc = *((uint32_t *)(data + command_packet_len - 4));
	cmd->response_length = cmd_table_size;

	CRC_VERIFICATION v =
		bootloader_verify_crc(data, command_packet_len - 4, host_crc);
	bootloader_send_command_response(v, cmd);
}
void bcmd_get_help_response_buffer(uint8_t *buffer)
{
	uint8_t offset = 2;
	for (uint8_t i = 0; i < cmd_table_size; i++) {
		*(buffer + i + offset) = cmd_table[i]->id;
	}
}

void bcmd_get_cid_response_buffer(uint8_t *buffer)
{
	uint8_t offset = 2;
	uint16_t cid = (DBGMCU->IDCODE) & 0xFFFF;
	*(uint16_t *)(buffer + offset) = cid;
}

void bcmd_get_rdp_level_response_buffer(uint8_t *buffer)
{
	uint8_t offset = 2;
	FLASH_OBProgramInitTypeDef ob;
	HAL_FLASHEx_OBGetConfig(&ob);
	*(buffer + offset) = (uint8_t)ob.RDPLevel;
}

uint8_t verify_address_space(uint32_t address)
{
	if (address < PERIPH_BASE) {
		return 0;
	}
	return 1;
}

void bcmd_jump_to_addr_handle(bootloader_cmd *cmd, uint8_t *data)
{
	uint32_t command_packet_len = data[0] + 1;
	uint32_t address = *((uint32_t *)(data + 2));
	uint32_t host_crc = *((uint32_t *)(data + command_packet_len - 4));
	CRC_VERIFICATION v =
		bootloader_verify_crc(data, command_packet_len - 4, host_crc);

	if (!verify_address_space(address)) {
		bootloader_send_command_response(v, cmd);
		address += 1; // Make T(THUMB) bit 1
		// 0x800b551
		void (*jump_to_addr)(void) = (void *)(address);
		jump_to_addr();
		return;
	}

	// CRC_VERIFICATION w = VERIFY_CRC_FAILED;
	// bootloader_send_command_response(v, cmd);
}

void bcmd_jump_to_addr_response_buffer(uint8_t *buffer)
{
	uint8_t offset = 2;
}

void bcmd_erase_flash_handle(bootloader_cmd *cmd, uint8_t *data)
{
	uint32_t command_packet_len = data[0] + 1;
	uint8_t bank = data[2];
	uint8_t start_page = data[3];
	uint8_t nb_pages = data[4];
	uint32_t host_crc = *((uint32_t *)(data + command_packet_len - 4));
	CRC_VERIFICATION v =
		bootloader_verify_crc(data, command_packet_len - 4, host_crc);
	bootloader_send_command_response(v, cmd);

	FLASH_EraseInitTypeDef flash_erase_handle;
	flash_erase_handle.TypeErase = FLASH_TYPEERASE_PAGES;
	// Set the correct bank
	if (bank == 1) {
		flash_erase_handle.Banks = FLASH_BANK_1;
	} else if (bank == 2) {
		flash_erase_handle.Banks = FLASH_BANK_2;
	}
	flash_erase_handle.Page = start_page;
	flash_erase_handle.NbPages = nb_pages;

	uint32_t sector_Error = 0;
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
	HAL_FLASH_Unlock();
	HAL_StatusTypeDef ret =
		HAL_FLASHEx_Erase(&flash_erase_handle, &sector_Error);
	HAL_FLASH_Lock();
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
	ret;
}
void bcmd_erase_flash_response_buffer(uint8_t *buffer)
{
}
*/