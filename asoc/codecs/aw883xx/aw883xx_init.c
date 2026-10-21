// SPDX-License-Identifier: GPL-2.0
/* aw883xx_init.c   aw883xx codec driver
 *
 * Copyright (c) 2020 AWINIC Technology CO., LTD
 *
 *  Author: Bruce zhao <zhaolei@awinic.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*#define DEBUG*/
#include <linux/module.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h>
#include <sound/control.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

#include "aw883xx.h"
#include "aw883xx_device.h"
#include "aw883xx_bin_parse.h"
#include "aw883xx_log.h"


static int aw883xx_dev_reg_read(struct aw_device *aw_dev,
			uint8_t reg_addr, uint16_t *reg_data)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_reg_read(aw883xx, reg_addr, reg_data);
}

static int aw883xx_dev_reg_write(struct aw_device *aw_dev,
			uint8_t reg_addr, uint16_t reg_data)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_reg_write(aw883xx, reg_addr, reg_data);
}

static int aw883xx_dev_reg_write_bits(struct aw_device *aw_dev,
			uint8_t reg_addr, uint16_t mask, uint16_t reg_data)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_reg_write_bits(aw883xx, reg_addr, mask, reg_data);
}

static int aw883xx_dev_dsp_write_bits(struct aw_device *aw_dev,
	uint16_t dsp_addr, uint32_t dsp_mask, uint32_t dsp_data, uint8_t data_type)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_write_bits(aw883xx, dsp_addr, dsp_mask, dsp_data, data_type);
}

static int aw883xx_dev_dsp_write(struct aw_device *aw_dev,
			uint16_t dsp_addr, uint32_t dsp_data, uint8_t data_type)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_write(aw883xx, dsp_addr, dsp_data, data_type);
}

static int aw883xx_dev_dsp_writes(struct aw_device *aw_dev,
			uint8_t *data, uint32_t len, uint16_t base)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_writes(aw883xx, data, len, base);
}

static int aw883xx_dev_dsp_read(struct aw_device *aw_dev,
			uint16_t dsp_addr, uint32_t *dsp_data, uint8_t data_type)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_read(aw883xx, dsp_addr, dsp_data, data_type);
}

static unsigned int aw883xx_dev_vol_reg_val_to_db(unsigned int value)
{
	return value;
}

static uint16_t aw883xx_dev_vol_db_val_to_reg(uint16_t value)
{
	return value;
}

static int aw883xx_dev_mem_init(struct aw_device *aw_pa)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_pa->private_data;

	aw_pa->prof_info.prof_desc = NULL;
	aw_pa->prof_info.count = 0;
	aw_pa->prof_info.prof_type = AW_DEV_NONE_TYPE_ID;
	aw_pa->channel = 0;
	aw_pa->i2c_lock = &aw883xx->i2c_lock;
	aw_pa->i2c = aw883xx->i2c;
	aw_pa->fw_status = AW_DEV_FW_FAILED;

	aw_pa->chip_id = aw883xx->chip_id;
	aw_pa->dev = aw883xx->dev;
	aw_pa->ops.aw_get_version = aw883xx_get_version;
	aw_pa->ops.aw_reg_write = aw883xx_dev_reg_write;
	aw_pa->ops.aw_reg_write_bits = aw883xx_dev_reg_write_bits;
	aw_pa->ops.aw_reg_read = aw883xx_dev_reg_read;
	aw_pa->ops.aw_dsp_write_bits = aw883xx_dev_dsp_write_bits;
	aw_pa->ops.aw_dsp_read = aw883xx_dev_dsp_read;
	aw_pa->ops.aw_dsp_write = aw883xx_dev_dsp_write;
	aw_pa->ops.aw_dsp_writes = aw883xx_dev_dsp_writes;
	aw_pa->ops.aw_get_dev_num = aw883xx_get_dev_num;

	return 0;
}

