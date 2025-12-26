#include "common_defines.h"
#include "fota_api.h"

extern uint8_t _fota_shared_data_start[];
void fota_api_get_app_version(fw_version_t *const version)
{
	if (version == NULL)
		return;

	// Pointer to start of shared region
	const void *src = (const void *)&_fota_shared_data_start[0];

	memcpy(version, src, sizeof(fw_version_t));
}

void fota_api_set_app_version(fw_version_t const *const version)
{
	if (version == NULL)
		return;

	// Pointer to start of shared region
	void *dst = (void *)&_fota_shared_data_start[0];

	memcpy(dst, version, sizeof(fw_version_t));
}

void fota_api_get_app_info(fw_info_t *const info)
{
	if (info == NULL) {
		return;
	}
	fw_info_t *dst = (fw_info_t *)_fota_shared_data_start;
	memcpy(info, dst, sizeof(fw_info_t));
}
