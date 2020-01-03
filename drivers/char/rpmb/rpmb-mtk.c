// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/memory.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <crypto/hash.h>

#include <linux/rpmb.h>

#include <linux/scatterlist.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include "queue.h"
#include <linux/of.h>

/* #define __RPMB_MTK_DEBUG_MSG */
/* #define __RPMB_MTK_DEBUG_HMAC_VERIFY */
/* #define __RPMB_IOCTL_SUPPORT */

/* TEE usage */
#if IS_ENABLED(CONFIG_TRUSTONIC_TEE_SUPPORT)
#include "mobicore_driver_api.h"
#include "drrpmb_gp_Api.h"
#include "drrpmb_Api.h"

static struct mc_uuid_t rpmb_gp_uuid = RPMB_GP_UUID;
static struct mc_session_handle rpmb_gp_session = {0};
static u32 rpmb_gp_devid = MC_DEVICE_ID_DEFAULT;
static struct dciMessage_t *rpmb_gp_dci;
#endif

/*
 * Dummy definition for MAX_RPMB_TRANSFER_BLK.
 *
 * For UFS RPMB driver, MAX_RPMB_TRANSFER_BLK will be always
 * used however it will NOT be defined in projects w/o Security
 * OS. Thus we add a dummy definition here to avoid build errors.
 *
 * For eMMC RPMB driver, MAX_RPMB_TRANSFER_BLK will be used
 * only if RPMB_MULTI_BLOCK_ACCESS is defined. thus
 * build error will not happen on projects w/o Security OS.
 *
 * NOTE: This dummy definition shall be located after
 *       #include "drrpmb_Api.h" and
 *       #include "rpmb-mtk.h"
 *       since MAX_RPMB_TRANSFER_BLK will be defined in those
 *       header files if security OS is enabled.
 */
#ifndef MAX_RPMB_TRANSFER_BLK
#define MAX_RPMB_TRANSFER_BLK (1U)
#endif

#define RPMB_NAME "rpmb"

//extern struct msdc_host *mtk_msdc_host[];

#define RPMB_IOCTL_PROGRAM_KEY  1
#define RPMB_IOCTL_WRITE_DATA   3
#define RPMB_IOCTL_READ_DATA    4

struct rpmb_ioc_param {
	unsigned char *keybytes;
	unsigned char *databytes;
	unsigned int  data_len;
	unsigned short addr;
	unsigned char *hmac;
	unsigned int hmac_len;
};

#define RPMB_SZ_MAC   32U
#define RPMB_SZ_DATA  256U
#define RPMB_SZ_STUFF 196U
#define RPMB_SZ_NONCE 16U

struct s_rpmb {
	unsigned char stuff[RPMB_SZ_STUFF];
	unsigned char mac[RPMB_SZ_MAC];
	unsigned char data[RPMB_SZ_DATA];
	unsigned char nonce[RPMB_SZ_NONCE];
	unsigned int write_counter;
	unsigned short address;
	unsigned short block_count;
	unsigned short result;
	unsigned short request;
};

enum {
	RPMB_SUCCESS = 0,
	RPMB_HMAC_ERROR,
	RPMB_RESULT_ERROR,
	RPMB_WC_ERROR,
	RPMB_NONCE_ERROR,
	RPMB_ALLOC_ERROR,
	RPMB_TRANSFER_NOT_COMPLETE,
};

#define RPMB_REQ               1       /* RPMB request mark */
#define RPMB_RESP              (1 << 1)/* RPMB response mark */
#define RPMB_AVAILABLE_SECTORS 8       /* 4K page size */

#define RPMB_TYPE_BEG          510
#define RPMB_RES_BEG           508
#define RPMB_BLKS_BEG          506
#define RPMB_ADDR_BEG          504
#define RPMB_WCOUNTER_BEG      500

#define RPMB_NONCE_BEG         484
#define RPMB_DATA_BEG          228
#define RPMB_MAC_BEG           196

struct emmc_rpmb_req {
	__u16 type;                     /* RPMB request type */
	__u16 *result;                  /* response or request result */
	__u16 blk_cnt;                  /* Number of blocks(half sector 256B) */
	__u16 addr;                     /* data address */
	__u32 *wc;                      /* write counter */
	__u8 *nonce;                    /* Ramdom number */
	__u8 *data;                     /* Buffer of the user data */
	__u8 *mac;                      /* Message Authentication Code */
	__u8 *data_frame;
};

#define DEFAULT_HANDLES_NUM (64)
#define MAX_OPEN_SESSIONS (0xffffffffU - 1)

/* Debug message event */
#define DBG_EVT_NONE (0) /* No event */
#define DBG_EVT_CMD  (1U << 0)/* SEC CMD related event */
#define DBG_EVT_FUNC (1U << 1U)/* SEC function event */
#define DBG_EVT_INFO (1U << 2U)/* SEC information event */
#define DBG_EVT_WRN  (1U << 30U) /* Warning event */
#define DBG_EVT_ERR  (0x80000000U) /* Error event, 1 << 31 */
#ifdef __RPMB_MTK_DEBUG_MSG
#define DBG_EVT_DBG_INFO  (DBG_EVT_ERR) /* Error event */
#else
#define DBG_EVT_DBG_INFO  (1U << 2U) /* Information event */
#endif
#define DBG_EVT_ALL  (0xffffffffU)

static u32 dbg_evt = DBG_EVT_ALL; //Chun-Hung
#define DBG_EVT_MASK (dbg_evt)

