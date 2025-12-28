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

void fota_api_set_app_info(fota_shared_t *const fota)
{
	if (fota == NULL)
		return;

	// Pointer to start of shared region
	void *dst = (void *)&_fota_shared_data_start[0];
	memcpy(dst, fota, sizeof(fota_shared_t));
}

void fota_api_get_app_info(fota_shared_t *const fota_shared)
{
	if (fota_shared == NULL) {
		return;
	}
	fota_shared_t *dst = (fota_shared_t *)_fota_shared_data_start;
	memcpy(fota_shared, dst, sizeof(fota_shared_t));
}
