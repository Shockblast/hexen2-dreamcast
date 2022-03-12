CC=sh-elf-gcc -ml -m4-single-only
KOS_BASE=/prog/kos-1.1.7

LWIPDIR = $(KOS_BASE)/addons/lwip/src
LWIPARCH = kos
LWIP_INCS = -I$(LWIPDIR)/include -I$(LWIPDIR)/arch/$(LWIPARCH)/include \
	-I$(LWIPDIR)/include/ipv4

KOS_INCS=-I$(KOS_BASE)/libc/include -I$(KOS_BASE)/kernel/arch/dreamcast/include -I$(KOS_BASE)/include 

BASE_CFLAGS=-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D__inline="static __inline__" -D_inline="static __inline__" $(KOS_INCS) $(LWIP_INCS) -Isupport -DQUAKE2RJ -DRJNET -DDC -DUSE_ZLIB
RELEASE_CFLAGS=$(BASE_CFLAGS) -g -O6 -ffast-math -funroll-loops -ffast-math \
	-fomit-frame-pointer -fexpensive-optimizations
CFLAGS=$(RELEASE_CFLAGS) -DGLQUAKE

COMMON_OBJS = chase.o cl_demo.o cl_input.o cl_main.o cl_parse.o cl_tent.o \
	cmd.o common.o console.o crc.o cvar.o \
	host.o host_cmd.o keys.o mathlib.o menu.o \
	net_loop.o net_main.o net_vcr.o \
	pr_cmds.o pr_edict.o pr_exec.o r_part.o sbar.o \
	sv_main.o sv_move.o sv_phys.o sv_user.o \
	view.o wad.o world.o zone.o \
	cl_effect.o # hexen2 only

SND_OBJS = 	snd_dma.o snd_mem.o snd_mix.o

NET_OBJS = net_bsd.o net_dgrm.o \
 net_udp.o  \
 netdb.o #DC

NET_NULL_OBJS = net_none.o

SOFT_OBJS = d_edge.o d_fill.o d_init.o d_modech.o d_part.o d_polyse.o d_scan.o \
	d_sky.o d_sprite.o d_surf.o d_vars.o d_zpoint.o draw.o  model.o \
	r_aclip.o r_alias.o r_bsp.o r_draw.o r_edge.o r_efrag.o r_light.o r_main.o r_misc.o r_sky.o \
	r_sprite.o r_surf.o r_vars.o screen.o

GL_OBJS = gl_draw.o gl_mesh.o gl_model.o gl_refrag.o gl_rlight.o gl_rmain.o gl_rmisc.o gl_rsurf.o \
	gl_screen.o gl_test.o gl_warp.o 

WIN_OBJS = conproc.o cd_win.o in_win.o net_win.o net_wins.o net_wipx.o snd_win.o sys_win.o midi.o  mstrconv.o 
WIN_SOFT_OBJS = vid_win.o 
WIN_GL_OBJS = gl_vidnt.o

DC_OBJS = cd_dc.o in_dc.o sys_dc.o midi_null.o snddma_dc.o snd_dma.o snd_mem.o snd_mix_dc.o \
	vmuheader.o support/fnmatch.o dc/aica.o  dc/fake_cdda.o dc/fs_mem.o

OBJS = $(COMMON_OBJS) $(SOFT_OBJS) $(DC_OBJS) $(NET_NULL_OBJS) vid_dc.o
GLOBJS = $(COMMON_OBJS) $(GL_OBJS) $(DC_OBJS) $(NET_NULL_OBJS) gl_viddc.o

LDFLAGS = -nostartfiles -Wl,-Ttext=0x8c010000
STARTUP = startup.o 

LDFLAGS = -nostartfiles -Wl,-Ttext=0x8c010000 startup.o syscalls.o

all: glhexen2.elf

hexen2.elf : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o hexen2.elf $(OBJS) -lz -lm -L/prog/kos-1.1.7/lib -lkallisti -llwip4 -lc -lm

glhexen2.elf : $(GLOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o glhexen2.elf $(GLOBJS) -lz -lm -L/prog/kos-1.1.7/lib -lgl -ldcutils -lkallisti -lc -lm


#	for optimize problem
screen.o : screen.c
	$(CC) $(CFLAGS) -c -O1 $<
gl_screen.o : gl_screen.c
	$(CC) $(CFLAGS) -c -O1 $<