#define MSG(evt, fmt, args...) \
do {\
	if (((DBG_EVT_##evt) & DBG_EVT_MASK) != 0U) { \
		(void)(pr_notice("[%s] "fmt, RPMB_NAME, ##args)); \
	} \
} while (false)

static struct task_struct *open_th;
static struct task_struct *rpmb_gp_Dci_th;


static struct cdev rpmb_cdev;
#ifdef __RPMB_IOCTL_SUPPORT
static struct class *mtk_rpmb_class;
#endif

static int hmac_sha256(const char *keybytes, u32 klen, const char *str,
			size_t len, u8 *hmac)
{
	struct shash_desc *shash;
	struct crypto_shash *hmacsha256 = crypto_alloc_shash("hmac(sha256)",
							     0, 0);
	size_t size = 0;
	int err = 0;
	int nbytes = (int)len;

	if (IS_ERR(hmacsha256))
		return -1;

	size = sizeof(struct shash_desc) + crypto_shash_descsize(hmacsha256);

	shash = kmalloc(size, GFP_KERNEL);
	if (shash == NULL) {
		err = -1;
		goto malloc_err;
	}
	shash->tfm = hmacsha256;
	shash->flags = 0x0;

	err = crypto_shash_setkey(hmacsha256, keybytes, klen);
	if (err != 0) {
		err = -1;
		goto hash_err;
	}

	err = crypto_shash_init(shash);
	if (err != 0) {
		err = -1;
		goto hash_err;
	}

	if (nbytes != crypto_shash_update(shash, str, (unsigned int)nbytes)) {
		err = -1;
		goto hash_err;
	}
	err = crypto_shash_final(shash, hmac);

hash_err:
	kfree(shash);
malloc_err:
	crypto_free_shash(hmacsha256);

	return err;
}

#ifdef __RPMB_MTK_DEBUG_HMAC_VERIFY
unsigned char rpmb_key[32] = {
	0x64, 0x76, 0xEE, 0xF0, 0xF1, 0x6B, 0x30, 0x47,
	0xE9, 0x79, 0x31, 0x58, 0xF6, 0x42, 0xDA, 0x46,
	0xF7, 0x3B, 0x53, 0xFD, 0xC5, 0xF8, 0x84, 0xCE,
	0x03, 0x73, 0x15, 0xBC, 0x54, 0x47, 0xD4, 0x6A
};

static int rpmb_cal_hmac(struct rpmb_frame *frame, int blk_cnt,
			u8 *key, u8 *key_mac)
{
	int i;
	u8 *buf, *buf_start;

	buf = buf_start = kzalloc(284 * blk_cnt, 0);

	for (i = 0; i < blk_cnt; i++) {
		memcpy(buf, frame[i].data, 284);
		buf += 284;
	}

	if (hmac_sha256(key, 32, buf_start, 284UL * blk_cnt, key_mac) != 0)
		MSG(ERR, "hmac_sha256() return error!\n");

	kfree(buf_start);

	return 0;
}
#endif

static void rpmb_dump_frame(u8 *data_frame)
{
	MSG(DBG_INFO, "mac, frame[196] = 0x%x\n", data_frame[196]);
	MSG(DBG_INFO, "mac, frame[197] = 0x%x\n", data_frame[197]);
	MSG(DBG_INFO, "mac, frame[198] = 0x%x\n", data_frame[198]);
	MSG(DBG_INFO, "data,frame[228] = 0x%x\n", data_frame[228]);
	MSG(DBG_INFO, "data,frame[229] = 0x%x\n", data_frame[229]);
	MSG(DBG_INFO, "nonce, frame[484] = 0x%x\n", data_frame[484]);
	MSG(DBG_INFO, "nonce, frame[485] = 0x%x\n", data_frame[485]);
	MSG(DBG_INFO, "nonce, frame[486] = 0x%x\n", data_frame[486]);
	MSG(DBG_INFO, "nonce, frame[487] = 0x%x\n", data_frame[487]);
	MSG(DBG_INFO, "wc, frame[500] = 0x%x\n", data_frame[500]);
	MSG(DBG_INFO, "wc, frame[501] = 0x%x\n", data_frame[501]);
	MSG(DBG_INFO, "wc, frame[502] = 0x%x\n", data_frame[502]);
	MSG(DBG_INFO, "wc, frame[503] = 0x%x\n", data_frame[503]);
	MSG(DBG_INFO, "addr, frame[504] = 0x%x\n", data_frame[504]);
	MSG(DBG_INFO, "addr, frame[505] = 0x%x\n", data_frame[505]);
	MSG(DBG_INFO, "blkcnt,frame[506] = 0x%x\n", data_frame[506]);
	MSG(DBG_INFO, "blkcnt,frame[507] = 0x%x\n", data_frame[507]);
	MSG(DBG_INFO, "result, frame[508] = 0x%x\n", data_frame[508]);
	MSG(DBG_INFO, "result, frame[509] = 0x%x\n", data_frame[509]);
	MSG(DBG_INFO, "type, frame[510] = 0x%x\n", data_frame[510]);
	MSG(DBG_INFO, "type, frame[511] = 0x%x\n", data_frame[511]);
}

static struct rpmb_frame *rpmb_alloc_frames(unsigned int cnt)
{
	return kzalloc(sizeof(struct rpmb_frame) * cnt, 0);
}

static int rpmb_req_get_wc(u8 *keybytes, u32 *wc, u8 *frame)
{
	struct rpmb_data rpmbdata;
	struct rpmb_dev *mmc_rpmb;
	u8 nonce[RPMB_SZ_NONCE] = {0};
	u8 hmac[RPMB_SZ_MAC];
	int ret, i;

	MSG(INFO, "%s start!!!\n", __func__);

	mmc_rpmb = rpmb_dev_get_by_type(RPMB_TYPE_EMMC);

	do {
		/*
		 * Initial frame buffers
		 */

		if (frame != NULL) {

			/*
			 * Use external frame if possible.
			 * External frame shall have below field ready,
			 *
			 * nonce
			 * req_resp
			 */
			rpmbdata.icmd.frames = (struct rpmb_frame *)frame;
			rpmbdata.ocmd.frames = (struct rpmb_frame *)frame;

		} else {

			rpmbdata.icmd.frames = rpmb_alloc_frames(1);

			if (rpmbdata.icmd.frames == NULL)
				return RPMB_ALLOC_ERROR;

			rpmbdata.ocmd.frames = rpmb_alloc_frames(1);

			if (rpmbdata.ocmd.frames == NULL)
				return RPMB_ALLOC_ERROR;
		}

		/*
		 * Prepare frame contents.
		 *
		 * Input frame (in view of device) only needs nonce
		 */

		rpmbdata.req_type = RPMB_GET_WRITE_COUNTER;
		rpmbdata.icmd.nframes = 1;

		/* Fill-in essential field in self-prepared frame */

		if (frame == NULL) {
			get_random_bytes(nonce, (int)RPMB_SZ_NONCE);
			rpmbdata.icmd.frames->req_resp =
					cpu_to_be16(RPMB_GET_WRITE_COUNTER);
			memcpy(rpmbdata.icmd.frames->nonce, nonce,
				RPMB_SZ_NONCE);
		}

		/* Output frame (in view of device) */

		rpmbdata.ocmd.nframes = 1;

		ret = rpmb_cmd_req(mmc_rpmb, &rpmbdata);

		if (ret != 0) {
			MSG(ERR, "%s, rpmb_cmd_req IO error!!!(0x%x)\n",
			    __func__, ret);
			break;
		}

		/* Verify HMAC only if key is available */

		if (keybytes != NULL) {
			if (strlen(keybytes) != 32UL) {
				MSG(ERR, "%s, error rpmb key len = 0x%x\n",
				    __func__, (unsigned int)strlen(keybytes));
				ret = RPMB_WC_ERROR;
				break;
			}

			/*
			 * Authenticate response write counter frame.
			 */
			if (hmac_sha256(keybytes, 32,
					rpmbdata.ocmd.frames->data,
					284UL, hmac) != 0)
				MSG(ERR, "hmac_sha256() return error!\n");

			if (memcmp(hmac, rpmbdata.ocmd.frames->key_mac,
				   RPMB_SZ_MAC) != 0) {
				MSG(ERR, "%s, hmac compare error!!!\n",
				    __func__);
				ret = RPMB_HMAC_ERROR;
			}

			/*
			 * DEVICE ISSUE:
			 * We found some devices will return hmac vale with
			 * all zeros.
			 * For this kind of device, bypass hmac comparison.
			 */
			if (ret == RPMB_HMAC_ERROR) {
				for (i = 0; i < 32; i++) {
					if (rpmbdata.ocmd.frames->key_mac[i] !=
					    0U) {
						MSG(ERR,
						    "%s, dev hmac not NULL!\n",
						    __func__);
						break;
					}
				}

				MSG(ERR,
				    "%s, device hmac has all zero, bypassed!\n",
				    __func__);
				ret = RPMB_SUCCESS;
			}
		}

		/*
		 * Verify nonce and result only in self-prepared frame
		 * External frame shall be verified by frame provider,
		 * for example, TEE.
		 */
		if (frame == NULL) {
			if (memcmp(nonce, rpmbdata.ocmd.frames->nonce,
				   RPMB_SZ_NONCE) != 0) {
				MSG(ERR, "%s, nonce compare error!!!\n",
				    __func__);
				rpmb_dump_frame((u8 *)rpmbdata.ocmd.frames);
				ret = RPMB_NONCE_ERROR;
				break;
			}

			if (rpmbdata.ocmd.frames->result != 0U) {
				MSG(ERR, "%s, result error!!! (0x%x)\n",
				    __func__,
				    cpu_to_be16(rpmbdata.ocmd.frames->result));
				ret = RPMB_RESULT_ERROR;
				break;
			}
		}

		if (wc != NULL) {
			*wc = cpu_to_be32(rpmbdata.ocmd.frames->write_counter);
			MSG(DBG_INFO, "%s: wc = %d (0x%x)\n",
			    __func__, *wc, *wc);
		}
	} while (false);

	MSG(DBG_INFO, "%s: end\n", __func__);

	if (frame == NULL) {
		kfree(rpmbdata.icmd.frames);
		kfree(rpmbdata.ocmd.frames);
	}

	return ret;
}

static int rpmb_req_read_data(u8 *frame, u32 blk_cnt)
{
	struct rpmb_data rpmbdata;
	struct rpmb_dev *mmc_rpmb;
	int ret;

	mmc_rpmb = rpmb_dev_get_by_type(RPMB_TYPE_EMMC);

	MSG(DBG_INFO, "%s: blk_cnt: %d\n", __func__, blk_cnt);

	rpmbdata.req_type = RPMB_READ_DATA;
	rpmbdata.icmd.nframes = 1;
	rpmbdata.icmd.frames = (struct rpmb_frame *)frame;

	/*
	 * We need to fill-in block_count by ourselves for UFS case.
	 * TEE does not fill-in this field because eMMC spec specifiy it as 0.
	 */
	rpmbdata.icmd.frames->block_count = cpu_to_be16((u16)blk_cnt);

	rpmbdata.ocmd.nframes = blk_cnt;
	rpmbdata.ocmd.frames = (struct rpmb_frame *)frame;

	ret = rpmb_cmd_req(mmc_rpmb, &rpmbdata);

	if (ret != 0)
		MSG(ERR, "%s: rpmb_cmd_req IO error, ret %d (0x%x)\n",
		    __func__, ret, ret);

	MSG(DBG_INFO, "%s: result 0x%x\n", __func__,
	    rpmbdata.ocmd.frames->result);

	MSG(DBG_INFO, "%s: ret 0x%x\n", __func__, ret);

	return ret;
}

static int rpmb_req_write_data(u8 *frame, u32 blk_cnt)
{
	struct rpmb_data rpmbdata;
	struct rpmb_dev *mmc_rpmb;
	int ret;
#ifdef __RPMB_MTK_DEBUG_HMAC_VERIFY
	u8 *key_mac;
#endif

	mmc_rpmb = rpmb_dev_get_by_type(RPMB_TYPE_EMMC);

	MSG(DBG_INFO, "%s: blk_cnt: %d\n", __func__, blk_cnt);

	/*
	 * Alloc output frame to avoid overwriting input frame buffer
	 * provided by TEE
	 */
	rpmbdata.ocmd.frames = rpmb_alloc_frames(1);

	if (rpmbdata.ocmd.frames == NULL)
		return RPMB_ALLOC_ERROR;

	rpmbdata.ocmd.nframes = 1;

	rpmbdata.req_type = RPMB_WRITE_DATA;
	rpmbdata.icmd.nframes = blk_cnt;
	rpmbdata.icmd.frames = (struct rpmb_frame *)frame;

#ifdef __RPMB_MTK_DEBUG_HMAC_VERIFY
	key_mac = kzalloc(32, 0);

	rpmb_cal_hmac((struct rpmb_frame *)frame, blk_cnt, rpmb_key, key_mac);

	if (memcmp(key_mac, ((struct rpmb_frame *)frame)[blk_cnt - 1].key_mac,
		   32)) {
		MSG(ERR, "%s, Key Mac is NOT matched!\n", __func__);
		kfree(key_mac);
		ret = 1;
		goto out;
	} else
		MSG(ERR, "%s, Key Mac check passed.\n", __func__);

	kfree(key_mac);
#endif

	ret = rpmb_cmd_req(mmc_rpmb, &rpmbdata);

	if (ret != 0)
		MSG(ERR, "%s: rpmb_cmd_req IO error, ret %d (0x%x)\n",
		    __func__, ret, ret);

	/*
	 * Microtrust TEE will check write counter in the first frame,
	 * thus we copy response frame to the first frame.
	 */
	memcpy(frame, rpmbdata.ocmd.frames, 512);

	MSG(DBG_INFO, "%s: result 0x%x\n", __func__,
	    rpmbdata.ocmd.frames->result);

	kfree(rpmbdata.ocmd.frames);

	MSG(DBG_INFO, "%s: ret 0x%x\n", __func__, ret);

#ifdef __RPMB_MTK_DEBUG_HMAC_VERIFY
out:
#endif

	return ret;
}


static int rpmb_req_ioctl_write_data(struct rpmb_ioc_param *param)
{
	struct rpmb_data rpmbdata;
	struct rpmb_dev *mmc_rpmb;
	u32 i, tran_size, left_size = param->data_len;
	u32 wc = 0xFFFFFFFFU;
	u16 iCnt, total_blkcnt, tran_blkcnt, left_blkcnt;
	u16 blkaddr;
	u8 hmac[RPMB_SZ_MAC];
	u8 *dataBuf, *dataBuf_start, *data_for_hmac;
	size_t size_for_hmac;
	int ret = 0;
	u8 user_param_data;

	MSG(DBG_INFO, "%s start!!!\n", __func__);

	ret = get_user(user_param_data, param->databytes);
	if (ret != 0)
		return -EFAULT;

	ret = get_user(user_param_data, param->keybytes);
	if (ret != 0)
		return -EFAULT;

	mmc_rpmb = rpmb_dev_get_by_type(RPMB_TYPE_EMMC);

	i = 0;
	tran_blkcnt = 0;
	dataBuf = NULL;
	dataBuf_start = NULL;

	total_blkcnt = (u16)(((param->data_len % RPMB_SZ_DATA) != 0U) ?
				(param->data_len / RPMB_SZ_DATA + 1U) :
				(param->data_len / RPMB_SZ_DATA));
	left_blkcnt = total_blkcnt;

	/*
	 * For RPMB write data, the elements we need in the input data frame is
	 * 1. address.
	 * 2. write counter.
	 * 3. data.
	 * 4. block count.
	 * 5. MAC
	 */

	blkaddr = param->addr;

	while (left_blkcnt > 0U) {

		if (left_blkcnt >= MAX_RPMB_TRANSFER_BLK)
			tran_blkcnt = MAX_RPMB_TRANSFER_BLK;
		else
			tran_blkcnt = left_blkcnt;

		MSG(DBG_INFO, "%s, total_blkcnt = 0x%x, tran_blkcnt = 0x%x\n",
		    __func__, left_blkcnt, tran_blkcnt);

		ret = rpmb_req_get_wc(param->keybytes, &wc, NULL);
		if (ret != 0) {
			MSG(ERR, "%s, rpmb_req_get_wc error!!!(0x%x)\n",
			    __func__, ret);
			return ret;
		}

		/*
		 * Initial frame buffers
		 */

		rpmbdata.icmd.frames = rpmb_alloc_frames(tran_blkcnt);

		if (rpmbdata.icmd.frames == NULL)
			return RPMB_ALLOC_ERROR;

		rpmbdata.ocmd.frames = rpmb_alloc_frames(1);

		if (rpmbdata.ocmd.frames == NULL)
			return RPMB_ALLOC_ERROR;

		/*
		 * Initial data buffer for HMAC computation.
		 * Since HAMC computation tool which we use needs
		 * consecutive data buffer.
		 * Pre-alloced it.
		 */

		dataBuf_start = dataBuf = kzalloc(284UL * tran_blkcnt, 0);

		/*
		 * Prepare frame contents
		 */

		rpmbdata.req_type = RPMB_WRITE_DATA;


		/* Output frames (in view of device) */

		rpmbdata.ocmd.nframes = 1;

		/*
		 * All input frames (in view of device) need below stuff,
		 * 1. address.
		 * 2. write counter.
		 * 3. data.
		 * 4. block count.
		 * 5. MAC
		 */

		rpmbdata.icmd.nframes = tran_blkcnt;

		/* size for hmac calculation: 512 - 228 = 284 */
		size_for_hmac = sizeof(struct rpmb_frame) -
				offsetof(struct rpmb_frame, data);

		for (iCnt = 0; iCnt < tran_blkcnt; iCnt++) {
			/*
			 * Prepare write data frame. need addr, wc, blkcnt,
			 * data and mac.
			 */
			rpmbdata.icmd.frames[iCnt].req_resp =
						cpu_to_be16(RPMB_WRITE_DATA);
			rpmbdata.icmd.frames[iCnt].addr = cpu_to_be16(blkaddr);
			rpmbdata.icmd.frames[iCnt].block_count =
						cpu_to_be16(tran_blkcnt);
			rpmbdata.icmd.frames[iCnt].write_counter =
						cpu_to_be32(wc);

			if (left_size >= RPMB_SZ_DATA)
				tran_size = RPMB_SZ_DATA;
			else
				tran_size = left_size;

			memcpy(rpmbdata.icmd.frames[iCnt].data,
			       param->databytes +
			       i * MAX_RPMB_TRANSFER_BLK * RPMB_SZ_DATA +
			       (iCnt * RPMB_SZ_DATA),
			       tran_size);
			left_size -= tran_size;

			data_for_hmac = rpmbdata.icmd.frames[iCnt].data;

			/* copy data part */
			memcpy(dataBuf, data_for_hmac, RPMB_SZ_DATA);

			/* copy left part */
			memcpy(dataBuf + RPMB_SZ_DATA,
			       data_for_hmac + RPMB_SZ_DATA,
			       size_for_hmac - RPMB_SZ_DATA);

			dataBuf = dataBuf + size_for_hmac;

		}

		iCnt--;

		if (hmac_sha256(param->keybytes, 32, dataBuf_start,
				284UL * tran_blkcnt,
				rpmbdata.icmd.frames[iCnt].key_mac) != 0)
			MSG(ERR, "hmac_sha256() return error!\n");

		/*
		 * Send write data request.
		 */

		ret = rpmb_cmd_req(mmc_rpmb, &rpmbdata);

		if (ret != 0) {
			MSG(ERR, "%s, rpmb_cmd_req IO error!!!(0x%x)\n",
			    __func__, ret);
			break;
		}

		/*
		 * Authenticate write result response.
		 * 1. authenticate hmac.
		 * 2. check result.
		 * 3. compare write counter is increamented.
		 */
		if (hmac_sha256(param->keybytes, 32,
				rpmbdata.ocmd.frames->data,
				284UL, hmac) != 0)
			MSG(ERR, "hmac_sha256() return error!\n");

		if (memcmp(hmac, rpmbdata.ocmd.frames->key_mac,
			   RPMB_SZ_MAC) != 0) {
			MSG(ERR, "%s, hmac compare error!!!\n", __func__);
			ret = RPMB_HMAC_ERROR;
			break;
		}

		if (rpmbdata.ocmd.frames->result != 0U) {
			MSG(ERR, "%s, result error!!! (0x%x)\n", __func__,
			    cpu_to_be16(rpmbdata.ocmd.frames->result));
			ret = RPMB_RESULT_ERROR;
			break;
		}

		if (cpu_to_be32(rpmbdata.ocmd.frames->write_counter) !=
		    wc + 1U) {
			MSG(ERR, "%s, write counter error!!! (0x%x)\n",
			    __func__,
			    cpu_to_be32(rpmbdata.ocmd.frames->write_counter));
			ret = RPMB_WC_ERROR;
			break;
		}

		blkaddr += tran_blkcnt;
		left_blkcnt -= tran_blkcnt;
		i++;

		kfree(rpmbdata.icmd.frames);
		kfree(rpmbdata.ocmd.frames);
		kfree(dataBuf_start);
	};

	if (ret != 0) {
		kfree(rpmbdata.icmd.frames);
		kfree(rpmbdata.ocmd.frames);
		kfree(dataBuf_start);
	}

	if (left_blkcnt != 0U || left_size != 0U) {
		MSG(ERR, "left_blkcnt or left_size is not empty!!!!!!\n");
		return RPMB_TRANSFER_NOT_COMPLETE;
	}

	MSG(DBG_INFO, "%s end!!!\n", __func__);

	return ret;
}

static int rpmb_req_ioctl_read_data(struct rpmb_ioc_param *param)
{
	struct rpmb_data rpmbdata;
	struct rpmb_dev *mmc_rpmb;
	u32 i, tran_size, left_size = param->data_len;
	u16 iCnt, total_blkcnt, tran_blkcnt, left_blkcnt;
	u16 blkaddr;
	u8 nonce[RPMB_SZ_NONCE] = {0};
	u8 hmac[RPMB_SZ_MAC];
	u8 *dataBuf, *dataBuf_start, *data_for_hmac;
	size_t size_for_hmac;
	int ret = 0;
	u8 user_param_data;

	MSG(DBG_INFO, "%s start!!!\n", __func__);

	ret = get_user(user_param_data, param->databytes);
	if (ret != 0)
		return -EFAULT;

	ret = get_user(user_param_data, param->keybytes);
	if (ret != 0)
		return -EFAULT;

	mmc_rpmb = rpmb_dev_get_by_type(RPMB_TYPE_EMMC);

	i = 0;
	tran_blkcnt = 0;
	dataBuf = NULL;
	dataBuf_start = NULL;

	total_blkcnt = (u16)(((param->data_len % RPMB_SZ_DATA) != 0U) ?
				(param->data_len / RPMB_SZ_DATA + 1U) :
				(param->data_len / RPMB_SZ_DATA));
	left_blkcnt = total_blkcnt;

	blkaddr = param->addr;

	while (left_blkcnt > 0U) {

		if (left_blkcnt >= MAX_RPMB_TRANSFER_BLK)
			tran_blkcnt = MAX_RPMB_TRANSFER_BLK;
		else
			tran_blkcnt = left_blkcnt;

		MSG(DBG_INFO, "%s, left_blkcnt = 0x%x, tran_blkcnt = 0x%x\n",
		    __func__, left_blkcnt, tran_blkcnt);

		/*
		 * initial frame buffers
		 */

		rpmbdata.icmd.frames = rpmb_alloc_frames(1);

		if (rpmbdata.icmd.frames == NULL)
			return RPMB_ALLOC_ERROR;

		rpmbdata.ocmd.frames = rpmb_alloc_frames(tran_blkcnt);

		if (rpmbdata.ocmd.frames == NULL)
			return RPMB_ALLOC_ERROR;

		/*
		 * Initial data buffer for HMAC computation.
		 * Since HAMC computation tool which we use needs
		 * consecutive data buffer.
		 * Pre-alloced it.
		 */

		dataBuf_start = dataBuf = kzalloc(284UL * tran_blkcnt, 0);

		get_random_bytes(nonce, (int)RPMB_SZ_NONCE);

		/*
		 * Prepare request read data frame.
		 *
		 * Input frame (in view of device) only needs addr and nonce.
		 */

		rpmbdata.req_type = RPMB_READ_DATA;
		rpmbdata.icmd.nframes = 1;
		rpmbdata.icmd.frames->req_resp = cpu_to_be16(RPMB_READ_DATA);
		rpmbdata.icmd.frames->addr = cpu_to_be16(blkaddr);
		rpmbdata.icmd.frames->block_count = cpu_to_be16(tran_blkcnt);
		memcpy(rpmbdata.icmd.frames->nonce, nonce, RPMB_SZ_NONCE);

		/* output frames (in view of device) */

		rpmbdata.ocmd.nframes = tran_blkcnt;

		ret = rpmb_cmd_req(mmc_rpmb, &rpmbdata);

		if (ret != 0) {
			MSG(ERR, "%s, rpmb_cmd_req IO error!!!(0x%x)\n",
			    __func__, ret);
			break;
		}

		/*
		 * Retrieve every data frame one by one.
		 */

		/* size for hmac calculation: 512 - 228 = 284 */
		size_for_hmac = sizeof(struct rpmb_frame) -
				offsetof(struct rpmb_frame, data);

		for (iCnt = 0; iCnt < tran_blkcnt; iCnt++) {

			if (left_size >= RPMB_SZ_DATA)
				tran_size = RPMB_SZ_DATA;
			else
				tran_size = left_size;

			/*
			 * dataBuf used for hmac calculation. we need to
			 * aggregate each block's data till to type field.
			 * each block has 284 bytes (size_for_hmac) need
			 * aggregation.
			 */
			data_for_hmac = rpmbdata.ocmd.frames[iCnt].data;

			/* copy data part */
			memcpy(dataBuf, data_for_hmac, RPMB_SZ_DATA);

			/* copy left part */
			memcpy(dataBuf + RPMB_SZ_DATA,
			       data_for_hmac + RPMB_SZ_DATA,
			       size_for_hmac - RPMB_SZ_DATA);

			dataBuf = dataBuf + size_for_hmac;

			/*
			 * Sorry, I shouldn't copy read data to user's
			 * buffer now, it should be later
			 * after checking no problem,
			 * but for convenience...you know...
			 */
			memcpy(param->databytes +
			       i * MAX_RPMB_TRANSFER_BLK * RPMB_SZ_DATA +
			       (iCnt * RPMB_SZ_DATA),
			       rpmbdata.ocmd.frames[iCnt].data,
			       tran_size);
			left_size -= tran_size;
		}

		iCnt--;

		/*
		 * Authenticate response read data frame.
		 */
		if (hmac_sha256(param->keybytes, 32,
			    dataBuf_start, size_for_hmac * tran_blkcnt,
			    hmac) != 0)
			MSG(ERR, "hmac_sha256() return error!\n");

		if (memcmp(hmac, rpmbdata.ocmd.frames[iCnt].key_mac,
			   RPMB_SZ_MAC) != 0) {
			MSG(ERR, "%s, hmac compare error!!!\n", __func__);
			ret = RPMB_HMAC_ERROR;
			break;
		}

		if (memcmp(nonce, rpmbdata.ocmd.frames[iCnt].nonce,
			   RPMB_SZ_NONCE) != 0) {
			MSG(ERR, "%s, nonce compare error!!!\n", __func__);
			ret = RPMB_NONCE_ERROR;
			break;
		}

		if (rpmbdata.ocmd.frames[iCnt].result != 0U) {
			MSG(ERR, "%s, result error!!! (0x%x)\n",
			    __func__,
			    cpu_to_be16p(&rpmbdata.ocmd.frames[iCnt].result));
			ret = RPMB_RESULT_ERROR;
			break;
		}

		blkaddr += tran_blkcnt;
		left_blkcnt -= tran_blkcnt;
		i++;

		kfree(rpmbdata.icmd.frames);
		kfree(rpmbdata.ocmd.frames);
		kfree(dataBuf_start);
	};

	if (ret != 0) {
		kfree(rpmbdata.icmd.frames);
		kfree(rpmbdata.ocmd.frames);
		kfree(dataBuf_start);
	}

	if (left_blkcnt != 0U || left_size != 0U) {
		MSG(ERR, "left_blkcnt or left_size is not empty!!!!!!\n");
		return RPMB_TRANSFER_NOT_COMPLETE;
	}

	MSG(DBG_INFO, "%s end!!!\n", __func__);

	return ret;
}

/*
 * End of above.
 *
 ******************************************************************************/


#if IS_ENABLED(CONFIG_TRUSTONIC_TEE_SUPPORT)

static enum mc_result rpmb_gp_execute(u32 cmdId)
{
	int ret;

	switch (cmdId) {

	case DCI_RPMB_CMD_READ_DATA:

		MSG(DBG_INFO, "%s: DCI_RPMB_CMD_READ_DATA\n", __func__);

		ret = rpmb_req_read_data(rpmb_gp_dci->request.frame,
					 rpmb_gp_dci->request.blks);

		break;

	case DCI_RPMB_CMD_GET_WCNT:

		MSG(DBG_INFO, "%s: DCI_RPMB_CMD_GET_WCNT\n", __func__);

		ret = rpmb_req_get_wc(NULL, NULL, rpmb_gp_dci->request.frame);

		break;

	case DCI_RPMB_CMD_WRITE_DATA:

		MSG(DBG_INFO, "%s: DCI_RPMB_CMD_WRITE_DATA\n", __func__);

		ret = rpmb_req_write_data(rpmb_gp_dci->request.frame,
					  rpmb_gp_dci->request.blks);

		break;

	default:
		MSG(ERR, "%s: receive an unknown command id(%d).\n",
		    __func__, cmdId);
		break;

	}

	return MC_DRV_OK;
}

static int rpmb_gp_listenDci(void *arg)
{
	enum mc_result mc_ret;
	u32 cmdId;

	MSG(INFO, "%s: DCI listener.\n", __func__);

	for (;;) {

		MSG(INFO, "%s: Waiting for notification\n", __func__);

		/* Wait for notification from SWd */
		mc_ret = mc_wait_notification(&rpmb_gp_session,
					      MC_INFINITE_TIMEOUT);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s: mcWaitNotification failed, mc_ret=%d\n",
			    __func__, mc_ret);
			break;
		}

		cmdId = rpmb_gp_dci->command.header.commandId;

		MSG(INFO, "%s: wait notification done!! cmdId = %x\n",
		    __func__, cmdId);

		/* Received exception. */
		mc_ret = rpmb_gp_execute(cmdId);

		/* Notify the STH*/
		mc_ret = mc_notify(&rpmb_gp_session);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s: mcNotify returned: %d\n",
			    __func__, mc_ret);
			break;
		}
	}

	return 0;
}

