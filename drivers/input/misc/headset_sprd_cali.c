/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/headset_sprd_cali.h>
#include <mach/board.h>
#include <linux/input.h>

#include <mach/adc.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/adi.h>
#include <linux/module.h>
#include <linux/wakelock.h>

#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#include <mach/sci_glb_regs.h>
#include <mach/arch_misc.h>

//#define SPRD_HEADSET_DBG
//#define SPRD_HEADSET_REG_DUMP
#define SPRD_HEADSET_SYS_SUPPORT
#define SPRD_HEADSET_AUXADC_CAL_NO         0
#define SPRD_HEADSET_AUXADC_CAL_NV         1
#define SPRD_HEADSET_AUXADC_CAL_CHIP       2

#ifdef SPRD_HEADSET_DBG
#define ENTER printk(KERN_INFO "[SPRD_HEADSET_DBG][%d] func: %s  line: %04d\n", adie_type, __func__, __LINE__);
#define PRINT_DBG(format,x...)  printk(KERN_INFO "[SPRD_HEADSET_DBG][%d] " format, adie_type, ## x)
#define PRINT_INFO(format,x...)  printk(KERN_INFO "[SPRD_HEADSET_INFO][%d] " format, adie_type, ## x)
#define PRINT_WARN(format,x...)  printk(KERN_INFO "[SPRD_HEADSET_WARN][%d] " format, adie_type, ## x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[SPRD_HEADSET_ERR][%d] func: %s  line: %04d  info: " format, adie_type, __func__, __LINE__, ## x)
#else
#define ENTER
#define PRINT_DBG(format,x...)
#define PRINT_INFO(format,x...)  printk(KERN_INFO "[SPRD_HEADSET_INFO][%d] " format, adie_type, ## x)
#define PRINT_WARN(format,x...)  printk(KERN_INFO "[SPRD_HEADSET_WARN][%d] " format, adie_type, ## x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[SPRD_HEADSET_ERR][%d] func: %s  line: %04d  info: " format, adie_type, __func__, __LINE__, ## x)
#endif

#if (defined(CONFIG_ARCH_SCX15))
        #define ADIE_CHID_LOW 0x0134
        #define ADIE_CHID_HIGH 0x0138
#else
        #if (defined(CONFIG_ARCH_SCX35))
                #define ADIE_CHID_LOW 0x0108
                #define ADIE_CHID_HIGH 0x010C
        #endif
#endif

#if (defined(CONFIG_ARCH_SCX15))
        #define CLK_AUD_HID_EN (BIT(4))
        #define CLK_AUD_HBD_EN (BIT(3))
#else
        #if (defined(CONFIG_ARCH_SCX35))
                #define CLK_AUD_HID_EN (BIT(5))
                #define CLK_AUD_HBD_EN (BIT(4))
        #endif
#endif

#define ADC_READ_COUNT_BUTTON (1)
#define ADC_READ_COUNT_DETECT (3)
#define ADC_GND (100)
#define DEBANCE_LOOP_COUNT_TYPE_DETECT (5)
#define DEBANCE_LOOP_COUNT_BUTTON_DETECT (1)
#define ADC_CHECK_INTERVAL_TYPE_DETECT (5)
#define ADC_CHECK_INTERVAL_BUTTON_DETECT (0)
#define ADC_DEBANCE_VALUE (300)

#define HEADMIC_DETECT_BASE (ANA_AUDCFGA_INT_BASE)
#define HEADMIC_DETECT_REG(X) (HEADMIC_DETECT_BASE + (X))

#define HDT_EN (BIT(5))
#define AUD_EN (BIT(4))

#define HEADMIC_BUTTON_BASE (ANA_HDT_INT_BASE)
#define HEADMIC_BUTTON_REG(X) (HEADMIC_BUTTON_BASE + (X))
#define HID_CFG0 (0x0080)
#define HID_CFG2 (0x0088)
#define HID_CFG3 (0x008C)
#define HID_CFG4 (0x0090)

#define ANA_CFG0 (0x0040)
#define ANA_CFG1 (0x0044)
#define ANA_CFG2 (0x0048)
#define ANA_CFG20 (0x00A0)
#define ANA_STS0 (0x00C0)

#define HEADMIC_DETECT_GLB_REG(X) (ANA_REGS_GLB_BASE + (X))

#define HID_TMR_T1T2_STEP_SHIFT (5)
#define HID_TMR_T0_SHIFT (0)
#define HID_TMR_T1_SHIFT (0)
#define HID_TMR_T2_SHIFT (0)

#define HID_TMR_T1T2_STEP_MASK (0x1F << HID_TMR_T1T2_STEP_SHIFT)
#define HID_TMR_T0_MASK (0x1F << HID_TMR_T0_SHIFT)
#define HID_TMR_T1_MASK (0xFFFF << HID_TMR_T1_SHIFT)
#define HID_TMR_T2_MASK (0xFFFF << HID_TMR_T2_SHIFT)

#define HID_TMR_T1T2_STEP_VAL (0x1)
#define HID_TMR_T0_VAL (0xF)
#define HID_TMR_T1_VAL (0xF)
#define HID_TMR_T2_VAL (0xF)

#define AUDIO_MICBIAS_HV_EN (BIT(2))
#define AUDIO_MICBIAS_V_SHIFT (3)
#define AUDIO_MICBIAS_V_MASK (0x3 << AUDIO_MICBIAS_V_SHIFT)
#define AUDIO_MICBIAS_V_1P7_OR_2P2 (0)
#define AUDIO_MICBIAS_V_1P9_OR_2P4 (1)
#define AUDIO_MICBIAS_V_2P1_OR_2P7 (2)
#define AUDIO_MICBIAS_V_2P3_OR_3P0 (3)

#if (defined(CONFIG_ARCH_SCX15))
        #define AUDIO_HEAD_SDET_SHIFT (4)
#else
        #if (defined(CONFIG_ARCH_SCX35))
                #define AUDIO_HEAD_SDET_SHIFT (5)
        #endif
#endif
#define AUDIO_HEAD_SDET_MASK (0x3 << AUDIO_HEAD_SDET_SHIFT)
#define AUDIO_HEAD_SDET_2P1_OR_1P4 (3)
#define AUDIO_HEAD_SDET_2P3_OR_1P5 (2)
#define AUDIO_HEAD_SDET_2P5_OR_1P6 (1)
#define AUDIO_HEAD_SDET_2P7_OR_1P7 (0)

#if (defined(CONFIG_ARCH_SCX15))
        #define AUDIO_HEAD_INS_VREF_SHIFT (7)
#else
        #if (defined(CONFIG_ARCH_SCX35))
                #define AUDIO_HEAD_INS_VREF_SHIFT (8)
        #endif
#endif
#define AUDIO_HEAD_INS_VREF_MASK (0x3 << AUDIO_HEAD_INS_VREF_SHIFT)
#define AUDIO_HEAD_INS_VREF_2P1_OR_1P4 (3)
#define AUDIO_HEAD_INS_VREF_2P3_OR_1P5 (2)
#define AUDIO_HEAD_INS_VREF_2P5_OR_1P6 (1)
#define AUDIO_HEAD_INS_VREF_2P7_OR_1P7 (0)

#if (defined(CONFIG_ARCH_SCX15))
        #define AUDIO_HEAD_SBUT_SHIFT (0)
#else
        #if (defined(CONFIG_ARCH_SCX35))
                #define AUDIO_HEAD_SBUT_SHIFT (1)
        #endif
#endif
#define AUDIO_HEAD_SBUT_MASK (0xF << AUDIO_HEAD_SBUT_SHIFT)

#define AUDIO_HEADMIC_ADC_SEL (BIT(13))
#define AUDIO_HEAD_INS_HMBIAS_EN (BIT(11))

#if (defined(CONFIG_ARCH_SCX15))
        #define AUDIO_V2ADC_EN (BIT(15))
        #define AUDIO_HEAD_BUF_EN (BIT(14))
        #define AUDIO_HEAD2ADC_SEL_SHIFT (12)
        #define AUDIO_HEAD2ADC_SEL_MASK (0x3 << AUDIO_HEAD2ADC_SEL_SHIFT)
        #define AUDIO_HEAD2ADC_SEL_DISABLE (0)
        #define AUDIO_HEAD2ADC_SEL_MIC_IN (1)
        #define AUDIO_HEAD2ADC_SEL_L_INT (2)
        #define AUDIO_HEAD2ADC_SEL_DRO_L (3)
