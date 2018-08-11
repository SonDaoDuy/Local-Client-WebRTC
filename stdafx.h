// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
//#include "PracticalSocket.h"
#include <stdio.h>
#include <tchar.h>

#define FRAME_HEIGHT 240
#define FRAME_WIDTH 320
#define FRAME_INTERVAL (1000/30)
#define PACK_SIZE 4096 //udp pack size; note that OSX limits < 8100 bytes
#define ENCODE_QUALITY 80
#define BUF_LEN 65540

static int sock = 0;

// TODO: reference additional headers your program requires here
