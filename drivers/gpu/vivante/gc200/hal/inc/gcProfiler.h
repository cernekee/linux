/****************************************************************************
*
*    Copyright (C) 2005 - 2010 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public Lisence as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public Lisence for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/


#ifndef __gc_profile_h_
#define __gc_profile_h_

#define GLVERTEX_OBJECT 10
#define GLVERTEX_OBJECT_BYTES 11

#define GLINDEX_OBJECT 20
#define GLINDEX_OBJECT_BYTES 21

#define GLTEXTURE_OBJECT 30
#define GLTEXTURE_OBJECT_BYTES 31

#if VIVANTE_PROFILER
#define gcmPROFILE_GC(hal, _enum, value) countGCProfiler(hal, _enum, value)
#else
#define gcmPROFILE_GC(hal, _enum, value)
#endif

void countGCProfiler( IN gcoHAL , gctUINT32 , int );

/* HW profile information. */
struct _gcoHWProfile
{
    /* HW static counters. */
    gctUINT32       gpuClock;
    gctUINT32       axiClock;
    gctUINT32       shaderClock;
    /* HW vairable counters. */
    gctUINT32       gpuClockStart;
    gctUINT32       gpuClockEnd;

    /* HW vairable counters. */
    gctUINT32       gpuCyclesCounter;
    gctUINT32       gpuTotalRead64BytesPerFrame;
    gctUINT32       gpuTotalWrite64BytesPerFrame;

    /* FE */
	/*
    gctUINT32       fe_counter_0;
    gctUINT32       fe_counter_1;
    gctUINT32       fe_counter_2;
    gctUINT32       fe_counter_3;
    gctUINT32       fe_counter_4;
    gctUINT32       fe_counter_5;
    gctUINT32       fe_counter_6;
    gctUINT32       fe_counter_7;
    gctUINT32       fe_counter_8;
    gctUINT32       fe_counter_9;
    gctUINT32       fe_counter_10;
    gctUINT32       fe_counter_11;
    gctUINT32       fe_counter_12;
    gctUINT32       fe_counter_13;
    gctUINT32       fe_counter_14;
	*/

    /* DE */
	/*
    gctUINT32       de_counter_0;
    gctUINT32       de_counter_1;
    gctUINT32       de_counter_2;
    gctUINT32       de_counter_3;
    gctUINT32       de_counter_4;
    gctUINT32       de_counter_5;
    gctUINT32       de_counter_6;
    gctUINT32       de_counter_7;
    gctUINT32       de_counter_8;
    gctUINT32       de_counter_9;
    gctUINT32       de_counter_10;
    gctUINT32       de_counter_11;
    gctUINT32       de_counter_12;
    gctUINT32       de_counter_13;
    gctUINT32       de_counter_14;
	*/

    /* PE */
    gctUINT32       pe_pixel_count_killed_by_color_pipe;
    gctUINT32       pe_pixel_count_killed_by_depth_pipe;
    gctUINT32       pe_pixel_count_drawn_by_color_pipe;
    gctUINT32       pe_pixel_count_drawn_by_depth_pipe;
	/*
    gctUINT32       pe_counter_4;
    gctUINT32       pe_counter_5;
    gctUINT32       pe_counter_6;
    gctUINT32       pe_counter_7;
    gctUINT32       pe_counter_8;
    gctUINT32       pe_counter_9;
    gctUINT32       pe_counter_10;
    gctUINT32       pe_counter_11;
    gctUINT32       pe_counter_12;
    gctUINT32       pe_counter_13;
    gctUINT32       pe_counter_14;
	*/

    /* SH */
	/*
    gctUINT32       sh_counter_0;
    gctUINT32       sh_counter_1;
    gctUINT32       sh_counter_2;
    gctUINT32       sh_counter_3;
    gctUINT32       sh_counter_4;
    gctUINT32       sh_counter_5;
    gctUINT32       sh_counter_6;
	*/
    gctUINT32       ps_inst_counter;
    gctUINT32       rendered_pixel_counter;
    gctUINT32       vs_inst_counter;
    gctUINT32       rendered_vertice_counter;
    gctUINT32       vtx_branch_inst_counter;
    gctUINT32       vtx_texld_inst_counter;
    gctUINT32       pxl_branch_inst_counter;
    gctUINT32       pxl_texld_inst_counter;

    /* PA */
	/*
    gctUINT32       pa_pixel_count_killed_by_color_pipe;
    gctUINT32       pa_pixel_count_killed_by_depth_pipe;
    gctUINT32       pa_pixel_count_drawn_by_color_pipe;
	*/
    gctUINT32       pa_input_vtx_counter;
    gctUINT32       pa_input_prim_counter;
    gctUINT32       pa_output_prim_counter;
    gctUINT32       pa_depth_clipped_counter;
    gctUINT32       pa_trivial_rejected_counter;
    gctUINT32       pa_culled_counter;
	/*
    gctUINT32       pa_counter_9;
    gctUINT32       pa_counter_10;
    gctUINT32       pa_counter_11;
    gctUINT32       pa_counter_12;
    gctUINT32       pa_counter_13;
    gctUINT32       pa_counter_14;
	*/

