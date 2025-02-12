/*
 * sound\soc\sunxi\sunxi_ahub_daudio.c
 * (C) Copyright 2015-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Wolfgang huang <huangjinhui@allwinnertechtech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "sunxi_ahub.h"
#include "sunxi-pcm.h"

#define DRV_NAME	"sunxi-ahub-daudio"

struct sunxi_ahub_daudio_priv {
	struct device *dev;
	struct regmap *regmap;
	struct clk *pllclk;
#ifdef AHUB_PLL_AUDIO_X4
	struct clk *pllclkx4;
#endif
	struct clk *moduleclk;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;
	struct snd_soc_dai_driver *cpudai;
	char cpudai_name[20];
	struct mutex mutex;
	int used_cnt;
	unsigned int pinconfig;
	unsigned int frame_type;
	unsigned int daudio_master;
	unsigned int pcm_lrck_period;
	unsigned int slot_width_select;
	unsigned int audio_format;
	unsigned int signal_inversion;
	unsigned int tdm_config;
	unsigned int tdm_num;
	unsigned int mclk_div;
};

static int sunxi_ahub_daudio_global_enable(struct sunxi_ahub_daudio_priv
		*sunxi_ahub_daudio, int enable, unsigned int id)
{
	if (enable) {
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(id),
				(1<<I2S_CTL_SDO0_EN), (1<<I2S_CTL_SDO0_EN));
		/* Special processing for HDMI hub playback module */
		if (sunxi_ahub_daudio->tdm_num  == SUNXI_AHUB_HDMI_ID) {
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(sunxi_ahub_daudio->tdm_num),
				(1<<I2S_CTL_SDO1_EN), (1<<I2S_CTL_SDO1_EN));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(sunxi_ahub_daudio->tdm_num),
				(1<<I2S_CTL_SDO2_EN), (1<<I2S_CTL_SDO2_EN));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(sunxi_ahub_daudio->tdm_num),
				(1<<I2S_CTL_SDO3_EN), (1<<I2S_CTL_SDO3_EN));
		}
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(id),
				(1<<I2S_CTL_GEN), (1<<I2S_CTL_GEN));
	} else {
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(id),
				(1<<I2S_CTL_GEN), (0<<I2S_CTL_GEN));
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(id),
				(1<<I2S_CTL_SDO0_EN), (0<<I2S_CTL_SDO0_EN));
		/* Special processing for HDMI hub playback module */
		if (sunxi_ahub_daudio->tdm_num  == SUNXI_AHUB_HDMI_ID) {
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(sunxi_ahub_daudio->tdm_num),
				(1<<I2S_CTL_SDO1_EN), (0<<I2S_CTL_SDO1_EN));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(sunxi_ahub_daudio->tdm_num),
				(1<<I2S_CTL_SDO2_EN), (0<<I2S_CTL_SDO2_EN));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(sunxi_ahub_daudio->tdm_num),
				(1<<I2S_CTL_SDO3_EN), (0<<I2S_CTL_SDO3_EN));
		}
	}
	return 0;
}

static int sunxi_ahub_daudio_mclk_setting(struct sunxi_ahub_daudio_priv
				*sunxi_ahub_daudio, unsigned int id)
{
	unsigned int mclk_div;

	if (sunxi_ahub_daudio->mclk_div) {
		switch (sunxi_ahub_daudio->mclk_div) {
		case	1:
			mclk_div = 1;
			break;
		case	2:
			mclk_div = 2;
			break;
		case	4:
			mclk_div = 3;
			break;
		case	6:
			mclk_div = 4;
			break;
		case	8:
			mclk_div = 5;
			break;
		case	12:
			mclk_div = 6;
			break;
		case	16:
			mclk_div = 7;
			break;
		case	24:
			mclk_div = 8;
			break;
		case	32:
			mclk_div = 9;
			break;
		case	48:
			mclk_div = 10;
			break;
		case	64:
			mclk_div = 11;
			break;
		case	96:
			mclk_div = 12;
			break;
		case	128:
			mclk_div = 13;
			break;
		case	176:
			mclk_div = 14;
			break;
		case	192:
			mclk_div = 15;
			break;
		default:
			dev_err(sunxi_ahub_daudio->dev,
					"unsupport  mclk_div\n");
			return -EINVAL;
		}
		/* setting Mclk as external codec input clk */
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CLKD(id),
				(0xf<<I2S_CLKD_MCLKDIV),
				(mclk_div<<I2S_CLKD_MCLKDIV));
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CLKD(id),
				(0x1<<I2S_CLKD_MCLK), (1<<I2S_CLKD_MCLK));
	} else {
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CLKD(id),
				(0x1<<I2S_CLKD_MCLK), (0<<I2S_CLKD_MCLK));
	}
	return 0;
}

