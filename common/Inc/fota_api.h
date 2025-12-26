#ifndef _INC_FOTA_API_H__
#define _INC_FOTA_API_H__

#include "common_defines.h"
#include "versions.h"

void fota_api_get_app_version(fw_version_t *const version);
void fota_api_set_app_version(fw_version_t const *const version);
void fota_api_get_app_info(fw_info_t *const info);

#endif // _INC_FOTA_API_H__