static enum mc_result rpmb_gp_open_session(void)
{
	int cnt = 0;
	enum mc_result mc_ret;

	MSG(INFO, "%s start\n", __func__);

	do {
		msleep(6000);

		/* open device */
		mc_ret = mc_open_device(rpmb_gp_devid);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s, mc_open_device failed: %d\n",
			    __func__, mc_ret);
			cnt++;
			continue;
		}

		MSG(INFO, "%s, mc_open_device success.\n", __func__);


		/* allocating WSM for DCI */
		mc_ret = mc_malloc_wsm(rpmb_gp_devid, 0,
				       (u32)sizeof(struct dciMessage_t),
				       (uint8_t **)&rpmb_gp_dci, 0);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s, mc_malloc_wsm failed: %d\n",
			    __func__, mc_ret);
			mc_ret = mc_close_device(rpmb_gp_devid);
			cnt++;
			continue;
		}

		MSG(INFO, "%s, mc_malloc_wsm success.\n", __func__);
		MSG(INFO, "uuid[0]=%d, uuid[1]=%d, uuid[2]=%d, uuid[3]=%d\n",
			rpmb_gp_uuid.value[0],
			rpmb_gp_uuid.value[1],
			rpmb_gp_uuid.value[2],
			rpmb_gp_uuid.value[3]
			);

		rpmb_gp_session.device_id = rpmb_gp_devid;

		/* open session */
		mc_ret = mc_open_session(&rpmb_gp_session,
					 &rpmb_gp_uuid,
					 (uint8_t *) rpmb_gp_dci,
					 (u32)sizeof(struct dciMessage_t));

		if (mc_ret != MC_DRV_OK) {
			MSG(ERR,
			    "%s, mc_open_session failed, result(%d), cnt(%d)\n",
				__func__, mc_ret, cnt);

			mc_ret = mc_free_wsm(rpmb_gp_devid,
					     (uint8_t *)rpmb_gp_dci);
			MSG(ERR, "%s, free wsm result (%d)\n",
			    __func__, mc_ret);

			mc_ret = mc_close_device(rpmb_gp_devid);
			MSG(ERR, "%s, try free wsm and close device\n",
			    __func__);
			cnt++;
			continue;
		}
		MSG(INFO, "%s, mc_open_session success.\n", __func__);

		/* create a thread for listening DCI signals */
		rpmb_gp_Dci_th = kthread_run(rpmb_gp_listenDci,
					     NULL, "rpmb_gp_Dci");
		if (IS_ERR(rpmb_gp_Dci_th))
			MSG(ERR, "%s, init kthread_run failed!\n", __func__);
		else
			break;

	} while (cnt < 60);

	if (cnt >= 60)
		MSG(ERR, "%s, open session failed!!!\n", __func__);


	MSG(ERR, "%s end, mc_ret = %x\n", __func__, mc_ret);

	return mc_ret;
}


