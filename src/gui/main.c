//#define SH2CORE_DYNAREC 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>

#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../yabause.h"
#include "../yui.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../vidsoft.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"
#include "../persdljoy.h"
#ifdef ARCH_IS_LINUX
#include "../perlinuxjoy.h"
#endif
#include "../m68kcore.h"
#ifdef SH2_DYNAREC
#include "../sh2_dynarec/sh2_dynarec.h"
#endif

#include "main.h"

#define DEFAUL_BIOS "sega_101.bin"

static char biospath[256] = "\0";
static char cdpath[256] = "\0";
static char buppath[256] = "\0";
static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";

M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_MUSASHI
&M68KMusashi,
#endif
#ifdef HAVE_C68K
&M68KC68K,
#endif
#ifdef HAVE_Q68
&M68KQ68,
#endif
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
#ifdef SH2_DYNAREC
&SH2Dynarec,
#endif
#ifdef HAVE_PLAY_JIT
&SH2Jit,
#endif
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
/*
#ifdef HAVE_LIBSDL
&PERSDLJoy,
#endif
#ifdef ARCH_IS_LINUX
&PERLinuxJoy,
#endif
*/
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
/*
#ifndef UNKNOWN_ARCH
&ArchCD,
#endif
*/
NULL
};

SoundInterface_struct *SNDCoreList[] = {
//&SNDDummy,
&SNDSDL,
#ifdef HAVE_LIBAL
&SNDAL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSoft,
NULL
};
/*
#ifdef YAB_PORT_OSD
OSD_struct *OSDCoreList[] = {
&OSDDummy,
#ifdef HAVE_LIBGLUT
&OSDGlut,
#endif
&OSDSoft,
NULL
};
#endif
*/

int YuiInit(int sh2core, int sndcore)   {
    static yabauseinit_struct yinit;
    static void * padbits;
    
	
    snprintf(biospath, sizeof(biospath), "%s/.yabause", getenv("HOME"));
	if (access( biospath, F_OK ) == -1)
	{
		mkdir(biospath, 0755);
	}
    snprintf(biospath, sizeof(biospath), "%s/.yabause/bios.bin", getenv("HOME"));

	memset(&yinit, 0, sizeof(yinit));
    yinit.percoretype = PERCORE_DUMMY;
    yinit.sh2coretype = sh2core;
    yinit.vidcoretype = VIDCORE_SOFT;
    yinit.m68kcoretype = M68KCORE_C68K;
	yinit.sndcoretype = sndcore;
	yinit.sh1coretype = SH2CORE_JIT;
    yinit.cdcoretype = CDCORE_ISO;
    yinit.carttype = CART_NONE;
    yinit.regionid = REGION_AUTODETECT;
    yinit.biospath = biospath;
    yinit.cdpath = cdpath[0]? cdpath : NULL;
    yinit.buppath = buppath[0]? buppath : NULL;
    yinit.mpegpath = mpegpath[0]? mpegpath : NULL;
    yinit.cartpath = cartpath[0]? cartpath : NULL;
    yinit.frameskip = 4;
    yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
    yinit.clocksync = 0;
	yinit.play_ssf = 0;
	yinit.use_new_scsp = 0;
	yinit.usethreads = 0;
	yinit.numthreads = 1;
	yinit.use_scsp_dsp_dynarec = 0;
	yinit.use_scu_dsp_jit = 0;
    yinit.basetime = 0;

    if(YabauseInit(&yinit) != 0)
      return -1;

    PerPortReset();
    padbits = PerPadAdd(&PORTDATA1);
    PerSetKey(SDLK_UP, PERPAD_UP, padbits);
    PerSetKey(SDLK_RIGHT, PERPAD_RIGHT, padbits);
    PerSetKey(SDLK_DOWN, PERPAD_DOWN, padbits);
    PerSetKey(SDLK_LEFT, PERPAD_LEFT, padbits);
    PerSetKey(SDLK_RETURN, PERPAD_START, padbits);
    PerSetKey(SDLK_SPACE, PERPAD_A, padbits);	//(X)
    PerSetKey(SDLK_LALT, PERPAD_B, padbits);			//(B)
    PerSetKey(SDLK_LSHIFT, PERPAD_C, padbits);	// <-
    PerSetKey(SDLK_LCTRL, PERPAD_X, padbits);		//(A)
    PerSetKey(SDLK_TAB, PERPAD_Y, padbits);		//(Y)
    PerSetKey(SDLK_BACKSPACE, PERPAD_Z, padbits);			// 0
    PerSetKey(SDLK_PAGEUP, PERPAD_RIGHT_TRIGGER, padbits);	//(R)
    PerSetKey(SDLK_PAGEDOWN, PERPAD_LEFT_TRIGGER, padbits);	//(L)

    int quit = 0;
    SDL_Event event;

    while(!quit) {
	    while(SDL_PollEvent(&event)){
	    	switch(event.type){
	    		case SDL_KEYDOWN:
	    			PerKeyDown(event.key.keysym.sym);
	    			break;
	    		case SDL_KEYUP:
	    			if (event.key.keysym.sym == SDLK_ESCAPE)
	    				quit = 1;
	    			else
	    				PerKeyUp(event.key.keysym.sym);
	    			break;
	    		case SDL_QUIT:
	    			quit = 1;
	    			break;
	    	}
	    }
        PERCore->HandleEvents();
    }

    return 0;
}

