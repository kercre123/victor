// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - LSC filter
//
//
// -----------------------------------------------------------------------------

#include "sippLSC.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#else
#include <cmath>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>


LscFilt::LscFilt(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k),
	lsc_gmb("input", Read) {

		// pObj, name, id and kernel_lines are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);
		
		Reset();
}

void LscFilt::Reset(void) {
	update_gm_idx = true;
	no_vert_interp      = false;
	no_vert_interp_next = false;
	y_even = 0;
}

void LscFilt::SetHorIncrement(int val, int reg) {
	lsc_params[reg].inc_h.full = val;
}

void LscFilt::SetVerIncrement(int val, int reg) {
	lsc_params[reg].inc_v.full = val;
}

void LscFilt::SetLscFormat(int val, int reg) {
	lsc_params[reg].lsc_format = val;

	if(lsc_params[reg].lsc_format == BAYER) {
		gm_lines = 4;
	} else {
		gm_lines = 2;
		gm_crt_line1 = 0;
		gm_crt_line2 = 1;
	}
}

void LscFilt::SetDataWidth(int val, int reg) {
	lsc_params[reg].data_width = val;
}

void LscFilt::SetLscGmWidth(int val, int reg) {
	if(val < 2 || val > 1023) {
		std::cerr << "Invalid gain mesh width: must be in range [2 - 1023]." << std::endl;
		abort();
	}
	lsc_params[reg].lsc_gm_width = val;

	if(lsc_params[reg].lsc_format == BAYER)
		actual_width = lsc_params[reg].lsc_gm_width >> 1;
	else
		actual_width = lsc_params[reg].lsc_gm_width;
}

void LscFilt::SetLscGmHeight(int val, int reg) {
	if(val < 2 || val > 1023) {
		std::cerr << "Invalid gain mesh height: must be in range [2 - 1023]." << std::endl;
		abort();
	}
	lsc_params[reg].lsc_gm_height = val;

	if(lsc_params[reg].lsc_format == BAYER)
		actual_height = lsc_params[reg].lsc_gm_height >> 1;
	else
		actual_height = lsc_params[reg].lsc_gm_height;
}

void LscFilt::getGainMapLines(FIXED8_8 ***gain_ptr) {
	int lstride, pstride;

	lstride = lsc_gmb.GetLineStride()  / lsc_gmb.GetFormat();
	pstride = lsc_gmb.GetPlaneStride() / lsc_gmb.GetFormat();
	
	for (int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
		for(int ln = 0; ln < gm_lines; ln++)
			gain_ptr[pl][ln] = (FIXED8_8 *)((uint16_t *)lsc_gmb.buffer 
				  + lstride * lsc_gmb.GetBufferIdx(ln)
				  + pstride * pl);
}

void LscFilt::VerticalAdvance(void) {
	if(j0 + 1 < (uint32_t)(actual_height - 1)) {
		update_gm_idx = true;
	} else {
		no_vert_interp_next = true;
		update_gm_idx = false;
	}
}

void LscFilt::BayerAdjustHCoordinates(int x) {
	if(no_horz_interp) {
		if(!(x % 2))
			i0 = lsc_gm_width - 2;
		else
			i0 = lsc_gm_width - 1;
	} else {
		if(!(x % 2))
			i0 = (i0 << 1);
		else
			i0 = (i0 << 1) + 1;
		i1 = i0 + 2;
	}
}

void LscFilt::PlanarAdjustHCoordinates(int x) {
	if(no_horz_interp) {
		i0 = lsc_gm_width - 1;
	} else {
		i1 = i0 + 1;
	}
}

void LscFilt::DetermineUsedMesh(int pl) {
	int gm_planes = lsc_gmb.GetNumPlanes() + 1;

	gm_crt_plane = pl % gm_planes;
}

void LscFilt::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	lsc_gmb.SelectBufferParameters();
	inc_h         = lsc_params[reg].inc_h;
	inc_v         = lsc_params[reg].inc_v;
	lsc_format    = lsc_params[reg].lsc_format;
	data_width    = lsc_params[reg].data_width;
	lsc_gm_width  = lsc_params[reg].lsc_gm_width;
	lsc_gm_height = lsc_params[reg].lsc_gm_height;
}