static int aw883xx_dev_basic_init(struct aw_device *aw_pa)
{
	aw_pa->fade_step = AW883XX_VOL_STEP_DB;

	aw_pa->ops.aw_reg_val_to_db = aw883xx_dev_vol_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw883xx_dev_vol_db_val_to_reg;

	aw_pa->soft_rst.reg = AW883XX_ID_REG;
	aw_pa->soft_rst.reg_value = AW883XX_SW_RESET_PASSWORD;

	aw_pa->dsp_mem_desc.dsp_madd_reg = AW883XX_DSPMADD_REG;
	aw_pa->dsp_mem_desc.dsp_mdat_reg = AW883XX_DSPMDAT_REG;
	aw_pa->dsp_mem_desc.dsp_rom_check_reg = AW883XX_DSP_ROM_CHECK_DSP;
	aw_pa->dsp_mem_desc.dsp_rom_check_data = AW883XX_DSP_ROM_CHECK_DATA;
	aw_pa->dsp_mem_desc.dsp_rom_data_type = AW_DSP_32_DATA;

	aw_pa->pwd_desc.reg = AW883XX_SYSCTRL_REG;
	aw_pa->pwd_desc.mask = AW883XX_PWDN_MASK;
	aw_pa->pwd_desc.enable = AW883XX_PWDN_ENABLE_VALUE;
	aw_pa->pwd_desc.disable = AW883XX_PWDN_DISABLE_VALUE;

	aw_pa->amppd_desc.reg = AW883XX_SYSCTRL_REG;
	aw_pa->amppd_desc.mask = AW883XX_AMPPD_MASK;
	aw_pa->amppd_desc.enable = AW883XX_AMPPD_ENABLE_VALUE;
	aw_pa->amppd_desc.disable = AW883XX_AMPPD_DISABLE_VALUE;

	aw_pa->mute_desc.reg = AW883XX_SYSCTRL_REG;
	aw_pa->mute_desc.mask = AW883XX_HMUTE_MASK;
	aw_pa->mute_desc.enable = AW883XX_HMUTE_ENABLE_VALUE;
	aw_pa->mute_desc.disable = AW883XX_HMUTE_DISABLE_VALUE;

	aw_pa->profctrl_desc.reg = AW883XX_SYSCTRL_REG;
	aw_pa->profctrl_desc.mask = AW883XX_EN_RCV_MASK;
	aw_pa->profctrl_desc.rcv_mode_val = AW883XX_EN_RCV_ENABLE_VALUE;

	aw_pa->dsp_en_desc.reg = AW883XX_SYSCTRL_REG;
	aw_pa->dsp_en_desc.mask = AW883XX_DSPBY_MASK;
	aw_pa->dsp_en_desc.enable = AW883XX_DSPBY_DISABLE_VALUE;
	aw_pa->dsp_en_desc.disable = AW883XX_DSPBY_ENABLE_VALUE;

	aw_pa->tx_en_desc.reg = AW883XX_I2SCTRL3_REG;
	aw_pa->tx_en_desc.mask = AW883XX_I2STXEN_MASK;
	aw_pa->tx_en_desc.enable = AW883XX_I2STXEN_ENABLE_VALUE;
	aw_pa->tx_en_desc.disable = AW883XX_I2STXEN_DISABLE_VALUE;

	aw_pa->int_desc.mask_reg = AW883XX_SYSINTM_REG;
	aw_pa->int_desc.mask_default = AW883XX_SYSINTM_DEFAULT;
	aw_pa->int_desc.int_mask = AW883XX_SYSINTM_DEFAULT;
	aw_pa->int_desc.st_reg = AW883XX_SYSINT_REG;
	aw_pa->int_desc.intst_mask = AW883XX_SYSINT_CHECK_MASK;

	aw_pa->chansel_desc.rxchan_reg = AW883XX_I2SCTRL_REG;
	aw_pa->chansel_desc.rxchan_mask = AW883XX_CHSEL_MASK;
	aw_pa->chansel_desc.rx_left = AW883XX_CHSEL_LEFT_VALUE;
	aw_pa->chansel_desc.rx_right = AW883XX_CHSEL_RIGHT_VALUE;

	aw_pa->sysst_desc.reg = AW883XX_SYSST_REG;
	aw_pa->sysst_desc.st_check = AW883XX_SYSST_CHECK_VALUE;
	aw_pa->sysst_desc.st_mask = AW883XX_SYSST_CHECK_MASK;
	aw_pa->sysst_desc.pll_check = AW883XX_PLL_CHECK_VALUE;
	aw_pa->sysst_desc.dsp_check = AW883XX_DSPS_DISABLE_VALUE;
	aw_pa->sysst_desc.dsp_mask = AW883XX_DSPS_MASK;

	aw_pa->memclk_pll_desc.reg = AW883XX_DBGCTRL_REG;
	aw_pa->memclk_pll_desc.mask = AW883XX_MEM_CLKSEL_MASK;
	aw_pa->memclk_pll_desc.enable = AW883XX_MEM_CLKSEL_PLL_VALUE;
	aw_pa->memclk_pll_desc.disable = AW883XX_MEM_CLKSEL_OSC_VALUE;

	aw_pa->dither_desc.reg = AW883XX_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW883XX_DITHER_EN_MASK;
	aw_pa->dither_desc.enable = AW883XX_DITHER_EN_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW883XX_DITHER_EN_DISABLE_VALUE;

	aw_pa->dsp_vol_desc.reg = AW883XX_DSPCFG_REG;
	aw_pa->dsp_vol_desc.mask = AW883XX_DSP_VOL_MASK;
	aw_pa->dsp_vol_desc.mute_st = AW883XX_DSP_VOL_MUTE_VALUE;
	aw_pa->dsp_vol_desc.noise_st = AW883XX_DSP_VOL_NOISE_VALUE;

	aw_pa->watch_dog_desc.reg = AW883XX_WDT_REG;
	aw_pa->watch_dog_desc.mask = AW883XX_WDT_CNT_MASK;

	aw_pa->cco_mux_desc.reg = AW883XX_PLLCTRL2_REG;
	aw_pa->cco_mux_desc.mask = AW883XX_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divider = AW883XX_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass = AW883XX_CCO_MUX_BYPASS_VALUE;

	aw_pa->volume_desc.reg = AW883XX_SYSCTRL2_REG;
	aw_pa->volume_desc.mask = AW883XX_VOL_MASK;
	aw_pa->volume_desc.shift = AW883XX_VOL_START;
	aw_pa->volume_desc.mute_volume = AW883XX_VOL_MUTE_VALUE;
	aw_pa->volume_desc.max_volume = AW883XX_VOL_DEFAULT_VALUE;
	aw_pa->volume_desc.ctl_volume = AW883XX_VOL_DEFAULT_VALUE;

	aw_pa->noise_gate_en.reg = AW883XX_PWMCTRL3_REG;
	aw_pa->noise_gate_en.noise_gate_mask = AW883XX_NOISE_GATE_EN_MASK;
	aw_pa->noise_gate_en.check_mask = AW883XX_SWS_MASK;

	aw_pa->efuse_check_desc.reg = AW883XX_DBGCTRL_REG;
	aw_pa->efuse_check_desc.mask = AW883XX_EF_DBMD_MASK;
	aw_pa->efuse_check_desc.or_val = AW883XX_EF_DBMD_OR_VALUE;
	aw_pa->efuse_check_desc.and_val = AW883XX_EF_DBMD_AND_VALUE;

	aw_pa->vsense_desc.vcalb_vsense_reg = AW883XX_VSNCTRL_REG;
	aw_pa->vsense_desc.vcalk_vdsel_mask = AW883XX_VDSEL_MASK;

	aw_pa->eps_en.reg = AW883XX_BSTCTRL10_REG;
	aw_pa->eps_en.eps_mask = AW883XX_EN_HVBAT_MASK;
	aw_pa->eps_en.check_mask = AW883XX_BSTS_MASK;

	aw_pa->dsp_ng_desc.reg = AW883XX_DBGCTRL_REG;
	aw_pa->dsp_ng_desc.mask = AW883XX_DSP_NG_EN_MASK;
	aw_pa->dsp_ng_desc.enable = AW883XX_DSP_NG_EN_ENABLE;
	aw_pa->dsp_ng_desc.disable = AW883XX_DSP_NG_EN_DISABLE;

	aw_pa->vcalb_desc.vcalb_adj_shift = AW883XX_VCALB_ADJ_SHIFT;
	aw_pa->vcalb_desc.cabl_base_value = AW883XX_CALK_BASE_VALUE;
	aw_pa->vcalb_desc.vcalb_accuracy = AW883XX_VCALB_ACCURACY;
	aw_pa->vcalb_desc.vcalb_reg = AW883XX_DSPVCALB_REG;
	aw_pa->vcalb_desc.icalk_desc.high_reg = AW883XX_EFRH1_REG;
	aw_pa->vcalb_desc.icalk_desc.high_reg_mask = AW883XX_EF_ISN_GESLP_H_MASK;
	aw_pa->vcalb_desc.icalk_desc.high_reg_shift = AW883XX_EF_ISN_GESLP_H_START;
	aw_pa->vcalb_desc.icalk_desc.low_reg = AW883XX_EFRL1_REG;
	aw_pa->vcalb_desc.icalk_desc.low_reg_mask = AW883XX_EF_ISN_GESLP_L_MASK;
	aw_pa->vcalb_desc.icalk_desc.low_reg_shift = AW883XX_EF_ISN_GESLP_L_START;
	aw_pa->vcalb_desc.icalk_desc.low_offset_mask = 0;
	aw_pa->vcalb_desc.icalk_desc.sign_mask = AW883XX_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_desc.neg_mask = AW883XX_EF_ISN_GESLP_SIGN_NEG;
	aw_pa->vcalb_desc.vcalk_desc.high_reg = AW883XX_EFRH2_REG;
	aw_pa->vcalb_desc.vcalk_desc.high_reg_mask = AW883XX_EF_VSN_GESLP_H_MASK;
	aw_pa->vcalb_desc.vcalk_desc.high_reg_shift = AW883XX_EF_VSN_GESLP_H_START;
	aw_pa->vcalb_desc.vcalk_desc.low_reg = AW883XX_EFRL2_REG;
	aw_pa->vcalb_desc.vcalk_desc.low_reg_mask = AW883XX_EF_VSN_GESLP_L_MASK;
	aw_pa->vcalb_desc.vcalk_desc.low_reg_shift = AW883XX_EF_VSN_GESLP_L_START;
	aw_pa->vcalb_desc.vcalk_desc.low_offset_mask = 0;
	aw_pa->vcalb_desc.vcalk_desc.sign_mask = AW883XX_EF_VSN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_desc.neg_mask = AW883XX_EF_VSN_GESLP_SIGN_NEG;
	aw_pa->vcalb_desc.vcalk_dac_desc.high_reg = AW883XX_EFRH3_REG;
	aw_pa->vcalb_desc.vcalk_dac_desc.high_reg_mask = AW883XX_INTERNAL_VSN_TRIM_H_MASK;
	aw_pa->vcalb_desc.vcalk_dac_desc.high_reg_shift = AW883XX_INTERNAL_VSN_TRIM_H_START;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_reg = AW883XX_EFRL3_REG;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_reg_mask = AW883XX_INTERNAL_VSN_TRIM_L_MASK;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_reg_shift = AW883XX_INTERNAL_VSN_TRIM_L_START;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_offset_mask = 0;
	aw_pa->vcalb_desc.vcalk_dac_desc.sign_mask = AW883XX_INTERNAL_VSN_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_dac_desc.neg_mask = AW883XX_INTERNAL_VSN_SIGN_NEG;

	aw_pa->cali_desc.spkr_temp_desc.reg = AW883XX_ASR2_REG;
	aw_pa->cali_desc.iv_desc.reg = AW883XX_ASR1_REG;
	aw_pa->cali_desc.iv_desc.reabs_mask = AW883XX_REABS_MASK;
	aw_pa->cali_desc.re_desc.location = AW_CALI_RE_STORE_REG;
	aw_pa->cali_desc.re_desc.reg.phy_reg.high_reg = AW883XX_ACR1_REG;
	aw_pa->cali_desc.re_desc.reg.phy_reg.low_reg = AW883XX_ACR2_REG;
	aw_pa->cali_desc.re_desc.shift = AW883XX_DSP_RE_SHIFT;

	aw_pa->re_range.re_min_default = AW_RE_MIN;
	aw_pa->re_range.re_max_default = AW_RE_MAX;

	aw_pa->monitor_desc.dsp_monitor_delay = AW_DSP_MONITOR_DELAY;
	aw_pa->monitor_desc.voltage_desc.reg = AW883XX_VBAT_REG;
	aw_pa->monitor_desc.voltage_desc.vbat_range = AW883XX_VBAT_RANGE;
	aw_pa->monitor_desc.voltage_desc.int_bit = AW883XX_INT_10BIT;
	aw_pa->monitor_desc.temp_desc.reg = AW883XX_TEMP_REG;
	aw_pa->monitor_desc.temp_desc.sign_mask = AW883XX_TEMP_SIGN_MASK;
	aw_pa->monitor_desc.temp_desc.neg_mask = AW883XX_TEMP_NEG_MASK;
	aw_pa->monitor_desc.ipeak_desc.reg = AW883XX_BSTCTRL2_REG;
	aw_pa->monitor_desc.ipeak_desc.mask = AW883XX_BST_IPEAK_MASK;
	aw_pa->monitor_desc.hw_monitor_desc.reg = AW883XX_SYSCTRL2_REG;
	aw_pa->monitor_desc.hw_monitor_desc.bop_mask = AW883XX_BOP_EN_MASK;
	aw_pa->monitor_desc.hw_monitor_desc.vol_mask = AW883XX_BOP_VOL_EN_MASK;
	aw_pa->monitor_desc.hw_monitor_desc.ipeak_mask = AW883XX_BOP_IPEAK_EN_MASK;

	aw_pa->crc_type = AW_HW_CRC_CHECK;
	aw_pa->crc_check_desc.ram_clk_reg = AW883XX_I2SCFG_REG;
	aw_pa->crc_check_desc.ram_clk_mask = AW883XX_RAM_CG_BYP_MASK;
	aw_pa->crc_check_desc.ram_clk_on = AW883XX_RAM_CG_BYP_ENABLE_VALUE;
	aw_pa->crc_check_desc.ram_clk_off = AW883XX_RAM_CG_BYP_DISABLE_VALUE;
	aw_pa->crc_check_desc.crc_ctrl_reg = AW883XX_CRCCTRL_REG;
	aw_pa->crc_check_desc.crc_end_addr_mask = AW883XX_CRC_END_ADDR_MASK;
	aw_pa->crc_check_desc.crc_cfg_check_en_mask = AW883XX_CRC_CFG_EN_MASK;
	aw_pa->crc_check_desc.crc_cfgcheck_disable = AW883XX_CRC_CFG_EN_DISABLE_VALUE;
	aw_pa->crc_check_desc.crc_cfgcheck_enable = AW883XX_CRC_CFG_EN_ENABLE_VALUE;
	aw_pa->crc_check_desc.crc_fw_check_en_mask = AW883XX_CRC_CODE_EN_MASK;
	aw_pa->crc_check_desc.crc_fwcheck_disable = AW883XX_CRC_CODE_EN_DISABLE_VALUE;
	aw_pa->crc_check_desc.crc_fwcheck_enable = AW883XX_CRC_CODE_EN_ENABLE_VALUE;
	aw_pa->crc_check_desc.crc_check_reg = AW883XX_HAGCST_REG;
	aw_pa->crc_check_desc.crc_check_mask = AW883XX_CRC_CHECK_MASK;
	aw_pa->crc_check_desc.crc_check_pass = AW883XX_CRC_CHECK_VALUE;
	aw_pa->crc_check_desc.crc_check_bits_shift = AW883XX_CRC_CFG_FAIL_START;

	aw_pa->dsp_crc_desc.ctl_reg = AW_REG_NONE;

	return 0;
}