static int sunxi_ahub_daudio_init_fmt(struct sunxi_ahub_daudio_priv
		*sunxi_ahub_daudio, unsigned int fmt, unsigned int id)
{
	unsigned int offset, mode;
	unsigned int lrck_polarity = 0;
	unsigned int brck_polarity;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case	SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(id),
				(1<<I2S_CTL_CLK_OUT), (0<<I2S_CTL_CLK_OUT));
		break;
	case	SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_CTL(id),
				(1<<I2S_CTL_CLK_OUT), (1<<I2S_CTL_CLK_OUT));
		break;
	default:
		dev_err(sunxi_ahub_daudio->dev, "unknown maser/slave format\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case	SND_SOC_DAIFMT_I2S:
		offset = 1;
		mode = 1;
		break;
	case	SND_SOC_DAIFMT_RIGHT_J:
		offset = 0;
		mode = 2;
		break;
	case	SND_SOC_DAIFMT_LEFT_J:
		offset = 0;
		mode = 1;
		break;
	case	SND_SOC_DAIFMT_DSP_A:
		offset = 1;
		mode = 0;
		break;
	case	SND_SOC_DAIFMT_DSP_B:
		offset = 0;
		mode = 0;
		break;
	default:
		dev_err(sunxi_ahub_daudio->dev, "format setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_CTL(id),
			(0x3<<I2S_CTL_MODE), (mode<<I2S_CTL_MODE));
	regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_SLOT0(id),
			(0x1<<I2S_OUT_OFFSET), (offset<<I2S_OUT_OFFSET));
	regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_SLOT1(id),
			(0x1<<I2S_OUT_OFFSET), (offset<<I2S_OUT_OFFSET));
	regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_SLOT2(id),
			(0x1<<I2S_OUT_OFFSET), (offset<<I2S_OUT_OFFSET));
	regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_SLOT3(id),
			(0x1<<I2S_OUT_OFFSET), (offset<<I2S_OUT_OFFSET));
	regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_IN_SLOT(id),
			(0x1<<I2S_IN_OFFSET), (offset<<I2S_IN_OFFSET));

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case	SND_SOC_DAIFMT_I2S:
	case	SND_SOC_DAIFMT_RIGHT_J:
	case	SND_SOC_DAIFMT_LEFT_J:
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case	SND_SOC_DAIFMT_NB_NF:
			lrck_polarity = 0;
			brck_polarity = 0;
			break;
		case	SND_SOC_DAIFMT_NB_IF:
			lrck_polarity = 1;
			brck_polarity = 0;
			break;
		case	SND_SOC_DAIFMT_IB_NF:
			lrck_polarity = 0;
			brck_polarity = 1;
			break;
		case	SND_SOC_DAIFMT_IB_IF:
			lrck_polarity = 1;
			brck_polarity = 1;
			break;
		default:
			dev_err(sunxi_ahub_daudio->dev,
					"invert clk setting failed\n");
			return -EINVAL;
		}
		break;
	case	SND_SOC_DAIFMT_DSP_A:
	case	SND_SOC_DAIFMT_DSP_B:
		/* frame inversion not valid for DSP modes */
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			lrck_polarity = 0;
			brck_polarity = 0;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			lrck_polarity = 0;
			brck_polarity = 1;
			break;
		default:
			dev_err(sunxi_ahub_daudio->dev, "dai fmt invalid\n");
			return -EINVAL;
		}
		break;
	default:
		dev_err(sunxi_ahub_daudio->dev, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_FMT0(id),
			(1<<I2S_FMT0_LRCK_POLARITY),
			(lrck_polarity<<I2S_FMT0_LRCK_POLARITY));
	regmap_update_bits(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_FMT0(id),
			(1<<I2S_FMT0_BCLK_POLARITY),
			(brck_polarity<<I2S_FMT0_BCLK_POLARITY));
	return 0;
}

