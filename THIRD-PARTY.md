# Third-party material

The CC0 dedication in [CC0-1.0.txt](CC0-1.0.txt) covers the build files and the documentation. It does not cover anything listed here. No redistribution licence is held for the material marked proprietary.

| Material | Where | Owner | Terms |
|---|---|---|---|
| Game source reconstructed from `HighPriest.exe` | `src/` outside the rows below | Gravity | proprietary |
| Lua 5.0 | `src/l*.c`, `src/l*.h`, `src/lua.h`, `src/lualib.h` | Tecgraf, PUC-Rio | MIT, below |
| MD5 by L. Peter Deutsch, adapted to a class | `src/device/md5.cpp`, `src/device/md5.h` | Aladdin Enterprises | zlib-style, below |
| Reference implementations of published ciphers and hashes: MacGuffin, Snefru, RIPEMD-128, SAFER, CAST-128, CRYPTON, Turtle, Tiger, FEAL-8, MISTY1 and SEAL | `src/CipherImpl.cpp`, `src/CipherImpl.h` | Xerox (Snefru), K.U.Leuven (RIPEMD), Richard De Moliner (SAFER) and the authors of the others | the Snefru and RIPEMD notices are below; the SAFER source names its author and states no terms; no notice is known for the others |

The rest of the 2008 client's third-party material (RC4 from OpenSSL, the GameGuard client library and the nProtect KeyCrypt header, the Visual C++ 6.0 runtime sources and compilers, the DirectX 8.0 SDK files, `WinCrypt.h`, `DbgHelp.h`, Miles' `cleanup.c`, the Intel JPEG Library header, wibo with the Wine DLLs, mimalloc and liburing it embeds) served only the Windows build and left this branch in M11.11; `main` keeps it, with its rows and notices. So did the Wine 11.0 Windows headers of `port/compat/winheaders/` (LGPL 2.1 or later), which the port's build read until then; they stay in the history for reference work (port/README.md, "The Windows headers for reference work").

## Lua

```
Lua License
-----------

Lua is licensed under the terms of the MIT license reproduced below.
This means that Lua is free software and can be used for both academic
and commercial purposes at absolutely no cost.

For details and rationale, see http://www.lua.org/license.html .

===============================================================================

Copyright (C) 2003-2004 Tecgraf, PUC-Rio.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

===============================================================================

(end of COPYRIGHT)
```

## MD5

```
Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

L. Peter Deutsch
ghost@aladdin.com
```

## Snefru

```
Copyright (c) Xerox Corporation 1989. All rights reserved.

License to copy and use this software is granted provided that it is
identified as the "Xerox Secure Hash Function" in all material mentioning
or referencing this software or this hash function.

License is also granted to make and use derivative works provided that such
works are identified as "derived from the Xerox Secure Hash Function" in
all material mentioning or referencing the derived work.

Xerox Corporation makes no representations concerning either the
merchantability of this software or the suitability of this software for
any particular purpose.  It is provided "as is" without express or implied
warranty of any kind.

These notices must be retained in any copies of any part of this software.
```

## RIPEMD

```
FILE:     rmd128.c

CONTENTS: A sample C-implementation of the RIPEMD-128
          hash-function. This function is a plug-in substitute
          for RIPEMD. A 160-bit hash result is obtained using
          RIPEMD-160.
TARGET:   any computer with an ANSI C compiler

AUTHOR:   Antoon Bosselaers, ESAT-COSIC
DATE:     1 March 1996
VERSION:  1.0

Copyright (c) Katholieke Universiteit Leuven
1996, All Rights Reserved

Conditions for use of the RIPEMD-160 Software

The RIPEMD-160 software is freely available for use under the terms and
conditions described hereunder, which shall be deemed to be accepted by
any user of the software and applicable on any use of the software:

1. K.U.Leuven Department of Electrical Engineering-ESAT/COSIC shall for
all purposes be considered the owner of the RIPEMD-160 software and of
all copyright, trade secret, patent or other intellectual property
rights therein.
2. The RIPEMD-160 software is provided on an "as is" basis without
warranty of any sort, express or implied. K.U.Leuven makes no
representation that the use of the software will not infringe any
patent or proprietary right of third parties. User will indemnify
K.U.Leuven and hold K.U.Leuven harmless from any claims or liabilities
which may arise as a result of its use of the software. In no
circumstances K.U.Leuven R&D will be held liable for any deficiency,
fault or other mishappening with regard to the use or performance of
the software.
3. User agrees to give due credit to K.U.Leuven in scientific publications
or communications in relation with the use of the RIPEMD-160 software
as follows: RIPEMD-160 software written by Antoon Bosselaers,
available at http://www.esat.kuleuven.ac.be/~cosicart/ps/AB-9601/.
```
