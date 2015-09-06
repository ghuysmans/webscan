#ifndef _STDAFX_H
#define _STDAFX_H


//FIXME handle Win64 too ^^
#ifdef WIN32
	#include "targetver.h"
#else
	#include <unistd.h>
	#include <error.h>
	#include <sys/socket.h>
	#include <netdb.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#endif //_STDAFX_H