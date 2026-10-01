// SPDX-License-Identifier: GPL-2.0
/*
 * Minimal MT6379 charger driver for the realme GT7 (MT6991) mainline port.
 *
 * The MT6379 is an SPMI PMIC (USID 0x0e) whose charger block the bootloader
 * already enables (BUCK_EN + CHG_EN, sane CV/MIVR), but it leaves IBUS_AICR at
 * the 500 mA USB-SDP default.  On this phone the screen-on system draw is
 * ~580 mA, so the battery gets ~0 until the input current limit is raised.
 *
 * The charger re-runs adapter detection on every plug and resets AICR, so a
 * one-shot write at boot is not enough: this driver re-asserts the limits on a
 * poll timer and exposes them (plus VBUS presence / charge state) through the
 * power_supply class.
 *
 * Register map and encodings come from the vendor mt6379-charger.c:
 *   0x70  CHG_STAT0   bit0 = PWR_RDY (VBUS present)
 *   0x121 CHG_STAT    bits[3:0] = charger state (0 sleep .. 7 done, 15 otg)
 *   0x109 IBUS_AICR   100000 + val*25000 uA
 *   0x10f ICHG        300000 + val*50000 uA
 *   0x114 CHG_WDT     bit5 BUCK_EN, bit4 CHG_EN, bit3 WDT_EN
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/spmi.h>
#include <linux/workqueue.h>

#define MT6379_REG_CHG_STAT0		0x70
#define MT6379_REG_CHG_STAT		0x121
#define MT6379_REG_CHG_IBUS_AICR	0x109
#define MT6379_REG_CHG_ICHG		0x10f

#define MT6379_CHG_STAT_DONE		7
#define MT6379_CHG_STAT_FAULT		8
#define MT6379_CHG_STAT_OTG		15

#define MT6379_AICR_MIN			100000
#define MT6379_AICR_MAX			4275000
#define MT6379_AICR_STEP		25000
#define MT6379_ICHG_MIN			300000
#define MT6379_ICHG_MAX			4300000
#define MT6379_ICHG_STEP		50000

#define MT6379_POLL_MS			3000

struct mt6379_chg {
	struct device *dev;
	struct regmap *regmap;
	struct power_supply *psy;
	struct delayed_work poll;
	u32 aicr;	/* uA */
	u32 ichg;	/* uA */
	bool online;
};

static int mt6379_set_aicr(struct mt6379_chg *c, u32 ua)
{
	u32 v;

	if (ua < MT6379_AICR_MIN)
		ua = MT6379_AICR_MIN;
	v = (ua - MT6379_AICR_MIN) / MT6379_AICR_STEP;
	if (v > 0xa7)
		v = 0xa7;

	return regmap_write(c->regmap, MT6379_REG_CHG_IBUS_AICR, v);
}

static int mt6379_set_ichg(struct mt6379_chg *c, u32 ua)
{
	u32 v;

	if (ua < MT6379_ICHG_MIN)
		ua = MT6379_ICHG_MIN;
	v = (ua - MT6379_ICHG_MIN) / MT6379_ICHG_STEP;
	if (v > 0x50)
		v = 0x50;

	return regmap_write(c->regmap, MT6379_REG_CHG_ICHG, v);
}

static int mt6379_apply(struct mt6379_chg *c)
{
	int ret;

	ret = mt6379_set_aicr(c, c->aicr);
	if (ret)
		return ret;

	return mt6379_set_ichg(c, c->ichg);
}

static void mt6379_poll_work(struct work_struct *work)
{
	struct mt6379_chg *c = container_of(work, struct mt6379_chg,
					    poll.work);
	unsigned int stat0 = 0;
	bool online;

	if (!regmap_read(c->regmap, MT6379_REG_CHG_STAT0, &stat0)) {
		online = !!(stat0 & BIT(0));
		if (online != c->online) {
			c->online = online;
			dev_info(c->dev, "VBUS %s\n",
				 online ? "present" : "absent");
			power_supply_changed(c->psy);
		}

		/* The charger resets AICR on every plug/re-detection. */
		mt6379_apply(c);
	}

	schedule_delayed_work(&c->poll, msecs_to_jiffies(MT6379_POLL_MS));
}