#else
        #if (defined(CONFIG_ARCH_SCX35))
                #define AUDIO_V2ADC_EN (BIT(14))
                #define AUDIO_HEAD_BUF_EN (BIT(15))
                #define AUDIO_HEAD2ADC_SEL_SHIFT (13)
                #define AUDIO_HEAD2ADC_SEL_MASK (0x1 << AUDIO_HEAD2ADC_SEL_SHIFT)
                #define AUDIO_HEAD2ADC_SEL_MIC_IN (1)
                #define AUDIO_HEAD2ADC_SEL_L_INT (0)
        #endif
#endif

#define ABS(x) (((x) < (0)) ? (-x) : (x))

#define headset_reg_read(addr)	\
    do {	\
	sci_adi_read(addr);	\
} while(0)

#define headset_reg_set_val(addr, val, mask, shift)  \
    do {    \
        uint32_t temp;    \
        temp = sci_adi_read(addr);  \
        temp = (temp & (~mask)) | ((val) << (shift));   \
        sci_adi_raw_write(addr, temp);    \
    } while(0)

#define headset_reg_clr_bit(addr, bit)   \
    do {    \
        uint32_t temp;    \
        temp = sci_adi_read(addr);  \
        temp = temp & (~bit);   \
        sci_adi_raw_write(addr, temp);  \
    } while(0)

#define headset_reg_set_bit(addr, bit)   \
    do {    \
        uint32_t temp;    \
        temp = sci_adi_read(addr);  \
        temp = temp | bit;  \
        sci_adi_raw_write(addr, temp);  \
    } while(0)


#ifdef SPRD_HEADSET_REG_DUMP
static struct delayed_work reg_dump_work;
static struct workqueue_struct *reg_dump_work_queue;
#endif

/***polling ana_sts0 to avoid the hardware defect***/
static struct delayed_work sts_check_work;
static struct workqueue_struct *sts_check_work_queue;
static int plug_state_class_g_on = 0;
static int sts_check_work_need_to_cancel = 1;
/***polling ana_sts0 to avoid the hardware defect***/

static DEFINE_SPINLOCK(headmic_bias_lock);
static DEFINE_SPINLOCK(irq_button_lock);
static DEFINE_SPINLOCK(irq_detect_lock);
static int adie_type = 0; //1=AC, 2=BA, 3=BB
static int gpio_detect_value_last = 0;
static int gpio_button_value_last = 0;
static int button_state_last = 0;
static int plug_state_last = 0; //if the hardware detected the headset is plug in, set plug_state_last = 1
static int active_status = 1;
static struct wake_lock headset_detect_wakelock;
static struct wake_lock headset_button_wakelock;
static struct semaphore headset_sem;
static struct sprd_headset headset = {
        .sdev = {
                .name = "h2w",
        },
};

static struct sprd_headset_power {
	struct regulator *head_mic;
	struct regulator *vcom_buf;
	struct regulator *vbo;
} sprd_hts_power;

static int sprdchg_adc3642_to_vol(int adcvalue);
static int sprd_headset_power_get(struct device *dev,
				  struct regulator **regu, const char *id)
{
	if (!*regu) {
		*regu = regulator_get(dev, id);
		if (IS_ERR(*regu)) {
			pr_err("ERR:Failed to request %ld: %s\n",
			       PTR_ERR(*regu), id);
			*regu = 0;
			return PTR_ERR(*regu);
		}
	}
	return 0;
}

static int sprd_headset_power_init(struct device *dev)
{
	int ret = 0;
	ret =
	    sprd_headset_power_get(dev, &sprd_hts_power.head_mic,
				   "HEADMICBIAS");
	if (ret) {
		sprd_hts_power.head_mic = 0;
		return ret;
	}

	ret = sprd_headset_power_get(dev, &sprd_hts_power.vcom_buf, "VCOM_BUF");
	if (ret) {
		sprd_hts_power.vcom_buf = 0;
		goto __err1;
	}

	ret = sprd_headset_power_get(dev, &sprd_hts_power.vbo, "VBO");
	if (ret) {
		sprd_hts_power.vbo = 0;
		goto __err2;
	}

	goto __ok;
__err2:
	regulator_put(sprd_hts_power.vcom_buf);
__err1:
	regulator_put(sprd_hts_power.head_mic);
__ok:
	return ret;
}

static void sprd_headset_power_deinit(void)
{
	regulator_put(sprd_hts_power.head_mic);
	regulator_put(sprd_hts_power.vcom_buf);
	regulator_put(sprd_hts_power.vbo);
}

static int sprd_headset_audio_block_is_running(struct device *dev)
{
	return regulator_is_enabled(sprd_hts_power.vcom_buf)
	    || regulator_is_enabled(sprd_hts_power.vbo);
}

static int sprd_headset_headmic_bias_control(struct device *dev, int on)
{
	int ret = 0;
	if (!sprd_hts_power.head_mic) {
		return -1;
	}
	if (on) {
		ret = regulator_enable(sprd_hts_power.head_mic);
	} else {
		ret = regulator_disable(sprd_hts_power.head_mic);
	}
	if (!ret) {
		/* Set HEADMIC_SLEEP when audio block closed */
		if (sprd_headset_audio_block_is_running(dev)) {
			ret =
			    regulator_set_mode(sprd_hts_power.head_mic,
					       REGULATOR_MODE_NORMAL);
		} else {
			ret =
			    regulator_set_mode(sprd_hts_power.head_mic,
					       REGULATOR_MODE_STANDBY);
		}
	}
	return ret;
}

/*  on = 0: open headmic detect circuit */
static void headset_detect_circuit(unsigned on)
{
        if (on) {
                headset_reg_clr_bit(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD_INS_HMBIAS_EN);
        } else {
                headset_reg_set_bit(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD_INS_HMBIAS_EN);
        }
}

static void headset_detect_clk_en(void)
{
        //address:0x4003_8800+0x00
        headset_reg_set_bit(HEADMIC_DETECT_GLB_REG(0x00), (HDT_EN | AUD_EN));
        //address:0x4003_8800+0x04
        headset_reg_set_bit(HEADMIC_DETECT_GLB_REG(0x04), (CLK_AUD_HID_EN | CLK_AUD_HBD_EN));
}

static void headset_detect_init(void)
{
        headset_detect_clk_en();
        headset_reg_set_bit(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD_BUF_EN);
        headset_reg_set_bit(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_V2ADC_EN);
        headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD2ADC_SEL_MIC_IN, AUDIO_HEAD2ADC_SEL_MASK, AUDIO_HEAD2ADC_SEL_SHIFT);
        /* set headset detect voltage */
        headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD_SDET_2P7_OR_1P7, AUDIO_HEAD_SDET_MASK, AUDIO_HEAD_SDET_SHIFT);
        headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD_INS_VREF_2P1_OR_1P4, AUDIO_HEAD_INS_VREF_MASK, AUDIO_HEAD_INS_VREF_SHIFT);
        /*set headmicbias voltage*/
        headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG0), AUDIO_MICBIAS_V_2P3_OR_3P0, AUDIO_MICBIAS_V_MASK, AUDIO_MICBIAS_V_SHIFT);
        headset_reg_set_bit(HEADMIC_DETECT_REG(ANA_CFG0), AUDIO_MICBIAS_HV_EN);
}

/* is_set = 1, headset_mic to AUXADC */
static void set_adc_to_headmic(unsigned is_set)
{
        if (is_set) {
                headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD2ADC_SEL_MIC_IN, AUDIO_HEAD2ADC_SEL_MASK, AUDIO_HEAD2ADC_SEL_SHIFT);
        } else {
                headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), AUDIO_HEAD2ADC_SEL_L_INT, AUDIO_HEAD2ADC_SEL_MASK, AUDIO_HEAD2ADC_SEL_SHIFT);
        }
}

static void headset_mic_level(int level)
{
        if (level)
                headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), 0x1, AUDIO_HEAD_SBUT_MASK, AUDIO_HEAD_SBUT_SHIFT);
        else
                headset_reg_set_val(HEADMIC_DETECT_REG(ANA_CFG20), 0xF, AUDIO_HEAD_SBUT_MASK, AUDIO_HEAD_SBUT_SHIFT);
}

