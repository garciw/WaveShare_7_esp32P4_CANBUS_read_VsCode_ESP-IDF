#include "globalVariables.h"

// Define the receivedData instance
Maxxecu receivedData;
bool dataReceivedFlag = false;

 uint32_t lastPacketTime = 0; // 🛑 ESP-IDF change
bool connectionLost = false;