static int sunxi_ahub_daudio_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
				snd_soc_dai_get_drvdata(dai);

	sunxi_ahub_daudio_init_fmt(sunxi_ahub_daudio, fmt,
					(sunxi_ahub_daudio->tdm_num));
	return 0;
}


static int sunxi_ahub_daudio_init(
		struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio, int id)
{

	sunxi_ahub_daudio_global_enable(sunxi_ahub_daudio, 1, id);

	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH0MAP0(id), 0x76543210);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH0MAP1(id), 0xFEDCBA98);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH1MAP0(id), 0x76543210);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH1MAP1(id), 0xFEDCBA98);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH2MAP0(id), 0x76543210);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH2MAP1(id), 0xFEDCBA98);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH3MAP0(id), 0x76543210);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_OUT_CH3MAP1(id), 0xFEDCBA98);
#if defined(CONFIG_ARCH_SUN50IW9)
	/* change the I2S CHMAP for ac107 capture success */
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_IN_CHMAP0(id), 0x03020100);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_IN_CHMAP1(id), 0x00000000);
#else
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_IN_CHMAP0(id), 0x76543210);
	regmap_write(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_IN_CHMAP1(id), 0xFEDCBA98);
#endif
	regmap_update_bits(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_FMT0(id),
			(1<<I2S_FMT0_LRCK_WIDTH),
			(sunxi_ahub_daudio->frame_type<<I2S_FMT0_LRCK_WIDTH));
	regmap_update_bits(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_FMT0(id),
		(0x1ff<<I2S_FMT0_LRCK_PERIOD),
		((sunxi_ahub_daudio->pcm_lrck_period-1)<<I2S_FMT0_LRCK_PERIOD));

	regmap_update_bits(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_FMT0(id),
		(0x7<<I2S_FMT0_SW),
		(((sunxi_ahub_daudio->slot_width_select>>2) - 1)<<I2S_FMT0_SW));

	/*
	 * MSB on the transmit format, always be first.
	 * default using Linear-PCM, without no companding.
	 * A-law<Eourpean standard> or U-law<US-Japan> not working ok.
	 */
	regmap_write(sunxi_ahub_daudio->regmap, SUNXI_AHUB_I2S_FMT1(id), 0x30);

	/* default setting the daudio fmt */
	sunxi_ahub_daudio_init_fmt(sunxi_ahub_daudio,
		(sunxi_ahub_daudio->audio_format
		| (sunxi_ahub_daudio->signal_inversion<<8)
		| (sunxi_ahub_daudio->daudio_master<<12)), id);

	sunxi_ahub_daudio_mclk_setting(sunxi_ahub_daudio, id);

	return 0;
}