static void headset_micbias_polling_en(int en)
{
        if(en) {
                //step 1: enable clk for register accessable
                headset_detect_clk_en();
                //step 2: start polling & set timer
                headset_reg_set_val(HEADMIC_BUTTON_REG(HID_CFG2), HID_TMR_T1T2_STEP_VAL, HID_TMR_T1T2_STEP_MASK, HID_TMR_T1T2_STEP_SHIFT);//step for T1 & T2 [9:5]
                headset_reg_set_val(HEADMIC_BUTTON_REG(HID_CFG2), HID_TMR_T0_VAL, HID_TMR_T0_MASK, HID_TMR_T0_SHIFT);//T0 timer count [4:0]
                headset_reg_set_val(HEADMIC_BUTTON_REG(HID_CFG3), HID_TMR_T1_VAL, HID_TMR_T1_MASK, HID_TMR_T1_SHIFT);//T1 timer count [15:0]
                headset_reg_set_val(HEADMIC_BUTTON_REG(HID_CFG4), HID_TMR_T2_VAL, HID_TMR_T2_MASK, HID_TMR_T2_SHIFT);//T2 timer count [15:0]
                headset_reg_set_bit(HEADMIC_BUTTON_REG(HID_CFG0), BIT(0));//polling enable [0]
                //step 3: disable headmicbias
                headset_reg_clr_bit(HEADMIC_DETECT_REG(ANA_CFG0), BIT(5));
                //headset_reg_set_bit(HEADMIC_DETECT_REG(ANA_CFG0), BIT(1));
                PRINT_INFO("headmicbias polling enable\n");
                PRINT_DBG("ANA_CFG0(0x%08X)  HID_CFG0(0x%08X)  HID_CFG2(0x%08X)  HID_CFG3(0x%08X)  HID_CFG4(0x%08X)\n",
                          sci_adi_read(HEADMIC_DETECT_REG(ANA_CFG0)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG0)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG2)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG3)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG4)));
        } else {
                //step 1: enable headmicbias
                //headset_reg_clr_bit(HEADMIC_DETECT_REG(ANA_CFG0), BIT(1));
                headset_reg_set_bit(HEADMIC_DETECT_REG(ANA_CFG0), BIT(5));
                //step 2: stop polling
                headset_reg_clr_bit(HEADMIC_BUTTON_REG(HID_CFG0), BIT(0));
                PRINT_INFO("headmicbias polling disable\n");
                PRINT_DBG("ANA_CFG0(0x%08X)  HID_CFG0(0x%08X)  HID_CFG2(0x%08X)  HID_CFG3(0x%08X)  HID_CFG4(0x%08X)\n",
                          sci_adi_read(HEADMIC_DETECT_REG(ANA_CFG0)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG0)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG2)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG3)),
                          sci_adi_read(HEADMIC_BUTTON_REG(HID_CFG4)));
        }
}

static void headset_irq_button_enable(int enable, unsigned int irq)
{
        unsigned long spin_lock_flags;
        static int current_irq_state = 1;//irq is enabled after request_irq()

        spin_lock_irqsave(&irq_button_lock, spin_lock_flags);
        if (1 == enable) {
                if (0 == current_irq_state) {
                        enable_irq(irq);
                        current_irq_state = 1;
                }
        } else {
                if (1 == current_irq_state) {
                        disable_irq_nosync(irq);
                        current_irq_state = 0;
                }
        }
        spin_unlock_irqrestore(&irq_button_lock, spin_lock_flags);

        return;
}

static void headset_irq_detect_enable(int enable, unsigned int irq)
{
        unsigned long spin_lock_flags;
        static int current_irq_state = 1;//irq is enabled after request_irq()

        spin_lock_irqsave(&irq_detect_lock, spin_lock_flags);
        if (1 == enable) {
                if (0 == current_irq_state) {
                        enable_irq(irq);
                        current_irq_state = 1;
                }
        } else {
                if (1 == current_irq_state) {
                        disable_irq_nosync(irq);
                        current_irq_state = 0;
                }
        }
        spin_unlock_irqrestore(&irq_detect_lock, spin_lock_flags);

        return;
}

static void headmicbias_power_on(struct device *dev, int on)
{
	unsigned long spin_lock_flags;
	static int current_power_state = 0;

	spin_lock_irqsave(&headmic_bias_lock, spin_lock_flags);
	if (1 == on) {
		if (0 == current_power_state) {
			sprd_headset_headmic_bias_control(dev, 1);
			current_power_state = 1;
		}
	} else {
		if (1 == current_power_state) {
			sprd_headset_headmic_bias_control(dev, 0);
			current_power_state = 0;
		}
	}
	spin_unlock_irqrestore(&headmic_bias_lock, spin_lock_flags);

	return;
}

static int array_get_min(int* array, int size)
{
        int ret = 0;
        int i = 0;

        ret =  array[0];
        for(i=1; i<size; i++) {
                if(ret > array[i])
                        ret = array[i];
        }

        return ret;
}

static int array_get_max(int* array, int size)
{
        int ret = 0;
        int i = 0;

        ret =  array[0];
        for(i=1; i<size; i++) {
                if(ret < array[i])
                        ret = array[i];
        }

        return ret;
}

static SPRD_HEADSET_TYPE headset_type_detect(int last_gpio_detect_value)
{
        struct sprd_headset *ht = &headset;
        struct sprd_headset_platform_data *pdata = ht->platform_data;
        int i = 0;
        int j = 0;
        int adc_value = 0;
        int adc_mic_average = 0;
        int adc_left_average = 0;
        int adc_mic[DEBANCE_LOOP_COUNT_TYPE_DETECT] = {0};
        int adc_left[DEBANCE_LOOP_COUNT_TYPE_DETECT] = {0};
        int adc_max = 0;
        int adc_min = 0;
        int gpio_detect_value_current = 0;
		int vol_value = 0;

        ENTER

        if(0 != pdata->gpio_switch)
                gpio_direction_output(pdata->gpio_switch, 0);
        else
                PRINT_INFO("automatic type switch is unsupported\n");

        headset_mic_level(1);
        headset_detect_init();
        headset_detect_circuit(1);

        //get adc value of mic
        set_adc_to_headmic(1);
        msleep(50);
        for(i=0; i< DEBANCE_LOOP_COUNT_TYPE_DETECT; i++) {
                for (j = 0; j < ADC_READ_COUNT_DETECT; j++) {
                        adc_value = sci_adc_get_value(ADC_CHANNEL_HEADMIC, 0);
                        PRINT_DBG("adc_value[%d] = %d\n", j, adc_value);
                        vol_value = sprdchg_adc3642_to_vol(adc_value);
                        PRINT_DBG("sprdchg_adc3642_to_vol vol_value %d\n", vol_value);
                        adc_mic[i] += adc_value;
                        msleep(ADC_CHECK_INTERVAL_TYPE_DETECT);
                }
                adc_mic[i] = adc_mic[i] /ADC_READ_COUNT_DETECT;
                adc_mic_average += adc_mic[i];
                PRINT_DBG("the average adc_mic[%d] = %d\n", i, adc_mic[i]);

                gpio_detect_value_current = gpio_get_value(pdata->gpio_detect);
                if(gpio_detect_value_current != last_gpio_detect_value) {
                        PRINT_INFO("software debance (step 2: gpio check)!!!(headset_type_detect)(mic)\n");
                        goto out;
                }
        }
        adc_max = array_get_max(adc_mic, DEBANCE_LOOP_COUNT_TYPE_DETECT);
        adc_min = array_get_min(adc_mic, DEBANCE_LOOP_COUNT_TYPE_DETECT);
        if((adc_max - adc_min) > ADC_DEBANCE_VALUE) {
                PRINT_INFO("software debance (step 3: adc check)!!!(headset_type_detect)(mic)\n");
                goto out;
        }
        adc_mic_average = adc_mic_average / DEBANCE_LOOP_COUNT_TYPE_DETECT;

        //get adc value of left
    //  set_adc_to_headmic(0);
        msleep(50);
        for(i=0; i< DEBANCE_LOOP_COUNT_TYPE_DETECT; i++) {
                for (j = 0; j < ADC_READ_COUNT_DETECT; j++) {
                        adc_value = sci_adc_get_value(ADC_CHANNEL_HEADMIC, 0);
                        PRINT_DBG("adc_value[%d] = %d\n", j, adc_value);
                        vol_value = sprdchg_adc3642_to_vol(adc_value);
                        PRINT_DBG("sprdchg_adc3642_to_vol vol_value %d\n", vol_value);
                        adc_left[i] += adc_value;
                        msleep(ADC_CHECK_INTERVAL_TYPE_DETECT);
                }
                adc_left[i] = adc_left[i] /ADC_READ_COUNT_DETECT;
                adc_left_average += adc_left[i];
                PRINT_DBG("the average adc_left[%d] = %d\n", i, adc_left[i]);

                gpio_detect_value_current = gpio_get_value(pdata->gpio_detect);
                if(gpio_detect_value_current != last_gpio_detect_value) {
                        PRINT_INFO("software debance (step 2: gpio check)!!!(headset_type_detect)(left)\n");
                        goto out;
                }
        }
        adc_max = array_get_max(adc_left, DEBANCE_LOOP_COUNT_TYPE_DETECT);
        adc_min = array_get_min(adc_left, DEBANCE_LOOP_COUNT_TYPE_DETECT);
        if((adc_max - adc_min) > ADC_DEBANCE_VALUE) {
                PRINT_INFO("software debance (step 3: adc check)!!!(headset_type_detect)(left)\n");
                goto out;
        }
        adc_left_average = adc_left_average / DEBANCE_LOOP_COUNT_TYPE_DETECT;

        PRINT_INFO("adc_mic_average = %d\n", adc_mic_average);
        PRINT_INFO("adc_left_average = %d\n", adc_left_average);
#ifdef CONFIG_INPUT_SPRD_CALI_HEADSET
	if((adc_mic_average <= ht->platform_data->headset_buttons[3].adc_max))
#else
        if((adc_left_average < ADC_GND) && (adc_mic_average < ADC_GND))
#endif
        {
		set_adc_to_headmic(0);
                PRINT_DBG("no mic adc_mic_average: %d, ht->platform_data->headset_buttons[3].adc_max: %d\n",adc_mic_average, ht->platform_data->headset_buttons[3].adc_max);
		return HEADSET_NO_MIC;
	}
#ifdef CONFIG_INPUT_SPRD_CALI_HEADSET
        else if((adc_mic_average <= ht->platform_data->headset_buttons[4].adc_max) && ((adc_mic_average >= ht->platform_data->headset_buttons[4].adc_min)))
	{
		PRINT_DBG("4_pole having mic adc_mic_average: %d, ht->platform_data->headset_buttons[4].adc_max: %d,ht->platform_data->headset_buttons[4].adc_max:%d\n",adc_mic_average, ht->platform_data->headset_buttons[4].adc_max,ht->platform_data->headset_buttons[4].adc_min);
		return HEADSET_NORTH_AMERICA;
	}
#else
        else if((adc_left_average < ADC_GND) && (adc_mic_average > ADC_GND))
                return HEADSET_NORMAL;
#endif
        else if((adc_left_average > ADC_GND) && (adc_mic_average > ADC_GND)
                && (ABS(adc_mic_average - adc_left_average) < ADC_GND))
                return HEADSET_NORTH_AMERICA;
        else
                return HEADSET_TYPE_ERR;
out:
        //headset_irq_detect_enable(1, ht->irq_detect);
        return HEADSET_TYPE_ERR;
}

