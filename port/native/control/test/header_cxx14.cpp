// The header compiles as C++14.
#include "bf_control.h"

int bf_control_test_header_cxx14()
{
	bf_control_resize_args args = {};
	bf_control_capture_info info = {};
	(void)args;
	(void)info;
	return bf_control_value_type(nullptr) == BF_CONTROL_T_ABSENT ? 1 : 0;
}