static int sunxi_ahub_daudio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
					snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct sndhdmi_priv *sunxi_hdmi = snd_soc_card_get_drvdata(card);

	/* default setting the daudio fmt */
	sunxi_ahub_daudio_init_fmt(sunxi_ahub_daudio,
		(sunxi_ahub_daudio->audio_format
			| (sunxi_ahub_daudio->signal_inversion<<8)
			| (sunxi_ahub_daudio->daudio_master<<12)),
		sunxi_ahub_daudio->tdm_num);

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		/*
		 * Special procesing for hdmi, HDMI card name is
		 * "sndhdmi" or sndhdmiraw. if card not HDMI,
		 * strstr func just return NULL, jump to right section.
		 * Not HDMI card, sunxi_hdmi maybe a NULL pointer.
		 */
		if (sunxi_ahub_daudio->tdm_num == SUNXI_AHUB_HDMI_ID) {
			if (sunxi_hdmi->hdmi_format > 1)
				regmap_update_bits(sunxi_ahub_daudio->regmap,
					SUNXI_AHUB_I2S_FMT0(
					sunxi_ahub_daudio->tdm_num),
					(7<<I2S_FMT0_SR), (5<<I2S_FMT0_SR));
			else
				regmap_update_bits(sunxi_ahub_daudio->regmap,
					SUNXI_AHUB_I2S_FMT0(
					sunxi_ahub_daudio->tdm_num),
					(7<<I2S_FMT0_SR), (3<<I2S_FMT0_SR));
		} else {
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_FMT0(sunxi_ahub_daudio->tdm_num),
					(7<<I2S_FMT0_SR), (3<<I2S_FMT0_SR));
		}
		break;
	case	SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_FMT0(sunxi_ahub_daudio->tdm_num),
				(7<<I2S_FMT0_SR), (5<<I2S_FMT0_SR));
		break;
	case	SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_FMT0(sunxi_ahub_daudio->tdm_num),
				(7<<I2S_FMT0_SR), (7<<I2S_FMT0_SR));
		break;
	default:
		dev_err(sunxi_ahub_daudio->dev, "unrecognized format\n");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_CHCFG(sunxi_ahub_daudio->tdm_num),
			(0xf<<I2S_CHCFG_TX_CHANNUM),
			((params_channels(params)-1)<<I2S_CHCFG_TX_CHANNUM));

		if (sunxi_ahub_daudio->tdm_num == SUNXI_AHUB_HDMI_ID) {
			regmap_write(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_CH0MAP0(
				sunxi_ahub_daudio->tdm_num),
				0x10);
			if (sunxi_hdmi->hdmi_format > 1) {
				regmap_write(sunxi_ahub_daudio->regmap,
					SUNXI_AHUB_I2S_OUT_CH1MAP0(
					sunxi_ahub_daudio->tdm_num),
					0x32);
				regmap_write(sunxi_ahub_daudio->regmap,
					SUNXI_AHUB_I2S_OUT_CH2MAP0(
					sunxi_ahub_daudio->tdm_num),
					0x54);
				regmap_write(sunxi_ahub_daudio->regmap,
					SUNXI_AHUB_I2S_OUT_CH3MAP0(
					sunxi_ahub_daudio->tdm_num),
					0x76);
			} else {
				if (params_channels(params) > 2) {
					regmap_write(sunxi_ahub_daudio->regmap,
						SUNXI_AHUB_I2S_OUT_CH1MAP0(
						sunxi_ahub_daudio->tdm_num),
						0x23);
				/* only 5.1 & 7.1 */
				if (params_channels(params) > 4) {
					if (params_channels(params) == 6)
						/* 5.1 hit this */
						regmap_write(
						sunxi_ahub_daudio->regmap,
						SUNXI_AHUB_I2S_OUT_CH2MAP0(
						sunxi_ahub_daudio->tdm_num),
						0x54);
					else
						/* 7.1 hit this */
						regmap_write(
						sunxi_ahub_daudio->regmap,
						SUNXI_AHUB_I2S_OUT_CH2MAP0(
						sunxi_ahub_daudio->tdm_num),
						0x76);
				}
				if (params_channels(params)  > 6)
					regmap_write(sunxi_ahub_daudio->regmap,
						SUNXI_AHUB_I2S_OUT_CH3MAP0(
						sunxi_ahub_daudio->tdm_num),
						0x54);
				}
			}

			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT0(
				sunxi_ahub_daudio->tdm_num),
				(0x0f<<I2S_OUT_SLOT_NUM),
				(0x01<<I2S_OUT_SLOT_NUM));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT0(
				sunxi_ahub_daudio->tdm_num),
				(0xffff<<I2S_OUT_SLOT_EN),
				(0x03<<I2S_OUT_SLOT_EN));

			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT1(
				sunxi_ahub_daudio->tdm_num),
				(0x0f<<I2S_OUT_SLOT_NUM),
				(0x01<<I2S_OUT_SLOT_NUM));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT1(
				sunxi_ahub_daudio->tdm_num),
				(0xffff<<I2S_OUT_SLOT_EN),
				(0x03<<I2S_OUT_SLOT_EN));

			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT2(
				sunxi_ahub_daudio->tdm_num),
				(0x0f<<I2S_OUT_SLOT_NUM),
				(0x01<<I2S_OUT_SLOT_NUM));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT2(
				sunxi_ahub_daudio->tdm_num),
				(0xffff<<I2S_OUT_SLOT_EN),
				(0x03<<I2S_OUT_SLOT_EN));

			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT3(
				sunxi_ahub_daudio->tdm_num),
				(0x0f<<I2S_OUT_SLOT_NUM),
				(0x01<<I2S_OUT_SLOT_NUM));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT3(
				sunxi_ahub_daudio->tdm_num),
				(0xffff<<I2S_OUT_SLOT_EN),
				(0x03<<I2S_OUT_SLOT_EN));
		} else {
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT0(
				sunxi_ahub_daudio->tdm_num),
				(0x0f<<I2S_OUT_SLOT_NUM),
			((params_channels(params)-1)<<I2S_OUT_SLOT_NUM));
			regmap_update_bits(sunxi_ahub_daudio->regmap,
				SUNXI_AHUB_I2S_OUT_SLOT0(
				sunxi_ahub_daudio->tdm_num),
				(0xffff<<I2S_OUT_SLOT_EN),
			(((1<<params_channels(params))-1)<<I2S_OUT_SLOT_EN));
		}
	} else {
		regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_CHCFG(sunxi_ahub_daudio->tdm_num),
			(0xf<<I2S_CHCFG_RX_CHANNUM),
			((params_channels(params)-1)<<I2S_CHCFG_RX_CHANNUM));
		regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_IN_SLOT(sunxi_ahub_daudio->tdm_num),
			(0xf<<I2S_IN_SLOT_NUM),
			((params_channels(params)-1)<<I2S_IN_SLOT_NUM));
	}

	return 0;
}