static int aw883xx_dev_common_init(struct aw_device *aw_pa, const struct aw_dev_property *property)
{
	aw_pa->reg_max_addr = property->max_addr;

	aw_pa->fw_ver_desc.version_reg = property->dsp_fw_addr;
	aw_pa->fw_ver_desc.data_type = AW_DSP_16_DATA;

	aw_pa->vcalb_desc.vscal_factor = property->vscal_factor;
	aw_pa->vcalb_desc.iscal_factor = property->iscal_factor;
	aw_pa->vcalb_desc.vscal_dac_factor = property->vscal_dac_factor;
	aw_pa->vcalb_desc.iscal_dac_factor = property->iscal_dac_factor;

	return 0;
}

static unsigned int aw_pid_2049_reg_val_to_db(unsigned int value)
{
	return (((value >> AW_PID_2049_VOL_6DB_START) * AW_PID_2049_VOLUME_STEP_DB) +
			((value & 0x3f) % AW_PID_2049_VOLUME_STEP_DB));
}

static uint16_t aw_pid_2049_db_val_to_reg(uint16_t value)
{
	return (((value / AW_PID_2049_VOLUME_STEP_DB) << AW_PID_2049_VOL_6DB_START) +
			(value % AW_PID_2049_VOLUME_STEP_DB));
}

static uint64_t aw_pid_2049_dsp_crc32_reflect(uint64_t ref, uint8_t ch)
{
	int i;
	uint64_t value = 0;

	for (i = 1; i < (ch + 1); i++) {
		if (ref & 1)
			value |= 1 << (ch - i);

		ref >>= 1;
	}

	return value;
}

static uint32_t aw_pid_2049_calc_dsp_cfg_crc32(uint8_t *buf, uint32_t len)
{
	uint8_t i;
	uint32_t crc = 0xffffffff;

	while (len--) {
		for (i = 1; i != 0; i <<= 1) {
			if ((crc & 0x80000000) != 0) {
				crc <<= 1;
				crc ^= 0x1EDC6F41;
			} else {
				crc <<= 1;
			}

			if ((*buf & i) != 0)
				crc ^= 0x1EDC6F41;
		}
		buf++;
	}

	return (aw_pid_2049_dsp_crc32_reflect(crc, 32)^0xffffffff);
}

static int aw_pid_2049_copy_to_crc_dsp_cfg(struct aw_device *aw_dev,
			uint8_t *data, uint32_t size)
{
	struct aw_sec_data_desc *crc_dsp_cfg = &aw_dev->crc_dsp_cfg;
	int ret = -1;

	if (crc_dsp_cfg->data == NULL) {
		crc_dsp_cfg->data = devm_kzalloc(aw_dev->dev, size, GFP_KERNEL);
		if (!crc_dsp_cfg->data)
			return -ENOMEM;

		crc_dsp_cfg->len = size;
	} else {
		if (crc_dsp_cfg->len != size) {
			devm_kfree(aw_dev->dev, crc_dsp_cfg->data);
			crc_dsp_cfg->data = NULL;
			crc_dsp_cfg->data = devm_kzalloc(aw_dev->dev, size, GFP_KERNEL);
			if (!crc_dsp_cfg->data) {
				aw_dev_err(aw_dev->dev, "error allocating memory");
				return -ENOMEM;
			}
			crc_dsp_cfg->len = size;
		} else {
			aw_dev_info(aw_dev->dev, "crc memory exist");
		}
	}

	memcpy(crc_dsp_cfg->data, data, size);
	ret = aw883xx_dev_dsp_data_order(aw_dev, crc_dsp_cfg->data, size);

	return ret;
}

static int aw_pid_2049_set_dsp_crc32(struct aw_device *aw_dev)
{
	uint32_t crc_value = 0;
	uint32_t crc_data_len = 0;
	int ret = -1;
	struct aw_sec_data_desc *crc_dsp_cfg = NULL;
	struct aw_prof_desc *set_prof_desc = NULL;
	struct aw_dsp_crc_desc *desc = &aw_dev->dsp_crc_desc;

	ret = aw883xx_dev_get_prof_data(aw_dev, aw_dev->cur_prof, &set_prof_desc);
	if (ret < 0)
		return ret;

	aw_dev->dsp_mem_desc.dsp_cfg_base_addr = set_prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].addr;
	aw_dev->dsp_mem_desc.dsp_fw_base_addr = set_prof_desc->sec_desc[AW_DATA_TYPE_DSP_FW].addr;

	ret = aw_pid_2049_copy_to_crc_dsp_cfg(aw_dev, set_prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].data,
		set_prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].len);
	if (ret < 0)
		return ret;

	crc_dsp_cfg = &aw_dev->crc_dsp_cfg;

	/*get crc data len*/
	crc_data_len = (uint32_t)((desc->dsp_reg - aw_dev->dsp_mem_desc.dsp_cfg_base_addr) * 2);
	if (crc_data_len > crc_dsp_cfg->len) {
		aw_dev_err(aw_dev->dev, "crc data len :%d > cfg_data len:%d",
			crc_data_len, crc_dsp_cfg->len);
		return -EINVAL;
	}

	if (crc_data_len % 4 != 0) {
		aw_dev_err(aw_dev->dev, "The crc data len :%d unsupport", crc_data_len);
		return -EINVAL;
	}

	crc_value = aw_pid_2049_calc_dsp_cfg_crc32(crc_dsp_cfg->data, crc_data_len);

	aw_dev_info(aw_dev->dev, "crc_value:0x%x", crc_value);
	ret = aw_dev->ops.aw_dsp_write(aw_dev, desc->dsp_reg, crc_value,
						desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "set dsp crc value failed");
		return ret;
	}

	return 0;
}

static int aw_pid_2049_dsp_crc_check_enable(struct aw_device *aw_dev, bool flag)
{
	struct aw_dsp_crc_desc *dsp_crc_desc = &aw_dev->dsp_crc_desc;
	int ret;

	aw_dev_info(aw_dev->dev, "enter,flag:%d", flag);
	if (flag) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, dsp_crc_desc->ctl_reg,
				dsp_crc_desc->ctl_mask, dsp_crc_desc->ctl_enable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "enable dsp crc failed");
			return ret;
		}
	} else {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, dsp_crc_desc->ctl_reg,
				dsp_crc_desc->ctl_mask, dsp_crc_desc->ctl_disable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "close dsp crc failed");
			return ret;
		}
	}

	return 0;
}


static int aw_pid_2049_dsp_st_check(struct aw_device *aw_dev)
{
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;
	struct aw_dsp_crc_desc *crc_desc = &aw_dev->dsp_crc_desc;
	int ret = -1;
	uint16_t reg_val = 0;
	uint32_t crc_val = 0;
	int i;

	for (i = 0; i < AW_DSP_ST_CHECK_MAX; i++) {
		ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read reg0x%x failed", desc->reg);
			continue;
		}

		if ((reg_val & (~desc->dsp_mask)) != desc->dsp_check) {
			aw_dev_err(aw_dev->dev, "check dsp st fail,reg_val:0x%04x", reg_val);
			ret = -EINVAL;
			aw_dev->ops.aw_dsp_read(aw_dev, crc_desc->check_reg, &crc_val,
				crc_desc->check_reg_data_type);
			aw_dev_err(aw_dev->dev, "read crc value:0x%04x", crc_val);
			usleep_range(AW_1000_US, AW_1000_US + 100);
			continue;
		} else {
			aw_dev_info(aw_dev->dev, "dsp st check ok, reg_val:0x%04x", reg_val);
			return 0;
		}
	}

	return ret;
}

