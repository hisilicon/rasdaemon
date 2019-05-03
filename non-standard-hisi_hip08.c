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
#define MODULE_ID_SAS	15
#define MODULE_ID_SATA	16

#define MODULE_ID_SMMU	0
#define MODULE_ID_HHA	1
#define MODULE_ID_HLLC	2

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

#define HISI_OEM_TYPE2_VALID_ERR_FR	BIT(6)
#define HISI_OEM_TYPE2_VALID_ERR_CTRL	BIT(7)
#define HISI_OEM_TYPE2_VALID_ERR_STATUS	BIT(8)
#define HISI_OEM_TYPE2_VALID_ERR_ADDR	BIT(9)
#define HISI_OEM_TYPE2_VALID_ERR_MISC_0	BIT(10)
#define HISI_OEM_TYPE2_VALID_ERR_MISC_1	BIT(11)

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

struct hisi_oem_type2_err_sec {
	uint32_t   val_bits;
	uint8_t    version;
	uint8_t    soc_id;
	uint8_t    socket_id;
	uint8_t    nimbus_id;
	uint8_t    module_id;
	uint8_t    sub_module_id;
	uint8_t    err_severity;
	uint8_t    reserv;
	uint32_t   err_fr_0;
	uint32_t   err_fr_1;
	uint32_t   err_ctrl_0;
	uint32_t   err_ctrl_1;
	uint32_t   err_status_0;
	uint32_t   err_status_1;
	uint32_t   err_addr_0;
	uint32_t   err_addr_1;
	uint32_t   err_misc0_0;
	uint32_t   err_misc0_1;
	uint32_t   err_misc1_0;
	uint32_t   err_misc1_1;
};

struct hisi_hip08_hw_error {
	uint32_t msk;
	const char *msg;
};