static int sunxi_ahub_daudio_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
					snd_soc_dai_get_drvdata(dai);

#ifdef AHUB_PLL_AUDIO_X4
	if (clk_set_rate(sunxi_ahub_daudio->pllclkx4, freq)) {
		dev_err(sunxi_ahub_daudio->dev, "set pllclkx4 rate failed\n");
		return -EBUSY;
	}

	if (clk_set_rate(sunxi_ahub_daudio->moduleclk, freq)) {
		dev_err(sunxi_ahub_daudio->dev, "set moduleclk rate failed\n");
		return -EBUSY;
	}
#else
	if (clk_set_rate(sunxi_ahub_daudio->pllclk, freq)) {
		dev_err(sunxi_ahub_daudio->dev, "set pllclk rate failed\n");
		return -EBUSY;
	}
#endif

	return 0;
}

static int sunxi_ahub_daudio_set_clkdiv(struct snd_soc_dai *dai,
				int clk_id, int clk_div)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
					snd_soc_dai_get_drvdata(dai);
	unsigned int bclk_div, div_ratio;

	if (sunxi_ahub_daudio->tdm_config)
		/* I2S/TDM two channel mode */
		div_ratio = clk_div / (sunxi_ahub_daudio->pcm_lrck_period * 2);
	else
		/* PCM mode */
		div_ratio = clk_div / sunxi_ahub_daudio->pcm_lrck_period;

	switch (div_ratio) {
	case	1:
		bclk_div = 1;
		break;
	case	2:
		bclk_div = 2;
		break;
	case	4:
		bclk_div = 3;
		break;
	case	6:
		bclk_div = 4;
		break;
	case	8:
		bclk_div = 5;
		break;
	case	12:
		bclk_div = 6;
		break;
	case	16:
		bclk_div = 7;
		break;
	case	24:
		bclk_div = 8;
		break;
	case	32:
		bclk_div = 9;
		break;
	case	48:
		bclk_div = 10;
		break;
	case	64:
		bclk_div = 11;
		break;
	case	96:
		bclk_div = 12;
		break;
	case	128:
		bclk_div = 13;
		break;
	case	176:
		bclk_div = 14;
		break;
	case	192:
		bclk_div = 15;
		break;
	default:
		dev_err(sunxi_ahub_daudio->dev, "unsupport clk_div\n");
		return -EINVAL;
	}

	/* setting bclk to driver external codec bit clk */
	regmap_update_bits(sunxi_ahub_daudio->regmap,
			SUNXI_AHUB_I2S_CLKD(sunxi_ahub_daudio->tdm_num),
			(0xf<<I2S_CLKD_BCLKDIV), (bclk_div<<I2S_CLKD_BCLKDIV));
	return 0;
}

static int sunxi_ahub_daudio_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			/* HMID module, we just keep this clk */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_ahub_daudio_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

static void sunxi_ahub_daudio_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
}

static struct snd_soc_dai_ops sunxi_ahub_cpu_dai_ops = {
	.hw_params = sunxi_ahub_daudio_hw_params,
	.set_sysclk = sunxi_ahub_daudio_set_sysclk,
	.set_clkdiv = sunxi_ahub_daudio_set_clkdiv,
	.set_fmt = sunxi_ahub_daudio_set_fmt,
	.startup = sunxi_ahub_daudio_startup,
	.trigger = sunxi_ahub_daudio_trigger,
	.shutdown = sunxi_ahub_daudio_shutdown,
};

