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

static const struct hisi_hip08_hw_error sllc_hw_intr0[] = {
	{ .msk = BIT(0), .msg = "int_sky_tx_req_fifo_ovf" },
	{ .msk = BIT(1), .msg = "int_sky_tx_snp_fifo_ovf" },
	{ .msk = BIT(2), .msg = "int_sky_tx_rsp_fifo_ovf" },
	{ .msk = BIT(3), .msg = "int_sky_tx_dat_fifo_ovf" },
	{ .msk = BIT(4), .msg = "int_phy_rx_flit_1bit_ecc_error" },
	{ .msk = BIT(5), .msg = "int_phy_rx_flit_2bit_ecc_error" },
	{ .msk = BIT(28), .msg = "int_sky_rx_req_fifo_ovf" },
	{ .msk = BIT(29), .msg = "int_sky_rx_snp_fifo_ovf" },
	{ .msk = BIT(30), .msg = "int_sky_rx_rsp_fifo_ovf" },
	{ .msk = BIT(31), .msg = "int_sky_rx_dat_fifo_ovf" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sllc_hw_intr1[] = {
	{ .msk = BIT(8), .msg = "int_phy_rx_align_fifo0_ovf" },
	{ .msk = BIT(9), .msg = "int_phy_rx_align_fifo1_ovf" },
	{ .msk = BIT(10), .msg = "int_phy_rx_align_fifo2_ovf" },
	{ .msk = BIT(11), .msg = "int_phy_rx_align_fifo3_ovf" },
	{ .msk = BIT(12), .msg = "int_phy_rx_train_ptr_err" },
	{ .msk = BIT(16), .msg = "int_phy_rx_async_fifo0_ovf" },
	{ .msk = BIT(17), .msg = "int_phy_rx_async_fifo1_ovf" },
	{ .msk = BIT(18), .msg = "int_phy_rx_async_fifo2_ovf" },
	{ .msk = BIT(19), .msg = "int_phy_rx_async_fifo3_ovf" },
	{ .msk = BIT(24), .msg = "int_phy_tx_async_fifo_ovf" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error aa_hw_intraw[] = {
	{ .msk = BIT(0), .msg = "dec_miss_error" },
	{ .msk = BIT(1), .msg = "daw_overlap_error" },
	{ .msk = BIT(2), .msg = "msd_overlap_error" },
	{ .msk = BIT(3), .msg = "dec_miss_ext_error" },
	{ .msk = BIT(4), .msg = "daw_overlap_ext_error" },
	{ .msk = BIT(5), .msg = "msd_overlap_ext_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sio_hw_int[] = {
	{ .msk = BIT(0), .msg = "tx_int_sts_train_overtime" },
	{ .msk = BIT(1), .msg = "tx_int_sts_freqch_dll_overtime" },
	{ .msk = BIT(2), .msg = "tx_int_sts_freqch_vldon_overtime" },
	{ .msk = BIT(3), .msg = "tx_int_sts_freqch_vldoff_overtime" },
	{ .msk = BIT(4), .msg = "tx_int_sts_deskew_vldon_overtime" },
	{ .msk = BIT(5), .msg = "tx_int_sts_deskew_vldoff_overtime" },
	{ .msk = BIT(6), .msg = "tx_int_sts_deskew_wait_sllc_overtime" },
	{ .msk = BIT(16), .msg = "rx_int_sts_train_err_phy0" },
	{ .msk = BIT(17), .msg = "rx_int_sts_train_err_phy1" },
	{ .msk = BIT(18), .msg = "rx_int_sts_train_err_phy2" },
	{ .msk = BIT(19), .msg = "rx_int_sts_train_err_phy3" },
	{ .msk = BIT(20), .msg = "rx_int_sts_dll_lock" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error poe_ecc_1bit_info_0[] = {
	{ .msk = BIT(0), .msg = "add_fifo1_ecc_1bit_error" },
	{ .msk = BIT(1), .msg = "bind_info_ecc_1bit_error" },
	{ .msk = BIT(2), .msg = "route_tab_ecc_1bit_error" },
	{ .msk = BIT(3), .msg = "fail_buf_ecc_1bit_error" },
	{ .msk = BIT(4), .msg = "ofrb_fifo4_ecc_1bit_error" },
	{ .msk = BIT(5), .msg = "ofrb_fifo5_ecc_1bit_error" },
	{ .msk = BIT(6), .msg = "ofrb_fifo8_ecc_1bit_error" },
	{ .msk = BIT(7), .msg = "nt_inq_ecc_1bit_error" },
	{ .msk = BIT(8), .msg = "dt_inq_ecc_1bit_error" },
	{ .msk = BIT(9), .msg = "nt_ovq_ecc_1bit_error" },
	{ .msk = BIT(10), .msg = "dt_ovq_ecc_1bit_error" },
	{ .msk = BIT(11), .msg = "indraw_ecc_1bit_error" },
	{ .msk = BIT(12), .msg = "nt_des_ecc_1bit_error" },
	{ .msk = BIT(13), .msg = "dt_des_ecc_1bit_error" },
	{ .msk = BIT(14), .msg = "get_pack_tab_ecc_1bit_error" },
	{ .msk = BIT(15), .msg = "rd_fifo1_ecc_1bit_error" },
	{ .msk = BIT(16), .msg = "add_pkt_tab_ecc_1bit_error" },
	{ .msk = BIT(17), .msg = "cnt_ddrq_ecc_1bit_error" },
	{ .msk = BIT(18), .msg = "low_th_ecc_1bit_error" },
	{ .msk = BIT(19), .msg = "high_th_ecc_1bit_error" },
	{ .msk = BIT(20), .msg = "q_cnt_ecc_1bit_error" },
	{ .msk = BIT(21), .msg = "oe_nt_ecc_1bit_error" },
	{ .msk = BIT(22), .msg = "oe_wt_ecc_1bit_error" },
	{ .msk = BIT(23), .msg = "oe_cpt_bf_ecc_1bit_error" },
	{ .msk = BIT(24), .msg = "oe_cpt_tg_ecc_1bit_error" },
	{ .msk = BIT(25), .msg = "oe_cpt_other_ecc_1bit_error" },
	{ .msk = BIT(26), .msg = "oe_efp_ecc_1bit_error" },
	{ .msk = BIT(27), .msg = "oe_qfp_ecc_1bit_error" },
	{ .msk = BIT(28), .msg = "oe_tt_ecc_1bit_error" },
	{ .msk = BIT(29), .msg = "ht_ddrq_ecc_1bit_error" },
	{ .msk = BIT(30), .msg = "afnq_info_ecc_1bit_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error poe_ecc_1bit_info_1[] = {
	{ .msk = BIT(0), .msg = "ht_ovq_ecc_1bit_error" },
	{ .msk = BIT(1), .msg = "ht_inq_ecc_1bit_error" },
	{ .msk = BIT(2), .msg = "ht_flowq_ecc_1bit_error" },
	{ .msk = BIT(3), .msg = "push_pkt_tab_ecc_0_1bit_error" },
	{ .msk = BIT(4), .msg = "push_pkt_tab_ecc_1_1bit_error" },
	{ .msk = BIT(5), .msg = "nt_ddrq0_ecc_1bit_error" },
	{ .msk = BIT(6), .msg = "nt_ddrq1_ecc_1bit_error" },
	{ .msk = BIT(7), .msg = "nt_ddrq2_ecc_1bit_error" },
	{ .msk = BIT(8), .msg = "nt_ddrq3_ecc_1bit_error" },
	{ .msk = BIT(9), .msg = "nt_ddrq4_ecc_1bit_error" },
	{ .msk = BIT(10), .msg = "nt_ddrq5_ecc_1bit_error" },
	{ .msk = BIT(11), .msg = "nt_ddrq6_ecc_1bit_error" },
	{ .msk = BIT(12), .msg = "nt_ddrq7_ecc_1bit_error" },
	{ .msk = BIT(13), .msg = "dispatch_0_ecc_1bit_error" },
	{ .msk = BIT(14), .msg = "dispatch_1_ecc_1bit_error" },
	{ .msk = BIT(15), .msg = "fwd_poe_tab_ecc_1bit_error" },
	{ .msk = BIT(16), .msg = "pull_pkt_tab_ecc_1bit_error" },
	{ .msk = BIT(17), .msg = "nt_pt_ecc_1bit_error" },
	{ .msk = BIT(18), .msg = "event_id_ecc_1bit_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error poe_ecc_2bit_info_0[] = {
	{ .msk = BIT(0), .msg = "add_fifo1_ecc_2bit_error" },
	{ .msk = BIT(1), .msg = "bind_info_ecc_2bit_error" },
	{ .msk = BIT(2), .msg = "route_tab_ecc_2bit_error" },
	{ .msk = BIT(3), .msg = "fail_buf_ecc_2bit_error" },
	{ .msk = BIT(4), .msg = "ofrb_fifo4_ecc_2bit_error" },
	{ .msk = BIT(5), .msg = "ofrb_fifo5_ecc_2bit_error" },
	{ .msk = BIT(6), .msg = "ofrb_fifo8_ecc_2bit_error" },
	{ .msk = BIT(7), .msg = "nt_inq_ecc_2bit_error" },
	{ .msk = BIT(8), .msg = "dt_inq_ecc_2bit_error" },
	{ .msk = BIT(9), .msg = "nt_ovq_ecc_2bit_error" },
	{ .msk = BIT(10), .msg = "dt_ovq_ecc_2bit_error" },
	{ .msk = BIT(11), .msg = "indraw_ecc_2bit_error" },
	{ .msk = BIT(12), .msg = "nt_des_ecc_2bit_error" },
	{ .msk = BIT(13), .msg = "dt_des_ecc_2bit_error" },
	{ .msk = BIT(14), .msg = "get_pack_tab_ecc_2bit_error" },
	{ .msk = BIT(15), .msg = "rd_fifo1_ecc_2bit_error" },
	{ .msk = BIT(16), .msg = "add_pkt_tab_ecc_2bit_error" },
	{ .msk = BIT(17), .msg = "cnt_ddrq_ecc_2bit_error" },
	{ .msk = BIT(18), .msg = "low_th_ecc_2bit_error" },
	{ .msk = BIT(19), .msg = "high_th_ecc_2bit_error" },
	{ .msk = BIT(20), .msg = "q_cnt_ecc_2bit_error" },
	{ .msk = BIT(21), .msg = "oe_nt_ecc_2bit_error" },
	{ .msk = BIT(22), .msg = "oe_wt_ecc_2bit_error" },
	{ .msk = BIT(23), .msg = "oe_cpt_bf_ecc_2bit_error" },
	{ .msk = BIT(24), .msg = "oe_cpt_tg_ecc_2bit_error" },
	{ .msk = BIT(25), .msg = "oe_cpt_other_ecc_2bit_error" },
	{ .msk = BIT(26), .msg = "oe_efp_ecc_2bit_error" },
	{ .msk = BIT(27), .msg = "oe_qfp_ecc_2bit_error" },
	{ .msk = BIT(29), .msg = "ht_ddrq_ecc_2bit_error" },
	{ .msk = BIT(30), .msg = "afnq_info_ecc_bit_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error poe_ecc_2bit_info_1[] = {
	{ .msk = BIT(0), .msg = "ht_ovq_ecc_2bit_error" },
	{ .msk = BIT(1), .msg = "ht_inq_ecc_2bit_error" },
	{ .msk = BIT(2), .msg = "ht_flowq_ecc_2bit_error" },
	{ .msk = BIT(3), .msg = "push_pkt_tab_ecc_0_2bit_error" },
	{ .msk = BIT(4), .msg = "push_pkt_tab_ecc_1_2bit_error" },
	{ .msk = BIT(5), .msg = "nt_ddrq0_ecc_2bit_error" },
	{ .msk = BIT(6), .msg = "nt_ddrq1_ecc_2bit_error" },
	{ .msk = BIT(7), .msg = "nt_ddrq2_ecc_2bit_error" },
	{ .msk = BIT(8), .msg = "nt_ddrq3_ecc_2bit_error" },
	{ .msk = BIT(9), .msg = "nt_ddrq4_ecc_2bit_error" },
	{ .msk = BIT(10), .msg = "nt_ddrq5_ecc_2bit_error" },
	{ .msk = BIT(11), .msg = "nt_ddrq6_ecc_2bit_error" },
	{ .msk = BIT(12), .msg = "nt_ddrq7_ecc_2bit_error" },
	{ .msk = BIT(13), .msg = "dispatch_0_ecc_2bit_error" },
	{ .msk = BIT(14), .msg = "dispatch_1_ecc_2bit_error" },
	{ .msk = BIT(15), .msg = "fwd_poe_tab_ecc_2bit_error" },
	{ .msk = BIT(16), .msg = "pull_pkt_tab_ecc_2bit_error" },
	{ .msk = BIT(17), .msg = "nt_pt_ecc_2bit_error" },
	{ .msk = BIT(18), .msg = "event_id_ecc_2bit_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error disp_intraw0[] = {
	{ .msk = BIT(0), .msg = "intraw_err_abnormal_cmd" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error disp_intraw1[] = {
	{ .msk = BIT(0), .msg = "intraw_trap_cmd" },
	{ .msk = BIT(1), .msg = "intraw_trap_rsp" },
	{ .msk = BIT(8), .msg = "intraw_rcv_err_rsp_port0" },
	{ .msk = BIT(9), .msg = "intraw_rcv_err_rsp_port1" },
	{ .msk = BIT(10), .msg = "intraw_rcv_err_rsp_port2" },
	{ .msk = BIT(11), .msg = "intraw_rcv_err_rsp_port3" },
	{ .msk = BIT(12), .msg = "intraw_rcv_err_rsp_port4" },
	{ .msk = BIT(13), .msg = "intraw_rcv_err_rsp_port5" },
	{ .msk = BIT(14), .msg = "intraw_rcv_err_rsp_port6" },
	{ .msk = BIT(15), .msg = "intraw_rcv_err_rsp_port7" },
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

	case MODULE_ID_PLL:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			trace_seq_printf(s, "SC_PLL_INT_STATUS=0x%x\n",
					 err->err_misc_0);
		break;

	case MODULE_ID_SLLC:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "SLLC_INT0_SRC",
					     sllc_hw_intr0,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(s, "SLLC_INT1_SRC",
					     sllc_hw_intr1,
					     err->err_misc_1);
		break;

	case MODULE_ID_AA:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "AA_INTRAW",
					     aa_hw_intraw,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			trace_seq_printf(s, "AA_DEC_ERR_OTHER=0x%x\n",
					 err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			trace_seq_printf(s, "AA_DEC_ERR_ADDRL_EXT=0x%x\n",
					 err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			trace_seq_printf(s, "AA_DEC_ERR_ADDRH_EXT=0x%x\n",
					 err->err_misc_3);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_4)
			trace_seq_printf(s, "AA_DEC_ERR_OTHER_EXT=0x%x\n",
					 err->err_misc_4);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_ADDR)
			trace_seq_printf(s, "AA_ERR_ADDR=0x%p\n",
					 (void *)err->err_addr);
		break;

	case MODULE_ID_SIOE:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "SIOE_INT_STS",
					     sio_hw_int,
					     err->err_misc_0);
		break;

	case MODULE_ID_POE:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "POE_ECC_1BIT_ERR_INFO_1",
					     poe_ecc_1bit_info_1,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(s, "POE_ECC_1BIT_ERR_INFO_0",
					     poe_ecc_1bit_info_0,
					     err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(s, "POE_ECC_2BIT_ERR_INFO_1",
					     poe_ecc_2bit_info_1,
					     err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			hisi_hip08_log_error(s, "POE_ECC_2BIT_ERR_INFO_0",
					     poe_ecc_2bit_info_0,
					     err->err_misc_3);
		break;

	case MODULE_ID_DISP:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0) {
			trace_seq_printf(s, "DISP_ERR_INFO_0=0x%x\n",
					 err->err_misc_0);
			trace_seq_printf(s, "err_opcode=0x%x\n",
					 err->err_misc_0 & 0x3F);
			trace_seq_printf(s, "err_lp_id=0x%x\n",
					 (err->err_misc_0 >> 8) & 0x7);
			trace_seq_printf(s, "err_src_id=0x%x\n",
					 (err->err_misc_0 >> 12) & 0x1FF);
		}

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1) {
			trace_seq_printf(s, "DISP_ERR_INFO_1=0x%x\n",
					 err->err_misc_1);
			trace_seq_printf(s, "err_daw_overlap_info=0x%x\n",
					 err->err_misc_1  & 0xFF);
			trace_seq_printf(s, "err_cmd_pcrdtype=0x%x\n",
					 (err->err_misc_1 >> 8) & 0xFF);
			if (err->err_misc_1 & 10000)
				trace_seq_printf(s, "err_cmd_static_req_ind\n");
		}

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(s, "DISP_INTRAW0",
					     disp_intraw0,
					     err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			hisi_hip08_log_error(s, "DISP_INTRAW1",
					     disp_intraw1,
					     err->err_misc_3);

		if ((err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_4) &&
		    (err->err_misc_4 & 0x1))
			trace_seq_printf(s, "%s:%s", "DISP_ERR_ACCESS_RST_PORT",
					 "slave port is in the reset state\n");

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_ADDR)
			trace_seq_printf(s, "DISP_ERR_ADDR=0x%p\n",
					 (void *)err->err_addr);
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
