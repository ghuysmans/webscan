#include "stdafx.h"

//Make this multiplatform by implementing an abstraction layer at least for X11
//http://stackoverflow.com/questions/1262310/simulate-keypress-in-a-linux-c-console-application

//Adds the given virtual key (with flags) at e[el++]
#define ENQUEUE(vk, flags) {\
	e[el].ki.wVk = vk; \
	e[el].ki.dwFlags = flags; \
	e[el].type = INPUT_KEYBOARD; \
	el++; \
	}

/**
 * Fills the system input buffer with keys converted from the given string.
 * @param s string to be queued
 * @param len length of s
 * @return 0 on success, -1 on memory allocation failure and a system error code on SendInput error
 */
int type(char *s, size_t len) {
	//worst case: Ctrl, Alt, Shift and a character (each time pressed and released).
	INPUT *e = calloc(8*len, sizeof(INPUT));
	size_t el = 0; //real length of e (in keystrokes)
	UINT ret; //SendInput return value
	if (!e)
		return -1;
	//create all press and release events
	//FIXME that's naïve but sub-optimal (ABCD gets converted to 16 events)
	for (; *s; s++) {
		WORD sc;
		if (*s == ',') {
			WORD vk = 0;
			switch (*(s + 1)) {
				case 't': vk = VK_TAB; break;
				case 'n': vk = VK_RETURN; break;
			}
			if (vk) {
				ENQUEUE(vk, 0);
				ENQUEUE(vk, KEYEVENTF_KEYUP);
				s++;
				continue; //don't display it
			}
			//else, don't try to interpret it
		}
		sc = VkKeyScan(*s);
		if (sc & 0x100) ENQUEUE(VK_LSHIFT, 0);
		if (sc & 0x200) ENQUEUE(VK_CONTROL, 0);
		if (sc & 0x400) ENQUEUE(VK_MENU, 0);
		ENQUEUE(sc & 0xFF, 0);
		ENQUEUE(sc & 0xFF, KEYEVENTF_KEYUP);
		if (sc & 0x400) ENQUEUE(VK_MENU, KEYEVENTF_KEYUP);
		if (sc & 0x200) ENQUEUE(VK_CONTROL, KEYEVENTF_KEYUP);
		if (sc & 0x100) ENQUEUE(VK_LSHIFT, KEYEVENTF_KEYUP);
	}
	ret = SendInput(el, e, sizeof(INPUT));
	if (ret == el)
		ret = 0;
	else
		//FIXME implement some kind of recovery?
		fprintf(stderr, "SendInput: error %d... oops!\n", ret = GetLastError());
	free(e);
	return ret;
}

int setup() {
	return 0;
}