static int aw_pid_2049_dsp_crc32_check(struct aw_device *aw_dev)
{
	int ret;
	struct aw_dev_switch *desc = &aw_dev->dsp_en_desc;

	if (aw_dev->fw_ver < AW_PID_2049_FIRMWARE_VERSION) {
		ret = aw_pid_2049_set_dsp_crc32(aw_dev);
		if (ret < 0) {
			aw_dev_info(aw_dev->dev, "set dsp crc32 failed");
			return ret;
		}
	}

	aw_pid_2049_dsp_crc_check_enable(aw_dev, true);

	/*dsp enable*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->enable);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "enable dsp failed");
		return ret;
	}

	usleep_range(AW_4000_US, AW_4000_US + 100);

	ret = aw_pid_2049_dsp_st_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "check crc32 fail");
		return ret;
	}

	aw_pid_2049_dsp_crc_check_enable(aw_dev, false);

	return 0;
}

static int aw_pid_2049_get_convert_r0(struct aw_device *aw_dev,
					uint32_t *re)
{
	int ret;
	struct aw_r0_desc *re_desc = &aw_dev->cali_desc.r0_desc;
	struct aw_ra_desc *ra_desc = &aw_dev->cali_desc.ra_desc;
	uint32_t dsp_re = 0;
	uint32_t show_re = 0;
	uint32_t re_cacl = 0;
	uint32_t ra = 0;
	uint32_t t0 = 0;
	int32_t te = 0;
	int32_t te_cacl = 0;
	uint32_t coil_alpha = 0;
	uint16_t pst_rpt = 0;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, re_desc->dsp_reg, &dsp_re, re_desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read re error");
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_read(aw_dev, ra_desc->dsp_reg, &ra, ra_desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read ra error");
		return ret;
	}

	re_cacl = dsp_re - ra;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, AW_PID_2049_DSP_CFG_ADPZ_T0, &t0, AW_DSP_16_DATA);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read t0 error");
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_read(aw_dev, AW_PID_2049_DSP_CFG_ADPZ_COILALPHA, &coil_alpha, AW_DSP_16_DATA);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read coil_alpha error");
		return ret;
	}

	ret = aw_dev->ops.aw_reg_read(aw_dev, aw_dev->cali_desc.spkr_temp_desc.reg, &pst_rpt);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read pst_rpt error");
		return ret;
	}

	te = (int32_t)((uint32_t)pst_rpt - t0);

	te_cacl = AW_TE_CACL_VALUE(te, (uint16_t)coil_alpha);

	show_re = AW_RE_REALTIME_VALUE((int32_t)re_cacl, te_cacl);

	*re = AW_DSP_RE_TO_SHOW_RE(show_re, re_desc->shift);
	aw_dev_dbg(aw_dev->dev, "real_r0:[%d]", *re);

	return 0;
}

static int aw_pid_2049_get_icalk(struct aw_device *aw_dev, int16_t *icalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_icalk = 0;

	ret = aw_dev->ops.aw_reg_read(aw_dev, AW_PID_2049_EFRM2_REG, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	reg_icalk = reg_val & (~AW_PID_2049_EF_ISN_GESLP_MASK);

	if (reg_icalk & (~AW_PID_2049_EF_ISN_GESLP_SIGN_MASK))
		reg_icalk = reg_icalk | AW_PID_2049_EF_ISN_GESLP_SIGN_NEG;

	*icalk = (int16_t)reg_icalk;

	return 0;
}

static int aw_pid_2049_get_vcalk(struct aw_device *aw_dev, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_vcalk = 0;

	ret = aw_dev->ops.aw_reg_read(aw_dev, AW_PID_2049_EFRH_REG, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	reg_val = reg_val >> AW_PID_2049_EF_VSENSE_GAIN_SHIFT;

	reg_vcalk = (uint16_t)reg_val & (~AW_PID_2049_EF_VSN_GESLP_MASK);

	if (reg_vcalk & (~AW_PID_2049_EF_VSN_GESLP_SIGN_MASK))
		reg_vcalk = reg_vcalk | AW_PID_2049_EF_VSN_GESLP_SIGN_NEG;

	*vcalk = (int16_t)reg_vcalk;

	return 0;
}

static int aw_pid_2049_get_vcalk_dac(struct aw_device *aw_dev, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_vcalk = 0;

	ret = aw_dev->ops.aw_reg_read(aw_dev, AW_PID_2049_EFRM2_REG, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	reg_vcalk = reg_val >> AW_PID_2049_EF_DAC_GESLP_SHIFT;

	if (reg_vcalk & AW_PID_2049_EF_DAC_GESLP_SIGN_MASK)
		reg_vcalk = reg_vcalk | AW_PID_2049_EF_DAC_GESLP_SIGN_NEG;

	*vcalk = (int16_t)reg_vcalk;

	return 0;
}

static int aw_pid_2049_init_vcalb_update(struct aw_device *aw_dev, backup_sec_t flag)
{
	int ret = -1;
	struct aw_vcalb_desc *desc = &aw_dev->vcalb_desc;

	if (flag == AW_RECOVERY_SEC_DATA) {
		ret = aw_dev->ops.aw_dsp_write(aw_dev, AW_PID_2049_DSP_REG_VCALB,
						(uint32_t)desc->last_value, AW_DSP_16_DATA);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write vcalb failed");
			return ret;
		}
	} else if (flag == AW_RECORD_SEC_DATA) {
		ret = aw_dev->ops.aw_dsp_read(aw_dev, AW_PID_2049_DSP_REG_VCALB,
						(uint32_t *)&desc->last_value, AW_DSP_16_DATA);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read vcalb failed");
			return ret;
		}
	} else {
		aw_dev_err(aw_dev->dev, "unsupported type:%d", flag);
		return -EINVAL;
	}

	return 0;
}

static int aw_pid_2049_set_vcalb(struct aw_device *aw_dev)
{
	int ret = -1;
	uint32_t reg_val = 0;
	int vcalb;
	int icalk;
	int vcalk;
	int16_t icalk_val = 0;
	int16_t vcalk_val = 0;
	uint32_t vcalb_adj;
	int vsense_select = -1;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, AW_PID_2049_DSP_REG_VCALB, &vcalb_adj, AW_DSP_16_DATA);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vcalb_adj failed");
		return ret;
	}

	ret = aw883xx_vsense_select(aw_dev, &vsense_select);
	if (ret < 0)
		return ret;
	aw_dev_dbg(aw_dev->dev, "vsense_select = %d", vsense_select);

	ret = aw_pid_2049_get_icalk(aw_dev, &icalk_val);
	if (ret < 0)
		return ret;
	icalk = AW_PID_2049_CABL_BASE_VALUE + AW_PID_2049_ICABLK_FACTOR * icalk_val;

	if (vsense_select == AW_DEV_VDSEL_VSENSE) {
		ret = aw_pid_2049_get_vcalk(aw_dev, &vcalk_val);
		if (ret < 0)
			return ret;

		vcalk = (int)(AW_PID_2049_CABL_BASE_VALUE + AW_PID_2049_VCABLK_FACTOR * vcalk_val);
		vcalb = (int)(AW_PID_2049_VCAL_FACTOR * AW_PID_2049_VSCAL_FACTOR /
			AW_PID_2049_ISCAL_FACTOR * icalk / vcalk * vcalb_adj);

		aw_dev_info(aw_dev->dev, "vcalk_factor=%d, vscal_factor=%d, icalk=%d, vcalk=%d",
				AW_PID_2049_VCABLK_FACTOR, AW_PID_2049_VSCAL_FACTOR, icalk, vcalk);
	} else if (vsense_select == AW_DEV_VDSEL_DAC) {
		ret = aw_pid_2049_get_vcalk_dac(aw_dev, &vcalk_val);
		if (ret < 0)
			return ret;
		vcalk = (int)(AW_PID_2049_CABL_BASE_VALUE + AW_PID_2049_VCABLK_FACTOR_DAC * vcalk_val);
		vcalb = (int)(AW_PID_2049_VCAL_FACTOR * AW_PID_2049_VSCAL_FACTOR_DAC /
			AW_PID_2049_ISCAL_FACTOR * icalk / vcalk * vcalb_adj);

		aw_dev_info(aw_dev->dev, "vcalk_dac_factor=%d, vscal_dac_factor=%d, icalk=%d, vcalk=%d",
				AW_PID_2049_VCABLK_FACTOR_DAC, AW_PID_2049_VSCAL_FACTOR_DAC, icalk, vcalk);
	} else {
		aw_dev_err(aw_dev->dev, "unsupport vsense status");
		return -EINVAL;
	}

	if ((vcalk == 0) || (AW_PID_2049_ISCAL_FACTOR == 0)) {
		aw_dev_err(aw_dev->dev, "vcalk:%d or desc->iscal_factor:%d unsupported",
			vcalk, AW_PID_2049_ISCAL_FACTOR);
		return -EINVAL;
	}

	vcalb = vcalb >> AW_PID_2049_VCALB_ADJ_FACTOR;
	reg_val = (uint32_t)vcalb;

	aw_dev_info(aw_dev->dev, "vcalb=%d, reg_val=0x%x, vcalb_adj =0x%x", vcalb, reg_val, vcalb_adj);

	ret = aw_dev->ops.aw_dsp_write(aw_dev, AW_PID_2049_DSP_REG_VCALB, reg_val, AW_DSP_16_DATA);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write vcalb failed");
		return ret;
	}

	aw_dev_info(aw_dev->dev, "done");

	return 0;
}

static int aw_dev_pid_2049_init(struct aw_device *aw_pa)
{
	aw_pa->reg_max_addr = AW_PID_2049_REG_MAX;

	/*call aw device init func*/
	aw_pa->fade_step = AW_PID_2049_VOLUME_STEP_DB;
	aw_pa->crc_type = AW_SW_CRC_CHECK;

	aw_pa->ops.aw_reg_val_to_db = aw_pid_2049_reg_val_to_db;
	aw_pa->ops.aw_db_val_to_reg = aw_pid_2049_db_val_to_reg;
	aw_pa->ops.aw_set_vcalb = aw_pid_2049_set_vcalb;
	aw_pa->ops.aw_get_r0 = aw_pid_2049_get_convert_r0;
	aw_pa->ops.aw_sw_crc_check = aw_pid_2049_dsp_crc32_check;
	aw_pa->ops.aw_init_vcalb_update = aw_pid_2049_init_vcalb_update;

	aw_pa->efuse_check_desc.reg = AW_REG_NONE;
	aw_pa->crc_check_desc.crc_ctrl_reg = AW_REG_NONE;

	aw_pa->int_desc.intst_mask = AW_PID_2049_BIT_SYSINT_CHECK;

	aw_pa->vcalb_desc.vcalb_reg = AW_REG_NONE;

	aw_pa->vsense_desc.vcalb_vsense_reg = AW_PID_2049_I2SCFG3_REG;
	aw_pa->vsense_desc.vcalk_vdsel_mask = AW_PID_2049_VDSEL_MASK;

	aw_pa->noise_gate_en.reg = AW_REG_NONE;

	aw_pa->eps_en.reg = AW_REG_NONE;

	aw_pa->volume_desc.reg = AW_PID_2049_SYSCTRL2_REG;
	aw_pa->volume_desc.mask = AW_PID_2049_VOL_MASK;
	aw_pa->volume_desc.shift = AW_PID_2049_VOL_START;
	aw_pa->volume_desc.mute_volume = AW_PID_2049_MUTE_VOL;
	aw_pa->volume_desc.max_volume = AW_PID_2049_VOL_DEFAULT_VALUE;
	aw_pa->volume_desc.ctl_volume = AW_PID_2049_VOL_DEFAULT_VALUE;

	aw_pa->memclk_pll_desc.reg = AW_PID_2049_DBGCTRL_REG;
	aw_pa->memclk_pll_desc.mask = AW_PID_2049_MEM_CLKSEL_MASK;
	aw_pa->memclk_pll_desc.enable = AW_PID_2049_MEM_CLKSEL_DAP_HCLK_VALUE;
	aw_pa->memclk_pll_desc.disable = AW_PID_2049_MEM_CLKSEL_OSC_CLK_VALUE;

	aw_pa->dsp_mem_desc.dsp_rom_check_reg = AW_PID_2049_DSP_ROM_CHECK_ADDR;
	aw_pa->dsp_mem_desc.dsp_rom_check_data = AW_PID_2049_DSP_ROM_CHECK_DATA;
	aw_pa->dsp_mem_desc.dsp_rom_data_type = AW_DSP_16_DATA;

	aw_pa->dsp_vol_desc.noise_st = AW_PID_2049_DSP_VOL_NOISE_ST;

	aw_pa->cali_desc.re_desc.location = AW_CALI_RE_STORE_DSP;
	aw_pa->cali_desc.re_desc.reg.dsp_reg = AW_PID_2049_DSP_REG_CFG_ADPZ_RE;
	aw_pa->cali_desc.re_desc.shift = AW_PID_2049_DSP_RE_SHIFT;

	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.ra_desc.dsp_reg = AW_PID_2049_DSP_REG_CFG_ADPZ_RA;
	aw_pa->cali_desc.ra_desc.data_type = AW_DSP_32_DATA;
	aw_pa->cali_desc.ra_desc.shift = AW_PID_2049_DSP_CFG_ADPZ_RA_SHIFT;

	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.cali_cfg_desc.actampth_reg = AW_PID_2049_DSP_REG_CFG_MBMEC_ACTAMPTH;
	aw_pa->cali_desc.cali_cfg_desc.actampth_data_type = AW_DSP_32_DATA;

	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.cali_cfg_desc.noiseampth_reg = AW_PID_2049_DSP_REG_CFG_MBMEC_NOISEAMPTH;
	aw_pa->cali_desc.cali_cfg_desc.noiseampth_data_type = AW_DSP_32_DATA;

	aw_pa->cali_desc.cali_cfg_desc.ustepn_reg = AW_PID_2049_DSP_REG_CFG_ADPZ_USTEPN;
	aw_pa->cali_desc.cali_cfg_desc.ustepn_data_type = AW_DSP_16_DATA;

	aw_pa->cali_desc.cali_cfg_desc.alphan_reg = AW_PID_2049_DSP_REG_CFG_RE_ALPHA;
	aw_pa->cali_desc.cali_cfg_desc.alphan_data_type = AW_DSP_16_DATA;

	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.r0_desc.dsp_reg = AW_PID_2049_DSP_REG_CFG_ADPZ_RE;
	aw_pa->cali_desc.r0_desc.data_type = AW_DSP_32_DATA;
	aw_pa->cali_desc.r0_desc.shift = AW_PID_2049_DSP_RE_SHIFT;

	aw_pa->cali_desc.dsp_re_desc.shift = AW_PID_2049_DSP_REG_CALRE_SHIFT;
	aw_pa->cali_desc.dsp_re_desc.dsp_reg = AW_PID_2049_DSP_REG_CALRE;
	aw_pa->cali_desc.dsp_re_desc.data_type = AW_DSP_16_DATA;

	aw_pa->cali_desc.noise_desc.dsp_reg = AW_PID_2049_DSP_REG_CFG_MBMEC_GLBCFG;
	aw_pa->cali_desc.noise_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.noise_desc.mask = AW_PID_2049_DSP_REG_NOISE_MASK;

	aw_pa->cali_desc.f0_desc.dsp_reg = AW_PID_2049_DSP_REG_RESULT_F0;
	aw_pa->cali_desc.f0_desc.shift = AW_PID_2049_DSP_F0_SHIFT;
	aw_pa->cali_desc.f0_desc.data_type = AW_DSP_16_DATA;

	aw_pa->cali_desc.q_desc.dsp_reg = AW_PID_2049_DSP_REG_RESULT_Q;
	aw_pa->cali_desc.q_desc.shift = AW_PID_2049_DSP_Q_SHIFT;
	aw_pa->cali_desc.q_desc.data_type = AW_DSP_16_DATA;

	/*32-bit data types need bypass dsp*/
	aw_pa->dsp_crc_desc.dsp_reg = AW_PID_2049_DSP_REG_CRC_ADDR;
	aw_pa->dsp_crc_desc.data_type = AW_DSP_32_DATA;

	aw_pa->dsp_crc_desc.check_reg = AW_PID_2049_CRC_VALUE_CHECK_REG;
	aw_pa->dsp_crc_desc.check_reg_data_type = AW_DSP_16_DATA;

	aw_pa->dsp_crc_desc.ctl_reg = AW_PID_2049_HAGCCFG7_REG;
	aw_pa->dsp_crc_desc.ctl_mask = AW_PID_2049_AGC_DSP_CTL_MASK;
	aw_pa->dsp_crc_desc.ctl_enable = AW_PID_2049_AGC_DSP_CTL_ENABLE_VALUE;
	aw_pa->dsp_crc_desc.ctl_disable = AW_PID_2049_AGC_DSP_CTL_DISABLE_VALUE;

	aw_pa->cco_mux_desc.reg = AW_PID_2049_PLLCTRL1_REG;
	aw_pa->cco_mux_desc.mask = AW883XX_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divider = AW883XX_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass = AW883XX_CCO_MUX_BYPASS_VALUE;

	aw_pa->monitor_desc.vmax_desc.dsp_reg = AW_PID_2049_DSP_REG_VMAX;
	aw_pa->monitor_desc.vmax_desc.data_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.ipeak_desc.reg = AW_PID_2049_SYSCTRL2_REG;
	aw_pa->monitor_desc.ipeak_desc.mask = AW_PID_2049_BST_IPEAK_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_reg = AW_PID_2049_DSP_REG_TEMP_SWITCH_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_mask = AW_PID_2049_DSP_EXTN_TEMP_EN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_reg = AW_PID_2049_DSP_REG_TEMP_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_reg = AW_PID_2049_DSP_REG_TEMP_SWITCH_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_en_mask = AW_PID_2049_DSP_MONITOR_TEMP_EN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_reg = AW_PID_2049_DSP_REG_CFG_MBMEC_GLBCFG;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_en_mask = AW_PID_2049_DSP_MONITOR_VOLT_EN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.hw_monitor_desc.reg = AW_REG_NONE;

	aw_pa->tx_en_desc.reg = AW_PID_2049_I2SCFG1_REG;
	aw_pa->tx_en_desc.mask = AW_PID_2049_I2STXEN_MASK;
	aw_pa->tx_en_desc.enable = AW_PID_2049_I2STXEN_ENABLE_VALUE;
	aw_pa->tx_en_desc.disable = AW_PID_2049_I2STXEN_DISABLE_VALUE;

	aw_pa->cali_desc.cali_delay_desc.dsp_reg = AW_PID_2049_DSP_CALI_F0_DELAY;
	aw_pa->cali_desc.cali_delay_desc.data_type = AW_DSP_16_DATA;

	aw_pa->dsp_st_desc.dsp_reg_s1 = AW_PID_2049_DSP_ST_S1;
	aw_pa->dsp_st_desc.dsp_reg_e1 = AW_PID_2049_DSP_ST_E1;
	aw_pa->dsp_st_desc.dsp_reg_s2 = AW_PID_2049_DSP_ST_S2;
	aw_pa->dsp_st_desc.dsp_reg_e2 = AW_PID_2049_DSP_REG_TEMP_ADDR;

	aw_pa->fw_ver_desc.version_reg = AW_PID_2049_FIRMWARE_VERSION_REG;
	aw_pa->fw_ver_desc.data_type = AW_DSP_16_DATA;

	aw_pa->dither_desc.reg = AW_PID_2049_DBGCTRL_REG;
	aw_pa->dither_desc.mask = AW_PID_2049_DITHER_EN_MASK;
	aw_pa->dither_desc.enable = AW_PID_2049_DITHER_EN_ENABLE_VALUE;
	aw_pa->dither_desc.disable = AW_PID_2049_DITHER_EN_DISABLE_VALUE;

	aw_pa->dsp_ng_desc.reg = AW_REG_NONE;
	aw_pa->dsp_ng_desc.dsp_lp_reg = AW_REG_NONE;

	return 0;
}

