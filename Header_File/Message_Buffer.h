/*
==============================
|| Created By YAHYA Mohamed ||
==============================
*/

// Message send process

#ifndef OS_STARTER_CODE_MESSAGE_BUFFER_H
#define OS_STARTER_CODE_MESSAGE_BUFFER_H

#include "Process_Struct.h"

typedef struct Message_Buffer {
    long mType;
    Process mProcess;
} Message;
#endif //OS_STARTER_CODE_MESSAGEBUFFER_H