struct hisi_hip08_hw_error_status {
	uint32_t val;
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

static const struct hisi_hip08_hw_error lpc_mem_access_st[] = {
	{ .msk = BIT(0), .msg = "lpc_mem_decoder_err" },
	{ .msk = BIT(1), .msg = "lpc_mem_read_error" },
	{ .msk = BIT(2), .msg = "lpc_mem_write_error" },
	{ .msk = BIT(3), .msg = "lpc_mem_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error lpc_sc_mem_ecc_st0[] = {
	{ .msk = BIT(0), .msg = "rfifo_ecc_mbit_err" },
	{ .msk = BIT(1), .msg = "rfifo_ecc_1bit_err" },
	{ .msk = BIT(2), .msg = "wfifo_ecc_mbit_err" },
	{ .msk = BIT(3), .msg = "wfifo_ecc_1bit_err" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error lpc_sc_mem_ecc_st1[] = {
	{ .msk = BIT(0), .msg = "wfifo_err_addr_1bit_err" },
	{ .msk = BIT(1), .msg = "wfifo_err_addr_1bit_err" },
	{ .msk = BIT(2), .msg = "wfifo_err_addr_1bit_err" },
	{ .msk = BIT(3), .msg = "wfifo_err_addr_1bit_err" },
	{ .msk = BIT(4), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(5), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(6), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(7), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(8), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(9), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(10), .msg = "wfifo_ecc_err_syn_1bit_err" },
	{ .msk = BIT(11), .msg = "wfifo_err_addr_mbit_err" },
	{ .msk = BIT(12), .msg = "wfifo_err_addr_mbit_err" },
	{ .msk = BIT(13), .msg = "wfifo_err_addr_mbit_err" },
	{ .msk = BIT(14), .msg = "wfifo_err_addr_mbit_err" },
	{ .msk = BIT(15), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ .msk = BIT(16), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ .msk = BIT(17), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ .msk = BIT(18), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ .msk = BIT(19), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ .msk = BIT(20), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ .msk = BIT(21), .msg = "wfifo_ecc_err_syn_mbit_err" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sas_ras_intr0[] = {
	{ .msk = BIT(0), .msg = "RXM_CFG_MEM3_ECC1B_INTR" },
	{ .msk = BIT(1), .msg = "RXM_CFG_MEM2_ECC1B_INTR" },
	{ .msk = BIT(2), .msg = "RXM_CFG_MEM1_ECC1B_INTR" },
	{ .msk = BIT(3), .msg = "RXM_CFG_MEM0_ECC1B_INTR" },
	{ .msk = BIT(4), .msg = "HGC_CQE_ECC1B_INTR" },
	{ .msk = BIT(5), .msg = "LM_CFG_IOSTL_ECC1B_INTR" },
	{ .msk = BIT(6), .msg = "LM_CFG_ITCTL_ECC1B_INTR" },
	{ .msk = BIT(7), .msg = "HGC_ITCT_ECC1B_INTR" },
	{ .msk = BIT(8), .msg = "HGC_IOST_ECC1B_INTR" },
	{ .msk = BIT(9), .msg = "HGC_DQE_ECC1B_INTR" },
	{ .msk = BIT(10), .msg = "DMAC0_RAM_ECC1B_INTR" },
	{ .msk = BIT(11), .msg = "DMAC1_RAM_ECC1B_INTR" },
	{ .msk = BIT(12), .msg = "DMAC2_RAM_ECC1B_INTR" },
	{ .msk = BIT(13), .msg = "DMAC3_RAM_ECC1B_INTR" },
	{ .msk = BIT(14), .msg = "DMAC4_RAM_ECC1B_INTR" },
	{ .msk = BIT(15), .msg = "DMAC5_RAM_ECC1B_INTR" },
	{ .msk = BIT(16), .msg = "DMAC6_RAM_ECC1B_INTR" },
	{ .msk = BIT(17), .msg = "DMAC7_RAM_ECC1B_INTR" },
	{ .msk = BIT(18), .msg = "OOO_RAM_ECC1B_INTR" },
	{ .msk = BIT(19), .msg = "HILINK_INT" },
	{ .msk = BIT(20), .msg = "HILINK_PLL0_OUT_OF_LOCK" },
	{ .msk = BIT(21), .msg = "HILINK_PLL1_OUT_OF_LOCK" },
	{ .msk = BIT(22), .msg = "HILINK_LOSS_OF_REFCLK0" },
	{ .msk = BIT(23), .msg = "HILINK_LOSS_OF_REFCLK1" },
	{ .msk = BIT(24), .msg = "DMAC0_TX_POISON" },
	{ .msk = BIT(25), .msg = "DMAC1_TX_POISON" },
	{ .msk = BIT(26), .msg = "DMAC2_TX_POISON" },
	{ .msk = BIT(27), .msg = "DMAC3_TX_POISON" },
	{ .msk = BIT(28), .msg = "DMAC4_TX_POISON" },
	{ .msk = BIT(29), .msg = "DMAC5_TX_POISON" },
	{ .msk = BIT(30), .msg = "DMAC6_TX_POISON" },
	{ .msk = BIT(31), .msg = "DMAC7_TX_POISON" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sas_ras_intr1[] = {
	{ .msk = BIT(0), .msg = "RXM_CFG_MEM3_ECC2B_INTR" },
	{ .msk = BIT(1), .msg = "RXM_CFG_MEM2_ECC2B_INTR" },
	{ .msk = BIT(2), .msg = "RXM_CFG_MEM1_ECC2B_INTR" },
	{ .msk = BIT(3), .msg = "RXM_CFG_MEM0_ECC2B_INTR" },
	{ .msk = BIT(4), .msg = "HGC_CQE_ECC2B_INTR" },
	{ .msk = BIT(5), .msg = "LM_CFG_IOSTL_ECC2B_INTR" },
	{ .msk = BIT(6), .msg = "LM_CFG_ITCTL_ECC2B_INTR" },
	{ .msk = BIT(7), .msg = "HGC_ITCT_ECC2B_INTR" },
	{ .msk = BIT(8), .msg = "HGC_IOST_ECC2B_INTR" },
	{ .msk = BIT(9), .msg = "HGC_DQE_ECC2B_INTR" },
	{ .msk = BIT(10), .msg = "DMAC0_RAM_ECC2B_INTR" },
	{ .msk = BIT(11), .msg = "DMAC1_RAM_ECC2B_INTR" },
	{ .msk = BIT(12), .msg = "DMAC2_RAM_ECC2B_INTR" },
	{ .msk = BIT(13), .msg = "DMAC3_RAM_ECC2B_INTR" },
	{ .msk = BIT(14), .msg = "DMAC4_RAM_ECC2B_INTR" },
	{ .msk = BIT(15), .msg = "DMAC5_RAM_ECC2B_INTR" },
	{ .msk = BIT(16), .msg = "DMAC6_RAM_ECC2B_INTR" },
	{ .msk = BIT(17), .msg = "DMAC7_RAM_ECC2B_INTR" },
	{ .msk = BIT(18), .msg = "OOO_RAM_ECC2B_INTR" },
	{ .msk = BIT(20), .msg = "HGC_DQE_POISON_INTR" },
	{ .msk = BIT(21), .msg = "HGC_IOST_POISON_INTR" },
	{ .msk = BIT(22), .msg = "HGC_ITCT_POISON_INTR" },
	{ .msk = BIT(23), .msg = "HGC_ITCT_NCQ_POISON_INTR" },
	{ .msk = BIT(24), .msg = "DMAC0_RX_POISON" },
	{ .msk = BIT(25), .msg = "DMAC1_RX_POISON" },
	{ .msk = BIT(26), .msg = "DMAC2_RX_POISON" },
	{ .msk = BIT(27), .msg = "DMAC3_RX_POISON" },
	{ .msk = BIT(28), .msg = "DMAC4_RX_POISON" },
	{ .msk = BIT(29), .msg = "DMAC5_RX_POISON" },
	{ .msk = BIT(30), .msg = "DMAC6_RX_POISON" },
	{ .msk = BIT(31), .msg = "DMAC7_RX_POISON" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sas_ras_intr2[] = {
	{ .msk = BIT(0), .msg = "DMAC0_AXI_BUS_ERR" },
	{ .msk = BIT(1), .msg = "DMAC1_AXI_BUS_ERR" },
	{ .msk = BIT(2), .msg = "DMAC2_AXI_BUS_ERR" },
	{ .msk = BIT(3), .msg = "DMAC3_AXI_BUS_ERR" },
	{ .msk = BIT(4), .msg = "DMAC4_AXI_BUS_ERR" },
	{ .msk = BIT(5), .msg = "DMAC5_AXI_BUS_ERR" },
	{ .msk = BIT(6), .msg = "DMAC6_AXI_BUS_ERR" },
	{ .msk = BIT(7), .msg = "DMAC7_AXI_BUS_ERR" },
	{ .msk = BIT(8), .msg = "DMAC0_FIFO_OMIT_ERR" },
	{ .msk = BIT(9), .msg = "DMAC1_FIFO_OMIT_ERR" },
	{ .msk = BIT(10), .msg = "DMAC2_FIFO_OMIT_ERR" },
	{ .msk = BIT(11), .msg = "DMAC3_FIFO_OMIT_ERR" },
	{ .msk = BIT(12), .msg = "DMAC4_FIFO_OMIT_ERR" },
	{ .msk = BIT(13), .msg = "DMAC5_FIFO_OMIT_ERR" },
	{ .msk = BIT(14), .msg = "DMAC6_FIFO_OMIT_ERR" },
	{ .msk = BIT(15), .msg = "DMAC7_FIFO_OMIT_ERR" },
	{ .msk = BIT(16), .msg = "HGC_RLSE_SLOT_UNMATCH" },
	{ .msk = BIT(17), .msg = "HGC_LM_ADD_FCH_LIST_ERR" },
	{ .msk = BIT(18), .msg = "HGC_AXI_BUS_ERR" },
	{ .msk = BIT(19), .msg = "HGC_FIFO_OMIT_ERR" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sata_ras_is[] = {
	{ .msk = BIT(0), .msg = "SATA_RAS_FE" },
	{ .msk = BIT(1), .msg = "SATA_RAS_CE" },
	{ .msk = BIT(2), .msg = "SATA_RAS_NFE" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error sata_ras_serr[] = {
	{ .msk = BIT(0), .msg = "p0_ecc_1b_rx_d_fifo" },
	{ .msk = BIT(1), .msg = "p0_ecc_1b_tx_d_fifo" },
	{ .msk = BIT(2), .msg = "p0_ecc_1b_cmd_sram" },
	{ .msk = BIT(3), .msg = "p0_ecc_mb_rx_d_fifo" },
	{ .msk = BIT(4), .msg = "p0_ecc_mb_tx_d_fifo" },
	{ .msk = BIT(5), .msg = "p0_ecc_mb_cmd_sram" },
	{ .msk = BIT(6), .msg = "p1_ecc_1b_rx_d_fifo" },
	{ .msk = BIT(7), .msg = "p1_ecc_1b_tx_d_fifo" },
	{ .msk = BIT(8), .msg = "p1_ecc_1b_cmd_sram" },
	{ .msk = BIT(9), .msg = "p1_ecc_mb_rx_d_fifo" },
	{ .msk = BIT(10), .msg = "p1_ecc_mb_tx_d_fifo" },
	{ .msk = BIT(11), .msg = "p1_ecc_mb_cmd_sram" },
	{ .msk = BIT(12), .msg = "p0_rx_berror" },
	{ .msk = BIT(13), .msg = "p0_rx_rerror" },
	{ .msk = BIT(14), .msg = "p0_tx_rerror" },
	{ .msk = BIT(15), .msg = "p1_rx_berror" },
	{ .msk = BIT(16), .msg = "p1_rx_rerror" },
	{ .msk = BIT(17), .msg = "p1_tx_rerror" },
	{ .msk = BIT(18), .msg = "dmac_poison_err" },
	{ .msk = BIT(20), .msg = "p0_hl_prbs_err" },
	{ .msk = BIT(21), .msg = "p0_hl_alos" },
	{ .msk = BIT(22), .msg = "p0_hl_los" },
	{ .msk = BIT(23), .msg = "p0_hl_cs_calib_done" },
	{ .msk = BIT(24), .msg = "p0_hl_pll_outoflock" },
	{ .msk = BIT(25), .msg = "p0_hl_loss_of_refclk" },
	{ .msk = BIT(26), .msg = "p1_hl_prbs_err" },
	{ .msk = BIT(27), .msg = "p1_hl_alos" },
	{ .msk = BIT(28), .msg = "p1_hl_los" },
	{ .msk = BIT(29), .msg = "p1_hl_cs_calib_done" },
	{ .msk = BIT(30), .msg = "p1_hl_pll_outoflock" },
	{ .msk = BIT(31), .msg = "p1_hl_loss_of_refclk" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status smmu_serr_status[] = {
	{ .val = 0x6, .msg = "context_cache_ecc_error" },
	{ .val = 0x8, .msg = "l2_tlb_ecc_error" },
	{ .val = 0xA, .msg = "wrbuff_ecc_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status smmu_ierr_status[] = {
	{ .val = 0x1, .msg = "ste_fetch_error" },
	{ .val = 0x2, .msg = "cd_fetch_error" },
	{ .val = 0x3, .msg = "walk_eabt_error" },
	{ .val = 0x4, .msg = "cmdq_fetch_error" },
	{ .val = 0x5, .msg = "write_eventq_abort_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status hha_serr_status[] = {
	{ .val = 0x01, .msg = "1bit_corrected_ecc_error" },
	{ .val = 0x02, .msg = "buffer_mem_ecc_error" },
	{ .val = 0x07, .msg = "directory_mem_ecc_error" },
	{ .val = 0x0D, .msg = "address_error" },
	{ .val = 0x0E, .msg = "illegal_request" },
	{ .val = 0x12, .msg = "ddrc_response_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status hha_ierr_status[] = {
	{ .val = 0x01, .msg = "unsupported_illegal_request" },
	{ .val = 0x02, .msg = "cmd_buffer_mbit_ecc_error" },
	{ .val = 0x03, .msg = "sdir_mbit_ecc_error" },
	{ .val = 0x04, .msg = "edir_mbit_ecc_error" },
	{ .val = 0x05, .msg = "default_slave_error" },
	{ .val = 0x06, .msg = "access_security_zone_from_non_sec_request" },
	{ .val = 0x07, .msg = "data_buffer_mbit_ecc_error" },
	{ .val = 0x08, .msg = "ddrc_response_error" },
	{ .val = 0x09, .msg = "msd_miss_from_software" },
	{ .val = 0x0A, .msg = "msd_overlap_from_software" },
	{ .val = 0x0B, .msg = "msd_invert_from_software" },
	{ .val = 0x0C, .msg = "access_non_security_zone_from_sec_request" },
	{ .val = 0x0D, .msg = "1bit_ecc_err_counter_overflow" },
	{ .val = 0x0E, .msg = "cmd_buffer_1bit_ecc_error" },
	{ .val = 0x0F, .msg = "data_buffer_1bit_ecc_error" },
	{ .val = 0x10, .msg = "sdir_1bit_ecc_error" },
	{ .val = 0x11, .msg = "edir_1bit_ecc_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error hllc_hw_err_misc1_l[] = {
	{ .msk = BIT(0), .msg = "hydra_tx_ch0_2bit_ecc_err" },
	{ .msk = BIT(1), .msg = "hydra_tx_ch1_2bit_ecc_err " },
	{ .msk = BIT(2), .msg = "hydra_tx_ch2_2bit_ecc_err" },
	{ .msk = BIT(3), .msg = "hydra_tx_retry_2bit_ecc_err" },
	{ .msk = BIT(4), .msg = "hydra_rx_ch0_2bit_ecc_err" },
	{ .msk = BIT(5), .msg = "hydra_rx_ch1_2bit_ecc_err" },
	{ .msk = BIT(6), .msg = "hydra_rx_ch2_2bit_ecc_err" },
	{ .msk = BIT(8), .msg = "phy_rx_retry_ptr_err" },
	{ .msk = BIT(9), .msg = "phy_tx_retry_buf_ptr_err" },
	{ .msk = BIT(10), .msg = "phy_tx_retry_ptr_err" },
	{ .msk = BIT(16), .msg = "hydra_tx_ch0_ovf" },
	{ .msk = BIT(17), .msg = "hydra_tx_ch1_ovf" },
	{ .msk = BIT(18), .msg = "hydra_tx_ch2_ovf" },
	{ .msk = BIT(19), .msg = "phy_tx_retry_buf_ovf" },
	{ .msk = BIT(20), .msg = "hydra_rx_ch0_ovf" },
	{ .msk = BIT(21), .msg = "hydra_rx_ch1_ovf" },
	{ .msk = BIT(22), .msg = "hydra_rx_ch2_ovf" },
	{ .msk = BIT(24),
	   .msg = "hydra_pcs_err0_hlink_cs_pll_out_of_lock_intr" },
	{ .msk = BIT(25), .msg = "hydra_pcs_err1_hlink_ds_alos_intr" },
	{ .msk = BIT(26), .msg = "hydra_pcs_err2_hlink_prbs_error_intr" },
	{ .msk = BIT(27),
	  .msg = "hydra_pcs_err3_hlink_cs_loss_of_ref_clk_intr" },
	{ .msk = BIT(28), .msg = "hydra_pcs_err4_hlink_ds_los_intr" },
	{ .msk = BIT(29), .msg = "hydra_pcs_err5_hlink_intr" },
	{ .msk = BIT(30),
	  .msg = "hydra_pcs_err6_hydra_pcs_rx_sync_header_err_intr" },
	{ .msk = BIT(31),
	  .msg = "hydra_pcs_err7_hydra_pcs_training_timeout_intr" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error hllc_hw_err_misc1_h[] = {
	{ .msk = BIT(0), .msg = "hydra_tx_ch0_1bit_ecc_err" },
	{ .msk = BIT(1), .msg = "hydra_tx_ch1_1bit_ecc_err" },
	{ .msk = BIT(2), .msg = "hydra_tx_ch2_1bit_ecc_err" },
	{ .msk = BIT(3), .msg = "phy_tx_retry_1bit_ecc_err" },
	{ .msk = BIT(4), .msg = "hydra_rx_ch0_1bit_ecc_err" },
	{ .msk = BIT(5), .msg = "hydra_rx_ch1_1bit_ecc_err" },
	{ .msk = BIT(6), .msg = "hydra_rx_ch2_1bit_ecc_err" },
	{ .msk = BIT(8), .msg = "phy_rx_flit_crc_err" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status hllc_serr_status[] = {
	{ .val = 0x01, .msg = "hllc_1bit_meme_ecc_error" },
	{ .val = 0x02, .msg = "hllc_crc_error" },
	{ .val = 0x81, .msg = "hllc_fatal_error" },
	{ .val = 0x82, .msg = "hydra_pcs_fatal_error" },
	{ .val = 0x83, .msg = "hydra_pcs_non_fatal_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status hllc_ierr_status[] = {
	{ .val = 0x1, .msg = "non_mem_ecc_error" },
	{ .val = 0x2, .msg = "mem_ecc_error " },
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
	case MODULE_ID_SAS: return "SAS";
	case MODULE_ID_SATA: return "SATA";
	}
	return "unknown";
}

static char *oem_type2_module_name(uint8_t module_id)
{
	switch (module_id) {
	case MODULE_ID_SMMU: return "SMMU";
	case MODULE_ID_HHA: return "HHA";
	case MODULE_ID_HLLC: return "HLLC";
	}
	return "unknown module";
}

static char *oem_type2_sub_module_id(char *p, uint8_t module_id,
				     uint8_t sub_module_id)
{
	switch (module_id) {
	case MODULE_ID_SMMU:
	case MODULE_ID_HLLC:
		p += sprintf(p, "%d ", sub_module_id);
		break;

	case MODULE_ID_HHA:
		if (sub_module_id == 0)
			p += sprintf(p, "TA HHA0 ");
		else if (sub_module_id == 1)
			p += sprintf(p, "TA HHA1 ");
		else if (sub_module_id == 2)
			p += sprintf(p, "TB HHA0 ");
		else if (sub_module_id == 3)
			p += sprintf(p, "TB HHA1 ");
		break;
	}

	return p;
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

static void
hisi_hip08_log_error_status(struct trace_seq *s, char *reg,
			    const struct hisi_hip08_hw_error_status *err_status,
			    uint32_t val)
{
	while (err_status->msg) {
		if (err_status->val == val)
			trace_seq_printf(s, "%s: %s\n", reg, err_status->msg);
		err_status++;
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

	case MODULE_ID_LPC:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "LPC_MEM_ACCESS_ST",
					     lpc_mem_access_st,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(s, "SC_MEM_ECC_ST0",
					     lpc_sc_mem_ecc_st0,
					     err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(s, "SC_MEM_ECC_ST1",
					     lpc_sc_mem_ecc_st1,
					     err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			trace_seq_printf(s, "OP_STATUS=0x%x\n",
					 err->err_misc_3);
		break;

	case MODULE_ID_SAS:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "SAS_RAS_INTR0",
					     sas_ras_intr0,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(s, "SAS_RAS_INTR1",
					     sas_ras_intr1,
					     err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(s, "SAS_RAS_INTR2",
					     sas_ras_intr2,
					     err->err_misc_2);
		break;

	case MODULE_ID_SATA:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(s, "SATA_RAS_IS",
					     sata_ras_is,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(s, "SATA_RAS_SERR",
					     sata_ras_serr,
					     err->err_misc_1);
		break;
	}
}

static void dec_type2_err_info(struct trace_seq *s,
			       const struct hisi_oem_type2_err_sec *err)
{
	uint16_t ierr_status;
	uint16_t serr_status;

	serr_status = err->err_status_0 & 0xFF;
	ierr_status = (err->err_status_0 >> 8) & 0xFF;
	trace_seq_printf(s, "Error Info:\n");

	switch (err->module_id) {
	case MODULE_ID_SMMU:
		hisi_hip08_log_error_status(s, "SMMU_ERR_STATUS_0:SERR",
					    smmu_serr_status, serr_status);
		hisi_hip08_log_error_status(s, "SMMU_ERR_STATUS_0:IERR",
					    smmu_ierr_status, ierr_status);
		break;

	case MODULE_ID_HHA:
		hisi_hip08_log_error_status(s, "HHA_ERR_STATUSL:SERR",
					    hha_serr_status, serr_status);
		hisi_hip08_log_error_status(s, "HHA_ERR_STATUSL:IERR",
					    hha_ierr_status, ierr_status);
		break;

	case MODULE_ID_HLLC:
		hisi_hip08_log_error_status(s, "HLLC_ERR_STATUSL:SERR",
					    hllc_serr_status, serr_status);
		hisi_hip08_log_error_status(s, "HLLC_ERR_STATUSL:IERR",
					    hllc_ierr_status, ierr_status);

		hisi_hip08_log_error(s, "HLLC_ERR_MISC1L",
				     hllc_hw_err_misc1_l, err->err_misc1_0);
		hisi_hip08_log_error(s, "HLLC_ERR_MISC1H",
				     hllc_hw_err_misc1_h, err->err_misc1_1);
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

static int decode_hip08_oem_type2_error(struct trace_seq *s, const void *error)
{
	const struct hisi_oem_type2_err_sec *err = error;
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
		p += sprintf(p, "module=%s ",
			     oem_type2_module_name(err->module_id));
	}

	if (err->val_bits & HISI_OEM_VALID_SUB_MODULE_ID)
		p =  oem_type2_sub_module_id(p, err->module_id,
					     err->sub_module_id);

	if (err->val_bits & HISI_OEM_VALID_ERR_SEVERITY)
		p += sprintf(p, "error severity=%s ",
			     err_severity(err->err_severity));
	p += sprintf(p, "]");
	trace_seq_printf(s, "\nHISI HIP08: OEM Type-2 Error\n");
	trace_seq_printf(s, "%s\n", buf);

	trace_seq_printf(s, "Reg Dump:\n");
	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_FR) {
		trace_seq_printf(s, "ERR_FR_0=0x%x\n", err->err_fr_0);
		trace_seq_printf(s, "ERR_FR_1=0x%x\n", err->err_fr_1);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_CTRL) {
		trace_seq_printf(s, "ERR_CTRL_0=0x%x\n", err->err_ctrl_0);
		trace_seq_printf(s, "ERR_CTRL_1=0x%x\n", err->err_ctrl_1);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_STATUS) {
		trace_seq_printf(s, "ERR_STATUS_0=0x%x\n", err->err_status_0);
		trace_seq_printf(s, "ERR_STATUS_1=0x%x\n", err->err_status_1);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_ADDR) {
		trace_seq_printf(s, "ERR_ADDR_0=0x%x\n", err->err_addr_0);
		trace_seq_printf(s, "ERR_ADDR_1=0x%x\n", err->err_addr_1);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_MISC_0) {
		trace_seq_printf(s, "ERR_MISC0_0=0x%x\n", err->err_misc0_0);
		trace_seq_printf(s, "ERR_MISC0_1=0x%x\n", err->err_misc0_1);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_MISC_1) {
		trace_seq_printf(s, "ERR_MISC1_0=0x%x\n", err->err_misc1_0);
		trace_seq_printf(s, "ERR_MISC1_1=0x%x\n", err->err_misc1_1);
	}

	dec_type2_err_info(s, err);

	return 0;
}

struct ras_ns_dec_tab hip08_ns_oem_tab[] = {
	{
		.sec_type = "1f8161e155d641e6bd107afd1dc5f7c5",
		.decode = decode_hip08_oem_type1_error,
	},
	{
		.sec_type = "45534ea6ce2341158535e07ab3aef91d",
		.decode = decode_hip08_oem_type2_error,
	},
};

__attribute__((constructor))
static void hip08_init(void)
{
	hip08_ns_oem_tab[0].len = ARRAY_SIZE(hip08_ns_oem_tab);
	register_ns_dec_tab(hip08_ns_oem_tab);
}
