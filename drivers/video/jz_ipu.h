#ifndef _JZ_IPU_H_
#define _JZ_IPU_H_

typedef struct {
   unsigned int coef;
   unsigned short int in_n;
   unsigned short int out_n;
} rsz_lut;

struct YuvCsc
{									// YUV(default)	or	YCbCr
	unsigned int csc0;				//	0x400			0x4A8
	unsigned int csc1;              //	0x59C   		0x662
	unsigned int csc2;              //	0x161   		0x191
	unsigned int csc3;              //	0x2DC   		0x341
	unsigned int csc4;              //	0x718   		0x811
	unsigned int chrom;             //	128				128
	unsigned int luma;              //	0				16
};

struct YuvStride
{
	unsigned int y;
	unsigned int u;
	unsigned int v;
	unsigned int out;
};


enum {
	ZOOM_MODE_BILINEAR = 0,
	ZOOM_MODE_BICUBE,
	ZOOM_MODE_BILINEAR_ENHANCE,
};


/* ipu driver ioctl command */
enum {
	IOCTL_IPU_OPEN = 1,
	IOCTL_IPU_CLOSE,
	IOCTL_IPU_INIT,
	IOCTL_IPU_DEINIT,
	IOCTL_IPU_REINIT,
	IOCTL_IPU_START,
	IOCTL_IPU_RESTART,
	IOCTL_IPU_STOP,
	IOCTL_IPU_RESIZE,
	IOCTL_IPU_SET_BUFF,
	IOCTL_IPU_SET_BUFF_SIZE,
	IOCTL_IPU_SWAP_BUFF,
	IOCTL_IPU_FB_SIZE,
	IOCTL_IPU_SET_CTRL_REG,
	IOCTL_IPU_SET_FMT_REG,
	IOCTL_IPU_SET_CSC,
	IOCTL_IPU_SET_STRIDE,
	IOCTL_IPU_SET_OUTSIZE,
	IOCTL_IPU_PENDING_OUTEND,
	IOCTL_IPU_GET_ADDR,
	IOCTL_IPU_DUMP_REGS,
};

//////////////////////////////////////////////////
	/* set ipu output mode */
//#define IPU_OUTPUT_TO_LCD_FG0           0x00000001 /* hw not support */
#define IPU_OUTPUT_TO_LCD_FG1           0x00000002
#define IPU_OUTPUT_TO_LCD_FB0           0x00000004
#define IPU_OUTPUT_TO_LCD_FB1           0x00000008
#define IPU_OUTPUT_TO_FRAMEBUFFER       0x00000010 /* output to user defined buffer */
#define IPU_OUTPUT_MODE_MASK            0x000000FF
#define IPU_DST_USE_COLOR_KEY           0x00000100
#define IPU_DST_USE_ALPHA               0x00000200
#define IPU_OUTPUT_BLOCK_MODE           0x00000400
#define IPU_OUTPUT_MODE_FG0_OFF         0x00000800
#define IPU_OUTPUT_MODE_FG1_TVOUT       0x00001000
#define IPU_DST_USE_PERPIXEL_ALPHA      0x00010000

struct ipu_img_param_t
{
	unsigned int 		version;			/* sizeof(struct ipu_img_param_t) */
	int			ipu_cmd;			// IPU command
	unsigned int		output_mode;			// IPU output mode: fb0, fb1, fg1, alpha, colorkey ...
//	unsigned int		ipu_ctrl;			// IPU Control Register
//	unsigned int		ipu_d_fmt;			// IPU Data Format Register
	unsigned int		alpha;
	unsigned int		colorkey;
	unsigned int		in_width;
	unsigned int		in_height;
	unsigned int		in_bpp;
	unsigned int		out_x;
	unsigned int		out_y;
	unsigned int		in_fmt;
	unsigned int		out_fmt;
	unsigned int		out_width;
	unsigned int		out_height;
	unsigned char*		y_buf_v; /* Y buffer virtual address */
	unsigned char*		u_buf_v;
	unsigned char*		v_buf_v;
	unsigned int		y_buf_p; /* Y buffer physical address */
	unsigned int		u_buf_p;
	unsigned int		v_buf_p;
	unsigned char*		out_buf_v;
	unsigned int		out_buf_p;
	unsigned int 		src_page_mapping;
	unsigned int 		dst_page_mapping;
	unsigned char*		y_t_addr;				// table address
	unsigned char*		u_t_addr;
	unsigned char*		v_t_addr;
	unsigned char*		out_t_addr;
	struct YuvCsc*		csc;
	struct YuvStride	stride;
	int 			Wdiff;
	int 			Hdiff;
	unsigned int		zoom_mode;
	int 			hcoef_real_heiht;
	int 			vcoef_real_heiht;
	int*			hoft_table;
	int* 			voft_table;
	int*			hcoef_table;
	int*			vcoef_table;
	void*			cube_hcoef_table;
	void*			cube_vcoef_table;
	int*                    pic_enhance_table;
};