static int aw_dev_pid_2066_init(struct aw_device *aw_pa)
{
	aw_pa->reg_max_addr = AW_PID_2066_REG_MAX;

	aw_pa->ops.aw_get_r0 = NULL;
	aw_pa->ops.aw_sw_crc_check = NULL;
	aw_pa->ops.aw_init_vcalb_update = NULL;

	aw_pa->vcalb_desc.vscal_factor = AW_PID_2066_VSCAL_FACTOR;
	aw_pa->vcalb_desc.iscal_factor = AW_PID_2066_ISCAL_FACTOR;

	aw_pa->vsense_desc.vcalb_vsense_reg = AW_REG_NONE;
	aw_pa->vsense_desc.vcalb_vsense_default = AW_DEV_VDSEL_VSENSE;
	aw_pa->vcalb_desc.icalk_desc.high_reg = AW_PID_2066_EFRM2_REG;
	aw_pa->vcalb_desc.icalk_desc.high_reg_mask = AW_PID_2066_EF_ISN_GESLP_MASK;
	aw_pa->vcalb_desc.icalk_desc.high_reg_shift = AW_PID_2066_EF_ISN_GESLP_START;
	aw_pa->vcalb_desc.icalk_desc.low_reg = AW_PID_2066_EFRL_REG;
	aw_pa->vcalb_desc.icalk_desc.low_reg_mask = AW_PID_2066_EF_ISN_H5BITS_MASK;
	aw_pa->vcalb_desc.icalk_desc.low_reg_shift = AW_PID_2066_EF_VSN_H3BITS_SHIFT;
	aw_pa->vcalb_desc.icalk_desc.low_offset_mask = AW_PID_2066_EF_ISN_H5BITS_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_desc.sign_mask = AW_PID_2066_ICALK_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_desc.neg_mask = AW_PID_2066_ICALK_NEG_MASK;
	aw_pa->vcalb_desc.icalk_desc.value_factor = AW_PID_2066_ICABLK_FACTOR;
	aw_pa->vcalb_desc.vcalk_desc.high_reg = AW_PID_2066_EFRM2_REG;
	aw_pa->vcalb_desc.vcalk_desc.high_reg_mask = AW_PID_2066_EF_VSN_GESLP_MASK;
	aw_pa->vcalb_desc.vcalk_desc.high_reg_shift = AW_PID_2066_EF_VSN_GESLP_START;
	aw_pa->vcalb_desc.vcalk_desc.low_reg = AW_PID_2066_EFRL_REG;
	aw_pa->vcalb_desc.vcalk_desc.low_reg_mask = AW_PID_2066_EF_VSN_H3BITS_MASK;
	aw_pa->vcalb_desc.vcalk_desc.low_reg_shift = AW_PID_2066_EF_VSN_H3BITS_SHIFT;
	aw_pa->vcalb_desc.vcalk_desc.low_offset_mask = AW_PID_2066_EF_VSN_H3BITS_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_desc.sign_mask = AW_PID_2066_VCALK_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_desc.neg_mask = AW_PID_2066_VCALK_NEG_MASK;
	aw_pa->vcalb_desc.vcalk_desc.value_factor = AW_PID_2066_VCABLK_FACTOR;

	aw_pa->eps_en.reg = AW_REG_NONE;

	aw_pa->dsp_mem_desc.dsp_rom_check_reg = AW_PID_2066_DSP_ROM_CHECK_ADDR;
	aw_pa->dsp_mem_desc.dsp_rom_check_data = AW_PID_2066_DSP_ROM_CHECK_DATA;
	aw_pa->dsp_mem_desc.dsp_rom_data_type = AW_DSP_16_DATA;

	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.ra_desc.dsp_reg = AW_PID_2066_DSP_REG_CFG_ADPZ_RA;
	aw_pa->cali_desc.ra_desc.data_type = AW_DSP_32_DATA;
	aw_pa->cali_desc.ra_desc.shift = AW_PID_2066_DSP_CFG_ADPZ_RA_SHIFT;
	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.cali_cfg_desc.actampth_reg = AW_PID_2066_DSP_REG_CFG_MBMEC_ACTAMPTH;
	aw_pa->cali_desc.cali_cfg_desc.actampth_data_type = AW_DSP_32_DATA;
	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.cali_cfg_desc.noiseampth_reg = AW_PID_2066_DSP_REG_CFG_MBMEC_NOISEAMPTH;
	aw_pa->cali_desc.cali_cfg_desc.noiseampth_data_type = AW_DSP_32_DATA;
	aw_pa->cali_desc.cali_cfg_desc.ustepn_reg = AW_PID_2066_DSP_REG_CFG_ADPZ_USTEPN;
	aw_pa->cali_desc.cali_cfg_desc.ustepn_data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.cali_cfg_desc.alphan_reg = AW_PID_2066_DSP_REG_CFG_RE_ALPHA;
	aw_pa->cali_desc.cali_cfg_desc.alphan_data_type = AW_DSP_16_DATA;
	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.r0_desc.dsp_reg = AW_PID_2066_DSP_REG_CFG_ADPZ_RE;
	aw_pa->cali_desc.r0_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.r0_desc.shift = AW_PID_2066_ADPZ_RE_SHIFT;
	aw_pa->cali_desc.dsp_re_desc.shift = AW_PID_2066_DSP_REG_CALRE_SHIFT;
	aw_pa->cali_desc.dsp_re_desc.dsp_reg = AW_PID_2066_DSP_REG_CALRE;
	aw_pa->cali_desc.dsp_re_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.noise_desc.dsp_reg = AW_PID_2066_DSP_REG_CFG_MBMEC_GLBCFG;
	aw_pa->cali_desc.noise_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.noise_desc.mask = AW_PID_2066_DSP_REG_NOISE_MASK;
	aw_pa->cali_desc.f0_desc.dsp_reg = AW_PID_2066_DSP_REG_RESULT_F0;
	aw_pa->cali_desc.f0_desc.shift = AW_PID_2066_DSP_F0_SHIFT;
	aw_pa->cali_desc.f0_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.q_desc.dsp_reg = AW_PID_2066_DSP_REG_RESULT_Q;
	aw_pa->cali_desc.q_desc.shift = AW_PID_2066_DSP_Q_SHIFT;
	aw_pa->cali_desc.q_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.cali_delay_desc.dsp_reg = AW_PID_2066_DSP_CALI_F0_DELAY;
	aw_pa->cali_desc.cali_delay_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cco_mux_desc.reg = AW_PID_2066_PLLCTRL2_REG;
	aw_pa->cco_mux_desc.mask = AW883XX_CCO_MUX_MASK;
	aw_pa->cco_mux_desc.divider = AW883XX_CCO_MUX_DIVIDED_VALUE;
	aw_pa->cco_mux_desc.bypass = AW883XX_CCO_MUX_BYPASS_VALUE;

	aw_pa->monitor_desc.vmax_desc.dsp_reg = AW_PID_2066_DSP_REG_VMAX;
	aw_pa->monitor_desc.vmax_desc.data_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_reg = AW_PID_2066_DSP_REG_EXTERN_TEMP_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_reg = AW_PID_2066_DSP_MONITOR_TEMPEN_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_mask = AW_PID_2066_DSP_MONITOR_EXTERN_TEMP_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_reg = AW_PID_2066_DSP_MONITOR_TEMPEN_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_en_mask = AW_PID_2066_DSP_MONITOR_TEMPEN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_reg = AW_PID_2066_DSP_REG_CFG_MBMEC_GLBCFG;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_en_mask = AW_PID_2066_DSP_MONITOR_VOLTEN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_reg_type = AW_DSP_16_DATA;

	aw_pa->dsp_st_desc.dsp_reg_s1 = AW_PID_2066_DSP_ST_S1;
	aw_pa->dsp_st_desc.dsp_reg_e1 = AW_PID_2066_DSP_ST_E1;
	aw_pa->dsp_st_desc.dsp_reg_s2 = AW_PID_2066_DSP_ST_S2;
	aw_pa->dsp_st_desc.dsp_reg_e2 = AW_PID_2066_DSP_ST_E2;

	aw_pa->crc_check_desc.crc_cfg_base_addr = AW_PID_2066_CRC_CFG_BASE_ADDR;
	aw_pa->crc_check_desc.crc_fw_base_addr = AW_PID_2066_CRC_FW_BASE_ADDR;

	aw_pa->crc_check_realtime_desc.addr = AW_PID_2066_CRC_REALTIME_ADDR;
	aw_pa->crc_check_realtime_desc.mask = AW_PID_2066_CRC_REALTIME_MASK;
	aw_pa->crc_check_realtime_desc.data_type = AW_DSP_16_DATA;
	aw_pa->crc_check_realtime_desc.enable = AW_PID_2066_CRC_REALTIME_ENABLE;
	aw_pa->crc_check_realtime_desc.disable = AW_PID_2066_CRC_REALTIME_DISABLE;
	aw_pa->crc_check_realtime_desc.check_addr = AW_PID_2066_CRC_REALTIME_CHECK_ADDR;
	aw_pa->crc_check_realtime_desc.check_mask = AW_PID_2066_CRC_REALTIME_CHECK_MASK;
	aw_pa->crc_check_realtime_desc.check_abnormal = AW_PID_2066_CRC_REALTIME_CHECK_ABNORMAL;
	aw_pa->crc_check_realtime_desc.check_normal = AW_PID_2066_CRC_REALTIME_CHECK_NORMAL;
	aw_pa->crc_check_realtime_desc.check_data_type = AW_DSP_16_DATA;

	aw_pa->fw_ver_desc.version_reg = AW_PID_2066_FIRMWARE_VERSION_REG;
	aw_pa->fw_ver_desc.data_type = AW_DSP_16_DATA;

	aw_pa->dsp_ng_desc.reg = AW_REG_NONE;
	aw_pa->dsp_ng_desc.dsp_lp_reg = AW_REG_NONE;

	return 0;
}

