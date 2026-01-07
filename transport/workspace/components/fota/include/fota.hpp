#ifndef _INCLUDE_FOTA_HPP__
#define _INCLUDE_FOTA_HPP__
#include "command.hpp"
namespace fota
{
    class FotaTransport
    {
    private:
        const char *TAG = "FOTA";

    public:
        FotaTransport();
        ~FotaTransport();
    };
} // namespace fota

#endif // _INCLUDE_FOTA_HPP__