static void headset_button_work_func(struct work_struct *work)
{
        struct sprd_headset *ht = &headset;
        struct sprd_headset_platform_data *pdata = ht->platform_data;
        int gpio_button_value_current = 0;
        int button_state_current = 0;
        int adc_mic[DEBANCE_LOOP_COUNT_BUTTON_DETECT] = {0};
        int adc_mic_average = 0;
        int adc_value = 0;
        int adc_max = 0;
        int adc_min = 0;
        int i = 0;
        int j = 0;
		int vol_value = 0;
        static int current_key_code = KEY_RESERVED;

        down(&headset_sem);

        ENTER

        gpio_button_value_current = gpio_get_value(pdata->gpio_button);
        if(gpio_button_value_current != gpio_button_value_last) {
                PRINT_INFO("software debance (step 1: gpio check)!!!(headset_button_work_func)\n");
                goto out;
        }

        if(1 == pdata->irq_trigger_level_button) {
                if(1 == gpio_button_value_current)
                        button_state_current = 1;
                else
                        button_state_current = 0;
        } else {
                if(0 == gpio_button_value_current)
                        button_state_current = 1;
                else
                        button_state_current = 0;
        }

        if(1 == button_state_current) {//pressed!
                for(i=0; i< DEBANCE_LOOP_COUNT_BUTTON_DETECT; i++) {
                        for (j = 0; j < ADC_READ_COUNT_BUTTON; j++) {
                                adc_value = sci_adc_get_value(ADC_CHANNEL_HEADMIC, 0);
                                PRINT_DBG("adc_value[%d] = %d\n", j, adc_value);
								vol_value = sprdchg_adc3642_to_vol(adc_value);
								PRINT_DBG("sprdchg_adc3642_to_vol vol_value %d\n", vol_value);
                                adc_mic[i] += adc_value;
                                msleep(ADC_CHECK_INTERVAL_BUTTON_DETECT);
                        }
                        adc_mic[i] = adc_mic[i] /ADC_READ_COUNT_BUTTON;
                        adc_mic_average += adc_mic[i];
                        PRINT_DBG("the average adc_mic[%d] = %d\n", i, adc_mic[i]);

#if 0
                        gpio_button_value_current = gpio_get_value(pdata->gpio_button);
                        if(gpio_button_value_current != gpio_button_value_last) {
                                PRINT_INFO("software debance (step 2: gpio check)!!!(headset_button_work_func)(pressed)\n");
                                goto out;
                        }
#endif

                }

                adc_max = array_get_max(adc_mic, DEBANCE_LOOP_COUNT_BUTTON_DETECT);
                adc_min = array_get_min(adc_mic, DEBANCE_LOOP_COUNT_BUTTON_DETECT);
                if((adc_max - adc_min) > ADC_DEBANCE_VALUE) {
                        PRINT_INFO("software debance (step 3: adc check)!!!(headset_button_work_func)(pressed)\n");
                        goto out;
                }

                adc_mic_average = adc_mic_average / DEBANCE_LOOP_COUNT_BUTTON_DETECT;
                PRINT_INFO("adc_mic_average = %d\n", adc_mic_average);
                for (i = 0; i < ht->platform_data->nbuttons; i++) {
                        if (adc_mic_average >= ht->platform_data->headset_buttons[i].adc_min &&
                            adc_mic_average < ht->platform_data->headset_buttons[i].adc_max) {
                                current_key_code = ht->platform_data->headset_buttons[i].code;
                                break;
                        }
                        current_key_code = KEY_RESERVED;
                }

                if(0 == button_state_last) {
                        input_event(ht->input_dev, EV_KEY, current_key_code, 1);
                        input_sync(ht->input_dev);
                        button_state_last = 1;
                        PRINT_INFO("headset button pressed! current_key_code = %d(0x%04X)\n", current_key_code, current_key_code);
                } else
                        PRINT_ERR("headset button pressed already! current_key_code = %d(0x%04X)\n", current_key_code, current_key_code);

                if (1 == ht->platform_data->irq_trigger_level_button) {
                        irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_LOW);
                } else {
                        irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_HIGH);
                }
                headset_irq_button_enable(1, ht->irq_button);
        } else { //released!

#if 0
                for(i=0; i< DEBANCE_LOOP_COUNT_BUTTON_DETECT; i++) {
                        for (j = 0; j < ADC_READ_COUNT_BUTTON; j++) {
                                adc_value = sci_adc_get_value(ADC_CHANNEL_HEADMIC, 0);
                                PRINT_DBG("adc_value[%d] = %d\n", j, adc_value);
                                adc_mic[i] += adc_value;
                                msleep(ADC_CHECK_INTERVAL_BUTTON_DETECT);
                        }
                        adc_mic[i] = adc_mic[i] /ADC_READ_COUNT_BUTTON;
                        PRINT_DBG("the average adc_mic[%d] = %d\n", i, adc_mic[i]);

                        gpio_button_value_current = gpio_get_value(pdata->gpio_button);
                        if(gpio_button_value_current != gpio_button_value_last) {
                                PRINT_INFO("software debance (step 2: gpio check)!!!(headset_button_work_func)(released)\n");
                                goto out;
                        }
                }

                adc_max = array_get_max(adc_mic, DEBANCE_LOOP_COUNT_BUTTON_DETECT);
                adc_min = array_get_min(adc_mic, DEBANCE_LOOP_COUNT_BUTTON_DETECT);
                if((adc_max - adc_min) > ADC_DEBANCE_VALUE) {
                        PRINT_INFO("software debance (step 3: adc check)!!!(headset_button_work_func)(released)\n");
                        goto out;
                }
#endif

                if(1 == button_state_last) {
                        input_event(ht->input_dev, EV_KEY, current_key_code, 0);
                        input_sync(ht->input_dev);
                        button_state_last = 0;
                        PRINT_INFO("headset button released! current_key_code = %d(0x%04X)\n", current_key_code, current_key_code);
                } else
                        PRINT_ERR("headset button released already! current_key_code = %d(0x%04X)\n", current_key_code, current_key_code);

                if (1 == ht->platform_data->irq_trigger_level_button) {
                        irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_HIGH);
                } else {
                        irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_LOW);
                }
                headset_irq_button_enable(1, ht->irq_button);
        }
