// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * Author: Zhiyong Tao <zhiyong.tao@mediatek.com>
 *
 */

#include "pinctrl-mtk-mt8168.h"
#include "pinctrl-paris.h"

#define PIN_FIELD_BASE(_s_pin, _e_pin, _i_base, _s_addr,	\
			   _x_addrs, _s_bit, _x_bits)	\
	PIN_FIELD_CALC(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs,	\
			   _s_bit, _x_bits, 32, 0)

#define PINS_FIELD_BASE(_s_pin, _e_pin, _i_base, _s_addr,	\
			   _x_addrs, _s_bit, _x_bits)	\
	PIN_FIELD_CALC(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs,	\
				_s_bit, _x_bits, 32, 1)

#define PIN_FIELD30(_s_pin, _e_pin, _s_addr, _x_addrs, _s_bit, _x_bits)	\
	PIN_FIELD_CALC(_s_pin, _e_pin, 0, _s_addr, _x_addrs, _s_bit,	\
		       _x_bits, 30, 0)

static const struct mtk_pin_field_calc mt8168_pin_mode_range[] = {
	PIN_FIELD30(0, 144, 0x1E0, 0x10, 0, 3),
};

static const struct mtk_pin_field_calc mt8168_pin_dir_range[] = {
	PIN_FIELD(0, 144, 0x140, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_di_range[] = {
	PIN_FIELD(0, 144, 0x000, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_do_range[] = {
	PIN_FIELD(0, 144, 0x0A0, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_pullen_range[] = {
	PIN_FIELD(0, 144, 0x860, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_pullsel_range[] = {
	PIN_FIELD(0, 144, 0x900, 0x10, 0, 1),
};


static const struct mtk_pin_field_calc mt8168_pin_ies_range[] = {
	PINS_FIELD_BASE(0, 3, 0, 0x410, 0x10, 0, 1),
	PINS_FIELD_BASE(4, 7, 0, 0x410, 0x10, 1, 1),
	PINS_FIELD_BASE(8, 11, 0, 0x410, 0x10, 2, 1),
	PINS_FIELD_BASE(12, 15, 0, 0x410, 0x10, 3, 1),
	PINS_FIELD_BASE(16, 18, 0, 0x410, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, 0, 0x410, 0x10, 5, 1),
	PINS_FIELD_BASE(20, 21, 0, 0x410, 0x10, 6, 1),
	PIN_FIELD_BASE(22, 22, 0, 0x410, 0x10, 7, 1),
	PINS_FIELD_BASE(23, 25, 0, 0x410, 0x10, 8, 1),
	PINS_FIELD_BASE(26, 29, 0, 0x410, 0x10, 9, 1),
	PINS_FIELD_BASE(30, 34, 0, 0x410, 0x10, 10, 1),
	PINS_FIELD_BASE(35, 40, 0, 0x410, 0x10, 11, 1),
	PINS_FIELD_BASE(41, 44, 0, 0x410, 0x10, 12, 1),
	PINS_FIELD_BASE(45, 48, 0, 0x410, 0x10, 13, 1),
	PINS_FIELD_BASE(49, 56, 0, 0x410, 0x10, 14, 1),
	PINS_FIELD_BASE(57, 58, 0, 0x410, 0x10, 15, 1),
	PINS_FIELD_BASE(59, 60, 0, 0x410, 0x10, 16, 1),
	PINS_FIELD_BASE(61, 62, 0, 0x410, 0x10, 17, 1),
	PINS_FIELD_BASE(63, 64, 0, 0x410, 0x10, 18, 1),
	PINS_FIELD_BASE(65, 70, 0, 0x410, 0x10, 19, 1),
	PINS_FIELD_BASE(71, 79, 0, 0x410, 0x10, 20, 1),
	PIN_FIELD_BASE(80, 80, 0, 0x410, 0x10, 21, 1),
	PIN_FIELD_BASE(81, 81, 0, 0x410, 0x10, 22, 1),
	PIN_FIELD_BASE(82, 82, 0, 0x410, 0x10, 23, 1),
	PIN_FIELD_BASE(83, 83, 0, 0x410, 0x10, 24, 1),
	PIN_FIELD_BASE(84, 84, 0, 0x410, 0x10, 25, 1),
	PIN_FIELD_BASE(85, 85, 0, 0x410, 0x10, 26, 1),
	PIN_FIELD_BASE(86, 86, 0, 0x410, 0x10, 27, 1),
	PIN_FIELD_BASE(87, 87, 0, 0x410, 0x10, 28, 1),
	PIN_FIELD_BASE(88, 88, 0, 0x410, 0x10, 29, 1),
	PIN_FIELD_BASE(89, 89, 0, 0x410, 0x10, 30, 1),
	PIN_FIELD_BASE(90, 90, 0, 0x410, 0x10, 31, 1),
	PIN_FIELD_BASE(91, 91, 0, 0x420, 0x10, 0, 1),
	PIN_FIELD_BASE(92, 92, 0, 0x420, 0x10, 1, 1),
	PIN_FIELD_BASE(93, 93, 0, 0x420, 0x10, 2, 1),
	PIN_FIELD_BASE(94, 94, 0, 0x420, 0x10, 3, 1),
	PIN_FIELD_BASE(95, 95, 0, 0x420, 0x10, 4, 1),
	PIN_FIELD_BASE(96, 96, 0, 0x420, 0x10, 5, 1),
	PIN_FIELD_BASE(97, 97, 0, 0X420, 0x10, 6, 1),
	PIN_FIELD_BASE(98, 98, 0, 0x420, 0x10, 7, 1),
	PIN_FIELD_BASE(99, 99, 0, 0x420, 0x10, 8, 1),
	PIN_FIELD_BASE(100, 100, 0, 0x420, 0x10, 9, 1),
	PIN_FIELD_BASE(101, 101, 0, 0x420, 0x10, 10, 1),
	PIN_FIELD_BASE(102, 102, 0, 0x420, 0x10, 11, 1),
	PIN_FIELD_BASE(103, 103, 0, 0x420, 0x10, 12, 1),
	PIN_FIELD_BASE(104, 104, 0, 0x420, 0x10, 13, 1),
	PINS_FIELD_BASE(105, 109, 0, 0x420, 0x10, 14, 1),
	PINS_FIELD_BASE(110, 113, 0, 0x420, 0x10, 15, 1),
	PINS_FIELD_BASE(114, 116, 0, 0x420, 0x10, 16, 1),
	PINS_FIELD_BASE(117, 119, 0, 0x420, 0x10, 17, 1),
	PINS_FIELD_BASE(120, 122, 0, 0x420, 0x10, 18, 1),
	PINS_FIELD_BASE(123, 125, 0, 0x420, 0x10, 19, 1),
	PINS_FIELD_BASE(126, 128, 0, 0x420, 0x10, 20, 1),
	PINS_FIELD_BASE(129, 135, 0, 0x420, 0x10, 21, 1),
	PINS_FIELD_BASE(136, 144, 0, 0x420, 0x10, 22, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_smt_range[] = {
	PINS_FIELD_BASE(0, 3, 0, 0x470, 0x10, 0, 1),
	PINS_FIELD_BASE(4, 7, 0, 0x470, 0x10, 1, 1),
	PINS_FIELD_BASE(8, 11, 0, 0x470, 0x10, 2, 1),
	PINS_FIELD_BASE(12, 15, 0, 0x470, 0x10, 3, 1),
	PINS_FIELD_BASE(16, 18, 0, 0x470, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, 0, 0x470, 0x10, 5, 1),
	PINS_FIELD_BASE(20, 21, 0, 0x470, 0x10, 6, 1),
	PIN_FIELD_BASE(22, 22, 0, 0x470, 0x10, 7, 1),
	PINS_FIELD_BASE(23, 25, 0, 0x470, 0x10, 8, 1),
	PINS_FIELD_BASE(26, 29, 0, 0x470, 0x10, 9, 1),
	PINS_FIELD_BASE(30, 34, 0, 0x470, 0x10, 10, 1),
	PINS_FIELD_BASE(35, 40, 0, 0x470, 0x10, 11, 1),
	PINS_FIELD_BASE(41, 44, 0, 0x470, 0x10, 12, 1),
	PINS_FIELD_BASE(45, 48, 0, 0x470, 0x10, 13, 1),
	PINS_FIELD_BASE(49, 56, 0, 0x470, 0x10, 14, 1),
	PINS_FIELD_BASE(57, 58, 0, 0x470, 0x10, 15, 1),
	PINS_FIELD_BASE(59, 60, 0, 0x470, 0x10, 16, 1),
	PINS_FIELD_BASE(61, 62, 0, 0x470, 0x10, 17, 1),
	PINS_FIELD_BASE(63, 64, 0, 0x470, 0x10, 18, 1),
	PINS_FIELD_BASE(65, 70, 0, 0x470, 0x10, 19, 1),
	PINS_FIELD_BASE(71, 79, 0, 0x470, 0x10, 20, 1),
	PIN_FIELD_BASE(80, 80, 0, 0x470, 0x10, 21, 1),
	PIN_FIELD_BASE(81, 81, 0, 0x470, 0x10, 22, 1),
	PIN_FIELD_BASE(82, 82, 0, 0x470, 0x10, 23, 1),
	PIN_FIELD_BASE(83, 83, 0, 0x470, 0x10, 24, 1),
	PIN_FIELD_BASE(84, 84, 0, 0x470, 0x10, 25, 1),
	PIN_FIELD_BASE(85, 85, 0, 0x470, 0x10, 26, 1),
	PIN_FIELD_BASE(86, 86, 0, 0x470, 0x10, 27, 1),
	PIN_FIELD_BASE(87, 87, 0, 0x470, 0x10, 28, 1),
	PIN_FIELD_BASE(88, 88, 0, 0x470, 0x10, 29, 1),
	PIN_FIELD_BASE(89, 89, 0, 0x470, 0x10, 30, 1),
	PIN_FIELD_BASE(90, 90, 0, 0x470, 0x10, 31, 1),
	PIN_FIELD_BASE(91, 91, 0, 0x480, 0x10, 0, 1),
	PIN_FIELD_BASE(92, 92, 0, 0x480, 0x10, 1, 1),
	PIN_FIELD_BASE(93, 93, 0, 0x480, 0x10, 2, 1),
	PIN_FIELD_BASE(94, 94, 0, 0x480, 0x10, 3, 1),
	PIN_FIELD_BASE(95, 95, 0, 0x480, 0x10, 4, 1),
	PIN_FIELD_BASE(96, 96, 0, 0x480, 0x10, 5, 1),
	PIN_FIELD_BASE(97, 97, 0, 0X480, 0x10, 6, 1),
	PIN_FIELD_BASE(98, 98, 0, 0x480, 0x10, 7, 1),
	PIN_FIELD_BASE(99, 99, 0, 0x480, 0x10, 8, 1),
	PIN_FIELD_BASE(100, 100, 0, 0x480, 0x10, 9, 1),
	PIN_FIELD_BASE(101, 101, 0, 0x480, 0x10, 10, 1),
	PIN_FIELD_BASE(102, 102, 0, 0x480, 0x10, 11, 1),
	PIN_FIELD_BASE(103, 103, 0, 0x480, 0x10, 12, 1),
	PIN_FIELD_BASE(104, 104, 0, 0x480, 0x10, 13, 1),
	PINS_FIELD_BASE(105, 109, 0, 0x480, 0x10, 14, 1),
	PINS_FIELD_BASE(110, 113, 0, 0x480, 0x10, 15, 1),
	PINS_FIELD_BASE(114, 116, 0, 0x480, 0x10, 16, 1),
	PINS_FIELD_BASE(117, 119, 0, 0x480, 0x10, 17, 1),
	PINS_FIELD_BASE(120, 122, 0, 0x480, 0x10, 18, 1),
	PINS_FIELD_BASE(123, 125, 0, 0x480, 0x10, 19, 1),
	PINS_FIELD_BASE(126, 128, 0, 0x480, 0x10, 20, 1),
	PINS_FIELD_BASE(129, 135, 0, 0x480, 0x10, 21, 1),
	PINS_FIELD_BASE(136, 144, 0, 0x480, 0x10, 22, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_drv_range[] = {
	PINS_FIELD_BASE(0, 3, 0, 0x710, 0x10, 0, 3),
	PINS_FIELD_BASE(4, 7, 0, 0x710, 0x10, 4, 3),
	PINS_FIELD_BASE(8, 11, 0, 0x710, 0x10, 8, 3),
	PINS_FIELD_BASE(12, 15, 0, 0x710, 0x10, 12, 3),
	PINS_FIELD_BASE(16, 18, 0, 0x710, 0x10, 16, 3),
	PIN_FIELD_BASE(19, 19, 0, 0x710, 0x10, 20, 3),
	PINS_FIELD_BASE(20, 21, 0, 0x710, 0x10, 24, 3),
	PIN_FIELD_BASE(22, 22, 0, 0x710, 0x10, 28, 3),
	PINS_FIELD_BASE(23, 25, 0, 0x720, 0x10, 0, 3),
	PINS_FIELD_BASE(26, 29, 0, 0x720, 0x10, 20, 3),
	PINS_FIELD_BASE(30, 34, 0, 0x720, 0x10, 8, 3),
	PINS_FIELD_BASE(35, 40, 0, 0x720, 0x10, 12, 3),
	PINS_FIELD_BASE(41, 44, 0, 0x720, 0x10, 16, 3),
	PINS_FIELD_BASE(45, 48, 0, 0x720, 0x10, 20, 3),
	PINS_FIELD_BASE(49, 56, 0, 0x720, 0x10, 24, 3),
	PINS_FIELD_BASE(57, 58, 0, 0x720, 0x10, 28, 3),
	PINS_FIELD_BASE(59, 60, 0, 0x730, 0x10, 0, 3),
	PINS_FIELD_BASE(61, 62, 0, 0x730, 0x10, 4, 3),
	PINS_FIELD_BASE(63, 64, 0, 0x730, 0x10, 8, 3),
	PINS_FIELD_BASE(65, 70, 0, 0x730, 0x10, 12, 3),
	PINS_FIELD_BASE(71, 79, 0, 0x730, 0x10, 16, 3),
	PIN_FIELD_BASE(80, 80, 0, 0x730, 0x10, 20, 3),
	PIN_FIELD_BASE(81, 81, 0, 0x730, 0x10, 24, 3),
	PINS_FIELD_BASE(82, 85, 0, 0x740, 0x10, 28, 3),
	PIN_FIELD_BASE(86, 86, 0, 0x740, 0x10, 12, 3),
	PIN_FIELD_BASE(87, 87, 0, 0x740, 0x10, 16, 3),
	PIN_FIELD_BASE(88, 88, 0, 0x740, 0x10, 20, 3),
	PINS_FIELD_BASE(89, 92, 0, 0x740, 0x10, 24, 3),
	PINS_FIELD_BASE(93, 96, 0, 0x750, 0x10, 8, 3),
	PIN_FIELD_BASE(97, 97, 0, 0x750, 0x10, 24, 3),
	PIN_FIELD_BASE(98, 98, 0, 0x750, 0x10, 28, 3),
	PIN_FIELD_BASE(99, 99, 0, 0x760, 0x10, 0, 3),
	PINS_FIELD_BASE(100, 103, 0, 0x760, 0x10, 8, 3),
	PIN_FIELD_BASE(104, 104, 0, 0x760, 0x10, 20, 3),
	PINS_FIELD_BASE(105, 109, 0, 0x760, 0x10, 24, 3),
	PINS_FIELD_BASE(110, 113, 0, 0x760, 0x10, 28, 3),
	PINS_FIELD_BASE(114, 116, 0, 0x770, 0x10, 0, 3),
	PINS_FIELD_BASE(117, 119, 0, 0x770, 0x10, 4, 3),
	PINS_FIELD_BASE(120, 122, 0, 0x770, 0x10, 8, 3),
	PINS_FIELD_BASE(123, 125, 0, 0x770, 0x10, 12, 3),
	PINS_FIELD_BASE(126, 128, 0, 0x770, 0x10, 16, 3),
	PINS_FIELD_BASE(129, 135, 0, 0x770, 0x10, 20, 3),
	PINS_FIELD_BASE(136, 144, 0, 0x770, 0x10, 24, 3),
};

static const struct mtk_pin_field_calc mt8168_pin_pupd_range[] = {
	PIN_FIELD_BASE(22, 22, 0, 0x070, 0x10, 0, 1),
	PIN_FIELD_BASE(23, 23, 0, 0x070, 0x10, 3, 1),
	PIN_FIELD_BASE(24, 24, 0, 0x070, 0x10, 6, 1),
	PIN_FIELD_BASE(25, 25, 0, 0x070, 0x10, 9, 1),
	PIN_FIELD_BASE(80, 80, 0, 0x070, 0x10, 14, 1),
	PIN_FIELD_BASE(81, 81, 0, 0x070, 0x10, 17, 1),
	PIN_FIELD_BASE(82, 82, 0, 0x070, 0x10, 20, 1),
	PIN_FIELD_BASE(83, 83, 0, 0x070, 0x10, 23, 1),
	PIN_FIELD_BASE(84, 84, 0, 0x070, 0x10, 26, 1),
	PIN_FIELD_BASE(85, 85, 0, 0x070, 0x10, 29, 1),
	PIN_FIELD_BASE(86, 86, 0, 0x080, 0x10, 2, 1),
	PIN_FIELD_BASE(87, 87, 0, 0x080, 0x10, 5, 1),
	PIN_FIELD_BASE(88, 88, 0, 0x080, 0x10, 8, 1),
	PIN_FIELD_BASE(89, 89, 0, 0x080, 0x10, 11, 1),
	PIN_FIELD_BASE(90, 90, 0, 0x080, 0x10, 14, 1),
	PIN_FIELD_BASE(91, 91, 0, 0x080, 0x10, 17, 1),
	PIN_FIELD_BASE(92, 92, 0, 0x080, 0x10, 20, 1),
	PIN_FIELD_BASE(93, 93, 0, 0x080, 0x10, 23, 1),
	PIN_FIELD_BASE(94, 94, 0, 0x080, 0x10, 26, 1),
	PIN_FIELD_BASE(95, 95, 0, 0x080, 0x10, 29, 1),
	PIN_FIELD_BASE(96, 96, 0, 0x090, 0x10, 2, 1),
	PIN_FIELD_BASE(97, 97, 8, 0x090, 0x10, 5, 1),
	PIN_FIELD_BASE(98, 98, 8, 0x090, 0x10, 8, 1),
	PIN_FIELD_BASE(99, 99, 8, 0x090, 0x10, 11, 1),
	PIN_FIELD_BASE(100, 100, 8, 0x090, 0x10, 14, 1),
	PIN_FIELD_BASE(101, 101, 8, 0x090, 0x10, 17, 1),
	PIN_FIELD_BASE(102, 102, 8, 0x090, 0x10, 20, 1),
	PIN_FIELD_BASE(103, 103, 8, 0x090, 0x10, 23, 1),
	PIN_FIELD_BASE(104, 104, 8, 0x090, 0x10, 26, 1),
	PIN_FIELD_BASE(105, 105, 8, 0x090, 0x10, 29, 1),
	PIN_FIELD_BASE(106, 106, 8, 0x0F0, 0x10, 2, 1),
	PIN_FIELD_BASE(107, 107, 8, 0x0F0, 0x10, 5, 1),
	PIN_FIELD_BASE(108, 108, 8, 0x0F0, 0x10, 8, 1),
	PIN_FIELD_BASE(109, 109, 8, 0x0F0, 0x10, 11, 1),
};


static const struct mtk_pin_field_calc mt8168_pin_r0_range[] = {
	PIN_FIELD_BASE(22, 22, 0, 0x070, 0x10, 1, 1),
	PIN_FIELD_BASE(23, 23, 0, 0x070, 0x10, 4, 1),
	PIN_FIELD_BASE(24, 24, 0, 0x070, 0x10, 7, 1),
	PIN_FIELD_BASE(25, 25, 0, 0x070, 0x10, 10, 1),
	PIN_FIELD_BASE(80, 80, 0, 0x070, 0x10, 12, 1),
	PIN_FIELD_BASE(81, 81, 0, 0x070, 0x10, 15, 1),
	PIN_FIELD_BASE(82, 82, 0, 0x070, 0x10, 18, 1),
	PIN_FIELD_BASE(83, 83, 0, 0x070, 0x10, 21, 1),
	PIN_FIELD_BASE(84, 84, 0, 0x070, 0x10, 24, 1),
	PIN_FIELD_BASE(85, 85, 0, 0x070, 0x10, 27, 1),
	PIN_FIELD_BASE(86, 86, 0, 0x080, 0x10, 0, 1),
	PIN_FIELD_BASE(87, 87, 0, 0x080, 0x10, 3, 1),
	PIN_FIELD_BASE(88, 88, 0, 0x080, 0x10, 6, 1),
	PIN_FIELD_BASE(89, 89, 0, 0x080, 0x10, 9, 1),
	PIN_FIELD_BASE(90, 90, 0, 0x080, 0x10, 12, 1),
	PIN_FIELD_BASE(91, 91, 0, 0x080, 0x10, 15, 1),
	PIN_FIELD_BASE(92, 92, 0, 0x080, 0x10, 18, 1),
	PIN_FIELD_BASE(93, 93, 0, 0x080, 0x10, 21, 1),
	PIN_FIELD_BASE(94, 94, 0, 0x080, 0x10, 24, 1),
	PIN_FIELD_BASE(95, 95, 0, 0x080, 0x10, 27, 1),
	PIN_FIELD_BASE(96, 96, 0, 0x090, 0x10, 0, 1),
	PIN_FIELD_BASE(97, 97, 8, 0x090, 0x10, 3, 1),
	PIN_FIELD_BASE(98, 98, 8, 0x090, 0x10, 6, 1),
	PIN_FIELD_BASE(99, 99, 8, 0x090, 0x10, 9, 1),
	PIN_FIELD_BASE(100, 100, 8, 0x090, 0x10, 12, 1),
	PIN_FIELD_BASE(101, 101, 8, 0x090, 0x10, 15, 1),
	PIN_FIELD_BASE(102, 102, 8, 0x090, 0x10, 18, 1),
	PIN_FIELD_BASE(103, 103, 8, 0x090, 0x10, 21, 1),
	PIN_FIELD_BASE(104, 104, 8, 0x090, 0x10, 24, 1),
	PIN_FIELD_BASE(105, 105, 8, 0x090, 0x10, 27, 1),
	PIN_FIELD_BASE(106, 106, 8, 0x0F0, 0x10, 0, 1),
	PIN_FIELD_BASE(107, 107, 8, 0x0F0, 0x10, 3, 1),
	PIN_FIELD_BASE(108, 108, 8, 0x0F0, 0x10, 6, 1),
	PIN_FIELD_BASE(109, 109, 8, 0x0F0, 0x10, 9, 1),
};

static const struct mtk_pin_field_calc mt8168_pin_r1_range[] = {
	PIN_FIELD_BASE(22, 22, 0, 0x070, 0x10, 2, 1),
	PIN_FIELD_BASE(23, 23, 0, 0x070, 0x10, 5, 1),
	PIN_FIELD_BASE(24, 24, 0, 0x070, 0x10, 8, 1),
	PIN_FIELD_BASE(25, 25, 0, 0x070, 0x10, 11, 1),
	PIN_FIELD_BASE(80, 80, 0, 0x070, 0x10, 13, 1),
	PIN_FIELD_BASE(81, 81, 0, 0x070, 0x10, 16, 1),
	PIN_FIELD_BASE(82, 82, 0, 0x070, 0x10, 19, 1),
	PIN_FIELD_BASE(83, 83, 0, 0x070, 0x10, 22, 1),
	PIN_FIELD_BASE(84, 84, 0, 0x070, 0x10, 25, 1),
	PIN_FIELD_BASE(85, 85, 0, 0x070, 0x10, 28, 1),
	PIN_FIELD_BASE(86, 86, 0, 0x080, 0x10, 1, 1),
	PIN_FIELD_BASE(87, 87, 0, 0x080, 0x10, 4, 1),
	PIN_FIELD_BASE(88, 88, 0, 0x080, 0x10, 7, 1),
	PIN_FIELD_BASE(89, 89, 0, 0x080, 0x10, 10, 1),
	PIN_FIELD_BASE(90, 90, 0, 0x080, 0x10, 13, 1),
	PIN_FIELD_BASE(91, 91, 0, 0x080, 0x10, 16, 1),
	PIN_FIELD_BASE(92, 92, 0, 0x080, 0x10, 19, 1),
	PIN_FIELD_BASE(93, 93, 0, 0x080, 0x10, 22, 1),
	PIN_FIELD_BASE(94, 94, 0, 0x080, 0x10, 25, 1),
	PIN_FIELD_BASE(95, 95, 0, 0x080, 0x10, 28, 1),
	PIN_FIELD_BASE(96, 96, 0, 0x090, 0x10, 1, 1),
	PIN_FIELD_BASE(97, 97, 8, 0x090, 0x10, 4, 1),
	PIN_FIELD_BASE(98, 98, 8, 0x090, 0x10, 7, 1),
	PIN_FIELD_BASE(99, 99, 8, 0x090, 0x10, 10, 1),
	PIN_FIELD_BASE(100, 100, 8, 0x090, 0x10, 13, 1),
	PIN_FIELD_BASE(101, 101, 8, 0x090, 0x10, 16, 1),
	PIN_FIELD_BASE(102, 102, 8, 0x090, 0x10, 19, 1),
	PIN_FIELD_BASE(103, 103, 8, 0x090, 0x10, 22, 1),
	PIN_FIELD_BASE(104, 104, 8, 0x090, 0x10, 25, 1),
	PIN_FIELD_BASE(105, 105, 8, 0x090, 0x10, 28, 1),
	PIN_FIELD_BASE(106, 106, 8, 0x0F0, 0x10, 1, 1),
	PIN_FIELD_BASE(107, 107, 8, 0x0F0, 0x10, 4, 1),
	PIN_FIELD_BASE(108, 108, 8, 0x0F0, 0x10, 7, 1),
	PIN_FIELD_BASE(109, 109, 8, 0x0F0, 0x10, 10, 1),
};

static const struct mtk_pin_reg_calc mt8168_reg_cals[PINCTRL_PIN_REG_MAX] = {
	[PINCTRL_PIN_REG_MODE] = MTK_RANGE(mt8168_pin_mode_range),
	[PINCTRL_PIN_REG_DIR] = MTK_RANGE(mt8168_pin_dir_range),
	[PINCTRL_PIN_REG_DI] = MTK_RANGE(mt8168_pin_di_range),
	[PINCTRL_PIN_REG_DO] = MTK_RANGE(mt8168_pin_do_range),
	[PINCTRL_PIN_REG_SMT] = MTK_RANGE(mt8168_pin_smt_range),
	[PINCTRL_PIN_REG_IES] = MTK_RANGE(mt8168_pin_ies_range),
	[PINCTRL_PIN_REG_PULLEN] = MTK_RANGE(mt8168_pin_pullen_range),
	[PINCTRL_PIN_REG_PULLSEL] = MTK_RANGE(mt8168_pin_pullsel_range),
	[PINCTRL_PIN_REG_DRV] = MTK_RANGE(mt8168_pin_drv_range),
	[PINCTRL_PIN_REG_PUPD] = MTK_RANGE(mt8168_pin_pupd_range),
	[PINCTRL_PIN_REG_R0] = MTK_RANGE(mt8168_pin_r0_range),
	[PINCTRL_PIN_REG_R1] = MTK_RANGE(mt8168_pin_r1_range),
};

static const char * const mt8168_pinctrl_register_base_names[] = {
	"iocfg0",
};

static const struct mtk_eint_hw mt8168_eint_hw = {
	.port_mask = 7,
	.ports     = 6,
	.ap_num    = 212,
	.db_cnt    = 13,
};

static const struct mtk_pin_soc mt8168_data = {
	.reg_cal = mt8168_reg_cals,
	.pins = mtk_pins_mt8168,
	.npins = ARRAY_SIZE(mtk_pins_mt8168),
	.ngrps = ARRAY_SIZE(mtk_pins_mt8168),
	.eint_hw = &mt8168_eint_hw,
	.gpio_m = 0,
	.ies_present = true,
	.base_names = mt8168_pinctrl_register_base_names,
	.nbase_names = ARRAY_SIZE(mt8168_pinctrl_register_base_names),
	.bias_disable_set = mtk_pinconf_bias_disable_set_rev1,
	.bias_disable_get = mtk_pinconf_bias_disable_get_rev1,
	.bias_set = mtk_pinconf_bias_set_rev1,
	.bias_get = mtk_pinconf_bias_get_rev1,
	.drive_set = mtk_pinconf_drive_set_rev1,
	.drive_get = mtk_pinconf_drive_get_rev1,
	.adv_pull_get = mtk_pinconf_adv_pull_get,
	.adv_pull_set = mtk_pinconf_adv_pull_set,
};

static const struct of_device_id mt8168_pinctrl_of_match[] = {
	{ .compatible = "mediatek,mt8168-pinctrl", },
	{ }
};

static int mt8168_pinctrl_probe(struct platform_device *pdev)
{
	return mtk_paris_pinctrl_probe(pdev, &mt8168_data);
}

static struct platform_driver mt8168_pinctrl_driver = {
	.driver = {
		.name = "mt8168-pinctrl",
		.of_match_table = mt8168_pinctrl_of_match,
	},
	.probe = mt8168_pinctrl_probe,
};

static int __init mt8168_pinctrl_init(void)
{
	return platform_driver_register(&mt8168_pinctrl_driver);
}
arch_initcall(mt8168_pinctrl_init);