static int aw_dev_pid_2183_init(struct aw_device *aw_pa)
{
	aw_pa->reg_max_addr = AW_PID_2183_REG_MAX;

	aw_pa->ops.aw_set_vcalb = NULL;
	aw_pa->ops.aw_get_r0 = NULL;
	aw_pa->ops.aw_sw_crc_check = NULL;
	aw_pa->ops.aw_init_vcalb_update = NULL;

	aw_pa->vcalb_desc.vscal_factor = AW_PID_2183_VSCAL_FACTOR;
	aw_pa->vcalb_desc.iscal_factor = AW_PID_2183_ISCAL_FACTOR;
	aw_pa->vcalb_desc.vscal_dac_factor = AW_PID_2183_VSCAL_DAC_FACTOR;
	aw_pa->vcalb_desc.iscal_dac_factor = AW_PID_2183_ISCAL_DAC_FACTOR;

	aw_pa->vcalb_desc.icalk_desc.high_reg = AW_PID_2183_EFRH4_REG;
	aw_pa->vcalb_desc.icalk_desc.high_reg_mask = AW_PID_2183_EF_ISN_GESLP_H_MASK;
	aw_pa->vcalb_desc.icalk_desc.high_reg_shift = AW_PID_2183_EF_ISN_GESLP_H_START;
	aw_pa->vcalb_desc.icalk_desc.low_reg = AW_PID_2183_EFRL4_REG;
	aw_pa->vcalb_desc.icalk_desc.low_reg_mask = AW_PID_2183_EF_ISN_GESLP_L_MASK;
	aw_pa->vcalb_desc.icalk_desc.low_reg_shift = AW_PID_2183_EF_ISN_GESLP_L_START;
	aw_pa->vcalb_desc.icalk_desc.low_offset_mask = 0;
	aw_pa->vcalb_desc.icalk_desc.sign_mask = AW_PID_2183_EF_ISN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.icalk_desc.neg_mask = AW_PID_2183_EF_ISN_GESLP_SIGN_NEG;
	aw_pa->vcalb_desc.icalk_desc.value_factor = AW_PID_2183_ICABLK_FACTOR;
	aw_pa->vcalb_desc.vcalk_desc.high_reg = AW_PID_2183_EFRH3_REG;
	aw_pa->vcalb_desc.vcalk_desc.high_reg_mask = AW_PID_2183_EF_VSN_GESLP_H_MASK;
	aw_pa->vcalb_desc.vcalk_desc.high_reg_shift = AW_PID_2183_EF_VSN_GESLP_H_START;
	aw_pa->vcalb_desc.vcalk_desc.low_reg = AW_PID_2183_EFRL3_REG;
	aw_pa->vcalb_desc.vcalk_desc.low_reg_mask = AW_PID_2183_EF_VSN_GESLP_L_MASK;
	aw_pa->vcalb_desc.vcalk_desc.low_reg_shift = AW_PID_2183_EF_VSN_GESLP_L_START;
	aw_pa->vcalb_desc.vcalk_desc.low_offset_mask = 0;
	aw_pa->vcalb_desc.vcalk_desc.sign_mask = AW_PID_2183_EF_VSN_GESLP_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_desc.neg_mask = AW_PID_2183_EF_VSN_GESLP_SIGN_NEG;
	aw_pa->vcalb_desc.vcalk_desc.value_factor = AW_PID_2183_VCABLK_FACTOR;
	aw_pa->vcalb_desc.vcalk_dac_desc.high_reg = AW_PID_2183_EFRH2_REG;
	aw_pa->vcalb_desc.vcalk_dac_desc.high_reg_mask = AW_PID_2183_INTERNAL_VSN_TRIM_H_MASK;
	aw_pa->vcalb_desc.vcalk_dac_desc.high_reg_shift = AW_PID_2183_INTERNAL_VSN_TRIM_H_START;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_reg = AW_PID_2183_EFRL2_REG;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_reg_mask = AW_PID_2183_INTERNAL_VSN_TRIM_L_MASK;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_reg_shift = AW_PID_2183_INTERNAL_VSN_TRIM_L_START;
	aw_pa->vcalb_desc.vcalk_dac_desc.low_offset_mask = 0;
	aw_pa->vcalb_desc.vcalk_dac_desc.sign_mask = AW_PID_2183_TEM4_SIGN_MASK;
	aw_pa->vcalb_desc.vcalk_dac_desc.neg_mask = AW_PID_2183_TEM4_SIGN_NEG;
	aw_pa->vcalb_desc.vcalk_dac_desc.value_factor = AW_PID_2183_VCABLK_DAC_FACTOR;

	aw_pa->dsp_mem_desc.dsp_rom_check_reg = AW_PID_2183_DSP_ROM_CHECK_ADDR;
	aw_pa->dsp_mem_desc.dsp_rom_check_data = AW_PID_2183_DSP_ROM_CHECK_DATA;
	aw_pa->dsp_mem_desc.dsp_rom_data_type = AW_DSP_16_DATA;

	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.ra_desc.dsp_reg = AW_PID_2183_DSP_REG_CFG_ADPZ_RA;
	aw_pa->cali_desc.ra_desc.data_type = AW_DSP_32_DATA;
	aw_pa->cali_desc.ra_desc.shift = AW_PID_2183_DSP_CFG_ADPZ_RA_SHIFT;
	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.cali_cfg_desc.actampth_reg = AW_PID_2183_DSP_REG_CFG_MBMEC_ACTAMPTH;
	aw_pa->cali_desc.cali_cfg_desc.actampth_data_type = AW_DSP_32_DATA;
	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.cali_cfg_desc.noiseampth_reg = AW_PID_2183_DSP_REG_CFG_MBMEC_NOISEAMPTH;
	aw_pa->cali_desc.cali_cfg_desc.noiseampth_data_type = AW_DSP_32_DATA;
	aw_pa->cali_desc.cali_cfg_desc.ustepn_reg = AW_PID_2183_DSP_REG_CFG_ADPZ_USTEPN;
	aw_pa->cali_desc.cali_cfg_desc.ustepn_data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.cali_cfg_desc.alphan_reg = AW_PID_2183_DSP_REG_CFG_RE_ALPHA;
	aw_pa->cali_desc.cali_cfg_desc.alphan_data_type = AW_DSP_16_DATA;
	/*32-bit data types need bypass dsp*/
	aw_pa->cali_desc.r0_desc.dsp_reg = AW_PID_2183_DSP_REG_CFG_ADPZ_RE;
	aw_pa->cali_desc.r0_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.r0_desc.shift = AW_PID_2183_ADPZ_RE_SHIFT;
	aw_pa->cali_desc.dsp_re_desc.dsp_reg = AW_PID_2183_DSP_REG_CALRE;
	aw_pa->cali_desc.dsp_re_desc.shift = AW_PID_2183_DSP_REG_CALRE_SHIFT;
	aw_pa->cali_desc.dsp_re_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.noise_desc.dsp_reg = AW_PID_2183_DSP_REG_CFG_MBMEC_GLBCFG;
	aw_pa->cali_desc.noise_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.noise_desc.mask = AW_PID_2183_DSP_REG_NOISE_MASK;
	aw_pa->cali_desc.f0_desc.dsp_reg = AW_PID_2183_DSP_REG_RESULT_F0;
	aw_pa->cali_desc.f0_desc.shift = AW_PID_2183_DSP_F0_SHIFT;
	aw_pa->cali_desc.f0_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.q_desc.dsp_reg = AW_PID_2183_DSP_REG_RESULT_Q;
	aw_pa->cali_desc.q_desc.shift = AW_PID_2183_DSP_Q_SHIFT;
	aw_pa->cali_desc.q_desc.data_type = AW_DSP_16_DATA;
	aw_pa->cali_desc.cali_delay_desc.dsp_reg = AW_PID_2183_DSP_CALI_F0_DELAY;
	aw_pa->cali_desc.cali_delay_desc.data_type = AW_DSP_16_DATA;

	aw_pa->monitor_desc.vmax_desc.dsp_reg = AW_PID_2183_DSP_REG_VMAX;
	aw_pa->monitor_desc.vmax_desc.data_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_reg = AW_PID_2183_DSP_REG_EXTERN_TEMP_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_reg = AW_PID_2183_DSP_MONITOR_TEMPEN_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_mask = AW_PID_2183_DSP_MONITOR_EXTERN_TEMP_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.extn_temp_en_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_reg = AW_PID_2183_DSP_MONITOR_TEMPEN_ADDR;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_en_mask = AW_PID_2183_DSP_MONITOR_TEMPEN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_temp_reg_type = AW_DSP_16_DATA;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_reg = AW_PID_2183_DSP_REG_CFG_MBMEC_GLBCFG;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_en_mask = AW_PID_2183_DSP_MONITOR_VOLTEN_MASK;
	aw_pa->monitor_desc.dsp_monitor_desc.dsp_volt_reg_type = AW_DSP_16_DATA;

	aw_pa->dsp_st_desc.dsp_reg_s1 = AW_PID_2183_DSP_ST_S1;
	aw_pa->dsp_st_desc.dsp_reg_e1 = AW_PID_2183_DSP_ST_E1;
	aw_pa->dsp_st_desc.dsp_reg_s2 = AW_PID_2183_DSP_ST_S2;
	aw_pa->dsp_st_desc.dsp_reg_e2 = AW_PID_2183_DSP_ST_E2;

	aw_pa->crc_check_desc.crc_cfg_base_addr = AW_PID_2183_CRC_CFG_BASE_ADDR;
	aw_pa->crc_check_desc.crc_fw_base_addr = AW_PID_2183_CRC_FW_BASE_ADDR;

	aw_pa->crc_check_realtime_desc.addr = AW_PID_2183_CRC_REALTIME_ADDR;
	aw_pa->crc_check_realtime_desc.mask = AW_PID_2183_CRC_REALTIME_MASK;
	aw_pa->crc_check_realtime_desc.data_type = AW_DSP_16_DATA;
	aw_pa->crc_check_realtime_desc.enable = AW_PID_2183_CRC_REALTIME_ENABLE;
	aw_pa->crc_check_realtime_desc.disable = AW_PID_2183_CRC_REALTIME_DISABLE;
	aw_pa->crc_check_realtime_desc.check_addr = AW_PID_2183_CRC_REALTIME_CHECK_ADDR;
	aw_pa->crc_check_realtime_desc.check_mask = AW_PID_2183_CRC_REALTIME_CHECK_MASK;
	aw_pa->crc_check_realtime_desc.check_abnormal = AW_PID_2183_CRC_REALTIME_CHECK_ABNORMAL;
	aw_pa->crc_check_realtime_desc.check_normal = AW_PID_2183_CRC_REALTIME_CHECK_NORMAL;
	aw_pa->crc_check_realtime_desc.check_data_type = AW_DSP_16_DATA;

	aw_pa->fw_ver_desc.version_reg = AW_PID_2183_FIRMWARE_VERSION_REG;
	aw_pa->fw_ver_desc.data_type = AW_DSP_16_DATA;

	aw_pa->dsp_ng_desc.dsp_lp_reg = AW_PID_2183_DSP_LOW_POWER_SWITCH_CFG_ADDR;
	aw_pa->dsp_ng_desc.dsp_lp_data_type = AW_DSP_16_DATA;
	aw_pa->dsp_ng_desc.dsp_lp_mask = AW_PID_2183_DSP_LOW_POWER_SWITCH_CFG_MASK;
	aw_pa->dsp_ng_desc.dsp_lp_enable = AW_PID_2183_DSP_LOW_POWER_SWITCH_CFG_ENABLE;
	aw_pa->dsp_ng_desc.dsp_lp_disable = AW_PID_2183_DSP_LOW_POWER_SWITCH_CFG_DISABLE;

	return 0;
}