static int sunxi_ahub_daudio_probe(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
					snd_soc_dai_get_drvdata(dai);
	int ret;

	mutex_init(&sunxi_ahub_daudio->mutex);
	sunxi_ahub_daudio_init(sunxi_ahub_daudio, (sunxi_ahub_daudio->tdm_num));

	if (sunxi_ahub_daudio->pinconfig) {
		sunxi_ahub_daudio->pinctrl =
				devm_pinctrl_get(sunxi_ahub_daudio->dev);
		if (IS_ERR_OR_NULL(sunxi_ahub_daudio->pinctrl)) {
			dev_err(sunxi_ahub_daudio->dev, "pinctrl get failed\n");
			return -ENODEV;
		}
	}

	if (sunxi_ahub_daudio->pinconfig) {
		sunxi_ahub_daudio->pinstate =
			pinctrl_lookup_state(sunxi_ahub_daudio->pinctrl,
					"default");
		if (IS_ERR_OR_NULL(sunxi_ahub_daudio->pinstate)) {
			dev_err(sunxi_ahub_daudio->dev,
					"pinctrl default state get failed\n");
			return -ENODEV;
		}
		sunxi_ahub_daudio->pinstate_sleep =
			pinctrl_lookup_state(sunxi_ahub_daudio->pinctrl,
					"sleep");
		if (IS_ERR_OR_NULL(sunxi_ahub_daudio->pinstate_sleep)) {
			dev_err(sunxi_ahub_daudio->dev,
					"pinctrl sleep state get failed\n");
			return -ENODEV;
		}
		ret = pinctrl_select_state(sunxi_ahub_daudio->pinctrl,
						sunxi_ahub_daudio->pinstate);
		if (ret) {
			dev_warn(sunxi_ahub_daudio->dev,
					"select default state failed\n");
			return -EBUSY;
		}
	}

	return 0;
}

static int sunxi_ahub_daudio_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_ahub_daudio_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
					snd_soc_dai_get_drvdata(dai);
	int ret;

	pr_debug("Enter %s\n", __func__);

	clk_disable_unprepare(sunxi_ahub_daudio->moduleclk);
#ifdef AHUB_PLL_AUDIO_X4
	clk_disable_unprepare(sunxi_ahub_daudio->pllclkx4);
#endif
	clk_disable_unprepare(sunxi_ahub_daudio->pllclk);

	if (sunxi_ahub_daudio->pinconfig) {
		ret = pinctrl_select_state(sunxi_ahub_daudio->pinctrl,
				sunxi_ahub_daudio->pinstate_sleep);
		if (ret) {
			dev_warn(sunxi_ahub_daudio->dev,
					"select i2s0-default state failed\n");
			return -EBUSY;
		}
	}

	/* Global disable I2S/TDM module */
	sunxi_ahub_daudio_global_enable(sunxi_ahub_daudio, 0,
				(sunxi_ahub_daudio->tdm_num));

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_ahub_daudio_resume(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
				snd_soc_dai_get_drvdata(dai);
	int ret;

	pr_debug("Enter %s\n", __func__);

	if (clk_prepare_enable(sunxi_ahub_daudio->pllclk)) {
		dev_err(sunxi_ahub_daudio->dev, "pllclk resume failed\n");
		return -EBUSY;
	}

#ifdef AHUB_PLL_AUDIO_X4
	if (clk_prepare_enable(sunxi_ahub_daudio->pllclkx4)) {
		dev_err(sunxi_ahub_daudio->dev, "pllclkx4 resume failed\n");
		return -EBUSY;
	}
#endif

	if (clk_prepare_enable(sunxi_ahub_daudio->moduleclk)) {
		dev_err(sunxi_ahub_daudio->dev, "moduleclk resume failed\n");
		return -EBUSY;
	}

	if (sunxi_ahub_daudio->pinconfig) {
		ret = pinctrl_select_state(sunxi_ahub_daudio->pinctrl,
						sunxi_ahub_daudio->pinstate);
		if (ret) {
			dev_warn(sunxi_ahub_daudio->dev,
					"select default state failed\n");
			return -EBUSY;
		}
	}

	sunxi_ahub_daudio_init(sunxi_ahub_daudio, sunxi_ahub_daudio->tdm_num);

	pr_debug("End %s\n", __func__);

	return 0;
}

