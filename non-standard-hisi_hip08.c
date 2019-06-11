/*
 * Copyright (c) 2019 Hisilicon Limited.
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
/* HISI OEM format1 error definitions */
#define HISI_OEM_MODULE_ID_MN	0
#define HISI_OEM_MODULE_ID_PLL	1
#define HISI_OEM_MODULE_ID_SLLC	2
#define HISI_OEM_MODULE_ID_AA	3
#define HISI_OEM_MODULE_ID_SIOE	4
#define HISI_OEM_MODULE_ID_POE	5
#define HISI_OEM_MODULE_ID_DISP	8
#define HISI_OEM_MODULE_ID_LPC	9
#define HISI_OEM_MODULE_ID_SAS	15
#define HISI_OEM_MODULE_ID_SATA	16

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

/* HISI OEM format2 error definitions */
#define HISI_OEM_MODULE_ID_SMMU	0
#define HISI_OEM_MODULE_ID_HHA	1
#define HISI_OEM_MODULE_ID_HLLC	2
#define HISI_OEM_MODULE_ID_PA	3
#define HISI_OEM_MODULE_ID_DDRC	4

#define HISI_OEM_TYPE2_VALID_ERR_FR	BIT(6)
#define HISI_OEM_TYPE2_VALID_ERR_CTRL	BIT(7)
#define HISI_OEM_TYPE2_VALID_ERR_STATUS	BIT(8)
#define HISI_OEM_TYPE2_VALID_ERR_ADDR	BIT(9)
#define HISI_OEM_TYPE2_VALID_ERR_MISC_0	BIT(10)
#define HISI_OEM_TYPE2_VALID_ERR_MISC_1	BIT(11)

/* HISI PCIe Local error definitions */
#define HISI_PCIE_SUB_MODULE_ID_AP	0
#define HISI_PCIE_SUB_MODULE_ID_TL	1
#define HISI_PCIE_SUB_MODULE_ID_MAC	2
#define HISI_PCIE_SUB_MODULE_ID_DL	3
#define HISI_PCIE_SUB_MODULE_ID_SDI	4

#define HISI_PCIE_LOCAL_VALID_VERSION		BIT(0)
#define HISI_PCIE_LOCAL_VALID_SOC_ID		BIT(1)
#define HISI_PCIE_LOCAL_VALID_SOCKET_ID		BIT(2)
#define HISI_PCIE_LOCAL_VALID_NIMBUS_ID		BIT(3)
#define HISI_PCIE_LOCAL_VALID_SUB_MODULE_ID	BIT(4)
#define HISI_PCIE_LOCAL_VALID_CORE_ID		BIT(5)
#define HISI_PCIE_LOCAL_VALID_PORT_ID		BIT(6)
#define HISI_PCIE_LOCAL_VALID_ERR_TYPE		BIT(7)
#define HISI_PCIE_LOCAL_VALID_ERR_SEVERITY	BIT(8)
#define HISI_PCIE_LOCAL_VALID_ERR_MISC		9

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

struct hisi_pcie_local_err_sec {
	uint64_t   val_bits;
	uint8_t    version;
	uint8_t    soc_id;
	uint8_t    socket_id;
	uint8_t    nimbus_id;
	uint8_t    sub_module_id;
	uint8_t    core_id;
	uint8_t    port_id;
	uint8_t    err_severity;
	uint16_t   err_type;
	uint8_t    reserv[2];
	uint32_t   err_misc[33];
};

enum hisi_oem_data_type {
	hisi_oem_data_type_int,
	hisi_oem_data_type_int64,
	hisi_oem_data_type_text,
};

enum {
	hip08_oem_type1_field_id,
	hip08_oem_type1_field_version,
	hip08_oem_type1_field_soc_id,
	hip08_oem_type1_field_socket_id,
	hip08_oem_type1_field_nimbus_id,
	hip08_oem_type1_field_module_id,
	hip08_oem_type1_field_sub_module_id,
	hip08_oem_type1_field_err_sev,
	hip08_oem_type1_field_err_misc_0,
	hip08_oem_type1_field_err_misc_1,
	hip08_oem_type1_field_err_misc_2,
	hip08_oem_type1_field_err_misc_3,
	hip08_oem_type1_field_err_misc_4,
	hip08_oem_type1_field_err_addr,
	hip08_oem_type1_field_err_info,
};

enum {
	hip08_oem_type2_field_id,
	hip08_oem_type2_field_version,
	hip08_oem_type2_field_soc_id,
	hip08_oem_type2_field_socket_id,
	hip08_oem_type2_field_nimbus_id,
	hip08_oem_type2_field_module_id,
	hip08_oem_type2_field_sub_module_id,
	hip08_oem_type2_field_err_sev,
	hip08_oem_type2_field_err_fr_0,
	hip08_oem_type2_field_err_fr_1,
	hip08_oem_type2_field_err_ctrl_0,
	hip08_oem_type2_field_err_ctrl_1,
	hip08_oem_type2_field_err_status_0,
	hip08_oem_type2_field_err_status_1,
	hip08_oem_type2_field_err_addr_0,
	hip08_oem_type2_field_err_addr_1,
	hip08_oem_type2_field_err_misc0_0,
	hip08_oem_type2_field_err_misc0_1,
	hip08_oem_type2_field_err_misc1_0,
	hip08_oem_type2_field_err_misc1_1,
	hip08_oem_type2_field_err_info,
};

enum {
	hip08_pcie_local_field_id,
	hip08_pcie_local_field_version,
	hip08_pcie_local_field_soc_id,
	hip08_pcie_local_field_socket_id,
	hip08_pcie_local_field_nimbus_id,
	hip08_pcie_local_field_sub_module_id,
	hip08_pcie_local_field_core_id,
	hip08_pcie_local_field_port_id,
	hip08_pcie_local_field_err_sev,
	hip08_pcie_local_field_err_type,
	hip08_pcie_local_field_err_misc,
};