out:
        headset_irq_button_enable(1, ht->irq_button);
        wake_unlock(&headset_button_wakelock);
        up(&headset_sem);
        return;
}

#define to_device(x) container_of((x), struct device, platform_data)
static void headset_detect_work_func(struct work_struct *work)
{
        struct sprd_headset *ht = &headset;
        struct sprd_headset_platform_data *pdata = ht->platform_data;
        SPRD_HEADSET_TYPE headset_type;
        int plug_state_current = 0;
        int gpio_detect_value_current = 0;

        down(&headset_sem);

        ENTER

        if(0 == plug_state_last) {
                if(adie_type >= 3) {
			headmicbias_power_on(to_device(pdata), 1);
                }
        }
        msleep(100);
        gpio_detect_value_current = gpio_get_value(pdata->gpio_detect);
        PRINT_INFO("gpio_detect_value_current = %d, gpio_detect_value_last = %d, plug_state_last = %d\n",
                   gpio_detect_value_current, gpio_detect_value_last, plug_state_last);

        if(gpio_detect_value_current != gpio_detect_value_last) {
                PRINT_INFO("software debance (step 1)!!!(headset_detect_work_func)\n");
                goto out;
        }

        if(1 == pdata->irq_trigger_level_detect) {
                if(1 == gpio_detect_value_current)
                        plug_state_current = 1;
                else
                        plug_state_current = 0;
        } else {
                if(0 == gpio_detect_value_current)
                        plug_state_current = 1;
                else
                        plug_state_current = 0;
        }

        if(1 == plug_state_current && 0 == plug_state_last) {
                headset_type = headset_type_detect(gpio_detect_value_last);
                switch (headset_type) {
                case HEADSET_TYPE_ERR:
                        PRINT_INFO("headset_type = %d (HEADSET_TYPE_ERR)\n", headset_type);
                        goto out;
                case HEADSET_NORTH_AMERICA:
                        PRINT_INFO("headset_type = %d (HEADSET_NORTH_AMERICA)\n", headset_type);
                        if(0 != pdata->gpio_switch)
                                gpio_direction_output(pdata->gpio_switch, 1);
                        break;
                case HEADSET_NORMAL:
                        PRINT_INFO("headset_type = %d (HEADSET_NORMAL)\n", headset_type);
                        if(0 != pdata->gpio_switch)
                                gpio_direction_output(pdata->gpio_switch, 0);
                        break;
                case HEADSET_NO_MIC:
                        PRINT_INFO("headset_type = %d (HEADSET_NO_MIC)\n", headset_type);
                        if(0 != pdata->gpio_switch)
                                gpio_direction_output(pdata->gpio_switch, 0);
                        break;
                case HEADSET_APPLE:
                        PRINT_INFO("headset_type = %d (HEADSET_APPLE)\n", headset_type);
                        PRINT_INFO("we have not yet implemented this in the code\n");
                        break;
                default:
                        PRINT_INFO("headset_type = %d (HEADSET_UNKNOWN)\n", headset_type);
                        break;
                }

                if(headset_type == HEADSET_NO_MIC)
                        ht->headphone = 1;
                else
                        ht->headphone = 0;

                if (ht->headphone) {
                        headset_mic_level(0);
                        headset_irq_button_enable(0, ht->irq_button);

                        ht->type = BIT_HEADSET_NO_MIC;
                        switch_set_state(&ht->sdev, ht->type);
                        PRINT_INFO("headphone plug in (headset_detect_work_func)\n");
                } else {
                        if (1 == ht->platform_data->irq_trigger_level_button) {
                                irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_HIGH);
                        } else {
                                irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_LOW);
                        }
                        headset_mic_level(1);
                        headset_irq_button_enable(1, ht->irq_button);

                        ht->type = BIT_HEADSET_MIC;
                        switch_set_state(&ht->sdev, ht->type);
                        PRINT_INFO("headset plug in (headset_detect_work_func)\n");
                }

                /***polling ana_sts0 to avoid the hardware defect***/
                if (1 != adie_type) {
                        plug_state_class_g_on = 1;
                        sts_check_work_need_to_cancel = 0;
                        queue_delayed_work(sts_check_work_queue, &sts_check_work, msecs_to_jiffies(1000));
                }
                /***polling ana_sts0 to avoid the hardware defect***/

                plug_state_last = 1;
                if(1 == pdata->irq_trigger_level_detect)
                        irq_set_irq_type(ht->irq_detect, IRQF_TRIGGER_LOW);
                else
                        irq_set_irq_type(ht->irq_detect, IRQF_TRIGGER_HIGH);

                headset_irq_detect_enable(1, ht->irq_detect);
        } else if(0 == plug_state_current && 1 == plug_state_last) {

                headset_irq_button_enable(0, ht->irq_button);

                /***polling ana_sts0 to avoid the hardware defect***/
                if (1 != adie_type) {
                        sts_check_work_need_to_cancel = 1;
                        if(0 == plug_state_class_g_on) {
                                PRINT_INFO("plug out already!!!\n");
                                goto plug_out_already;
                        }
                        plug_state_class_g_on = 0;
                }
                /***polling ana_sts0 to avoid the hardware defect***/

                if (ht->headphone) {
                        PRINT_INFO("headphone plug out (headset_detect_work_func)\n");
                } else {
                        PRINT_INFO("headset plug out (headset_detect_work_func)\n");
                }
                ht->type = BIT_HEADSET_OUT;
                switch_set_state(&ht->sdev, ht->type);
plug_out_already:
                plug_state_last = 0;
                if(1 == pdata->irq_trigger_level_detect)
                        irq_set_irq_type(ht->irq_detect, IRQF_TRIGGER_HIGH);
                else
                        irq_set_irq_type(ht->irq_detect, IRQF_TRIGGER_LOW);

                headset_irq_detect_enable(1, ht->irq_detect);
        } else {
                PRINT_INFO("irq_detect must be enabled anyway!!!\n");
                goto out;
        }
out:

        if(0 == plug_state_last) {
                if(adie_type >= 3) {
			headmicbias_power_on(to_device(pdata), 0);
                        msleep(100);
                }
        }

        headset_irq_detect_enable(1, ht->irq_detect);
        wake_unlock(&headset_detect_wakelock);
        up(&headset_sem);
        return;
}