static enum power_supply_property mt6379_chg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
};

static int mt6379_chg_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct mt6379_chg *c = power_supply_get_drvdata(psy);
	unsigned int v;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = c->online;
		return 0;
	case POWER_SUPPLY_PROP_STATUS:
		if (!c->online) {
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			return 0;
		}
		if (regmap_read(c->regmap, MT6379_REG_CHG_STAT, &v))
			v = MT6379_CHG_STAT_DONE;
		switch (v & 0xf) {
		case MT6379_CHG_STAT_DONE:
			val->intval = POWER_SUPPLY_STATUS_FULL;
			break;
		case MT6379_CHG_STAT_FAULT:
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		case MT6379_CHG_STAT_OTG:
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		default:
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
			break;
		}
		return 0;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		val->intval = c->aicr;
		return 0;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = c->ichg;
		return 0;
	default:
		return -EINVAL;
	}
}

static int mt6379_chg_set_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   const union power_supply_propval *val)
{
	struct mt6379_chg *c = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		c->aicr = val->intval;
		return mt6379_set_aicr(c, c->aicr);
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		c->ichg = val->intval;
		return mt6379_set_ichg(c, c->ichg);
	default:
		return -EINVAL;
	}
}

static const struct power_supply_desc mt6379_chg_desc = {
	.name			= "mt6379-charger",
	.type			= POWER_SUPPLY_TYPE_USB,
	.properties		= mt6379_chg_props,
	.num_properties		= ARRAY_SIZE(mt6379_chg_props),
	.get_property		= mt6379_chg_get_property,
	.set_property		= mt6379_chg_set_property,
};

static const struct regmap_config mt6379_regmap_config = {
	.reg_bits	= 16,
	.val_bits	= 8,
	.max_register	= 0x1ffff,
};

static int mt6379_chg_probe(struct spmi_device *sdev)
{
	struct power_supply_config cfg = {};
	struct mt6379_chg *c;

	c = devm_kzalloc(&sdev->dev, sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	c->dev = &sdev->dev;
	c->regmap = devm_regmap_init_spmi_ext(sdev, &mt6379_regmap_config);
	if (IS_ERR(c->regmap))
		return dev_err_probe(c->dev, PTR_ERR(c->regmap),
				     "failed to init SPMI regmap\n");

	/* Conservative 5V default; the sysfs/power_supply can raise these. */
	c->aicr = 3000000;
	c->ichg = 3000000;

	cfg.drv_data = c;
	cfg.fwnode = dev_fwnode(c->dev);
	c->psy = devm_power_supply_register(c->dev, &mt6379_chg_desc, &cfg);
	if (IS_ERR(c->psy))
		return dev_err_probe(c->dev, PTR_ERR(c->psy),
				     "failed to register power supply\n");

	spmi_device_set_drvdata(sdev, c);

	mt6379_apply(c);
	INIT_DELAYED_WORK(&c->poll, mt6379_poll_work);
	schedule_delayed_work(&c->poll, msecs_to_jiffies(MT6379_POLL_MS));

	dev_info(c->dev, "MT6379 charger registered (AICR %u uA, ICHG %u uA)\n",
		 c->aicr, c->ichg);

	return 0;
}

static void mt6379_chg_remove(struct spmi_device *sdev)
{
	struct mt6379_chg *c = spmi_device_get_drvdata(sdev);

	cancel_delayed_work_sync(&c->poll);
}

static const struct of_device_id mt6379_chg_of_match[] = {
	{ .compatible = "mediatek,mt6379" },
	{ }
};
MODULE_DEVICE_TABLE(of, mt6379_chg_of_match);

static struct spmi_driver mt6379_chg_driver = {
	.probe = mt6379_chg_probe,
	.remove = mt6379_chg_remove,
	.driver = {
		.name = "mt6379-charger",
		.of_match_table = mt6379_chg_of_match,
	},
};
module_spmi_driver(mt6379_chg_driver);

MODULE_AUTHOR("realme GT7 mainline port");
MODULE_DESCRIPTION("MT6379 5V charger driver (MT6991)");
MODULE_LICENSE("GPL");
