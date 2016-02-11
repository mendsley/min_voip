/**
 * Copyright 2011-2015 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TINY__PLATFORM_H
#define TINY__PLATFORM_H

#if !defined(TINY_PLATFORM_X360)
#	if defined(_XBOX_VER)
#		define TINY_PLATFORM_X360 1
#	else
#		define TINY_PLATFORM_X360 0
#	endif // _XBOX_VER
#endif // ... TINY_PLATFORM_X360

#if !defined(TINY_PLATFORM_XB1)
#	if defined(_DURANGO)
#		define TINY_PLATFORM_XB1 1
#	else
#		define TINY_PLATFORM_XB1 0
#	endif // _DURANGO
#endif // ... TINY_PLATFORM_XB1

#if !defined(TINY_PLATFORM_PS4)
#	if defined(__ORBIS__)
#		define TINY_PLATFORM_PS4 1
#	else
#		define TINY_PLATFORM_PS4 0
#	endif // __ORBIS__
#endif // ... TINY_PLATFORM_PS4

#if !defined(TINY_PLATFORM_WINDOWS)
#	if defined(_WIN32) && !TINY_PLATFORM_X360 && !TINY_PLATFORM_XB1
#		define TINY_PLATFORM_WINDOWS 1
#	else
#		define TINY_PLATFORM_WINDOWS 0
#	endif // _WIN32
#endif // ... TINY_PLATFORM_WINDOWS

namespace tiny
{
	bool platformStartup();
	void platformShutdown();
}

#endif // TINY__PLATFORM_H