/***polling ana_sts0 to avoid the hardware defect***/
static void headset_sts_check_func(struct work_struct *work)
{
        int ana_sts0 = 0;
        int ana_sts0_debance = 0;
        struct sprd_headset *ht = &headset;
        struct sprd_headset_platform_data *pdata = ht->platform_data;
        SPRD_HEADSET_TYPE headset_type;

        down(&headset_sem);

        ENTER

        if(1 == sts_check_work_need_to_cancel)
                goto out;

        ana_sts0_debance = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_STS0);//arm base address:0x40038600
        msleep(100);
        ana_sts0 = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_STS0);//arm base address:0x40038600
        if((0x00000060 & ana_sts0) != (0x00000060 & ana_sts0_debance)) {
                PRINT_INFO("software debance!!!(headset_sts_check_func)\n");
                goto out;
        }

        if(((0x00000060 & ana_sts0) != 0x00000060) && (1 == plug_state_class_g_on)) {

                headset_irq_button_enable(0, ht->irq_button);

                if (ht->headphone) {
                        PRINT_INFO("headphone plug out (headset_sts_check_func)\n");
                } else {
                        PRINT_INFO("headset plug out (headset_sts_check_func)\n");
                }
                ht->type = BIT_HEADSET_OUT;
                switch_set_state(&ht->sdev, ht->type);
                plug_state_class_g_on = 0;
        }
        if(((0x00000060 & ana_sts0) == 0x00000060) && (0 == plug_state_class_g_on)) {
                headset_type = headset_type_detect(gpio_get_value(pdata->gpio_detect));
                switch (headset_type) {
                case HEADSET_TYPE_ERR:
                        PRINT_INFO("headset_type = %d (HEADSET_TYPE_ERR)\n", headset_type);
                        goto out;
                case HEADSET_NORTH_AMERICA:
                        PRINT_INFO("headset_type = %d (HEADSET_NORTH_AMERICA)\n", headset_type);
                        if(0 != pdata->gpio_switch)
                                gpio_direction_output(pdata->gpio_switch, 1);
                        break;
                case HEADSET_NORMAL:
                        PRINT_INFO("headset_type = %d (HEADSET_NORMAL)\n", headset_type);
                        if(0 != pdata->gpio_switch)
                                gpio_direction_output(pdata->gpio_switch, 0);
                        break;
                case HEADSET_NO_MIC:
                        PRINT_INFO("headset_type = %d (HEADSET_NO_MIC)\n", headset_type);
                        if(0 != pdata->gpio_switch)
                                gpio_direction_output(pdata->gpio_switch, 0);
                        break;
                case HEADSET_APPLE:
                        PRINT_INFO("headset_type = %d (HEADSET_APPLE)\n", headset_type);
                        PRINT_INFO("we have not yet implemented this in the code\n");
                        break;
                default:
                        PRINT_INFO("headset_type = %d (HEADSET_UNKNOWN)\n", headset_type);
                        break;
                }

                if(headset_type == HEADSET_NO_MIC)
                        ht->headphone = 1;
                else
                        ht->headphone = 0;

                if (ht->headphone) {
                        headset_mic_level(0);
                        headset_irq_button_enable(0, ht->irq_button);

                        ht->type = BIT_HEADSET_NO_MIC;
                        switch_set_state(&ht->sdev, ht->type);
                        PRINT_INFO("headphone plug in (headset_sts_check_func)\n");
                } else {
                        if (1 == ht->platform_data->irq_trigger_level_button) {
                                irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_HIGH);
                        } else {
                                irq_set_irq_type(ht->irq_button, IRQF_TRIGGER_LOW);
                        }
                        headset_mic_level(1);
                        headset_irq_button_enable(1, ht->irq_button);

                        ht->type = BIT_HEADSET_MIC;
                        switch_set_state(&ht->sdev, ht->type);
                        PRINT_INFO("headset plug in (headset_sts_check_func)\n");
                }
                plug_state_class_g_on = 1;
        }
out:
        if(0 == sts_check_work_need_to_cancel)
                queue_delayed_work(sts_check_work_queue, &sts_check_work, msecs_to_jiffies(1000));
        else
                PRINT_INFO("sts_check_work cancelled\n");

        up(&headset_sem);
        return;
}
/***polling ana_sts0 to avoid the hardware defect***/

#ifdef SPRD_HEADSET_REG_DUMP
static void reg_dump_func(struct work_struct *work)
{
        int gpio_detect = 0;
        int gpio_button = 0;

        int ana_sts0 = 0;
        int ana_cfg0 = 0;
        int ana_cfg1 = 0;
        int ana_cfg20 = 0;

        int hid_cfg0 = 0;
        int hid_cfg2 = 0;
        int hid_cfg3 = 0;
        int hid_cfg4 = 0;

        int arm_module_en = 0;
        int arm_clk_en = 0;
        int i = 0;
        int hbd[20] = {0};

        gpio_detect = gpio_get_value(headset.platform_data->gpio_detect);
        gpio_button = gpio_get_value(headset.platform_data->gpio_button);

        sci_adi_write(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_AUD_EN, BIT_ANA_AUD_EN);//arm base address:0x40038800 for register accessable
        ana_cfg0 = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_CFG0);//arm base address:0x40038600
        ana_cfg1 = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_CFG1);//arm base address:0x40038600
        ana_cfg20 = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_CFG20);//arm base address:0x40038600
        ana_sts0 = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_STS0);//arm base address:0x40038600

        sci_adi_write(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_HDT_EN, BIT_ANA_HDT_EN);//arm base address:0x40038800 for register accessable
        sci_adi_write(ANA_REG_GLB_ARM_CLK_EN, BIT_CLK_AUD_HID_EN, BIT_CLK_AUD_HID_EN);//arm base address:0x40038800 for register accessable

        hid_cfg2 = sci_adi_read(ANA_HDT_INT_BASE+HID_CFG2);//arm base address:0x40038700
        hid_cfg3 = sci_adi_read(ANA_HDT_INT_BASE+HID_CFG3);//arm base address:0x40038700
        hid_cfg4 = sci_adi_read(ANA_HDT_INT_BASE+HID_CFG4);//arm base address:0x40038700
        hid_cfg0 = sci_adi_read(ANA_HDT_INT_BASE+HID_CFG0);//arm base address:0x40038700

        arm_module_en = sci_adi_read(ANA_REG_GLB_ARM_MODULE_EN);
        arm_clk_en = sci_adi_read(ANA_REG_GLB_ARM_CLK_EN);

        PRINT_INFO("GPIO_%03d(det)=%d    GPIO_%03d(but)=%d\n",
                   headset.platform_data->gpio_detect, gpio_detect,
                   headset.platform_data->gpio_button, gpio_button);
        PRINT_INFO("arm_module_en|arm_clk_en|ana_cfg0  |ana_cfg1  |ana_cfg20 |ana_sts0  |hid_cfg2  |hid_cfg3  |hid_cfg4  |hid_cfg0\n");
        PRINT_INFO("0x%08X   |0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X\n",
                   arm_module_en, arm_clk_en, ana_cfg0, ana_cfg1, ana_cfg20, ana_sts0, hid_cfg2, hid_cfg3, hid_cfg4, hid_cfg0);
#if 0
        for(i=0; i<20; i++)
                hbd[i] = sci_adi_read(ANA_HDT_INT_BASE + (i*4));

        PRINT_INFO(" hbd_cfg0 | hbd_cfg1 | hbd_cfg2 | hbd_cfg3 | hbd_cfg4 | hbd_cfg5 | hbd_cfg6 | hbd_cfg7 | hbd_cfg8 | hbd_cfg9\n");
        PRINT_INFO("0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X\n",
                   hbd[0], hbd[1], hbd[2], hbd[3], hbd[4], hbd[5], hbd[6], hbd[7], hbd[8], hbd[9]);

        PRINT_INFO("hbd_cfg10 |hbd_cfg11 |hbd_cfg12 |hbd_cfg13 |hbd_cfg14 |hbd_cfg15 |hbd_cfg16 | hbd_sts0 | hbd_sts1 | hbd_sts2\n");
        PRINT_INFO("0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X|0x%08X\n",
                   hbd[10], hbd[11], hbd[12], hbd[13], hbd[14], hbd[15], hbd[16], hbd[17], hbd[18], hbd[19]);
#endif
        queue_delayed_work(reg_dump_work_queue, &reg_dump_work, msecs_to_jiffies(500));
        return;
}
#endif

static irqreturn_t headset_button_irq_handler(int irq, void *dev)
{
        struct sprd_headset *ht = dev;

        headset_irq_button_enable(0, ht->irq_button);
        wake_lock(&headset_button_wakelock);
        gpio_button_value_last = gpio_get_value(ht->platform_data->gpio_button);
        PRINT_DBG("headset_button_irq_handler: IRQ_%d(GPIO_%d) = %d\n",
                  ht->irq_button, ht->platform_data->gpio_button, gpio_button_value_last);
        queue_work(ht->button_work_queue, &ht->work_button);
        return IRQ_HANDLED;
}

static irqreturn_t headset_detect_irq_handler(int irq, void *dev)
{
        struct sprd_headset *ht = dev;

        headset_irq_detect_enable(0, ht->irq_detect);
        wake_lock(&headset_detect_wakelock);
        gpio_detect_value_last = gpio_get_value(ht->platform_data->gpio_detect);
        PRINT_DBG("headset_detect_irq_handler: IRQ_%d(GPIO_%d) = %d\n",
                  ht->irq_detect, ht->platform_data->gpio_detect, gpio_detect_value_last);
        queue_delayed_work(ht->detect_work_queue, &ht->work_detect, msecs_to_jiffies(0));
        return IRQ_HANDLED;
}

#ifdef SPRD_HEADSET_SYS_SUPPORT
/***create sys fs for debug***/
static int headset_suspend(struct platform_device *dev, pm_message_t state);
static int headset_resume(struct platform_device *dev);

static ssize_t headset_suspend_show(struct kobject *kobj, struct kobj_attribute *attr, char *buff)
{
        PRINT_INFO("headset_suspend_show. current active_status = %d\n", active_status);
        return sprintf(buff, "%d\n", active_status);
}

