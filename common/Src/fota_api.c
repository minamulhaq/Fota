#include "common_defines.h"
#include "fota_api.h"

extern uint8_t _fota_shared_data_start[];
void fota_api_get_app_version(version_t *const version)
{
	if (version == NULL)
		return;

	// Pointer to start of shared region
	const void *src = (const void *)&_fota_shared_data_start[0];

	memcpy(version, src, sizeof(version_t));
}

void fota_api_set_app_version(version_t const *const version)
{
	if (version == NULL)
		return;

	// Pointer to start of shared region
	void *dst = (void *)&_fota_shared_data_start[0];

	memcpy(dst, version, sizeof(version_t));
}
