#ifdef GLQUAKE
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// vid_null.c -- null video driver to aid porting efforts

#include "quakedef.h"
#include <dc/pvr.h>
#include <dc/sq.h>

viddef_t	vid;				// global video state

#define	BASEWIDTH	320
#define	BASEHEIGHT	240

//byte	vid_buffer[BASEWIDTH*BASEHEIGHT];
//short	zbuffer[BASEWIDTH*BASEHEIGHT];
//byte	surfcache[256*1024];

float RTint[256],GTint[256],BTint[256];

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned	d_8to24TranslucentTable[256];
byte globalcolormap[VID_GRADES*256];

qboolean	vid_initialized = false;


/* cvar_t		vid_mode = {"vid_mode","5",false};
cvar_t		vid_redrawfull = {"vid_redrawfull","0",false};
cvar_t		vid_waitforrefresh = {"vid_waitforrefresh","0",true}; */

int		texture_mode = GL_LINEAR;
 
int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

cvar_t	gl_ztrick = {"gl_ztrick","0"};

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean is8bit = false;

unsigned char inverse_pal[(1<<INVERSE_PAL_TOTAL_BITS)+1];

#define	RGB565(r,g,b)	( (((r)>>3)<<11) |(((g)>>2)<<5)|((b)>>3) )

int ColorIndex[16] =
{
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[16] =
{
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};

static int ConvertTrueColorToPal( unsigned char *true_color, unsigned char *palette )
{
	int i;
	long min_dist;
	int min_index;
	long r, g, b;

	min_dist = 256 * 256 + 256 * 256 + 256 * 256;
	min_index = -1;
	r = ( long )true_color[0];
	g = ( long )true_color[1];
	b = ( long )true_color[2];

	for( i = 0; i < 256; i++ )
	{
		long palr, palg, palb, dist;
		long dr, dg, db;

		palr = palette[3*i];
		palg = palette[3*i+1];
		palb = palette[3*i+2];
		dr = palr - r;
		dg = palg - g;
		db = palb - b;
		dist = dr * dr + dg * dg + db * db;
		if( dist < min_dist )
		{
			min_dist = dist;
			min_index = i;
		}
	}
	return min_index;
}

void	VID_CreateInversePalette( unsigned char *palette )
{
#if 1
//	FILE *FH;

	long r, g, b;
	long index = 0;
	unsigned char true_color[3];
	static qboolean been_here = false;

	if( been_here )
		return;

	been_here = true;
	
	for( r = 0; r < ( 1 << INVERSE_PAL_R_BITS ); r++ )
	{
		for( g = 0; g < ( 1 << INVERSE_PAL_G_BITS ); g++ )
		{
			for( b = 0; b < ( 1 << INVERSE_PAL_B_BITS ); b++ )
			{
				true_color[0] = ( unsigned char )( r << ( 8 - INVERSE_PAL_R_BITS ) );
				true_color[1] = ( unsigned char )( g << ( 8 - INVERSE_PAL_G_BITS ) );
				true_color[2] = ( unsigned char )( b << ( 8 - INVERSE_PAL_B_BITS ) );
				inverse_pal[index] = ConvertTrueColorToPal( true_color, palette );
				index++;
			}
		}
	}

//	FH = fopen("data1\\gfx\\invpal.lmp","wb");
//	fwrite(inverse_pal,1,sizeof(inverse_pal),FH);
//	fclose(FH);
#else
	COM_LoadStackFile ("gfx/invpal.lmp",inverse_pal,sizeof(inverse_pal));
#endif
}

void VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	int		r,g,b,v;
	int		i,c,p;
	unsigned	*table;
	
	
//	if( is_3dfx || is_PowerVR)
		VID_CreateInversePalette( palette );

//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}

	d_8to24table[255] &= 0xffffff;	// 255 is transparent

//	if( is_3dfx || is_PowerVR )
		VID_Init8bitPalette();

	pal = palette;
	table = d_8to24TranslucentTable;

	for (i=0; i<16;i++)
	{
		c = ColorIndex[i]*3;

		r = pal[c];
		g = pal[c+1];
		b = pal[c+2];

		for(p=0;p<16;p++)
		{
			v = (ColorPercent[15-p]<<24) + (r<<0) + (g<<8) + (b<<16);
			//v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			*table++ = v;

			RTint[i*16+p] = ((float)r) / ((float)ColorPercent[15-p]) ;
			GTint[i*16+p] = ((float)g) / ((float)ColorPercent[15-p]);
			BTint[i*16+p] = ((float)b) / ((float)ColorPercent[15-p]);
		}
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
//	VID_SetPalette(palette);
}

void VID_Init8bitPalette(void)
{
	Con_SafePrintf("... Using GL_EXT_shared_texture_palette\n");
	glEnable( GL_SHARED_TEXTURE_PALETTE_EXT );

	glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, d_8to24table );
	is8bit = true;
}

#define PM_RGB555	0
#define PM_RGB565	1
#define PM_RGB888	3
#define DM_320x240	1

pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 16 */
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

        /* Vertex buffer size 1024K */
        512*1024
};

static void dmyfunc()
{
}

void	VID_Init (unsigned char *palette)
{
/*	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&vid_redrawfull);
	Cvar_RegisterVariable (&vid_waitforrefresh); */
	Cvar_RegisterVariable (&gl_ztrick);

	vid_initialized = true;

	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 2;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
//	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;
	
//	d_pzbuffer = zbuffer;
//	D_InitCaches (surfcache, sizeof(surfcache));

	vid_set_mode(DM_320x240, PM_RGB565);
	pvr_init(&params);

	GL_Init();

//	Check_Gamma(palette);
	VID_SetPalette(palette);

	VID_Init8bitPalette();
/*
	qglMTexCoord2fSGIS = dmyfunc;
	qglSelectTextureSGIS = dmyfunc;
	gl_mtexable = true;
*/

	vid.recalc_refdef = 1;				// force a surface cache flush
}

void	VID_Shutdown (void)
{
}

qboolean VID_Is8bit(void)
{
	return is8bit;
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	glKosInit();

	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

//	CheckMultiTextureExtensions ();

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
/*
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
*/
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

//	glKosBeginFrame();
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;
	float w,h;

	glKosGetScreenSize(&w,&h);

	*x = *y = 0;
	*width = w;
	*height = h;


//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
//	printf("begin\n");
	glKosBeginFrame();
}


void GL_EndRendering (void)
{
//	glFlush();
//	fxMesaSwapBuffers();
	glKosFinishFrame();
//	glKosBeginFrame();
}

void D_ShowLoadingSize(void)
{
	if (!vid_initialized)
		return;

#if 1
	glDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	glDrawBuffer  (GL_BACK);
#endif
}

#endif