static struct snd_soc_dai_driver sunxi_ahub_daudio_mod = {
	.probe = sunxi_ahub_daudio_probe,
	.remove = sunxi_ahub_daudio_remove,
	.suspend = sunxi_ahub_daudio_suspend,
	.resume = sunxi_ahub_daudio_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 16,
		.rates = SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 16,
		.rates = SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	 },
	.ops = &sunxi_ahub_cpu_dai_ops,
};

static const struct snd_soc_component_driver sunxi_ahub_daudio_component = {
	.name		= DRV_NAME,
};

static int sunxi_ahub_daudio_dev_probe(struct platform_device *pdev)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

	sunxi_ahub_daudio = devm_kzalloc(&pdev->dev,
			sizeof(struct sunxi_ahub_daudio_priv), GFP_KERNEL);
	if (!sunxi_ahub_daudio) {
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_ahub_daudio);
	sunxi_ahub_daudio->dev = &pdev->dev;

	sunxi_ahub_daudio->cpudai = devm_kzalloc(&pdev->dev,
			sizeof(struct snd_soc_dai_driver), GFP_KERNEL);
	if (!sunxi_ahub_daudio->cpudai) {
		ret = -ENOMEM;
		goto err_devm_kfree;
	} else {
		memcpy(sunxi_ahub_daudio->cpudai, &sunxi_ahub_daudio_mod,
			sizeof(struct snd_soc_dai_driver));
	}

	sunxi_ahub_daudio->pllclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_ahub_daudio->pllclk)) {
		dev_err(&pdev->dev, "pllclk get failed\n");
		ret = PTR_ERR(sunxi_ahub_daudio->pllclk);
		goto err_cpudai_kfree;
	}

#ifdef AHUB_PLL_AUDIO_X4
	sunxi_ahub_daudio->pllclkx4 = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(sunxi_ahub_daudio->pllclkx4)) {
		dev_err(&pdev->dev, "pllclkx4 get failed\n");
		ret = PTR_ERR(sunxi_ahub_daudio->pllclkx4);
		goto err_cpudai_kfree;
	}

	sunxi_ahub_daudio->moduleclk = of_clk_get(np, 2);
	if (IS_ERR_OR_NULL(sunxi_ahub_daudio->moduleclk)) {
		dev_err(&pdev->dev, "moduleclk get failed\n");
		ret = PTR_ERR(sunxi_ahub_daudio->moduleclk);
		goto err_pllclk_put;
	}

	if (clk_set_parent(sunxi_ahub_daudio->moduleclk,
				sunxi_ahub_daudio->pllclkx4)) {
		dev_err(&pdev->dev, "set parent of moduleclk to pllclkx4 fail\n");
		ret = -EBUSY;
		goto err_moduleclk_put;
	}
	clk_prepare_enable(sunxi_ahub_daudio->pllclk);
	clk_prepare_enable(sunxi_ahub_daudio->pllclkx4);
	clk_prepare_enable(sunxi_ahub_daudio->moduleclk);
#else
	sunxi_ahub_daudio->moduleclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(sunxi_ahub_daudio->moduleclk)) {
		dev_err(&pdev->dev, "moduleclk get failed\n");
		ret = PTR_ERR(sunxi_ahub_daudio->moduleclk);
		goto err_pllclk_put;
	}

	if (clk_set_parent(sunxi_ahub_daudio->moduleclk,
				sunxi_ahub_daudio->pllclk)) {
		dev_err(&pdev->dev, "set parent of moduleclk to pllclk fail\n");
		ret = -EBUSY;
		goto err_moduleclk_put;
	}
	clk_prepare_enable(sunxi_ahub_daudio->pllclk);
	clk_prepare_enable(sunxi_ahub_daudio->moduleclk);