const struct aw_dev_property g_aw_dev_property_registry[] = {
	{
		.id = AW883XX_PID_2049,
		.dev_init_func = aw_dev_pid_2049_init,
	},
	{
		.id = AW883XX_PID_2066,
		.dev_init_func = aw_dev_pid_2066_init,
	},
	{
		.id = AW883XX_PID_2183,
		.dev_init_func = aw_dev_pid_2183_init,
	},
};

int aw883xx_init_check_chipid(struct aw883xx *aw883xx, uint16_t chipid)
{
	switch (chipid) {
	case AW883XX_PID_2049:
	case AW883XX_PID_2066:
	case AW883XX_PID_2183:
		aw883xx->chip_id = chipid;
		break;
	default:
		aw_dev_err(aw883xx->dev, "unsupported chip_id 0x%04x", chipid);
		return -EINVAL;
	}
	return 0;
}

int aw883xx_init(struct aw883xx *aw883xx)
{
	int ret = 0;
	int i = 0;
	struct aw_device *aw_pa = NULL;

	aw_pa = devm_kzalloc(aw883xx->dev, sizeof(struct aw_device), GFP_KERNEL);
	if (aw_pa == NULL)
		return -ENOMEM;

	aw_pa->private_data = (void *)aw883xx;
	aw883xx->aw_pa = aw_pa;

	aw883xx_dev_mem_init(aw_pa);

	aw883xx_dev_basic_init(aw_pa);

	for (i = 0; i < sizeof(g_aw_dev_property_registry) / sizeof(struct aw_dev_property); i++) {
		if (aw883xx->chip_id == g_aw_dev_property_registry[i].id) {
			if (g_aw_dev_property_registry[i].dev_init_func != NULL)
				ret = g_aw_dev_property_registry[i].dev_init_func(aw_pa);
			else
				ret = aw883xx_dev_common_init(aw_pa, &g_aw_dev_property_registry[i]);

			break;
		}
	}
	if (ret < 0) {
		aw_dev_err(aw883xx->dev, "product pid_%x init failed", aw883xx->chip_id);
		return -EINVAL;
	}

	if (i == sizeof(g_aw_dev_property_registry) / sizeof(struct aw_dev_property)) {
		aw_dev_err(aw883xx->dev, "unsupported chip_id 0x%04x", aw883xx->chip_id);
		return -EINVAL;
	}

	ret = aw883xx_device_probe(aw_pa);
	if (ret < 0) {
		aw_dev_err(aw883xx->dev, "device_probe failed");
		return -EINVAL;
	}

	return 0;
}

