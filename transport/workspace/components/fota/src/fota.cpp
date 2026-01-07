#include "fota.hpp"
#include "esp_log.h"
#include "sdkconfig.h"
#include "command.hpp"

CommandRetransmit cmd_rtransmit;
CommandGetBootloaderVersion cmd_gbv;

const Command *supported[] = {
    &cmd_rtransmit,
    &cmd_gbv};

fota::FotaTransport::FotaTransport()
{
}


fota::FotaTransport::~FotaTransport() {}