static ssize_t headset_suspend_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{
        pm_message_t pm_message_t_temp = {
                .event = 0,
        };
        unsigned long active = simple_strtoul(buff, NULL, 10);
        PRINT_INFO("headset_suspend_store. buff = %s\n", buff);
        PRINT_INFO("headset_suspend_store. set_val = %ld\n", active);
        if(0 == active && 1 == active_status) {
                headset_suspend(NULL, pm_message_t_temp);
                return len;
        }
        if(1 == active && 0 == active_status) {
                headset_resume(NULL);
                return len;
        }
        PRINT_ERR("suspend set ERROR!Maybe the current active state is alreay what you want!\n");
        return len;
}

static struct kobject *headset_suspend_kobj = NULL;
static struct kobj_attribute headset_suspend_attr =
        __ATTR(active, 0644, headset_suspend_show, headset_suspend_store);

static int headset_suspend_sysfs_init(void)
{
        int ret = -1;

        headset_suspend_kobj = kobject_create_and_add("headset", kernel_kobj);
        if (headset_suspend_kobj == NULL) {
                ret = -ENOMEM;
                PRINT_ERR("register sysfs failed. ret = %d\n", ret);
                return ret;
        }

        ret = sysfs_create_file(headset_suspend_kobj, &headset_suspend_attr.attr);
        if (ret) {
                PRINT_ERR("create sysfs failed. ret = %d\n", ret);
                return ret;
        }

        PRINT_INFO("headset_suspend_sysfs_init success\n");
        return ret;
}
/***create sys fs for debug***/
#endif

static int headset_detect_probe(struct platform_device *pdev)
{
        struct sprd_headset_platform_data *pdata = pdev->dev.platform_data;
        struct sprd_headset *ht = &headset;
        unsigned long irqflags = 0;
        struct input_dev *input_dev = NULL;
        int i = 0;
        int ret = -1;
        int ana_sts0 = 0;
        int adie_chip_id_low = 0;
        int adie_chip_id_high = 0;

        adie_chip_id_low = sci_adi_read(ANA_CTL_GLB_BASE+ADIE_CHID_LOW);//A-die chip id LOW
        adie_chip_id_high = sci_adi_read(ANA_CTL_GLB_BASE+ADIE_CHID_HIGH);//A-die chip id HIGH

        sci_adi_write(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_AUD_EN, BIT_ANA_AUD_EN);//arm base address:0x40038800 for register accessable
        ana_sts0 = sci_adi_read(ANA_AUDCFGA_INT_BASE+ANA_STS0);//arm base address:0x40038600

        if (0x2713 == adie_chip_id_high) {
                if (0xA000 == adie_chip_id_low)
                        adie_type = 1;//AC
                else if (0xA001 == adie_chip_id_low) {
                        if ((0x00000040 & ana_sts0) == 0x00000040)
                                adie_type = 2;//BA
                        else if ((0x00000040 & ana_sts0) == 0x00000000)
                                adie_type = 3;//BB
                }
        } else {
                if (0x2711 == adie_chip_id_high)
                        adie_type = 4;//Dolphin
                else
                        adie_type = 5;//unknow
        }

        ENTER

        ht->platform_data = pdata;

	ret = sprd_headset_power_init(&pdev->dev);
	if (ret)
		goto failed_to_request_gpio_switch;

        if(adie_type < 3)
		headmicbias_power_on(&pdev->dev, 1);
        msleep(5);//this time delay is necessary here

        PRINT_INFO("D-die chip id = 0x%08X\n", __raw_readl(REG_AON_APB_CHIP_ID));
        PRINT_INFO("A-die chip id HIGH = 0x%08X\n", adie_chip_id_high);
        PRINT_INFO("A-die chip id LOW = 0x%08X\n", adie_chip_id_low);

        if (1 == adie_type) {
                pdata->gpio_detect -= 1;
                PRINT_INFO("use EIC_AUD_HEAD_INST (EIC4, GPIO_%d) for insert detecting\n", pdata->gpio_detect);
        } else {
                PRINT_INFO("use EIC_AUD_HEAD_INST2 (EIC5, GPIO_%d) for insert detecting\n", pdata->gpio_detect);
        }

        if(0 != pdata->gpio_switch) {
                ret = gpio_request(pdata->gpio_switch, "headset_switch");
                if (ret < 0) {
                        PRINT_ERR("failed to request GPIO_%d(headset_switch)\n", pdata->gpio_switch);
                        goto failed_to_request_gpio_switch;
                }
        }
        else
                PRINT_INFO("automatic type switch is unsupported\n");

        ret = gpio_request(pdata->gpio_detect, "headset_detect");
        if (ret < 0) {
                PRINT_ERR("failed to request GPIO_%d(headset_detect)\n", pdata->gpio_detect);
                goto failed_to_request_gpio_detect;
        }

        ret = gpio_request(pdata->gpio_button, "headset_button");
        if (ret < 0) {
                PRINT_ERR("failed to request GPIO_%d(headset_button)\n", pdata->gpio_button);
                goto failed_to_request_gpio_button;
        }

        if(0 != pdata->gpio_switch)
                gpio_direction_output(pdata->gpio_switch, 0);
        gpio_direction_input(pdata->gpio_detect);
        gpio_direction_input(pdata->gpio_button);
        ht->irq_detect = gpio_to_irq(pdata->gpio_detect);
        ht->irq_button = gpio_to_irq(pdata->gpio_button);

        ret = switch_dev_register(&ht->sdev);
        if (ret < 0) {
                PRINT_ERR("switch_dev_register failed!\n");
                goto failed_to_register_switch_dev;
        }

        input_dev = input_allocate_device();
        if ( !input_dev) {
                PRINT_ERR("input_allocate_device for headset_button failed!\n");
                ret = -ENOMEM;
                goto failed_to_allocate_input_device;
        }

        input_dev->name = "headset-keyboard";
        input_dev->id.bustype = BUS_HOST;
        ht->input_dev = input_dev;

        for (i = 0; i < pdata->nbuttons; i++) {
                struct headset_buttons *buttons = &pdata->headset_buttons[i];
                unsigned int type = buttons->type ?: EV_KEY;
                input_set_capability(input_dev, type, buttons->code);
        }

        ret = input_register_device(input_dev);
        if (ret) {
                PRINT_ERR("input_register_device for headset_button failed!\n");
                goto failed_to_register_input_device;
        }

        sema_init(&headset_sem, 1);

        INIT_WORK(&ht->work_button, headset_button_work_func);
        ht->button_work_queue = create_singlethread_workqueue("headset_button");
        if(ht->button_work_queue == NULL) {
                PRINT_ERR("create_singlethread_workqueue for headset_button failed!\n");
                goto failed_to_create_singlethread_workqueue_for_headset_button;
        }

        INIT_DELAYED_WORK(&ht->work_detect, headset_detect_work_func);
        ht->detect_work_queue = create_singlethread_workqueue("headset_detect");
        if(ht->detect_work_queue == NULL) {
                PRINT_ERR("create_singlethread_workqueue for headset_detect failed!\n");
                goto failed_to_create_singlethread_workqueue_for_headset_detect;
        }

        /***polling ana_sts0 to avoid the hardware defect***/
        INIT_DELAYED_WORK(&sts_check_work, headset_sts_check_func);
        sts_check_work_queue = create_singlethread_workqueue("headset_sts_check");
        if(sts_check_work_queue == NULL) {
                PRINT_ERR("create_singlethread_workqueue for headset_sts_check failed!\n");
                goto failed_to_create_singlethread_workqueue_for_headset_sts_check;
        }
        /***polling ana_sts0 to avoid the hardware defect***/

#ifdef SPRD_HEADSET_REG_DUMP
        INIT_DELAYED_WORK(&reg_dump_work, reg_dump_func);
        reg_dump_work_queue = create_singlethread_workqueue("headset_reg_dump");
        if(reg_dump_work_queue == NULL) {
                PRINT_ERR("create_singlethread_workqueue for headset_reg_dump failed!\n");
                goto failed_to_create_singlethread_workqueue_for_headset_reg_dump;
        }
        queue_delayed_work(reg_dump_work_queue, &reg_dump_work, msecs_to_jiffies(500));
#endif

        wake_lock_init(&headset_detect_wakelock, WAKE_LOCK_SUSPEND, "headset_detect_wakelock");
        wake_lock_init(&headset_button_wakelock, WAKE_LOCK_SUSPEND, "headset_button_wakelock");

        irqflags = pdata->irq_trigger_level_button ? IRQF_TRIGGER_HIGH : IRQF_TRIGGER_LOW;
        ret = request_irq(ht->irq_button, headset_button_irq_handler, irqflags | IRQF_NO_SUSPEND, "headset_button", ht);
        if (ret) {
                PRINT_ERR("failed to request IRQ_%d(GPIO_%d)\n", ht->irq_button, pdata->gpio_button);
                goto failed_to_request_irq_headset_button;
        }
        headset_irq_button_enable(0, ht->irq_button);//disable button irq before headset detected

        irqflags = pdata->irq_trigger_level_detect ? IRQF_TRIGGER_HIGH : IRQF_TRIGGER_LOW;
        ret = request_irq(ht->irq_detect, headset_detect_irq_handler, irqflags | IRQF_NO_SUSPEND, "headset_detect", ht);
        if (ret < 0) {
                PRINT_ERR("failed to request IRQ_%d(GPIO_%d)\n", ht->irq_detect, pdata->gpio_detect);
                goto failed_to_request_irq_headset_detect;
        }

#ifdef SPRD_HEADSET_SYS_SUPPORT
        ret = headset_suspend_sysfs_init();
#endif

        PRINT_INFO("headset_detect_probe success\n");
        return ret;

failed_to_request_irq_headset_detect:
        free_irq(ht->irq_button, ht);
failed_to_request_irq_headset_button:

#ifdef SPRD_HEADSET_REG_DUMP
        cancel_delayed_work_sync(&reg_dump_work);
        destroy_workqueue(reg_dump_work_queue);
failed_to_create_singlethread_workqueue_for_headset_reg_dump:
#endif

        destroy_workqueue(sts_check_work_queue);
failed_to_create_singlethread_workqueue_for_headset_sts_check:
        destroy_workqueue(ht->detect_work_queue);
failed_to_create_singlethread_workqueue_for_headset_detect:
        destroy_workqueue(ht->button_work_queue);
failed_to_create_singlethread_workqueue_for_headset_button:
        input_unregister_device(input_dev);
failed_to_register_input_device:
        input_free_device(input_dev);
failed_to_allocate_input_device:
        switch_dev_unregister(&ht->sdev);
failed_to_register_switch_dev:
        gpio_free(pdata->gpio_button);
failed_to_request_gpio_button:
        gpio_free(pdata->gpio_detect);
failed_to_request_gpio_detect:
        if(0 != pdata->gpio_switch)
                gpio_free(pdata->gpio_switch);
failed_to_request_gpio_switch:

	headmicbias_power_on(&pdev->dev, 0);
        PRINT_ERR("headset_detect_probe failed\n");
        return ret;
}