static int rpmb_thread(void *context)
{
	enum mc_result ret;

	MSG(INFO, "%s start\n", __func__);

	ret = rpmb_gp_open_session();
	MSG(INFO, "%s rpmb_gp_open_session, ret = %x\n", __func__, ret);

	return 0;
}
#endif

static int rpmb_open(struct inode *pinode, struct file *pfile)
{
	return 0;
}

static long rpmb_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	unsigned long n;
	struct rpmb_ioc_param param;

	n = copy_from_user(&param, (void *)arg, sizeof(param));

	if (n != 0UL) {
		MSG(ERR, "%s, copy from user failed: %x\n", __func__, err);
		return -EFAULT;
	}

	switch (cmd) {

	case RPMB_IOCTL_PROGRAM_KEY:

		MSG(DBG_INFO, "%s, cmd = RPMB_IOCTL_PROGRAM_KEY not support\n",
		    __func__);

		break;

	case RPMB_IOCTL_READ_DATA:

		MSG(DBG_INFO, "%s, cmd = RPMB_IOCTL_READ_DATA!!!!\n", __func__);

		err = rpmb_req_ioctl_read_data(&param);

		if (err != 0) {
			MSG(ERR, "%s, rpmb_req_ioctl_read_data() error!(%x)\n",
			    __func__, err);
			break;
		}

		n = copy_to_user((void *)arg, &param, sizeof(param));

		if (n != 0UL) {
			MSG(ERR, "%s, copy to user user failed: %x\n",
			    __func__, err);
			err = -EFAULT;
			break;
		}

		break;

	case RPMB_IOCTL_WRITE_DATA:

		MSG(DBG_INFO, "%s, cmd = RPMB_IOCTL_WRITE_DATA!!!\n", __func__);

		err = rpmb_req_ioctl_write_data(&param);

		if (err != 0)
			MSG(ERR, "%s, rpmb_req_ioctl_write_data() error!(%x)\n",
			    __func__, err);

		break;

	default:
		MSG(ERR, "%s, wrong ioctl code (%d)!!!\n", __func__, cmd);
		err = -ENOTTY;
		break;
	}

	return err;
}