void LscFilt::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **   split_in_ptr_8  = 0;
	uint8_t  **         in_ptr_8  = 0;
	uint16_t **   split_in_ptr_16 = 0;
	uint16_t **         in_ptr_16 = 0;
	uint8_t  *         out_ptr_8  = 0;
	uint16_t *         out_ptr_16 = 0;
	static FIXED8_8 ***gain_ptr   = 0;
	 
	void **in_ptr;
	void *out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  *[kernel_lines];
		      in_ptr_8  = new uint8_t  *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t *[kernel_lines];
		      in_ptr_16 = new uint16_t *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
	}

	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Prepare gain mesh map buffer line pointers
	if(update_gm_idx) {
		gain_ptr = new FIXED8_8 **[lsc_gmb.GetNumPlanes() + 1];
		for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
			gain_ptr[pl] = new FIXED8_8 *[gm_lines];
		getGainMapLines(gain_ptr);
	}

	// Determine the vertical coordinate within the gain mesh map
	// and the interpolation lines in case of Bayer format
	if(lsc_format == BAYER) {
		if(!(gm_crt_line1 = line_idx % 2))
			v.full = MUL160_016(y_even++, inc_v);
		gm_crt_line2 = gm_crt_line1 + 2;
	} else {
		v.full = MUL160_016(line_idx, inc_v);
	}
	j0 = v.part.integer;

	// Determine if the gain mesh map buffer index ought to be increased
	if((ADD1616_016(v, inc_v) >= ((j0 + 1) << 16))) {
		if(lsc_format == BAYER) {
			if(line_idx % 2) {
				VerticalAdvance();
			} else {
				update_gm_idx = false;
			}
		} else {
			VerticalAdvance();
		}
	} else {
		update_gm_idx = false;
	}

	// Filter a line from each plane
	for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
		// Determine the gain mesh map plane
		DetermineUsedMesh(pl);

		// Prepare filter buffer line pointers then run
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			getKernelLines(split_in_ptr_8 , kernel_lines, pl);
			PackInputLines(split_in_ptr_8 , kernel_lines, in_ptr_8 );
		} else {
			getKernelLines(split_in_ptr_16, kernel_lines, pl);
			PackInputLines(split_in_ptr_16, kernel_lines, in_ptr_16);
		}
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			setOutputLine(&out_ptr_8 , pl);
			out_ptr = (void *)out_ptr_8 ;
		} else {
			setOutputLine(&out_ptr_16, pl);
			out_ptr = (void *)out_ptr_16;
		}
		Run(in_ptr, (void ***)gain_ptr, out_ptr);
	}
	// Clear start bit, update buffer fill levels and input frame line index
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();

	// Skip vertical interpolation for subsequent runs
	no_vert_interp = no_vert_interp_next;

	// Increment gain mesh map buffer index if necessary
	bool deleted = false;
	if(update_gm_idx) {
		lsc_gmb.IncBufferIdx();
		if(lsc_format == BAYER)
			lsc_gmb.IncBufferIdx();
		for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
			delete[] gain_ptr[pl];
		delete[] gain_ptr;
		deleted = true;
	}

	// Trigger end of frame interrupt and reset the gain mesh map index
	if (EndOfFrame()) {
		Reset();
		lsc_gmb.SetBufferIdx(0);
		if(!deleted) {
			for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
				delete[] gain_ptr[pl];
			delete[] gain_ptr;
		}

		EndOfFrameIrq();
	}

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] split_in_ptr_8 ;
	else
		delete[] split_in_ptr_16;
	delete[] in_ptr;
}

void LscFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  **   split_in_ptr_8  = 0;
	uint8_t  **         in_ptr_8  = 0;
	uint16_t **   split_in_ptr_16 = 0;
	uint16_t **         in_ptr_16 = 0;
	uint8_t  *         out_ptr_8  = 0;
	uint16_t *         out_ptr_16 = 0;
	static FIXED8_8 ***gain_ptr   = 0;

	void **in_ptr;
	void *out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first dimension of pointers to split input lines
	// and allocate pointers to hold packed incoming data
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		split_in_ptr_8  = new uint8_t  *[kernel_lines];
		      in_ptr_8  = new uint8_t  *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_8 [ln] = new uint8_t [width];
		in_ptr = (void **)in_ptr_8;
	} else {
		split_in_ptr_16 = new uint16_t *[kernel_lines];
		      in_ptr_16 = new uint16_t *[kernel_lines];
		for(int ln = 0; ln < kernel_lines; ln++)
			in_ptr_16[ln] = new uint16_t[width];
		in_ptr = (void **)in_ptr_16;
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_lines)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Prepare gain mesh map buffer line pointers
		if(update_gm_idx) {
			gain_ptr = new FIXED8_8 **[lsc_gmb.GetNumPlanes() + 1];
			for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
				gain_ptr[pl] = new FIXED8_8 *[gm_lines];
			getGainMapLines(gain_ptr);
		}

		// Determine the vertical coordinate within the gain mesh map
		// and the interpolation lines in case of Bayer format
		if(lsc_format == BAYER) {
			if(!(gm_crt_line1 = line_idx % 2))
				v.full = MUL160_016(y_even++, inc_v);
			gm_crt_line2 = gm_crt_line1 + 2;
		} else {
			v.full = MUL160_016(line_idx, inc_v);
		}
		j0 = v.part.integer;

		// Determine if the gain mesh map buffer index ought to be increased
		if((ADD1616_016(v, inc_v) >= ((j0 + 1) << 16))) {
			if(lsc_format == BAYER) {
				if(line_idx % 2) {
					VerticalAdvance();
				} else {
					update_gm_idx = false;
				}
			} else {
				VerticalAdvance();
			}
		} else {
			update_gm_idx = false;
		}

		// Filter a line from each plane
		for (int pl = 0; pl <= in.GetNumPlanes(); pl++) {
			// Determine the gain mesh map plane
			DetermineUsedMesh(pl);

			// Prepare filter buffer line pointers then run
			if(in.GetFormat() == SIPP_FORMAT_8BIT) {
				getKernelLines(split_in_ptr_8 , kernel_lines, pl);
				PackInputLines(split_in_ptr_8 , kernel_lines, in_ptr_8 );
			} else {
				getKernelLines(split_in_ptr_16, kernel_lines, pl);
				PackInputLines(split_in_ptr_16, kernel_lines, in_ptr_16);
			}
			if(out.GetFormat() == SIPP_FORMAT_8BIT) {
				setOutputLine(&out_ptr_8 , pl);
				out_ptr = (void *)out_ptr_8 ;
			} else {
				setOutputLine(&out_ptr_16, pl);
				out_ptr = (void *)out_ptr_16;
			}
			Run(in_ptr, (void ***)gain_ptr, out_ptr);
		}
		// Update buffer fill levels and input frame line index
		UpdateBuffers();
		IncLineIdx();

		// Skip vertical interpolation for subsequent runs
		no_vert_interp = no_vert_interp_next;

		// Increment gain mesh map buffer index if necessary
		bool deleted = false;
		if(update_gm_idx) {
			lsc_gmb.IncBufferIdx();
			if(lsc_format == BAYER)
				lsc_gmb.IncBufferIdx();
			for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
				delete[] gain_ptr[pl];
			delete[] gain_ptr;
			deleted = true;
		}

		// At the end of the frame
		if (EndOfFrame()) {
			// Reset the gain map mesh index
			Reset();
			lsc_gmb.SetBufferIdx(0);
			if(!deleted) {
				for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++)
					delete[] gain_ptr[pl];
				delete[] gain_ptr;
			}

			// Trigger end of frame interrupt
			EndOfFrameIrq();

			// Disable the filter unless the
			// input buffer is circular
			if(in.GetBuffLines() == 0) {
				Disable();
				break;
			}
		}
	}
	cs.Leave();

	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] split_in_ptr_8 ;
	else
		delete[] split_in_ptr_16;
	delete[] in_ptr;
}

