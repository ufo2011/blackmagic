#include "general.h"
#include "exception.h"
#include "avr.h"
#include "jtag_scan.h"
#include "jtagtap.h"

#define PDI_DELAY	0xDBU

void avr_jtag_dp_handler(uint8_t jd_index, uint32_t j_idcode)
{
	AVR_DP_t *dp = calloc(1, sizeof(*dp));
	if (!dp) {			/* calloc failed: heap exhaustion */
		DEBUG_WARN("calloc: failed in %s\n", __func__);
		return;
	}
	(void)j_idcode;

	dp->dp_jd_index = jd_index;
	dp->idcode = jtag_devs[jd_index].jd_idcode;
	if ((PC_HOSTED == 0) || (!platform_avr_jtag_dp_init(dp))) {
		//
	}
	avr_dp_init(dp);
}

bool avr_jtag_shift_dr(jtag_proc_t *jp, uint8_t jd_index, uint8_t *dout, const uint8_t din)
{
	jtag_dev_t *d = &jtag_devs[jd_index];
	uint8_t result = 0;
	uint16_t request = 0, response = 0;
	uint8_t *data;
	if (!dout)
		return false;
	do
	{
		data = (uint8_t *)&request;
		jtagtap_shift_dr();
		jp->jtagtap_tdi_seq(0, ones, d->dr_prescan);
		data[0] = din;
		// Calculate the parity bit
		for (uint8_t i = 0; i < 8; ++i)
			data[1] ^= (din >> i) & 1U;
		jp->jtagtap_tdi_tdo_seq((uint8_t *)&response, 1, (uint8_t *)&request, 9);
		jp->jtagtap_tdi_seq(1, ones, d->dr_postscan);
		jtagtap_return_idle();
		data = (uint8_t *)&response;
	}
	while (data[0] == PDI_DELAY && data[1] == 1);
	// Calculate the parity bit
	for (uint8_t i = 0; i < 8; ++i)
		result ^= (data[0] >> i) & 1U;
	*dout = data[0];
	DEBUG_INFO("Sent 0x%x to target, response was 0x%x (0x%x)\n", din, data[0], data[1]);
	return result == data[1];
}