static int rpmb_close(struct inode *pinode, struct file *pfile)
{
	int ret = 0;

	MSG(INFO, "%s, !!!!!!!!!!!!\n", __func__);

	return ret;
}

static const struct file_operations rpmb_fops = {
	.owner = THIS_MODULE,
	.open = rpmb_open,
	.release = rpmb_close,
	.unlocked_ioctl = rpmb_ioctl,
	.write = NULL,
	.read = NULL,
};

static int __init rpmb_init(void)
{
	int alloc_ret;
	int cdev_ret = -1;
	unsigned int major;
	dev_t dev;
	struct device_node *np;
	struct device *device = NULL;

	np = of_find_compatible_node(NULL, NULL, "mediatek,rpmb-kinibi");
	if (np == NULL)
		goto out_err;

	MSG(INFO, "%s start\n", __func__);

	alloc_ret = alloc_chrdev_region(&dev, 0, 1, RPMB_NAME);

	if (alloc_ret != 0) {
		MSG(ERR, "%s, init alloc_chrdev_region failed!\n", __func__);
		goto error;
	}

	major = MAJOR(dev);

	cdev_init(&rpmb_cdev, &rpmb_fops);
	rpmb_cdev.owner = THIS_MODULE;

	cdev_ret = cdev_add(&rpmb_cdev, MKDEV(major, 0U), 1);
	if (cdev_ret != 0) {
		MSG(ERR, "%s, init cdev_add failed!\n", __func__);
		goto error;
	}

#ifdef __RPMB_IOCTL_SUPPORT
	mtk_rpmb_class = class_create(THIS_MODULE, RPMB_NAME);

	if (IS_ERR(mtk_rpmb_class)) {
		MSG(ERR, "%s, init class_create failed!\n", __func__);
		goto error;
	}

	device = device_create(mtk_rpmb_class, NULL, MKDEV(major, 0), NULL,
		RPMB_NAME "%d", 0);
#endif
	if (IS_ERR(device)) {
		MSG(ERR, "%s, init device_create failed!\n", __func__);
		goto error;
	}

#if IS_ENABLED(CONFIG_TRUSTONIC_TEE_SUPPORT)
	open_th = kthread_run(rpmb_thread, NULL, "rpmb_open");
	if (IS_ERR(open_th))
		MSG(ERR, "%s, init kthread_run failed!\n", __func__);
#endif

	MSG(INFO, "%s end!!!!\n", __func__);

	return 0;

error:
#ifdef __RPMB_IOCTL_SUPPORT
	if (mtk_rpmb_class)
		class_destroy(mtk_rpmb_class);
#endif
	if (cdev_ret == 0)
		cdev_del(&rpmb_cdev);

	if (alloc_ret == 0)
		unregister_chrdev_region(dev, 1);

out_err:
	return -1;
}

late_initcall(rpmb_init);

MODULE_DESCRIPTION("RPMB class");
MODULE_LICENSE("GPL v2");
