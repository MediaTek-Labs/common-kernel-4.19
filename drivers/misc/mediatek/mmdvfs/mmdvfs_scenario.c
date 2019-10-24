// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

struct mmdvfs_scen_data {
	struct regulator *reg;
	int high_volt;
};

static ssize_t scen_voltage_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	s32 ret;
	u32 voltage = 0;
	struct mmdvfs_scen_data *scen_data = dev_get_drvdata(dev);

	if (!scen_data || IS_ERR_OR_NULL(scen_data->reg)) {
		dev_notice(dev, "%s fail\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoint(buf, 10, &voltage);
	if (ret) {
		dev_notice(dev, "wrong scen voltage string:%d\n", ret);
		return ret;
	}

	ret = regulator_set_voltage(scen_data->reg,
				    voltage, scen_data->high_volt);
	if (ret) {
		dev_notice(dev, "set regulator volt fail:%d\n", ret);
		return ret;
	}

	dev_notice(dev, "%s set voltage=%d\n", __func__, voltage);

	return count;
}
static DEVICE_ATTR_WO(scen_voltage);

static int mmdvfs_scen_probe(struct platform_device *pdev)
{
	struct mmdvfs_scen_data *scen_data;
	s32 ret, count;

	scen_data = devm_kzalloc(&pdev->dev, sizeof(*scen_data), GFP_KERNEL);
	if (!scen_data)
		return -ENOMEM;

	scen_data->reg = devm_regulator_get(&pdev->dev, "dvfsrc-vcore");
	if (IS_ERR(scen_data->reg)) {
		dev_notice(&pdev->dev, "get regulator fail:%ld\n",
			   PTR_ERR(scen_data->reg));
		return PTR_ERR(scen_data->reg);
	}

	count = regulator_count_voltages(scen_data->reg);
	if (count <= 0) {
		dev_notice(&pdev->dev, "get voltages fail:%d\n", count);
		return -EINVAL;
	}

	scen_data->high_volt = regulator_list_voltage(scen_data->reg, count-1);
	if (scen_data->high_volt <= 0) {
		dev_notice(&pdev->dev, "get high voltage fail:%d\n",
			   scen_data->high_volt);
		return -EINVAL;
	}

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_scen_voltage.attr);
	if (ret)
		dev_notice(&pdev->dev, "sysfs_create_file fail:%d\n", ret);
	dev_set_drvdata(&pdev->dev, scen_data);
	return ret;
}

static int mmdvfs_scen_remove(struct platform_device *pdev)
{
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_scen_voltage.attr);
	return 0;
}

static const struct of_device_id mmdvfs_scen_of_ids[] = {
	{
		.compatible = "mediatek,mmdvfs-scen",
	},
	{}
};

static struct platform_driver mmdvfs_scen_driver = {
	.probe = mmdvfs_scen_probe,
	.remove = mmdvfs_scen_remove,
	.driver = {
		.name = "mmdvfs_scen",
		.of_match_table = mmdvfs_scen_of_ids,
	},
};
module_platform_driver(mmdvfs_scen_driver);

MODULE_DESCRIPTION("MTK MMDVFS Scenario Driver");
MODULE_AUTHOR("Anthony Huang<anthony.huang@mediatek.com>");
MODULE_LICENSE("GPL v2");