struct hisi_hip08_hw_error {
	uint64_t msk;
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

static const struct hisi_hip08_hw_error pa_hw_err_misc1_l[] = {
	{ .msk = BIT(24), .msg = "err_req_dec_miss_st" },
	{ .msk = BIT(25), .msg = "err_req_daw_overlap" },
	{ .msk = BIT(26), .msg = "err_req_msd_overlap" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error pa_hw_err_misc1_h[] = {
	{ .msk = BIT(0), .msg = "h0_rx_req_daw_err" },
	{ .msk = BIT(1), .msg = "h1_rx_req_daw_err" },
	{ .msk = BIT(2), .msg = "h2_rx_req_daw_err" },
	{ .msk = BIT(27), .msg = "err_comprsp_err" },
	{ .msk = BIT(28), .msg = "err_compdat_err" },
	{ .msk = BIT(29), .msg = "h0_tx_rsp_err" },
	{ .msk = BIT(30), .msg = "h1_tx_rsp_err" },
	{ .msk = BIT(31), .msg = "h2_tx_rsp_err" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status pa_serr_status[] = {
	{ .val = 0x2, .msg = "memory_ecc_error" },
	{ .val = 0xD, .msg = "daw_error" },
	{ .val = 0x12, .msg = "rsp_error" },
	{ .val = 0x14, .msg = "buffer_overflow_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status pa_ierr_status[] = {
	{ .val = 0x1, .msg = "memory_2bit_ecc_error" },
	{ .val = 0x2, .msg = "buffer_overflow_error" },
	{ .val = 0x3, .msg = "rsp_error" },
	{ .val = 0x4, .msg = "daw_error" },
	{ .val = 0x5, .msg = "memory_1bit_ecc_error" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error_status ddrc_ierr_status[] = {
	{ .val = 0x0B, .msg = "ha_alarm_int" },
	{ .val = 0x0C, .msg = "vls_correctable_int" },
	{ .val = 0x0D, .msg = "rvls_correctable_error" },
	{ .val = 0x0E, .msg = "ebram_serr_int" },
	{ .val = 0x47, .msg = "pa_correctable_error" },
	{ .val = 0x48, .msg = "vls_int" },
	{ .val = 0x49, .msg = "sp_dev_int" },
	{ .val = 0x4A, .msg = "sp_rnk_int" },
	{ .val = 0x60, .msg = "ha_uncorrected_error" },
	{ .val = 0x61, .msg = "pa_uncorrected_error" },
	{ .val = 0x62, .msg = "sp_uncorrected_error" },
	{ .val = 0x63, .msg = "sp_rb_uncorrected_error" },
	{ .val = 0x64, .msg = "vls_uncorrected_error" },
	{ .val = 0x65, .msg = "rvls_uncorrected_error" },
	{ .val = 0x66, .msg = "ebram_merr_int" },
	{ .val = 0x91, .msg = "ext_mem_serr_int" },
	{ .val = 0x92, .msg = "wsram_serr_int" },
	{ .val = 0x93, .msg = "sbram_serr_int" },
	{ .val = 0x94, .msg = "recram_serr_int" },
	{ .val = 0x95, .msg = "rpram_serr_int" },
	{ .val = 0x96, .msg = "fwdram_serr_int" },
	{ .val = 0xA0, .msg = "rdtimeout_int" },
	{ .val = 0xA1, .msg = "aref_alarm_int" },
	{ .val = 0xA2, .msg = "sbram_merr_int" },
	{ .val = 0xA3, .msg = "recram_merr_int" },
	{ .val = 0xA4, .msg = "rpram_merr_int" },
	{ .val = 0xA5, .msg = "fwdram_merr_int" },
	{ .val = 0xA6, .msg = "rec_err_int" },
	{ .val = 0xCB, .msg = "rec_int" },
	{ .val = 0xCC, .msg = "stadat_max_int" },
	{ .val = 0xCD, .msg = "stadat_min_int" },
	{ .val = 0xCE, .msg = "stacmd_max_int" },
	{ .val = 0xCF, .msg = "stacmd_min_int" },
	{ .val = 0xD0, .msg = "flux_int" },
	{ .val = 0xE7, .msg = "sref_err_int " },
	{ .val = 0xE8, .msg = "dimm_parity_err_int" },
	{ .val = 0xE9, .msg = "ext_mem_merr_int" },
	{ .val = 0xEA, .msg = "wsram_merr_int" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error hisi_pcie_ap_err_misc[] = {
	{ .msk = BIT(9), .msg = "PCIE_GLOBAL_REG_AP_TIMEOUT_NUM" },
	{ .msk = BIT(10), .msg = "PCIE_GLOBAL_REG_APB_TIMEOUT_INFO" },
	{ .msk = BIT(11), .msg = "AP_TX_REG_IOB_TX_INT_STATUS1" },
	{ .msk = BIT(12), .msg = "AP_TX_REG_IOB_TX_CHI_P2P_UNMATCH_ADDR_L" },
	{ .msk = BIT(13), .msg = "AP_TX_REG_IOB_TX_CHI_P2P_UNMATCH_ADDR_H" },
	{ .msk = BIT(14), .msg = "AP_TX_REG_IOB_TX_CPL_RAM_ERR_INFO" },
	{ .msk = BIT(15), .msg = "AP_TX_REG_IOB_TX_INT_STATUS2" },
	{ .msk = BIT(16), .msg = "AP_TX_REG_IOB_TX_INT_STATUS3" },
	{ .msk = BIT(17), .msg = "AP_TX_REG_IOB_TX_INT_STATUS4" },
	{ .msk = BIT(18), .msg = "AP_TX_REG_IOB_TX_INT_STATUS5" },
	{ .msk = BIT(19), .msg = "AP_TX_REG_IOB_TX_CHI_UNEXP_REQ_RCVD" },
	{ .msk = BIT(20), .msg = "AP_RX_REG_IOB_ODR_INT_SRC" },
	{ .msk = BIT(21), .msg = "AP_RX_REG_IOB_ODR_SRAM_ECC_STS_1" },
	{ .msk = BIT(22), .msg = "AP_RX_REG_IOB_ODR_SRAM_ECC_STS_0" },
	{ .msk = BIT(23), .msg = "AP_REG_SDI_AXIM_INT_SRC" },
	{ .msk = BIT(24), .msg = "AP_REG_DMA_QUEUE_INT_RO" },
	{ .msk = BIT(25), .msg = "AP_REG_MCTP_INTRPT_STAT" },
	{ .msk = BIT(26), .msg = "AP_REG_ECC_ERR_INT_STS" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error hisi_pcie_tl_err_misc[] = {
	{ .msk = BIT(9), .msg = "TL_REG_TL_INT_STATUS0" },
	{ .msk = BIT(10), .msg = "TL_REG_TL_RX_ERR_STATUS" },
	{ .msk = BIT(11), .msg = "TL_REG_TL_RX_ECC_ERROR_STATUS" },
	{ .msk = BIT(12), .msg = "TL_REG_TL_TX_ECC_ERROR_STATUS" },
	{ .msk = BIT(13), .msg = "TL_REG_TL_TX_ECC_2BIT_ERR_CNT" },
	{ .msk = BIT(14), .msg = "TL_REG_TL_ECC_2BIT_ERR_CNT" },
	{ .msk = BIT(15), .msg = "TL_REG_TL_ECC_2BIT_ERR" },
	{ .msk = BIT(16), .msg = "TL_REG_TL_TX_ECC_2BIT_ERR_ADDR" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error hisi_pcie_mac_err_misc[] = {
	{ .msk = BIT(9), .msg = "MAC_REG_MAC_INT_STATUS" },
	{ .msk = BIT(10), .msg = "MAC_REG_LINK_INFO" },
	{ .msk = BIT(11), .msg = "MAC_REG_DEBUG_PIPE7" },
	{ .msk = BIT(12), .msg = "MAC_REG_DEBUG_PIPE8" },
	{ .msk = BIT(13), .msg = "MAC_REG_DEBUG_PIPE9" },
	{ .msk = BIT(14), .msg = "MAC_REG_DEBUG_PIPE10" },
	{ .msk = BIT(15), .msg = "MAC_REG_DEBUG_PIPE11" },
	{ .msk = BIT(16), .msg = "MAC_REG_SYMBOL_UNLOCK_COUNTER" },
	{ .msk = BIT(17), .msg = "MAC_REG_TEST_COUNTER" },
	{ .msk = BIT(18), .msg = "MAC_REG_PCS_RX_ERR_CNT" },
	{ /* sentinel */ }
};


static const struct hisi_hip08_hw_error hisi_pcie_dl_err_misc[] = {
	{ .msk = BIT(9), .msg = "DL_REG_DL_INT_STATUS" },
	{ .msk = BIT(10), .msg = "DL_REG_DL_RX_NAK_COUNT" },
	{ .msk = BIT(11), .msg = "DL_REG_DFX_RX_BAD_DLLP_TYPE" },
	{ .msk = BIT(12), .msg = "DL_REG_DFX_MAC_BP_TIMER" },
	{ .msk = BIT(13), .msg = "DL_REG_DFX_RETRY_CNT" },
	{ .msk = BIT(14), .msg = "DL_REG_DFX_LCRC_ERR_NUM" },
	{ .msk = BIT(15), .msg = "DL_REG_DFX_DCRC_ERR_NUM" },
	{ .msk = BIT(16), .msg = "DL_REG_DFX_FSM_STATE" },
	{ /* sentinel */ }
};

static const struct hisi_hip08_hw_error hisi_pcie_sdi_err_misc[] = {
	{ .msk = BIT(9), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG0" },
	{ .msk = BIT(10), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG1" },
	{ .msk = BIT(11), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG2" },
	{ .msk = BIT(12), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG3" },
	{ .msk = BIT(13), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG4" },
	{ .msk = BIT(14), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG5" },
	{ .msk = BIT(15), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG6" },
	{ .msk = BIT(16), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG7" },
	{ .msk = BIT(17), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG8" },
	{ .msk = BIT(18), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG9" },
	{ .msk = BIT(19), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG10" },
	{ .msk = BIT(20), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG11" },
	{ .msk = BIT(21), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG12" },
	{ .msk = BIT(22), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG13" },
	{ .msk = BIT(23), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG14" },
	{ .msk = BIT(24), .msg = "PCIE_NVME_GLOBAL_REG_CQ_ERR_INT_CFG15" },
	{ .msk = BIT(25), .msg = "PCIE_NVME_GLOBAL_REG_SQ_ERR_STS0" },
	{ .msk = BIT(26), .msg = "PCIE_NVME_GLOBAL_REG_SQ_SIZE_RAM_ECC_INJECT" },
	{ .msk = BIT(27), .msg = "PCIE_NVME_GLOBAL_REG_SQ_BAL_RAM_ECC_INJECT" },
	{ .msk = BIT(28), .msg = "PCIE_NVME_GLOBAL_REG_SQ_BAH_RAM_ECC_INJECT" },
	{ .msk = BIT(29), .msg = "PCIE_NVME_GLOBAL_REG_RD_PTR_RAM_ECC_INJECT" },
	{ .msk = BIT(30), .msg = "PCIE_NVME_GLOBAL_REG_SQ_DES_RAM_ECC_INJECT" },
	{ .msk = BIT(31), .msg = "PCIE_NVME_GLOBAL_REG_PREFETCH_FIFO_RAM_ECC_INJECT" },
	{ .msk = BIT(32), .msg = "PCIE_NVME_GLOBAL_REG_BRESP_ERR_INT" },
	{ .msk = BIT(33), .msg = "PCIE_VIRTIO_GLOBAL_REG_ECC_INT_STATUS" },
	{ .msk = BIT(34), .msg = "PCIE_VIRTIO_QUEUE_REG_ENGN_INT_STS" },
	{ .msk = BIT(35), .msg = "PCIE_VIRTIO_QUEUE_REG_LOC_RPTR_CSR_RAM_EC_ADDR" },
	{ .msk = BIT(36), .msg = "PCIE_VIRTIO_QUEUE_REG_LOC_WPTR_CSR_RAM_ECC_ADDR" },
	{ .msk = BIT(37), .msg = "PCIE_VIRTIO_QUEUE_REG_LOC_AV_IDX_CSR_RAM_ECC_ADDR" },
	{ .msk = BIT(38), .msg = "PCIE_VIRTIO_QUEUE_REG_FETCH_AV_CPL_ERR_VQ_NUM" },
	{ .msk = BIT(39), .msg = "PCIE_VIRTIO_QUEUE_REG_DMA_CQ_ERR_STS" },
	{ .msk = BIT(40), .msg = "PCIE_VIRTIO_QUEUE_REG_FETCH_AV_IDX_ERR_QN" },
	{ .msk = BIT(41), .msg = "PCIE_VIRTIO_QUEUE_REG_ENGN_GET_DT_INF_BASE_ERR_PF" },
	{ /* sentinel */ }
};

#ifdef HAVE_SQLITE3
static const struct db_fields hip08_oem_type1_event_fields[] = {
	{ .name = "id",			.type = "INTEGER PRIMARY KEY" },
	{ .name = "version",		.type = "INTEGER" },
	{ .name = "soc_id",		.type = "INTEGER" },
	{ .name = "socket_id",		.type = "INTEGER" },
	{ .name = "nimbus_id",		.type = "INTEGER" },
	{ .name = "module_id",		.type = "TEXT" },
	{ .name = "sub_module_id",	.type = "INTEGER" },
	{ .name = "err_severity",	.type = "TEXT" },
	{ .name = "err_misc_0",		.type = "INTEGER" },
	{ .name = "err_misc_1",		.type = "INTEGER" },
	{ .name = "err_misc_2",		.type = "INTEGER" },
	{ .name = "err_misc_3",		.type = "INTEGER" },
	{ .name = "err_misc_4",		.type = "INTEGER" },
	{ .name = "err_addr",		.type = "INTEGER" },
	{ .name = "err_info",		.type = "TEXT" },
};

static const struct db_table_descriptor hip08_oem_type1_event_tab = {
	.name = "hip08_oem_type1_event",
	.fields = hip08_oem_type1_event_fields,
	.num_fields = ARRAY_SIZE(hip08_oem_type1_event_fields),
};

static const struct db_fields hip08_oem_type2_event_fields[] = {
	{ .name = "id",                 .type = "INTEGER PRIMARY KEY" },
	{ .name = "version",            .type = "INTEGER" },
	{ .name = "soc_id",             .type = "INTEGER" },
	{ .name = "socket_id",          .type = "INTEGER" },
	{ .name = "nimbus_id",          .type = "INTEGER" },
	{ .name = "module_id",          .type = "TEXT" },
	{ .name = "sub_module_id",      .type = "INTEGER" },
	{ .name = "err_severity",       .type = "TEXT" },
	{ .name = "err_fr_0",		.type = "INTEGER" },
	{ .name = "err_fr_1",		.type = "INTEGER" },
	{ .name = "err_ctrl_0",		.type = "INTEGER" },
	{ .name = "err_ctrl_1",		.type = "INTEGER" },
	{ .name = "err_status_0",	.type = "INTEGER" },
	{ .name = "err_status_1",	.type = "INTEGER" },
	{ .name = "err_addr_0",         .type = "INTEGER" },
	{ .name = "err_addr_1",         .type = "INTEGER" },
	{ .name = "err_misc0_0",	.type = "INTEGER" },
	{ .name = "err_misc0_1",	.type = "INTEGER" },
	{ .name = "err_misc1_0",	.type = "INTEGER" },
	{ .name = "err_misc1_1",	.type = "INTEGER" },
	{ .name = "err_info",		.type = "TEXT" },
};

static const struct db_table_descriptor hip08_oem_type2_event_tab = {
	.name = "hip08_oem_type2_event",
	.fields = hip08_oem_type2_event_fields,
	.num_fields = ARRAY_SIZE(hip08_oem_type2_event_fields),
};

static const struct db_fields hip08_pcie_local_event_fields[] = {
	{ .name = "id",                 .type = "INTEGER PRIMARY KEY" },
	{ .name = "version",            .type = "INTEGER" },
	{ .name = "soc_id",             .type = "INTEGER" },
	{ .name = "socket_id",          .type = "INTEGER" },
	{ .name = "nimbus_id",          .type = "INTEGER" },
	{ .name = "sub_module_id",      .type = "TEXT" },
	{ .name = "core_id",		.type = "INTEGER" },
	{ .name = "port_id",		.type = "INTEGER" },
	{ .name = "err_severity",       .type = "TEXT" },
	{ .name = "err_type",		.type = "TEXT" },
	{ .name = "err_misc0",		.type = "INTEGER" },
	{ .name = "err_misc1",		.type = "INTEGER" },
	{ .name = "err_misc2",		.type = "INTEGER" },
	{ .name = "err_misc3",		.type = "INTEGER" },
	{ .name = "err_misc4",		.type = "INTEGER" },
	{ .name = "err_misc5",		.type = "INTEGER" },
	{ .name = "err_misc6",		.type = "INTEGER" },
	{ .name = "err_misc7",		.type = "INTEGER" },
	{ .name = "err_misc8",		.type = "INTEGER" },
	{ .name = "err_misc9",		.type = "INTEGER" },
	{ .name = "err_misc10",		.type = "INTEGER" },
	{ .name = "err_misc11",		.type = "INTEGER" },
	{ .name = "err_misc12",		.type = "INTEGER" },
	{ .name = "err_misc13",		.type = "INTEGER" },
	{ .name = "err_misc14",		.type = "INTEGER" },
	{ .name = "err_misc15",		.type = "INTEGER" },
	{ .name = "err_misc16",		.type = "INTEGER" },
	{ .name = "err_misc17",		.type = "INTEGER" },
	{ .name = "err_misc18",		.type = "INTEGER" },
	{ .name = "err_misc19",		.type = "INTEGER" },
	{ .name = "err_misc20",		.type = "INTEGER" },
	{ .name = "err_misc21",		.type = "INTEGER" },
	{ .name = "err_misc22",		.type = "INTEGER" },
	{ .name = "err_misc23",		.type = "INTEGER" },
	{ .name = "err_misc24",		.type = "INTEGER" },
	{ .name = "err_misc25",		.type = "INTEGER" },
	{ .name = "err_misc26",		.type = "INTEGER" },
	{ .name = "err_misc27",		.type = "INTEGER" },
	{ .name = "err_misc28",		.type = "INTEGER" },
	{ .name = "err_misc29",		.type = "INTEGER" },
	{ .name = "err_misc30",		.type = "INTEGER" },
	{ .name = "err_misc31",		.type = "INTEGER" },
	{ .name = "err_misc32",		.type = "INTEGER" },
};

static const struct db_table_descriptor hip08_pcie_local_event_tab = {
	.name = "hip08_pcie_local_event",
	.fields = hip08_pcie_local_event_fields,
	.num_fields = ARRAY_SIZE(hip08_pcie_local_event_fields),
};

static void record_vendor_data(struct ras_ns_dec_tab *dec_tab,
			       enum hisi_oem_data_type data_type,
			       int id, int64_t data, const char *text)
{
	switch (data_type) {
	case hisi_oem_data_type_int:
		sqlite3_bind_int(dec_tab->stmt_dec_record, id, data);
		break;
	case hisi_oem_data_type_int64:
		sqlite3_bind_int64(dec_tab->stmt_dec_record, id, data);
		break;
	case hisi_oem_data_type_text:
		sqlite3_bind_text(dec_tab->stmt_dec_record, id, text, -1, NULL);
		break;
	default:
		break;
	}
}

static int step_vendor_data_tab(struct ras_ns_dec_tab *dec_tab, char *name)
{
	int rc;

	rc = sqlite3_step(dec_tab->stmt_dec_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		log(TERM, LOG_ERR,
		    "Failed to do %s step on sqlite: error = %d\n", name, rc);

	rc = sqlite3_reset(dec_tab->stmt_dec_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		log(TERM, LOG_ERR,
		    "Failed reset %s on sqlite: error = %d\n", name, rc);

	rc = sqlite3_clear_bindings(dec_tab->stmt_dec_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		log(TERM, LOG_ERR,
		    "Failed to clear bindings %s on sqlite: error = %d\n", name, rc);

	return rc;
}

#else

static void record_vendor_data(struct ras_ns_dec_tab *dec_tab,
			       enum hisi_oem_data_type data_type,
			       int id, int64_t data, const char *text)
{ }

static int step_vendor_data_tab(struct ras_ns_dec_tab *dec_tab, char *name)
{
	return 0;
}
#endif

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
	case HISI_OEM_MODULE_ID_MN: return "MN";
	case HISI_OEM_MODULE_ID_PLL: return "PLL";
	case HISI_OEM_MODULE_ID_SLLC: return "SLLC";
	case HISI_OEM_MODULE_ID_AA: return "AA";
	case HISI_OEM_MODULE_ID_SIOE: return "SIOE";
	case HISI_OEM_MODULE_ID_POE: return "POE";
	case HISI_OEM_MODULE_ID_DISP: return "DISP";
	case HISI_OEM_MODULE_ID_LPC: return "LPC";
	case HISI_OEM_MODULE_ID_SAS: return "SAS";
	case HISI_OEM_MODULE_ID_SATA: return "SATA";
	}
	return "unknown";
}

static char *oem_type2_module_name(uint8_t module_id)
{
	switch (module_id) {
	case HISI_OEM_MODULE_ID_SMMU: return "SMMU";
	case HISI_OEM_MODULE_ID_HHA: return "HHA";
	case HISI_OEM_MODULE_ID_HLLC: return "HLLC";
	case HISI_OEM_MODULE_ID_PA: return "PA";
	case HISI_OEM_MODULE_ID_DDRC: return "DDRC";
	}
	return "unknown module";
}

static char *oem_type2_sub_module_id(char *p, uint8_t module_id,
				     uint8_t sub_module_id)
{
	switch (module_id) {
	case HISI_OEM_MODULE_ID_SMMU:
	case HISI_OEM_MODULE_ID_HLLC:
	case HISI_OEM_MODULE_ID_PA:
		p += sprintf(p, "%d ", sub_module_id);
		break;

	case HISI_OEM_MODULE_ID_HHA:
		if (sub_module_id == 0)
			p += sprintf(p, "TA HHA0 ");
		else if (sub_module_id == 1)
			p += sprintf(p, "TA HHA1 ");
		else if (sub_module_id == 2)
			p += sprintf(p, "TB HHA0 ");
		else if (sub_module_id == 3)
			p += sprintf(p, "TB HHA1 ");
		break;

	case HISI_OEM_MODULE_ID_DDRC:
		if (sub_module_id == 0)
			p += sprintf(p, "TA DDRC0 ");
		else if (sub_module_id == 1)
			p += sprintf(p, "TA DDRC1 ");
		else if (sub_module_id == 2)
			p += sprintf(p, "TA DDRC2 ");
		else if (sub_module_id == 3)
			p += sprintf(p, "TA DDRC3 ");
		else if (sub_module_id == 4)
			p += sprintf(p, "TB DDRC0 ");
		else if (sub_module_id == 5)
			p += sprintf(p, "TB DDRC1 ");
		else if (sub_module_id == 6)
			p += sprintf(p, "TB DDRC2 ");
		else if (sub_module_id == 7)
			p += sprintf(p, "TB DDRC3 ");
		break;
	}

	return p;
}

static char *pcie_local_sub_module_name(uint8_t id)
{
	switch (id) {
	case HISI_PCIE_SUB_MODULE_ID_AP: return "AP Layer";
	case HISI_PCIE_SUB_MODULE_ID_TL: return "TL Layer";
	case HISI_PCIE_SUB_MODULE_ID_MAC: return "MAC Layer";
	case HISI_PCIE_SUB_MODULE_ID_DL: return "DL Layer";
	case HISI_PCIE_SUB_MODULE_ID_SDI: return "SDI Layer";
	}
	return "unknown";
}

static char *pcie_ap_err_type(int etype)
{
	switch (etype) {
	case 0: return "AP: IOB_TX_INT_STATUS1: IOB tx cpl ram 2 bit error";
	case 1: return "AP: IOB_TX_INT_STATUS1: IOB tx cpl ram 1 bit error";
	case 2: return "AP: IOB_TX_INT_STATUS1: IOB tx addr not match ATU/ECAM";
	case 3: return "AP: IOB_TX_INT_STATUS1: IOB tx non post request timeout";
	case 4: return "AP: IOB_TX_INT_STATUS1: chi p2p pcpl traffic not match";
	case 32: return "AP: IOB_TX_INT_STATUS2: chi cpu traffic ram 2 bit error";
	case 33: return "AP: IOB_TX_INT_STATUS2: chi cpu traffic ram 1 bit error";
	case 34: return "AP: IOB_TX_INT_STATUS2: chi p2p traffic ram 2 bit error";
	case 35: return "AP: IOB_TX_INT_STATUS2: chi p2p traffic ram 1 bit error";
	case 36: return "AP: IOB_TX_INT_STATUS2: IOB tx port request timeout";
	case 64: return "AP: IOB_TX_INT_STATUS3: IOB tx completion timeout";
	case 96: return "AP: IOB_TX_INT_STATUS4: IOB tx cfg retry";
	case 128: return "AP: IOB_TX_INT_STATUS5: CPL timeout for NP";
	case 160: return "AP: IOB_TX_CHI_UNEXP_REQ_RCVD: unexpected request received";
	case 192: return "AP: IB_ODR_INT_SRC: axi b channel response error";
	case 193: return "AP: IB_ODR_INT_SRC: axi r channel poison data";
	case 194: return "AP: IB_ODR_INT_SRC: axi r channel response error";
	case 195: return "AP: IB_ODR_INT_SRC: P/cpl sbm data buffer 2 bit error";
	case 196: return "AP: IB_ODR_INT_SRC: P/cpl sbm data buffer 1 bit error";
	case 197: return "AP: IB_ODR_INT_SRC: npq data buffer 2 bit error";
	case 198: return "AP: IB_ODR_INT_SRC: npq data buffer 1 bit error";
	case 224: return "AP: SDI_AXIM_INT_SRC : axi bresp error";
	case 225: return "AP: SDI_AXIM_INT_SRC : axi r channel read data error";
	case 226: return "AP: SDI_AXIM_INT_SRC : axi r channel read response error";
	case 227: return "AP: SDI_AXIM_INT_SRC : write data buffer mbit ecc error";
	case 228: return "AP: SDI_AXIM_INT_SRC : write data buffer 1bit ecc error";
	case 229: return "AP: SDI_AXIM_INT_SRC : read data buffer mbit ecc error";
	case 230: return "AP: SDI_AXIM_INT_SRC : read data buffer 1bit ecc error";
	case 256: return "AP: DMA_QUEUE_INT_RO: link down";
	case 257: return "AP: DMA_QUEUE_INT_RO: data poison in remote side";
	case 258: return "AP: DMA_QUEUE_INT_RO: data poison when SQ read";
	case 259: return "AP: DMA_QUEUE_INT_RO: CQ queue full status";
	case 260: return "AP: DMA_QUEUE_INT_RO: CQ write back error";
	case 261: return "AP: DMA_QUEUE_INT_RO: data poison in local side";
	case 262: return "AP: DMA_QUEUE_INT_RO: axi master write response error";
	case 263: return "AP: DMA_QUEUE_INT_RO: axi master read response error";
	case 264: return "AP: DMA_QUEUE_INT_RO: read response error in remote side";
	case 265: return "AP: DMA_QUEUE_INT_RO: write response error in remote side";
	case 266: return "AP: DMA_QUEUE_INT_RO: drop occured";
	case 267: return "AP: DMA_QUEUE_INT_RO: invalid length field occured";
	case 268: return "AP: DMA_QUEUE_INT_RO: invalid opcode occured";
	case 269: return "AP: DMA_QUEUE_INT_RO: submission descriptor read response";
	case 288: return "AP: MCTP_INTRPT_STAT: mctp rx ram 2 bit ecc error";
	case 289: return "AP: MCTP_INTRPT_STAT: mctp tx ram 2 bit ecc error";
	case 290: return "AP: MCTP_INTRPT_STAT: axi bresponse error";
	case 291: return "AP: MCTP_INTRPT_STAT: axi read response error";
	case 292: return "AP: MCTP_INTRPT_STAT: mctp rx discards other vdm packets";
	case 293: return "AP: MCTP_INTRPT_STAT: mctp rx discards received invalite and pri msg packets";
	case 294: return "AP: MCTP_INTRPT_STAT: mctp rx discards received mctp packets";
	case 295: return "AP: MCTP_INTRPT_STAT: mctp tx discards pkts read from ddr";
	case 296: return "AP: MCTP_INTRPT_STAT: mctp rx discards packets, invalid pkt length";
	case 297: return "AP: MCTP_INTRPT_STAT: mctp tx packet verification failed";
	case 320: return "AP: MCTP_INTRPT_STAT: msix coal table 2 bit ecc error";
	case 321: return "AP: MCTP_INTRPT_STAT: msix coal table 1bit ecc error";
	case 322: return "AP: MCTP_INTRPT_STAT: msix table 2 bit ecc error";
	case 323: return "AP: MCTP_INTRPT_STAT: msix table 1 bit ecc error";
	}
	return "unknown error";
}

static char *pcie_tl_err_type(int etype)
{
	switch (etype) {
	case 384: return "TL: TL_INT_STATUS0, TL_RX_ERR_STATUS: tl rx p/np/cpl/ccix write fifo full error";
	case 385: return "TL: TL_INT_STATUS0, TL_RX_ERR_STATUS: tl rx ccix opt tlp length error";
	case 386: return "TL: TL_INT_STATUS0: tl_tx_rp_pf_vf_err";
	case 387: return "TL: TL_INT_STATUS0: tl_tx_ccix_vc_p_tc_map_err";
	case 388: return "TL: TL_INT_STATUS0, TL_TX_TC_MAPPER_TC: tl_tx_vc0_cpl_tc_map_err";
	case 389: return "TL: TL_INT_STATUS0, TL_TX_TC_MAPPER_TC: tl_tx_vc0_np_tc_map_err";
	case 390: return "TL: TL_INT_STATUS0, TL_TX_TC_MAPPER_TC: tl_tx_vc0_p_tc_map_err";
	case 391: return "TL: TL_INT_STATUS0: tl_tx_bus_master_en_err";
	case 392: return "TL: TL_INT_STATUS0: tl_rx_pfx_mal_err";
	case 393: return "TL: TL_INT_STATUS0: tl_rx_cpl_credit_overflow_err";
	case 394: return "TL: TL_INT_STATUS0: tl_rx_aer_err";
	case 395: return "TL: TL_INT_STATUS0: tl_ecc_2bit_err";
	case 396: return "TL: TL_INT_STATUS0: tl_ecc_1bit_err";
	case 416: return "TL: DPC_CAP_0X0C: mem request completion timeout";
	case 417: return "TL: DPC_CAP_0X0C: mem request received CA completion";
	case 418: return "TL: DPC_CAP_0X0C: mem request received UR completion";
	case 419: return "TL: DPC_CAP_0X0C: I/O request completion timeout";
	case 420: return "TL: DPC_CAP_0X0C: I/O request received CA completion";
	case 421: return "TL: DPC_CAP_0X0C: I/O request received UR completion";
	case 422: return "TL: DPC_CAP_0X0C: config request completion timeout";
	case 423: return "TL: DPC_CAP_0X0C: config request received CA completion";
	case 424: return "TL: DPC_CAP_0X0C: config request received UR completion";
	case 425: return "TL: DPC_CAP_0X0C: DPC interrupt status";
	case 426: return "TL: DPC_CAP_0X0C: DPC trigger status";

	}
	return "unknown error";
}

static char *pcie_mac_err_type(int etype)
{
	switch (etype) {
	case 448: return "MAC: MAC_REG_MAC_INT_STATUS: mac entry L1 timeout error";
	case 449: return "MAC: MAC_REG_MAC_INT_STATUS: MACDESKEW fifo overflow error";
	case 450: return "MAC: MAC_REG_MAC_INT_STATUS: PCS appear symbol unlock";
	case 451: return "MAC: MAC_REG_MAC_INT_STATUS: MACDESKEW unlocked";
	case 452: return "MAC: MAC_REG_MAC_INT_STATUS: pcie link down";
	}
	return "unknown error";
}

static char *pcie_dl_err_type(int etype)
{
	switch (etype) {
	case 480: return "DL: DL_INT_STATUS: rx_fc_vc1_update_timeout_cpl_int";
	case 481: return "DL: DL_INT_STATUS: rx_fc_vc1_update_timeout_np_int";
	case 482: return "DL: DL_INT_STATUS: rx_fc_vc1_update_timeout_p_int";
	case 483: return "DL: DL_INT_STATUS: rx_fc_vc0_update_timeout_cpl_int";
	case 484: return "DL: DL_INT_STATUS: rx_fc_vc0_update_timeout_np_int";
	case 485: return "DL: DL_INT_STATUS: rx_fc_vc0_update_timeout_p_int";
	case 486: return "DL: DL_INT_STATUS: dl_corrected_err_cnt_timeout_int";
	case 487: return "DL: DL_INT_STATUS: rx_fc_update_timeout_int";
	case 488: return "DL: DL_INT_STATUS: dl_mac_retrain_cnt_overflow_int";
	case 489: return "DL: DL_INT_STATUS: tl_dl_credit_null_timeout_int";
	case 490: return "DL: DL_INT_STATUS: dl_nak_timer_timeout_int";
	case 491: return "DL: DL_INT_STATUS: dl_dup_tlp_ack_cnt_rollover_int";
	case 492: return "DL: DL_INT_STATUS: link_fail_dl_int";
	case 493: return "DL: DL_INT_STATUS: retrain_dl_int";
	case 494: return "DL: DL_INT_STATUS: ecc_2bit_err_dl_int";
	case 495: return "DL: DL_INT_STATUS: ecc_1bit_err_dl_int";
	case 496: return "DL: DL_INT_STATUS: init_timeout_dl_int";
	}
	return "unknown error";
}

static char *pcie_sdi_err_type(int etype)
{
	if (etype >= 512 && etype <= 543)
		return "SDI: CQ_ERR_INT_CFG: cq ret by dma in a queue task is abnormal";

	switch (etype) {
	case 544: return "SDI: SQ_ERR_STS0: incorrect sqdb pointer delivered by the host";
	case 545: return "SDI: SQ_ERR_STS0: incorrect hd pointer of the SQ queue";
	case 576: return "SDI: SQ_SIZE_RAM_ECC_INJECT: sq_size_ram_2bit_ecc_error";
	case 577: return "SDI: SQ_SIZE_RAM_ECC_INJECT: sq_size_ram_1bit_ecc_error";
	case 608: return "SDI: SQ_BAL_RAM_ECC_INJECT: sq_bal_ram_2bit_ecc_error";
	case 609: return "SDI: SQ_BAL_RAM_ECC_INJECT: sq_bal_ram_1bit_ecc_error";
	case 640: return "SDI: SQ_BAH_RAM_ECC_INJECT: sq_bah_ram_2bit_ecc_error";
	case 641: return "SDI: SQ_BAH_RAM_ECC_INJECT: sq_bah_ram_1bit_ecc_error";
	case 672: return "SDI: RD_PTR_RAM_ECC_INJECT: rd_ptr_ram_2bit_ecc_error";
	case 673: return "SDI: RD_PTR_RAM_ECC_INJECT: rd_ptr_ram_1bit_ecc_error";
	case 704: return "SDI: SQ_DES_RAM_ECC_INJECT: sq_des_ram_2bit_ecc_error";
	case 705: return "SDI: SQ_DES_RAM_ECC_INJECT: sq_des_ram_1bit_ecc_error";
	case 736: return "SDI: PREFETCH_FIFO_RAM_ECC_INJECT: prefetch_fifo_ram_2bit_ecc_error";
	case 737: return "SDI: PREFETCH_FIFO_RAM_ECC_INJECT: prefetch_fifo_ram_1bit_ecc_error";
	case 768: return "SDI: BRESP_ERR_INT: bresp_p_operation_abnormal";
	case 800: return "SDI: ENGN_INT_STS: virtio_engine_local_read_pointer_data_ram_1bit_ecc_error";
	case 801: return "SDI: ENGN_INT_STS: virtio_engine_local_write_pointer_data_ram_1bit_ecc_error";
	case 802: return "SDI: ENGN_INT_STS: virtio_engine_local_avail_index_data_ram_1bit_ecc_error";
	case 803: return "SDI: ENGN_INT_STS: virtio_engine_local_read_pointer_data_ram_multi_bit_ecc_error";
	case 804: return "SDI: ENGN_INT_STS: virtio_engine_local_write_pointer_data_ram_multi_bit_ecc_error";
	case 805: return "SDI: ENGN_INT_STS: virtio_engine_local_avail_idx_data_ram_multi_bit_ecc_error";
	case 806: return "SDI: ENGN_INT_STS: error when virtio engine reads data from the avail ring";
	case 807: return "SDI: ENGN_INT_STS: error when virtio engine reads avail index data";
	case 808: return "SDI: ENGN_INT_STS: error when virtio engine sends write_operation to the axi bus";
	case 809: return "SDI: ENGN_INT_STS: virtion engine - incorrect cq completion status";
	case 810: return "SDI: ENGN_INT_STS: virtio engine - incorrect config parameter of the vq";
	case 811: return "SDI: ENGN_INT_STS: virtio engine - incorrect local ring buffer base addr config parameter";
	}
	return "unknown error";
}

static void hisi_hip08_log_error(struct ras_ns_dec_tab *dec_tab, int pidx,
				 char *tab_name, struct trace_seq *s, char *reg,
				 const struct hisi_hip08_hw_error *err,
				 uint32_t err_status)
{
	char buf[512];

	while (err->msg) {
		if (err->msk & err_status) {
			trace_seq_printf(s, "%s: %s\n", reg, err->msg);
			sprintf(buf, "%s: %s", reg, err->msg);
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   pidx, 0, buf);
			step_vendor_data_tab(dec_tab, tab_name);
		}
		err++;
	}
}

static void
hisi_hip08_log_error_status(struct ras_ns_dec_tab *dec_tab, int pidx,
			    char *tab_name, struct trace_seq *s, char *reg,
			    const struct hisi_hip08_hw_error_status *err_status,
			    uint32_t val)
{
	char buf[512];

	while (err_status->msg) {
		if (err_status->val == val) {
			trace_seq_printf(s, "%s: %s\n", reg, err_status->msg);
			sprintf(buf, "%s: %s", reg, err_status->msg);
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   pidx, 0, buf);
			step_vendor_data_tab(dec_tab, tab_name);
		}
		err_status++;
	}
}

/* error data decoding functions */
static void dec_type1_misc_err_data(struct ras_ns_dec_tab *dec_tab,
				    struct trace_seq *s,
				    const struct hisi_oem_type1_err_sec *err)
{
	trace_seq_printf(s, "Error Info:\n");
	switch (err->module_id) {
	case HISI_OEM_MODULE_ID_MN:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "MN_RINT", mn_hw_intr,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "MN_INTS", mn_hw_intr,
					     err->err_misc_1);
		break;

	case HISI_OEM_MODULE_ID_PLL:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			trace_seq_printf(s, "SC_PLL_INT_STATUS=0x%x\n",
					 err->err_misc_0);
		break;

	case HISI_OEM_MODULE_ID_SLLC:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SLLC_INT0_SRC",
					     sllc_hw_intr0,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SLLC_INT1_SRC",
					     sllc_hw_intr1,
					     err->err_misc_1);
		break;

	case HISI_OEM_MODULE_ID_AA:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "AA_INTRAW",
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

	case HISI_OEM_MODULE_ID_SIOE:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SIOE_INT_STS",
					     sio_hw_int,
					     err->err_misc_0);
		break;

	case HISI_OEM_MODULE_ID_POE:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "POE_ECC_1BIT_ERR_INFO_1",
					     poe_ecc_1bit_info_1,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "POE_ECC_1BIT_ERR_INFO_0",
					     poe_ecc_1bit_info_0,
					     err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "POE_ECC_2BIT_ERR_INFO_1",
					     poe_ecc_2bit_info_1,
					     err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "POE_ECC_2BIT_ERR_INFO_0",
					     poe_ecc_2bit_info_0,
					     err->err_misc_3);
		break;

	case HISI_OEM_MODULE_ID_DISP:
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
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "DISP_INTRAW0",
					     disp_intraw0,
					     err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "DISP_INTRAW1",
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

	case HISI_OEM_MODULE_ID_LPC:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "LPC_MEM_ACCESS_ST",
					     lpc_mem_access_st,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SC_MEM_ECC_ST0",
					     lpc_sc_mem_ecc_st0,
					     err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SC_MEM_ECC_ST1",
					     lpc_sc_mem_ecc_st1,
					     err->err_misc_2);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3)
			trace_seq_printf(s, "OP_STATUS=0x%x\n",
					 err->err_misc_3);
		break;

	case HISI_OEM_MODULE_ID_SAS:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SAS_RAS_INTR0",
					     sas_ras_intr0,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SAS_RAS_INTR1",
					     sas_ras_intr1,
					     err->err_misc_1);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SAS_RAS_INTR2",
					     sas_ras_intr2,
					     err->err_misc_2);
		break;

	case HISI_OEM_MODULE_ID_SATA:
		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SATA_RAS_IS",
					     sata_ras_is,
					     err->err_misc_0);

		if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1)
			hisi_hip08_log_error(dec_tab, hip08_oem_type1_field_err_info,
					     "hip08_oem_type1_event_tab",
					     s, "SATA_RAS_SERR",
					     sata_ras_serr,
					     err->err_misc_1);
		break;
	}
}

static void dec_type2_err_info(struct ras_ns_dec_tab *dec_tab,
			       struct trace_seq *s,
			       const struct hisi_oem_type2_err_sec *err)
{
	uint16_t ierr_status;
	uint16_t serr_status;

	serr_status = err->err_status_0 & 0xFF;
	ierr_status = (err->err_status_0 >> 8) & 0xFF;
	trace_seq_printf(s, "Error Info:\n");

	switch (err->module_id) {
	case HISI_OEM_MODULE_ID_SMMU:
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "SMMU_ERR_STATUS_0:SERR",
					    smmu_serr_status, serr_status);
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "SMMU_ERR_STATUS_0:IERR",
					    smmu_ierr_status, ierr_status);
		break;

	case HISI_OEM_MODULE_ID_HHA:
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "HHA_ERR_STATUSL:SERR",
					    hha_serr_status, serr_status);
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "HHA_ERR_STATUSL:IERR",
					    hha_ierr_status, ierr_status);
		break;

	case HISI_OEM_MODULE_ID_HLLC:
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "HLLC_ERR_STATUSL:SERR",
					    hllc_serr_status, serr_status);
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "HLLC_ERR_STATUSL:IERR",
					    hllc_ierr_status, ierr_status);

		hisi_hip08_log_error(dec_tab, hip08_oem_type2_field_err_info,
				     "hip08_oem_type2_event_tab",
				     s, "HLLC_ERR_MISC1L",
				     hllc_hw_err_misc1_l, err->err_misc1_0);
		hisi_hip08_log_error(dec_tab, hip08_oem_type2_field_err_info,
				     "hip08_oem_type2_event_tab",
				     s, "HLLC_ERR_MISC1H",
				     hllc_hw_err_misc1_h, err->err_misc1_1);
		break;

	case HISI_OEM_MODULE_ID_PA:
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "PA_ERR_STATUSL:SERR",
					    pa_serr_status, serr_status);
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "PA_ERR_STATUSL:IERR",
					    pa_ierr_status, ierr_status);

		hisi_hip08_log_error(dec_tab, hip08_oem_type2_field_err_info,
				     "hip08_oem_type2_event_tab",
				     s, "PA_ERR_MISC1L", pa_hw_err_misc1_l,
				     err->err_misc1_0);
		hisi_hip08_log_error(dec_tab, hip08_oem_type2_field_err_info,
				     "hip08_oem_type2_event_tab",
				     s, "PA_ERR_MISC1H", pa_hw_err_misc1_h,
				     err->err_misc1_1);
		break;

	case HISI_OEM_MODULE_ID_DDRC:
		hisi_hip08_log_error_status(dec_tab, hip08_oem_type2_field_err_info,
					    "hip08_oem_type2_event_tab",
					    s, "ARER_ERR_STATUS_L:IERR",
					    ddrc_ierr_status, ierr_status);
		break;
	}
}

/* error data decoding functions */
static int decode_hip08_oem_type1_error(struct ras_events *ras,
					struct ras_ns_dec_tab *dec_tab,
					struct trace_seq *s, const void *error)
{
	const struct hisi_oem_type1_err_sec *err = error;
	char buf[1024];
	char *p = buf;

	if (err->val_bits == 0) {
		trace_seq_printf(s, "%s: no valid error information\n",
				 __func__);
		return -1;
	}

#ifdef HAVE_SQLITE3
	if (!dec_tab->stmt_dec_record) {
		if (ras_mc_add_vendor_table(ras, &dec_tab->stmt_dec_record,
					    &hip08_oem_type1_event_tab)
			!= SQLITE_OK) {
			trace_seq_printf(s,
					"create sql hip08_oem_type1_event_tab fail\n");
			return -1;
		}
	}
#endif

	p += sprintf(p, "[ ");
	p += sprintf(p, "Table version=%d ", err->version);
	record_vendor_data(dec_tab, hisi_oem_data_type_int,
			   hip08_oem_type1_field_version, err->version, NULL);

	if (err->val_bits & HISI_OEM_VALID_SOC_ID) {
		p += sprintf(p, "SOC ID=%d ", err->soc_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_soc_id,
				   err->soc_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_SOCKET_ID) {
		p += sprintf(p, "socket ID=%d ", err->socket_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_socket_id,
				   err->socket_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_NIMBUS_ID) {
		p += sprintf(p, "nimbus ID=%d ", err->nimbus_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_nimbus_id,
				   err->nimbus_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_MODULE_ID) {
		p += sprintf(p, "module=%s-",
			     oem_type1_module_name(err->module_id));
		record_vendor_data(dec_tab, hisi_oem_data_type_text,
				   hip08_oem_type1_field_module_id,
				   0, oem_type1_module_name(err->module_id));
		if (err->val_bits & HISI_OEM_VALID_SUB_MODULE_ID) {
			p += sprintf(p, "%d ", err->sub_module_id);
			record_vendor_data(dec_tab, hisi_oem_data_type_int,
					   hip08_oem_type1_field_sub_module_id,
					   err->sub_module_id, NULL);
		}
	}

	if (err->val_bits & HISI_OEM_VALID_ERR_SEVERITY) {
		p += sprintf(p, "error severity=%s ",
			     err_severity(err->err_severity));
		record_vendor_data(dec_tab, hisi_oem_data_type_text,
				   hip08_oem_type1_field_err_sev,
				   0, err_severity(err->err_severity));
	}

	p += sprintf(p, "]");
	trace_seq_printf(s, "\nHISI HIP08: OEM Type-1 Error\n");
	trace_seq_printf(s, "%s\n", buf);

	trace_seq_printf(s, "Reg Dump:\n");
	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_0) {
		trace_seq_printf(s, "ERR_MISC0=0x%x\n", err->err_misc_0);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_err_misc_0,
				   err->err_misc_0, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_1) {
		trace_seq_printf(s, "ERR_MISC1=0x%x\n", err->err_misc_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_err_misc_1,
				   err->err_misc_1, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_2) {
		trace_seq_printf(s, "ERR_MISC2=0x%x\n", err->err_misc_2);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_err_misc_2,
				   err->err_misc_2, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_3) {
		trace_seq_printf(s, "ERR_MISC3=0x%x\n", err->err_misc_3);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_err_misc_3,
				   err->err_misc_3, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_MISC_4) {
		trace_seq_printf(s, "ERR_MISC4=0x%x\n", err->err_misc_4);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type1_field_err_misc_4,
				   err->err_misc_4, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE1_VALID_ERR_ADDR) {
		trace_seq_printf(s, "ERR_ADDR=0x%p\n", (void *)err->err_addr);
		record_vendor_data(dec_tab, hisi_oem_data_type_int64,
				   hip08_oem_type1_field_err_addr,
				   err->err_addr, NULL);
	}
	step_vendor_data_tab(dec_tab, "hip08_oem_type1_event_tab");

	dec_type1_misc_err_data(dec_tab, s, err);


	return 0;
}

static int decode_hip08_oem_type2_error(struct ras_events *ras,
					struct ras_ns_dec_tab *dec_tab,
					struct trace_seq *s, const void *error)
{
	const struct hisi_oem_type2_err_sec *err = error;
	char buf[1024];
	char *p = buf;

	if (err->val_bits == 0) {
		trace_seq_printf(s, "%s: no valid error information\n",
				 __func__);
		return -1;
	}

#ifdef HAVE_SQLITE3
	if (!dec_tab->stmt_dec_record) {
		if (ras_mc_add_vendor_table(ras, &dec_tab->stmt_dec_record,
			&hip08_oem_type2_event_tab) != SQLITE_OK) {
			trace_seq_printf(s,
				"create sql hip08_oem_type2_event_tab fail\n");
			return -1;
		}
	}
#endif
	p += sprintf(p, "[ ");
	p += sprintf(p, "Table version=%d ", err->version);
	record_vendor_data(dec_tab, hisi_oem_data_type_int,
			   hip08_oem_type2_field_version,
			   err->version, NULL);
	if (err->val_bits & HISI_OEM_VALID_SOC_ID) {
		p += sprintf(p, "SOC ID=%d ", err->soc_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_soc_id,
				   err->soc_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_SOCKET_ID) {
		p += sprintf(p, "socket ID=%d ", err->socket_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_socket_id,
				   err->socket_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_NIMBUS_ID) {
		p += sprintf(p, "nimbus ID=%d ", err->nimbus_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_nimbus_id,
				   err->nimbus_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_MODULE_ID) {
		p += sprintf(p, "module=%s ",
			     oem_type2_module_name(err->module_id));
		record_vendor_data(dec_tab, hisi_oem_data_type_text,
				   hip08_oem_type2_field_module_id,
				   0, oem_type2_module_name(err->module_id));
	}

	if (err->val_bits & HISI_OEM_VALID_SUB_MODULE_ID) {
		p =  oem_type2_sub_module_id(p, err->module_id,
					     err->sub_module_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_sub_module_id,
				   err->sub_module_id, NULL);
	}

	if (err->val_bits & HISI_OEM_VALID_ERR_SEVERITY) {
		p += sprintf(p, "error severity=%s ",
			     err_severity(err->err_severity));
		record_vendor_data(dec_tab, hisi_oem_data_type_text,
				   hip08_oem_type2_field_err_sev,
				   0, err_severity(err->err_severity));
	}

	p += sprintf(p, "]");
	trace_seq_printf(s, "\nHISI HIP08: OEM Type-2 Error\n");
	trace_seq_printf(s, "%s\n", buf);

	trace_seq_printf(s, "Reg Dump:\n");
	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_FR) {
		trace_seq_printf(s, "ERR_FR_0=0x%x\n", err->err_fr_0);
		trace_seq_printf(s, "ERR_FR_1=0x%x\n", err->err_fr_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_fr_0,
				   err->err_fr_0, NULL);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_fr_1,
				   err->err_fr_1, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_CTRL) {
		trace_seq_printf(s, "ERR_CTRL_0=0x%x\n", err->err_ctrl_0);
		trace_seq_printf(s, "ERR_CTRL_1=0x%x\n", err->err_ctrl_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_ctrl_0,
				   err->err_ctrl_0, NULL);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_ctrl_1,
				   err->err_ctrl_1, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_STATUS) {
		trace_seq_printf(s, "ERR_STATUS_0=0x%x\n", err->err_status_0);
		trace_seq_printf(s, "ERR_STATUS_1=0x%x\n", err->err_status_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_status_0,
				   err->err_status_0, NULL);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_status_1,
				   err->err_status_1, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_ADDR) {
		trace_seq_printf(s, "ERR_ADDR_0=0x%x\n", err->err_addr_0);
		trace_seq_printf(s, "ERR_ADDR_1=0x%x\n", err->err_addr_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_addr_0,
				   err->err_addr_0, NULL);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_addr_1,
				   err->err_addr_1, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_MISC_0) {
		trace_seq_printf(s, "ERR_MISC0_0=0x%x\n", err->err_misc0_0);
		trace_seq_printf(s, "ERR_MISC0_1=0x%x\n", err->err_misc0_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_misc0_0,
				   err->err_misc0_0, NULL);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_misc0_1,
				   err->err_misc0_1, NULL);
	}

	if (err->val_bits & HISI_OEM_TYPE2_VALID_ERR_MISC_1) {
		trace_seq_printf(s, "ERR_MISC1_0=0x%x\n", err->err_misc1_0);
		trace_seq_printf(s, "ERR_MISC1_1=0x%x\n", err->err_misc1_1);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_misc1_0,
				   err->err_misc1_0, NULL);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_oem_type2_field_err_misc1_1,
				   err->err_misc1_1, NULL);
	}
	step_vendor_data_tab(dec_tab, "hip08_oem_type2_event_tab");

	dec_type2_err_info(dec_tab, s, err);

	return 0;
}

static int decode_hip08_pcie_local_error(struct ras_events *ras,
					 struct ras_ns_dec_tab *dec_tab,
					 struct trace_seq *s, const void *error)
{
	const struct hisi_pcie_local_err_sec *err = error;
	char buf[1024];
	char *p = buf;
	uint32_t i;

	if (err->val_bits == 0) {
		trace_seq_printf(s, "%s: no valid error information\n",
				 __func__);
		return -1;
	}

#ifdef HAVE_SQLITE3
	if (!dec_tab->stmt_dec_record) {
		if (ras_mc_add_vendor_table(ras, &dec_tab->stmt_dec_record,
				&hip08_pcie_local_event_tab) != SQLITE_OK) {
			trace_seq_printf(s,
				"create sql hip08_pcie_local_event_tab fail\n");
			return -1;
		}
	}
#endif
	p += sprintf(p, "[ ");
	p += sprintf(p, "Table version=%d ", err->version);
	record_vendor_data(dec_tab, hisi_oem_data_type_int,
			   hip08_pcie_local_field_version,
			   err->version, NULL);
	if (err->val_bits & HISI_PCIE_LOCAL_VALID_SOC_ID) {
		p += sprintf(p, "SOC ID=%d ", err->soc_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_pcie_local_field_soc_id,
				   err->soc_id, NULL);
	}

	if (err->val_bits & HISI_PCIE_LOCAL_VALID_SOCKET_ID) {
		p += sprintf(p, "socket ID=%d ", err->socket_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_pcie_local_field_socket_id,
				   err->socket_id, NULL);
	}

	if (err->val_bits & HISI_PCIE_LOCAL_VALID_NIMBUS_ID) {
		p += sprintf(p, "nimbus ID=%d ", err->nimbus_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_pcie_local_field_nimbus_id,
				   err->nimbus_id, NULL);
	}

	if (err->val_bits & HISI_PCIE_LOCAL_VALID_SUB_MODULE_ID) {
		p += sprintf(p, "sub module=%s ",
			     pcie_local_sub_module_name(err->sub_module_id));
		record_vendor_data(dec_tab, hisi_oem_data_type_text,
				   hip08_pcie_local_field_sub_module_id,
				   0, pcie_local_sub_module_name(err->sub_module_id));
	}

	if (err->val_bits & HISI_PCIE_LOCAL_VALID_CORE_ID) {
		p += sprintf(p, "core ID=core%d ", err->core_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_pcie_local_field_core_id,
				   err->core_id, NULL);
	}

	if (err->val_bits & HISI_PCIE_LOCAL_VALID_PORT_ID) {
		p += sprintf(p, "port ID=port%d ", err->port_id);
		record_vendor_data(dec_tab, hisi_oem_data_type_int,
				   hip08_pcie_local_field_port_id,
				   err->port_id, NULL);
	}

	if (err->val_bits & HISI_PCIE_LOCAL_VALID_ERR_SEVERITY) {
		p += sprintf(p, "error severity=%s ",
			     err_severity(err->err_severity));
		record_vendor_data(dec_tab, hisi_oem_data_type_text,
				   hip08_pcie_local_field_err_sev,
				   0, err_severity(err->err_severity));
	}
	p += sprintf(p, "]");

	trace_seq_printf(s, "\nHISI HIP08: PCIe local error\n");
	trace_seq_printf(s, "%s\n", buf);

	trace_seq_printf(s, "Reg Dump:\n");
	for (i = 0; i < 33; i++) {
		if (err->val_bits & BIT(HISI_PCIE_LOCAL_VALID_ERR_MISC + i)) {
			trace_seq_printf(s, "ERR_MISC_%d=0x%x\n", i,
					 err->err_misc[i]);
			record_vendor_data(dec_tab, hisi_oem_data_type_int,
					   (hip08_pcie_local_field_err_misc + i),
					   err->err_misc[i], NULL);
		}
	}

	trace_seq_printf(s, "Error Info:\n");
	switch (err->sub_module_id) {
	case HISI_PCIE_SUB_MODULE_ID_AP:
		if (err->val_bits & HISI_PCIE_LOCAL_VALID_ERR_TYPE) {
			trace_seq_printf(s, "error type=%s\n",
					 pcie_ap_err_type(err->err_type));
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   hip08_pcie_local_field_err_type,
					   0, pcie_ap_err_type(err->err_type));
		}

		for (i = 0; hisi_pcie_ap_err_misc[i].msg; i++) {
			if (err->val_bits & hisi_pcie_ap_err_misc[i].msk)
				trace_seq_printf(s, "%s=0x%x\n",
						 hisi_pcie_ap_err_misc[i].msg,
						 err->err_misc[i]);
		}
		break;

	case HISI_PCIE_SUB_MODULE_ID_TL:
		if (err->val_bits & HISI_PCIE_LOCAL_VALID_ERR_TYPE) {
			trace_seq_printf(s, "error type=%s\n",
					 pcie_tl_err_type(err->err_type));
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   hip08_pcie_local_field_err_type,
					   0, pcie_tl_err_type(err->err_type));
		}

		for (i = 0; hisi_pcie_tl_err_misc[i].msg; i++) {
			if (err->val_bits & hisi_pcie_tl_err_misc[i].msk)
				trace_seq_printf(s, "%s=0x%x\n",
						 hisi_pcie_tl_err_misc[i].msg,
						 err->err_misc[i]);
		}
		break;

	case HISI_PCIE_SUB_MODULE_ID_MAC:
		if (err->val_bits & HISI_PCIE_LOCAL_VALID_ERR_TYPE) {
			trace_seq_printf(s, "error type=%s\n",
					 pcie_mac_err_type(err->err_type));
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   hip08_pcie_local_field_err_type,
					   0, pcie_mac_err_type(err->err_type));
		}

		for (i = 0; hisi_pcie_mac_err_misc[i].msg; i++) {
			if (err->val_bits & hisi_pcie_mac_err_misc[i].msk)
				trace_seq_printf(s, "%s=0x%x\n",
						 hisi_pcie_mac_err_misc[i].msg,
						 err->err_misc[i]);
		}
		break;

	case HISI_PCIE_SUB_MODULE_ID_DL:
		if (err->val_bits & HISI_PCIE_LOCAL_VALID_ERR_TYPE) {
			trace_seq_printf(s, "error type=%s\n",
					 pcie_dl_err_type(err->err_type));
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   hip08_pcie_local_field_err_type,
					   0, pcie_dl_err_type(err->err_type));
		}

		for (i = 0; hisi_pcie_dl_err_misc[i].msg; i++) {
			if (err->val_bits & hisi_pcie_dl_err_misc[i].msk)
				trace_seq_printf(s, "%s=0x%x\n",
						 hisi_pcie_dl_err_misc[i].msg,
						 err->err_misc[i]);
		}
		break;

	case HISI_PCIE_SUB_MODULE_ID_SDI:
		if (err->val_bits & HISI_PCIE_LOCAL_VALID_ERR_TYPE) {
			trace_seq_printf(s, "error type=%s\n",
					 pcie_sdi_err_type(err->err_type));
			record_vendor_data(dec_tab, hisi_oem_data_type_text,
					   hip08_pcie_local_field_err_type,
					   0, pcie_sdi_err_type(err->err_type));
		}

		for (i = 0; hisi_pcie_sdi_err_misc[i].msg; i++) {
			if (err->val_bits & hisi_pcie_sdi_err_misc[i].msk)
				trace_seq_printf(s, "%s=0x%x\n",
						 hisi_pcie_sdi_err_misc[i].msg,
						 err->err_misc[i]);
		}
		break;
	}

	step_vendor_data_tab(dec_tab, "hip08_pcie_local_event_tab");

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
	{
		.sec_type = "b2889fc9e7d74f9da867af42e98be772",
		.decode = decode_hip08_pcie_local_error,
	},
};

__attribute__((constructor))
static void hip08_init(void)
{
	hip08_ns_oem_tab[0].len = ARRAY_SIZE(hip08_ns_oem_tab);
	register_ns_dec_tab(hip08_ns_oem_tab);
}