void LscFilt::Run(void **in_ptr, void ***gm_ptr, void *out_ptr) {
	uint8_t  **        src_8  = 0;
	uint16_t **        src_16 = 0;
	uint8_t  *         dst_8  = 0;
	uint8_t  *  packed_dst_8  = 0;
	uint16_t *         dst_16 = 0;
	uint16_t *  packed_dst_16 = 0;
	FIXED8_8 ***gain_mesh_map = 0;

	// Allocating array of pointers to input lines
	// and setting input pointers
	if(in.GetFormat() == SIPP_FORMAT_8BIT) {
		src_8  = new uint8_t  *[kernel_lines];
		// Setting input pointers
		for (int i = 0; i < kernel_lines; i++) {
			src_8 [i] = (uint8_t  *)in_ptr[i];
		}
	} else {
		src_16 = new uint16_t *[kernel_lines];
		// Setting input pointers
		for (int i = 0; i < kernel_lines; i++) {
			src_16[i] = (uint16_t *)in_ptr[i];
		}
	}

	// Allocating array of pointers to gain mesh map planes
	gain_mesh_map = new FIXED8_8 **[lsc_gmb.GetNumPlanes() + 1];

	// Set gain mesh map pointers
	for(int pl = 0; pl <= lsc_gmb.GetNumPlanes(); pl++) {
		gain_mesh_map[pl] = (FIXED8_8 **)gm_ptr[pl];
		for(int ln = 0; ln < gm_lines; ln++)
			gain_mesh_map[pl][ln] = (FIXED8_8 *)gm_ptr[pl][ln];
	}

	// Setting output pointer and allocating temporary packed output buffer
	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		       dst_8  = (uint8_t  *)out_ptr;
		packed_dst_8  = new uint8_t [width];
	} else {
		       dst_16 = (uint16_t *)out_ptr;
		packed_dst_16 = new uint16_t[width];
	}

	FIXED8_8 gain_v[2];
	FIXED8_8 gain;
	FIXED0_16 alpha, beta;
	FIXED8_8 fixed_diff;
	FIXED8_8 fixed_mul;
	bool add;

	uint32_t out_val;

	int x = 0;
	x_even = 0;
	no_horz_interp = no_horz_interp_next = false;

	// Get the fractional part of v
	beta.full = v.part.fraction;

	while(x < width) {
		// Determine the horizontal coordinate within the gain mesh map
		if(lsc_format == BAYER) {
			if(!(x % 2))
				u.full = MUL160_016(x_even++, inc_h);
		} else {
			u.full = MUL160_016(x, inc_h);
		}

		// Map the (u, v) value into a 2x2 square and get the fractional part of u
		i0 = u.part.integer;
		alpha.full = u.part.fraction;

		if(ADD1616_016(u, inc_h) >= ((i0 + 1) << 16))
			if(lsc_format == BAYER) {
				if(x % 2)
					no_horz_interp_next = ((i0 + 1) >= (uint32_t)(actual_width - 1)) ? true : false;
			} else {
				no_horz_interp_next = ((i0 + 1) >= (uint32_t)(actual_width - 1)) ? true : false;
			}
		
		// Reach the correct position within the gain mesh map
		if(lsc_format == BAYER)
			BayerAdjustHCoordinates(x);
		else
			PlanarAdjustHCoordinates(x);
		
		// Decide whether interpolations are required or not
		if(no_vert_interp) {
			if(no_horz_interp) {
				gain.full = gain_mesh_map[gm_crt_plane][gm_crt_line2][i0].full;
			} else {
				gain_v[0].full = gain_mesh_map[gm_crt_plane][gm_crt_line2][i0].full;
				gain_v[1].full = gain_mesh_map[gm_crt_plane][gm_crt_line2][i1].full;
			}
		} else {
			// First pair of vertically interpolated gain mesh values
			if(add = (gain_mesh_map[gm_crt_plane][gm_crt_line2][i0].full >=
			          gain_mesh_map[gm_crt_plane][gm_crt_line1][i0].full))
				fixed_diff.full = SUB8_8(gain_mesh_map[gm_crt_plane][gm_crt_line2][i0],
				                         gain_mesh_map[gm_crt_plane][gm_crt_line1][i0]);
			else
				fixed_diff.full = SUB8_8(gain_mesh_map[gm_crt_plane][gm_crt_line1][i0],
				                         gain_mesh_map[gm_crt_plane][gm_crt_line2][i0]);
			fixed_mul.full = MUL88_016_R88(fixed_diff, beta);

			// Change operation accordingly
			/* Linear interpolation is defined as follows:
			 *                G = x * (1 - a) + y * a
			 * What the hardware actually does is:
			 *                G = x + a * (y - x)
			 * Because the subtraction is done is fixed point arithmetic, one has to check
			 * that the minuend is greater than the subtrahend. Otherwise, the order of
			 * the operation has to be changed. For the latter, the way the gain(G) is
			 * computed has to be altered as well: G = x - a * (x - y) 
			 */
			if(add)
				gain_v[0].full = ADD8_8(gain_mesh_map[gm_crt_plane][gm_crt_line1][i0], fixed_mul);
			else
				gain_v[0].full = SUB8_8(gain_mesh_map[gm_crt_plane][gm_crt_line1][i0], fixed_mul);

			// Second pair of vertically interpolated gain mesh values
			// Note: this is necessary iff horizontal interpolation is performed
			if(!no_horz_interp) {
				if(add = (gain_mesh_map[gm_crt_plane][gm_crt_line2][i1].full >=
				          gain_mesh_map[gm_crt_plane][gm_crt_line1][i1].full))
					fixed_diff.full = SUB8_8(gain_mesh_map[gm_crt_plane][gm_crt_line2][i1],
					                         gain_mesh_map[gm_crt_plane][gm_crt_line1][i1]);
				else
					fixed_diff.full = SUB8_8(gain_mesh_map[gm_crt_plane][gm_crt_line1][i1],
					                         gain_mesh_map[gm_crt_plane][gm_crt_line2][i1]);
				fixed_mul.full = MUL88_016_R88(fixed_diff, beta);

				// Change operation accordingly
				if(add)
					gain_v[1].full = ADD8_8(gain_mesh_map[gm_crt_plane][gm_crt_line1][i1], fixed_mul);
				else
					gain_v[1].full = SUB8_8(gain_mesh_map[gm_crt_plane][gm_crt_line1][i1], fixed_mul);
			}
		}

		// Interpolate horizontally if required
		if(no_horz_interp) {
			if(!no_vert_interp)
				gain.full = gain_v[0].full;
		} else {
			if(add = (gain_v[1].full >= gain_v[0].full))
				fixed_diff.full = SUB8_8(gain_v[1], gain_v[0]);
			else
				fixed_diff.full = SUB8_8(gain_v[0], gain_v[1]);
			fixed_mul.full = MUL88_016_R88(fixed_diff, alpha);

			// Change operation accordingly
			if(add)
				gain.full = ADD8_8(gain_v[0], fixed_mul);
			else
				gain.full = SUB8_8(gain_v[0], fixed_mul);
		}

		// Output the saturated value
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			if(in.GetFormat() == SIPP_FORMAT_8BIT)
				out_val = MUL88_80_R80 (gain, src_8 [0][x]);
			else
				out_val = MUL88_160_R80(gain, src_16[0][x]);
			packed_dst_8 [x] = (uint8_t )(ClampWR((float)out_val, 0.0f, (float)((1 << (data_width + 1)) - 1)));
		} else {
			if(in.GetFormat() == SIPP_FORMAT_8BIT)
				out_val = MUL88_80_R160 (gain, src_8 [0][x]);
			else
				out_val = MUL88_160_R160(gain, src_16[0][x]);
			packed_dst_16[x] = (uint16_t)(ClampWR((float)out_val, 0.0f, (float)((1 << (data_width + 1)) - 1)));
		}
		no_horz_interp = no_horz_interp_next;
		x++;
	}

	if(out.GetFormat() == SIPP_FORMAT_8BIT) {
		SplitOutputLine(packed_dst_8 , width, dst_8 );
		delete[] packed_dst_8 ;
	} else {
		SplitOutputLine(packed_dst_16, width, dst_16);
		delete[] packed_dst_16;
	}

	delete[] gain_mesh_map;
	if(in.GetFormat() == SIPP_FORMAT_8BIT)
		delete[] src_8;
	else
		delete[] src_16;
}

// Not used in this particular filter as it has his own version of Run
void *LscFilt::Run(void **in_ptr, void *out_ptr)
{
	return NULL;
}