    /* SE */
    gctUINT32       se_culled_triangle_count;
    gctUINT32       se_culled_lines_count;
	/*
    gctUINT32       se_counter_2;
    gctUINT32       se_counter_3;
    gctUINT32       se_counter_4;
    gctUINT32       se_counter_5;
    gctUINT32       se_counter_6;
    gctUINT32       se_counter_7;
    gctUINT32       se_counter_8;
    gctUINT32       se_counter_9;
    gctUINT32       se_counter_10;
    gctUINT32       se_counter_11;
    gctUINT32       se_counter_12;
    gctUINT32       se_counter_13;
    gctUINT32       se_counter_14;
	*/

    /* RA */
    gctUINT32       ra_valid_pixel_count;
    gctUINT32       ra_total_quad_count;
    gctUINT32       ra_valid_quad_count_after_early_z;
    gctUINT32       ra_total_primitive_count;
	/*
    gctUINT32       ra_counter_4;
    gctUINT32       ra_counter_5;
    gctUINT32       ra_counter_6;
    gctUINT32       ra_counter_7;
    gctUINT32       ra_counter_8;
	*/
    gctUINT32       ra_pipe_cache_miss_counter;
    gctUINT32       ra_prefetch_cache_miss_counter;
	gctUINT32       ra_eez_culled_counter;
	/*
    gctUINT32       ra_counter_11;
    gctUINT32       ra_counter_12;
    gctUINT32       ra_counter_13;
    gctUINT32       ra_counter_14;
	*/

    /* TX */
    gctUINT32       tx_total_bilinear_requests;
    gctUINT32       tx_total_trilinear_requests;
    gctUINT32       tx_total_discarded_texture_requests;
    gctUINT32       tx_total_texture_requests;
	/*
    gctUINT32       tx_counter_4;
	*/
    gctUINT32       tx_mem_read_count;
    gctUINT32       tx_mem_read_in_8B_count;
    gctUINT32       tx_cache_miss_count;
    gctUINT32       tx_cache_hit_texel_count;
    gctUINT32       tx_cache_miss_texel_count;
	/*
    gctUINT32       tx_counter_10;
    gctUINT32       tx_counter_11;
    gctUINT32       tx_counter_12;
    gctUINT32       tx_counter_13;
    gctUINT32       tx_counter_14;
	*/

    /* MC */
	/*
    gctUINT32       mc_counter_0;
	*/
    gctUINT32       mc_total_read_req_8B_from_pipeline;
    gctUINT32       mc_total_read_req_8B_from_IP;
    gctUINT32       mc_total_write_req_8B_from_pipeline;
	/*
    gctUINT32       mc_counter_4;
    gctUINT32       mc_counter_5;
    gctUINT32       mc_counter_6;
    gctUINT32       mc_counter_7;
    gctUINT32       mc_counter_8;
    gctUINT32       mc_counter_9;
    gctUINT32       mc_counter_10;
    gctUINT32       mc_counter_11;
    gctUINT32       mc_counter_12;
    gctUINT32       mc_counter_13;
    gctUINT32       mc_counter_14;
	*/
    /* HI */
    gctUINT32       hi_axi_cycles_read_request_stalled;
    gctUINT32       hi_axi_cycles_write_request_stalled;
    gctUINT32       hi_axi_cycles_write_data_stalled;
	/*
    gctUINT32       hi_counter_3;
    gctUINT32       hi_counter_4;
    gctUINT32       hi_counter_5;
    gctUINT32       hi_counter_6;
    gctUINT32       hi_counter_7;
    gctUINT32       hi_counter_8;
    gctUINT32       hi_counter_9;
    gctUINT32       hi_counter_10;
    gctUINT32       hi_counter_11;
    gctUINT32       hi_counter_12;
    gctUINT32       hi_counter_13;
    gctUINT32       hi_counter_14;
	*/
};

typedef struct _gcoHWProfile *	gcoHWProfile;


/* HAL profile information. */
struct _gcProfile
{
	gcoOS			os;

    gctFILE         file;
	gctUINT8		fileTempString[256];
	gctSIZE_T		tempMsgLen;

    /* Aggregate Information */

    /* Clock Info */
    gctUINT32       frameStart;
    gctUINT32       frameEnd;

    /* Current frame information */
    gctUINT32       frameNumber;
    gctUINT32       frameStartTimeMsec;
    gctUINT32       frameEndTimeMsec;

#if PROFILE_HAL_COUNTERS
    gctUINT32       vertexBufferTotalBytesAlloc;
    gctUINT32       vertexBufferNewBytesAlloc;
    int             vertexBufferTotalObjectsAlloc;
    int             vertexBufferNewObjectsAlloc;

    gctUINT32       indexBufferTotalBytesAlloc;
    gctUINT32       indexBufferNewBytesAlloc;
    int             indexBufferTotalObjectsAlloc;
    int             indexBufferNewObjectsAlloc;

    gctUINT32       textureBufferTotalBytesAlloc;
    gctUINT32       textureBufferNewBytesAlloc;
    int             textureBufferTotalObjectsAlloc;
    int             textureBufferNewObjectsAlloc;

    gctUINT32       numCommits;
    gctUINT32       drawPointCount;
    gctUINT32       drawLineCount;
    gctUINT32       drawTriangleCount;
    gctUINT32       drawVertexCount;
    gctUINT32       redundantStateChangeCalls;
#endif

#if PROFILE_HW_COUNTERS
    gcoHWProfile    hwProfile;
#endif

};


#endif

