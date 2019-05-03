/*
 * Copyright (c) 2018 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ras-record.h"
#include "ras-logger.h"
#include "ras-report.h"
#include "ras-non-standard-handler.h"

/* HISI OEM error definitions */
#define MODULE_ID_MN	0
#define MODULE_ID_PLL	1
#define MODULE_ID_SLLC	2
#define MODULE_ID_AA	3
#define MODULE_ID_SIOE	4
#define MODULE_ID_POE	5
#define MODULE_ID_DISP	8
#define MODULE_ID_LPC	9

#define HISI_OEM_VALID_SOC_ID		BIT(0)
#define HISI_OEM_VALID_SOCKET_ID	BIT(1)
#define HISI_OEM_VALID_NIMBUS_ID	BIT(2)
#define HISI_OEM_VALID_MODULE_ID	BIT(3)
#define HISI_OEM_VALID_SUB_MODULE_ID	BIT(4)
#define HISI_OEM_VALID_ERR_SEVERITY	BIT(5)

#define HISI_OEM_TYPE1_VALID_ERR_MISC_0	BIT(6)
#define HISI_OEM_TYPE1_VALID_ERR_MISC_1	BIT(7)
#define HISI_OEM_TYPE1_VALID_ERR_MISC_2	BIT(8)
#define HISI_OEM_TYPE1_VALID_ERR_MISC_3	BIT(9)
#define HISI_OEM_TYPE1_VALID_ERR_MISC_4	BIT(10)
#define HISI_OEM_TYPE1_VALID_ERR_ADDR	BIT(11)

struct hisi_oem_type1_err_sec {
	uint32_t   val_bits;
	uint8_t    version;
	uint8_t    soc_id;
	uint8_t    socket_id;
	uint8_t    nimbus_id;
	uint8_t    module_id;
	uint8_t    sub_module_id;
	uint8_t    err_severity;
	uint8_t    reserv;
	uint32_t   err_misc_0;
	uint32_t   err_misc_1;
	uint32_t   err_misc_2;
	uint32_t   err_misc_3;
	uint32_t   err_misc_4;
	uint64_t   err_addr;
};

struct hisi_hip08_hw_error {
	uint32_t msk;
	const char *msg;
};

static const struct hisi_hip08_hw_error mn_hw_intr[] = {
	{ .msk = BIT(0), .msg = "event_counter_0_overflow_int" },
	{ .msk = BIT(1), .msg = "event_counter_1_overflow_int" },
	{ .msk = BIT(2), .msg = "event_counter_2_overflow_int" },
	{ .msk = BIT(3), .msg = "event_counter_3_overflow_int" },
	{ /* sentinel */ }
};

/* helper functions */
static char *err_severity(uint8_t err_sev)
{
	switch (err_sev) {
	case 0: return "recoverable";
	case 1: return "fatal";
	case 2: return "corrected";
	case 3: return "none";
	}
	return "unknown";
}

static char *oem_type1_module_name(uint8_t module_id)
{
	switch (module_id) {
	case MODULE_ID_MN: return "MN";
	case MODULE_ID_PLL: return "PLL";
	case MODULE_ID_SLLC: return "SLLC";
	case MODULE_ID_AA: return "AA";
	case MODULE_ID_SIOE: return "SIOE";
	case MODULE_ID_POE: return "POE";
	case MODULE_ID_DISP: return "DISP";
	case MODULE_ID_LPC: return "LPC";
	}
	return "unknown";
}

static void hisi_hip08_log_error(struct trace_seq *s, char *reg,
				  const struct hisi_hip08_hw_error *err,
				  uint32_t err_status)
{
	while (err->msg) {
		if (err->msk & err_status)
			trace_seq_printf(s, "%s: %s\n", reg, err->msg);
		err++;
	}
}

/* error data decoding functions */
static void dec_type1_misc_err_data(struct trace_seq *s,
				    const struct hisi_oem_type1_err_sec *err)
{
	trace_seq_printf(s, "Error Info:\n");
	switch (err->module_id) {
	case MODULE_ID_MN:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "MN_RINT", mn_hw_intr,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(s, "MN_INTS", mn_hw_intr,
					     err->err_misc_1);
		break;
	}
}

static int decode_hip08_oem_type1_error(struct trace_seq *s, const void *error)
{
	const struct hisi_oem_type1_err_sec *err = error;
	char buf[1024];
	char *p = buf;

	if (err->val_bits == 0) {
		trace_seq_printf(s, "%s: no valid error information\n",
				 __func__);
		return -1;
	}
	p += sprintf(p, "[ ");
	if (err->val_bits & HISI_OEM_VALID_SOC_ID)
		p += sprintf(p, "SOC ID=%d ", err->soc_id);
	if (err->val_bits & HISI_OEM_VALID_SOCKET_ID)
		p += sprintf(p, "socket ID=%d ", err->socket_id);

	if (err->val_bits & HISI_OEM_VALID_NIMBUS_ID)
		p += sprintf(p, "nimbus ID=%d ", err->nimbus_id);

	if (err->val_bits & HISI_OEM_VALID_MODULE_ID) {
		p += sprintf(p, "module=%s-",
			     oem_type1_module_name(err->module_id));
		if (err->val_bits & HISI_OEM_VALID_SUB_MODULE_ID)
			p += sprintf(p, "%d ", err->sub_module_id);
	}

	if (err->val_bits & HISI_OEM_VALID_ERR_SEVERITY)
		p += sprintf(p, "error severity=%s ",
			     err_severity(err->err_severity));
	p += sprintf(p, "]");
	trace_seq_printf(s, "\nHISI HIP08: OEM Type-1 Error\n");
	trace_seq_printf(s, "%s\n", buf);

	trace_seq_printf(s, "Reg Dump:\n");
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
		trace_seq_printf(s, "ERR_MISC0=0x%x\n", err->err_misc_0);
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
		trace_seq_printf(s, "ERR_MISC1=0x%x\n", err->err_misc_1);
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
		trace_seq_printf(s, "ERR_MISC2=0x%x\n", err->err_misc_2);
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
		trace_seq_printf(s, "ERR_MISC3=0x%x\n", err->err_misc_3);
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_4)
		trace_seq_printf(s, "ERR_MISC4=0x%x\n", err->err_misc_4);
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_ADDR)
		trace_seq_printf(s, "ERR_ADDR=0x%p\n", (void *)err->err_addr);

	dec_type1_misc_err_data(s, err);

	return 0;
}

struct ras_ns_dec_tab hip08_ns_oem_tab[] = {
	{
		.sec_type = "1f8161e155d641e6bd107afd1dc5f7c5",
		.decode = decode_hip08_oem_type1_error,
	},
};

__attribute__((constructor))
static void hip08_init(void)
{
	hip08_ns_oem_tab[0].len = ARRAY_SIZE(hip08_ns_oem_tab);
	register_ns_dec_tab(hip08_ns_oem_tab);
}