void YuiErrorMsg(const char *error_text)    {
    fprintf(stderr, "Error: %s\n", error_text);
    exit(-1);
}

void YuiSwapBuffers(void)   {
    // Show the screen !
    static int old_width = 0;
    static int old_height = 0;
    int buf_width, buf_height;

    VIDCore->GetGlSize(&buf_width, &buf_height);
	// Soft part
    static SDL_Surface *screen = NULL;

    if ((old_width!=buf_width) || (old_height!=buf_height)) {
    	// set new mode
    	screen = SDL_SetVideoMode(buf_width, buf_height, 16, SDL_HWSURFACE
    	#ifdef SDL_TRIPLEBUF
    	| SDL_TRIPLEBUF
    	#else
    	| SDL_DOUBLEBUF
    	#endif
    	);	// create a window
        SDL_ShowCursor(SDL_DISABLE);
        #ifdef DEBUG
    	printf("switch screen from %dx%d to %dx%d\n", old_width, old_height, buf_width, buf_height);
    	#endif
    	old_width = buf_width;
    	old_height = buf_height;
    }
    
    #ifdef DEBUG
    if (!screen)
    	return;

	if (!dispbuffer) {
		printf("Warning, NULL dispbuffer\n");
		return;
	}
	#endif

	//SDL_BlitSurface(pixdata, NULL, screen, NULL);
	//screen->pixels = dispbuffer;
	memcpy(screen->pixels, dispbuffer, (buf_width * buf_height) * 2);
	
	// update the screen buffer
	SDL_Flip(screen);
}

// utility fonction
int file_exists(const char * filename)
{
    if (access(filename, F_OK)==0)
    {
        return 1;
    }
    return 0;
}

// Setup
void set_biospath()
{
	if (biospath[0])
		return;
	if (file_exists(DEFAUL_BIOS)) {
		printf("bios file found\n");
		strcpy(biospath, DEFAUL_BIOS);
	} else {
		printf("bios not found (" DEFAUL_BIOS "), using bios emulation\n");
	}
}

void print_help()
{

	printf("command line arguments\n-------------\n\n");

	printf("-b|--bios filename\t: load filename as bios (" DEFAUL_BIOS " tried by default)\n");
	printf("-i|--image filename\t: load filename as CD image\n");
	//printf("-c|--core n\t\t: use SH2 core #n (0=int, 1=debug, %d=dynarec, %d=jit)\n", SH2CORE_DYNAREC, SH2CORE_JIT);
	printf("-s|--sound n\t\t: use Sound core #n (0=dummy, %d=sdl)\n", SNDCORE_SDL);
	printf("-h|--help\t\t: print this help\n");
	printf("\n");
}

int main(int argc, char *argv[])    {
    int core;
	int snd;
    int parse_status;
#ifdef SH2_DYNAREC
    core = SH2CORE_DYNAREC;
#else
	core = 0;
#endif
    snd = SNDCORE_SDL;
	printf("Yabause for Pandora\n===================\n\n");

    // parse command lines arguments
    parse_status = 0;	
    // hack to allow just the image...
    if ((argc==2) && (argv[1][0]!='-'))
    	parse_status = 2;

    for (int i = 1; i < argc; i++) {
   		switch (parse_status) {
   			case 0:
		    	if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) 
		    	{
		    		print_help();
		    		return 0;
		    	}
		    	else if (!strcmp(argv[i], "-i"))
		    		parse_status = 2;
		    	else if(!strcmp(argv[i], "--image"))
		    		parse_status = 2;
		    	else if (!strcmp(argv[i], "-b"))
		    		parse_status = 1;
		    	else if(!strcmp(argv[i], "--bios"))
		    		parse_status = 1;
				else if(!strcmp(argv[i], "-c"))
					parse_status = 3;
				else if(!strcmp(argv[i], "--core"))
					parse_status = 3;
				else if(!strcmp(argv[i], "-s"))
					parse_status = 4;
				else if(!strcmp(argv[i], "--sound"))
					parse_status = 4;
				else
					printf("Error on command line (\"%s\")\n", argv[i]);
				break;
			case 1: printf("Setting Bios to \"%s\"\n", argv[i]);
				if (file_exists(argv[i]))
					strcpy(biospath, argv[i]);
				else
					printf("Bios File not found");
				parse_status = 0;
				break;
			case 2: printf("Setting Disc Image to \"%s\"\n", argv[i]);
				strcpy(cdpath, argv[i]);
				parse_status = 0;
				break;
			case 3: 
				core = atoi(argv[i]);
				if(core<0 || core>SH2CORE_JIT) core=0;
				printf("Setting SH2 Core to \"%d\"\n", core);
				parse_status = 0;
				break;
			case 4: 
				snd = atoi(argv[i]);
				if(snd<0 || snd>SNDCORE_SDL) snd=0;
				printf("Setting Sound Core to \"%d\"\n", snd);
				parse_status = 0;
				break;
    	}
    }
    // incase no bios has been set using command line
    set_biospath();


    // Init SDL
    int flags = SDL_INIT_TIMER|SDL_INIT_VIDEO;
    if(snd==SNDCORE_SDL) flags |= SDL_INIT_AUDIO;
    if (SDL_Init(flags)) {
    	printf("Error initializing SDL...\n");
    	exit(-1);
    }

    printf("Starting emulation...\n");

    YuiInit(core, snd);

    printf("\n*****************\nFinished\n");
    SDL_Quit();

    return 0;
}
