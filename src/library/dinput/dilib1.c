/* DirectInput's mouse data format: the DirectX 8.0 SDK's
   dinput.lib member obj\i386\dilib1.obj (dinput\dx7\dll\dilib1.c), compiled
   by the Windows build lab's C frontend 13.00.8831 and backend 13.00.8830
   (tools/ddk2266). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dinput.h>

/* The SDK's dinput.h has no name for the optional-object bit the table uses. */
#define MOUSE_OPTIONAL_OBJECT 0x80000000UL

/* The member keeps the object table in its code section: initialized data
   under data_seg(".text") gets the section's code characteristics. */
#pragma data_seg(".text")
static DIOBJECTDATAFORMAT c_rgodfDIMouse[] = {
    { &GUID_XAxis, DIMOFS_X, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
    { &GUID_YAxis, DIMOFS_Y, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
    { &GUID_ZAxis, DIMOFS_Z, DIDFT_AXIS | DIDFT_ANYINSTANCE | MOUSE_OPTIONAL_OBJECT, 0 },
    { 0, DIMOFS_BUTTON0, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, DIMOFS_BUTTON1, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, DIMOFS_BUTTON2, DIDFT_BUTTON | DIDFT_ANYINSTANCE | MOUSE_OPTIONAL_OBJECT, 0 },
    { 0, DIMOFS_BUTTON3, DIDFT_BUTTON | DIDFT_ANYINSTANCE | MOUSE_OPTIONAL_OBJECT, 0 }
};
#pragma data_seg()

const DIDATAFORMAT c_dfDIMouse = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    sizeof(DIMOUSESTATE),
    sizeof(c_rgodfDIMouse) / sizeof(c_rgodfDIMouse[0]),
    c_rgodfDIMouse
};
