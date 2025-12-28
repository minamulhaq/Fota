#ifndef _INC_FOTA_API_H__
#define _INC_FOTA_API_H__

#include "common_defines.h"
#include "versions.h"

void fota_api_get_app_version(fw_version_t *const version);
void fota_api_set_app_info(fota_shared_t *const fota);
void fota_api_get_app_info(fota_shared_t *const fota_shared);

#endif // _INC_FOTA_API_H__
