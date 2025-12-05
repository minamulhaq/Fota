#include "fota_api.h"

extern uint8_t _fota_shared_data_start;
void fota_api_get_app_version(version_t *const version)
{
	if (NULL != version) {
		version_t *v = (void *)(_fota_shared_data_start);
		memcpy(version, (void *)v, sizeof(version_t));
	}
}

void fota_api_set_app_version(version_t const *const version)
{
	if (NULL != version) {
		version_t *v = (version_t *)(_fota_shared_data_start);
		memcpy(v, version, sizeof(version_t));
	}
}