#endif

	ret = of_property_read_u32(np, "tdm_num", &temp_val);
	if (ret < 0) {
		dev_err(&pdev->dev, "tdm_num configuration invalid\n");
		goto err_moduleclk_put;
	} else {
		sunxi_ahub_daudio->tdm_num = temp_val;
	}

	ret = of_property_read_u32(np, "pinconfig", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pinconfig configuration invalid\n");
		sunxi_ahub_daudio->pinconfig = 0;
	} else {
		sunxi_ahub_daudio->pinconfig = temp_val;
	}

	ret = of_property_read_u32(np, "frametype", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "frametype configuration invalid\n");
		sunxi_ahub_daudio->frame_type = 0;
	} else {
		sunxi_ahub_daudio->frame_type = temp_val;
	}

	ret = of_property_read_u32(np, "daudio_master", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "daudio_master configuration invalid\n");
		sunxi_ahub_daudio->daudio_master = 4;
	} else {
		sunxi_ahub_daudio->daudio_master = temp_val;
	}

	ret = of_property_read_u32(np, "pcm_lrck_period", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pcm_lrck_period configuration invalid\n");
		sunxi_ahub_daudio->pcm_lrck_period = 0;
	} else {
		sunxi_ahub_daudio->pcm_lrck_period = temp_val;
	}

	ret = of_property_read_u32(np, "slot_width_select", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "slot_width_select configuration inval\n");
		sunxi_ahub_daudio->slot_width_select = 0;
	} else {
		sunxi_ahub_daudio->slot_width_select = temp_val;
	}

	ret = of_property_read_u32(np, "audio_format", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "audio_format configuration invalid\n");
		sunxi_ahub_daudio->audio_format = 1;
	} else {
		sunxi_ahub_daudio->audio_format = temp_val;
	}

	ret = of_property_read_u32(np, "signal_inversion", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "signal_inversion configuration inval\n");
		sunxi_ahub_daudio->signal_inversion = 1;
	} else {
		sunxi_ahub_daudio->signal_inversion = temp_val;
	}

	ret = of_property_read_u32(np, "tdm_config", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "tdm_config configuration invalid\n");
		sunxi_ahub_daudio->tdm_config = 1;
	} else {
		sunxi_ahub_daudio->tdm_config = temp_val;
	}

	ret = of_property_read_u32(np, "mclk_div", &temp_val);
	if (ret < 0)
		sunxi_ahub_daudio->mclk_div = 0;
	else
		sunxi_ahub_daudio->mclk_div = temp_val;

	sunxi_ahub_daudio->regmap = sunxi_ahub_regmap_init(pdev);
	if (!sunxi_ahub_daudio->regmap) {
		dev_err(&pdev->dev, "regmap not init ok\n");
		ret = -ENOMEM;
		goto err_moduleclk_put;
	}

	/* diff handle for dais  even we use tdm_num as dai id  ----->roy */
	sunxi_ahub_daudio->cpudai->id = sunxi_ahub_daudio->tdm_num;
	sprintf(sunxi_ahub_daudio->cpudai_name, "sunxi-ahub-cpu-aif%d",
					sunxi_ahub_daudio->tdm_num);
	sunxi_ahub_daudio->cpudai->name = sunxi_ahub_daudio->cpudai_name;

	ret = snd_soc_register_component(&pdev->dev,
			&sunxi_ahub_daudio_component,
			sunxi_ahub_daudio->cpudai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -EBUSY;
		goto err_moduleclk_put;
	}
	return 0;

err_moduleclk_put:
	clk_put(sunxi_ahub_daudio->moduleclk);
err_pllclk_put:
	clk_put(sunxi_ahub_daudio->pllclk);
err_cpudai_kfree:
	kfree(sunxi_ahub_daudio->cpudai);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_ahub_daudio);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_ahub_daudio_dev_remove(struct platform_device *pdev)
{
	struct sunxi_ahub_daudio_priv *sunxi_ahub_daudio =
				dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	clk_put(sunxi_ahub_daudio->moduleclk);
#ifdef AHUB_PLL_AUDIO_X4
	clk_put(sunxi_ahub_daudio->pllclkx4);
#endif
	clk_put(sunxi_ahub_daudio->pllclk);
	return 0;
}

static const struct of_device_id sunxi_ahub_daudio_of_match[] = {
	{ .compatible = "allwinner,sunxi-ahub-daudio", },
	{},
};

static struct platform_driver sunxi_ahub_daudio_driver = {
	.probe = sunxi_ahub_daudio_dev_probe,
	.remove = __exit_p(sunxi_ahub_daudio_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_ahub_daudio_of_match,
	},
};

module_platform_driver(sunxi_ahub_daudio_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI Audio Hub ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-ahub");