#ifdef __KERNEL__

/* match HAL_PIXEL_FORMAT_ in hardware/libhardware/include/hardware/hardware.h */
enum {
    HAL_PIXEL_FORMAT_RGBA_8888    = 1,
    HAL_PIXEL_FORMAT_RGBX_8888    = 2,
    HAL_PIXEL_FORMAT_RGB_888      = 3,
    HAL_PIXEL_FORMAT_RGB_565      = 4,
    HAL_PIXEL_FORMAT_BGRA_8888    = 5,
    HAL_PIXEL_FORMAT_BGRX_8888    = 0x8000, /* Add BGRX_8888, Wolfgang, 2010-07-24 */
    HAL_PIXEL_FORMAT_RGBA_5551    = 6,
    HAL_PIXEL_FORMAT_RGBA_4444    = 7,
    HAL_PIXEL_FORMAT_YCbCr_422_SP = 0x10,
    HAL_PIXEL_FORMAT_YCbCr_420_SP = 0x11,
    HAL_PIXEL_FORMAT_YCbCr_422_P  = 0x12,
    HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,
    HAL_PIXEL_FORMAT_YCbCr_420_B  = 0x24,
    HAL_PIXEL_FORMAT_YCbCr_422_I  = 0x14,
    HAL_PIXEL_FORMAT_YCbCr_420_I  = 0x15,
    HAL_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
    HAL_PIXEL_FORMAT_CbYCrY_420_I = 0x17,

    /* suport for YUV420 */
    HAL_PIXEL_FORMAT_JZ_YUV_420_P       = 0x47700001, // YUV_420_P
    HAL_PIXEL_FORMAT_JZ_YUV_420_B       = 0x47700002, // YUV_420_P BLOCK MODE
};



#include "jz47xx_ipu.h"

/* 
 * ============================================================ 
 * IPU driver's native data
 */

#define IPU_LUT_LEN                                     (32)

struct ipu_driver_priv {
	struct ipu_img_param_t img;
	struct jz47xxlcd_info *lcd_info;
	int inited;
	int rtable_len;

	unsigned int fb_w;
	unsigned int fb_h;
	unsigned int ipu_state;
//	struct ipu_img_param_t ipu_img_param;
	//struct ipu_img_param_t *ipu_img = &g_ipu_img_param;
	rsz_lut h_lut[IPU_LUT_LEN];
	rsz_lut v_lut[IPU_LUT_LEN];

	/* jz47xx's ipu HRSZ_COEF_LUT*/
	int hoft_table[IPU_LUT_LEN+1]; /*  */
	int hcoef_table[IPU_LUT_LEN+1];
	/* jz47xx's ipu VRSZ_COEF_LUT*/
	int voft_table[IPU_LUT_LEN+1];
	int vcoef_table[IPU_LUT_LEN+1];
	/* BICUBE */
	int cube_hcoef_table[32][4];
	int cube_vcoef_table[32][4];

	int pic_enhance_table[256];

	spinlock_t	update_lock;
	wait_queue_head_t frame_wq;
	int frame_requested;
	int frame_done;
	int restart_trigger;
	int is_suspended;
};

extern struct ipu_driver_priv *ipu_priv;


// Function prototype

int ipu_driver_open(struct ipu_driver_priv *ipu);
int ipu_driver_close(struct ipu_driver_priv *ipu);
int ipu_driver_init(struct ipu_driver_priv *ipu, struct ipu_img_param_t *new_img);
int ipu_driver_deinit(struct ipu_driver_priv *ipu, struct ipu_img_param_t *new_img);
int ipu_driver_start(struct ipu_driver_priv *ipu);
int ipu_driver_stop(struct ipu_driver_priv *ipu);
int ipu_driver_swapBuffer(struct ipu_driver_priv *ipu, struct ipu_img_param_t *new_img);
int ipu_driver_setBuffer(struct ipu_driver_priv *ipu, struct ipu_img_param_t *new_img);
int ipu_driver_resize(struct ipu_driver_priv *ipu, struct ipu_img_param_t *new_img);
int ipu_driver_register_irq(struct ipu_driver_priv *ipu);
int ipu_driver_dump_regs(struct ipu_driver_priv *ipu);

int ipu_driver_ioctl(struct ipu_driver_priv *ipu, void *uimg);

int ipu_driver_suspend(struct ipu_driver_priv *ipu);
int ipu_driver_resume(struct ipu_driver_priv *ipu);

#endif	/* #ifdef __KERNEL__ */

#endif // _JZ_IPU_H_
