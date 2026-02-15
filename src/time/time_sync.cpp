#include "time/time_sync.h"
#include <time.h>

namespace timeutil {

void beginNtp() {
  // UTC (0,0). La timezone la gestisci dopo se ti serve.
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}

bool isTimeValid() {
  time_t now;
  time(&now);
  return now > 1700000000; // ~2023, evita 0
}

uint32_t unixTime() {
  if (!isTimeValid()) return 0;
  return (uint32_t)time(nullptr);
}

} // namespace timeutil