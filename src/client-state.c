#include "client-state.h"

#include <string.h>

void cs_reset() {
    memset(&cs_state, 0, sizeof(cs_state));
    cs_state.connection_fd = -1;
    cs_state.connected = -1;
}