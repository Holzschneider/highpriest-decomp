/* The header compiles as C. */
#include "bf_control.h"

int bf_control_test_header_c(void)
{
	bf_control_config config = {0, 0};
	bf_control_hooks hooks = {0};
	bf_control_set_hooks(&hooks);
	(void)config;
	return BF_CONTROL_PROTOCOL + (bf_control_code_name(BF_CONTROL_E_IO) != 0);
}