#ifdef CONFIG_PM
static int headset_suspend(struct platform_device *dev, pm_message_t state)
{
        PRINT_INFO("plug_state_last = %d,  plug_state_class_g_on = %d\n", plug_state_last, plug_state_class_g_on);
        PRINT_INFO("suspend (det_irq=%d    but_irq=%d)\n", headset.irq_detect, headset.irq_button);
        active_status = 0;
        return 0;
}

static int headset_resume(struct platform_device *dev)
{
        PRINT_INFO("plug_state_last = %d,  plug_state_class_g_on = %d\n", plug_state_last, plug_state_class_g_on);
        PRINT_INFO("resume (det_irq=%d    but_irq=%d)\n", headset.irq_detect, headset.irq_button);
        active_status = 1;
        return 0;
}
#else
#define headset_suspend NULL
#define headset_resume NULL
#endif

struct sprd_headset_auxadc_cal adc_cal_hs = {
        4200, 3310,
        3600, 2832,
        400,  410,//theory
        SPRD_HEADSET_AUXADC_CAL_NO,
};

static int __init adc_cal_start(char *str)
{
        unsigned int adc_data[2] = { 0 };
        char *cali_data = &str[1];
        if (str) {
                pr_info("adc_cal%s!\n", str);
                sscanf(cali_data, "%d,%d", &adc_data[0], &adc_data[1]);
                pr_info("adc_data_hs: 0x%x 0x%x!\n", adc_data[0], adc_data[1]);
                adc_cal_hs.p0_vol = adc_data[0] & 0xffff;
                adc_cal_hs.p0_adc = (adc_data[0] >> 16) & 0xffff;
                adc_cal_hs.p1_vol = adc_data[1] & 0xffff;
                adc_cal_hs.p1_adc = (adc_data[1] >> 16) & 0xffff;
                adc_cal_hs.cal_type = SPRD_HEADSET_AUXADC_CAL_NV;
                printk
                    ("auxadc cal from cmdline ok!!! adc_data[0]: 0x%x, adc_data[1]:0x%x\n",
                     adc_data[0], adc_data[1]);
        }
        return 1;
}

__setup("adc_cal", adc_cal_start);

static int sprdchg_adc3642_to_vol(int adcvalue)
{
	int temp = 0;

	PRINT_DBG("sprdchg_adc3642_to_vol! p0_adc: %d, p1_adc:%d,p0_vol:%d,p1_vol:%d\n",
                     adc_cal_hs.p0_adc,adc_cal_hs.p1_adc, adc_cal_hs.p0_vol,adc_cal_hs.p1_vol);
    if(((adc_cal_hs.p0_adc - adc_cal_hs.p1_adc)==0) ||((adc_cal_hs.p1_adc - adc_cal_hs.p2_adc) == 0))
		return ;
    if(adc_cal_hs.cal_type == SPRD_HEADSET_AUXADC_CAL_NO ||((adc_cal_hs.cal_type == SPRD_HEADSET_AUXADC_CAL_CHIP)&&(adcvalue >= adc_cal_hs.p1_adc)))
	{
		temp = adc_cal_hs.p0_vol - adc_cal_hs.p1_vol;
		temp = temp * (adcvalue - adc_cal_hs.p0_adc);
		temp = temp / (adc_cal_hs.p0_adc - adc_cal_hs.p1_adc);
		temp = temp + adc_cal_hs.p0_vol;
    }else if((adc_cal_hs.cal_type == SPRD_HEADSET_AUXADC_CAL_CHIP)&&(adcvalue < adc_cal_hs.p1_adc))
    {
        temp = adc_cal_hs.p1_vol - adc_cal_hs.p2_vol;
		temp = temp * (adcvalue - adc_cal_hs.p1_adc);
		temp = temp / (adc_cal_hs.p1_adc - adc_cal_hs.p2_adc);
		temp = temp + adc_cal_hs.p1_vol;
	}

    temp = temp*133/198;
	return temp;
}


static struct platform_driver headset_detect_driver = {
        .driver = {
                .name = "headset-detect",
                .owner = THIS_MODULE,
        },
        .probe = headset_detect_probe,
        .suspend = headset_suspend,
        .resume = headset_resume,
};

static int __init headset_init(void)
{
        int ret;
        if (adc_cal_hs.cal_type == SPRD_HEADSET_AUXADC_CAL_NO) {
		extern int sci_efuse_get_cal(unsigned int * pdata, int num);
		unsigned int efuse_cal_data[3] = { 0 };
		if (!(sci_efuse_get_cal(efuse_cal_data,3))) {
			adc_cal_hs.p0_vol = efuse_cal_data[0] & 0xffff;
			adc_cal_hs.p0_adc = (efuse_cal_data[0] >> 16) & 0xffff;
			adc_cal_hs.p1_vol = efuse_cal_data[1] & 0xffff;
			adc_cal_hs.p1_adc = (efuse_cal_data[1] >> 16) & 0xffff;
			adc_cal_hs.p2_vol = efuse_cal_data[2] & 0xffff;
			adc_cal_hs.p2_adc = (efuse_cal_data[2] >> 16) & 0xffff;
			adc_cal_hs.cal_type = SPRD_HEADSET_AUXADC_CAL_CHIP;
			PRINT_DBG("auxadc cal from efuse ok!!! efuse_cal_data[0]: 0x%x, efuse_cal_data[1]:0x%x\n",
			     efuse_cal_data[0], efuse_cal_data[1]);
		}
	}
        ret = platform_driver_register(&headset_detect_driver);
        return ret;
}

static void __exit headset_exit(void)
{
	sprd_headset_power_deinit();
	platform_driver_unregister(&headset_detect_driver);
}

module_init(headset_init);
module_exit(headset_exit);

MODULE_DESCRIPTION("headset & button detect driver");
MODULE_AUTHOR("Yaochuan Li <yaochuan.li@spreadtrum.com>");
MODULE_LICENSE("GPL");
