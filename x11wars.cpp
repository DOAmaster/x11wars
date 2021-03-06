//Modified by: Derrick Alden
//program: x11wars.cpp
//github: https://github.com/doamaster/x11wars
//author:  Gordon Griesel
//date:    2018
//Depending on your Linux distribution,
//may have to install these packages:
// libx11-dev
// libglew1.6
// libglew1.6-dev
//
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctime>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <X11/Xlib.h>
//#include <X11/Xutil.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

typedef float Flt;
typedef Flt Vec[3];
//macros to manipulate vectors
#define MakeVector(x,y,z,v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecNegate(a) (a)[0]=(-(a)[0]); (a)[1]=(-(a)[1]); (a)[2]=(-(a)[2]);
#define VecDot(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];
#define VecCopy2d(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];
#define VecNegate2d(a) (a)[0]=(-(a)[0]); (a)[1]=(-(a)[1]);
#define VecDot2d(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1])
#define MAX_PARTICLES 10000
#define GRAVITY 0.1
#define PI 3.141592653589793

#define xReserve 900;
#define yReserve 700;


//-----------------------------------------------------------------------------

int xres=640, yres=480;
int xres3, xres4;
unsigned char *screendata = NULL;
void setup_screen_res(const int w, const int h)
{
        xres = w;
        yres = h;
        xres3 = xres * 3;
        xres4 = xres * 4;
        if (screendata)
                delete [] screendata;
        screendata = new unsigned char[yres * xres4];
}



//-----------------------------------------------------------------------------



//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

Flt pos[3]={20.0,200.0,0.0};



//might use for future ai 
const float gravity = 0.2f;
const int MAX_BULLETS = 11;

//static int xres=800, yres=600;
void setup_screen_res(const int w, const int h);
//void showFrameRate();

struct Shape {

	float width, height;
	float radius;
	Vec center;

};


struct Particle {
	Shape s;
	Vec velocity;
	int count;
	bool thrust;
};


class Bullet {
    public:
	Vec pos;
	Vec vel;
	float color[3];
	struct timespec time;
    public:
	Bullet() { }

};

//show fps component
//setup timers-------------------------------
const double oothousand = 1.0 / 1000.0;
struct timeval gamestarttv;
//My version of GetTickCount() or SDL_GetTicks()
int xxGetTicks() {
        struct timeval end;
        gettimeofday(&end, NULL);
        //long seconds  = end.tv_sec  - gamestarttv.tv_sec;
        //long useconds = end.tv_usec - gamestarttv.tv_usec;
        //long mtime = (seconds*1000 + useconds*oothousand) + 0.5;
        //return (int)mtime;
        //code above compressed...
        return (int)((end.tv_sec - gamestarttv.tv_sec) * 1000 +
                (end.tv_usec - gamestarttv.tv_usec) * oothousand) + 0.5;
}
//end of timer stuff
//-----------------------------------------------------------


void initXWindows(int w, int h)
{
        //Fullscreen implementation
        //usage:
        //   for fullscreen call: initXWindows(0, 0);
        //   for windowed call: initXWindows(640, 480);
        //                      initXWindows(1024, 768);
        //                      etc.
        //
        GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        dpy = XOpenDisplay(NULL);
        //Screen *s = DefaultScreenOfDisplay(dpy);
        if (dpy == NULL) {
                printf("ERROR: cannot connect to X server!\n"); fflush(stdout);
                exit(EXIT_FAILURE);
        }
        Window root = DefaultRootWindow(dpy);
        //for fullscreen
        XWindowAttributes getWinAttr;
    XGetWindowAttributes(dpy, root, &getWinAttr);
        int fullscreen = 0;
        xres = w;
        yres = h;
        if (!w && !h) {
                //Go to fullscreen.
                xres = getWinAttr.width;
                yres = getWinAttr.height;
                printf("getWinAttr: %i %i\n", w, h); fflush(stdout);
                //When window is fullscreen, there is no client window
                //so keystrokes are linked to the root window.
                XGrabKeyboard(dpy, root, False,
                        GrabModeAsync, GrabModeAsync, CurrentTime);
                fullscreen = 1;
        }
        XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
        if (vi == NULL) {
                printf("ERROR: no appropriate visual found\n"); fflush(stdout);
                exit(EXIT_FAILURE);
        }
        Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
        XSetWindowAttributes swa;
        swa.colormap = cmap;
        //List your desired events here.
        swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                StructureNotifyMask | SubstructureNotifyMask;
        unsigned int winops = CWBorderPixel|CWColormap|CWEventMask;
        if (fullscreen) {
                winops |= CWOverrideRedirect;
                swa.override_redirect = True;
        }
        printf("2 getWinAttr: %i %i\n", w, h); fflush(stdout);
        win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0,
                vi->depth, InputOutput, vi->visual, winops, &swa);
        glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
        glXMakeCurrent(dpy, win, glc);
        //set_title();
}


class X11_wrapper {
	//X Windows variables
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	X11_wrapper() {
		Window root;
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
		XVisualInfo *vi;
		Colormap cmap;
		XSetWindowAttributes swa;
		setup_screen_res(800, 600);
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			printf("\n\tcannot connect to X server\n\n");
			exit(EXIT_FAILURE);
		}
		root = DefaultRootWindow(dpy);
		vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			printf("\n\tno appropriate visual found\n\n");
			exit(EXIT_FAILURE);
		} 
		//else {
		//	// %p creates hexadecimal output like in glxinfo
		//	printf("\n\tvisual %p selected\n", (void *)vi->visualid);
		//}
		cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		swa.colormap = cmap;
		swa.event_mask =
						ExposureMask |
						KeyPressMask |
						KeyReleaseMask |
						PointerMotionMask |
						ButtonPressMask |
						ButtonReleaseMask |
						StructureNotifyMask |
						SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0,
						vi->depth, InputOutput, vi->visual,
						CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	void set_title(void) {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "X11 WARS");
	}
	bool getPending() {
		return XPending(dpy);
	}
	void getNextEvent(XEvent *e) {
		XNextEvent(dpy, e);
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
} x11;


class Image {
public:
	int width, height;
	unsigned char *data;
	~Image() { delete [] data; }
	Image(const char *fname) {
		if (fname[0] == '\0')
			return;
		//printf("fname **%s**\n", fname);
		int ppmFlag = 0;
		char name[40];
		strcpy(name, fname);
		int slen = strlen(name);
		char ppmname[80];
		if (strncmp(name+(slen-4), ".ppm", 4) == 0)
			ppmFlag = 1;
		if (ppmFlag) {
			strcpy(ppmname, name);
		} else {
			name[slen-4] = '\0';
			//printf("name **%s**\n", name);
			sprintf(ppmname,"%s.ppm", name);
			//printf("ppmname **%s**\n", ppmname);
			char ts[100];
			//system("convert eball.jpg eball.ppm");
			sprintf(ts, "convert %s %s", fname, ppmname);
			system(ts);
		}
		//sprintf(ts, "%s", name);
		FILE *fpi = fopen(ppmname, "r");
		if (fpi) {
			char line[200];
			fgets(line, 200, fpi);
			fgets(line, 200, fpi);
			while (line[0] == '#')
				fgets(line, 200, fpi);
			sscanf(line, "%i %i", &width, &height);
			fgets(line, 200, fpi);
			//get pixel data
			int n = width * height * 3;			
			data = new unsigned char[n];			
			for (int i=0; i<n; i++)
				data[i] = fgetc(fpi);
			fclose(fpi);
		} else {
			printf("ERROR opening image: %s\n",ppmname);
			exit(0);
		}
		if (!ppmFlag)
			unlink(ppmname);
	}
};
Image img[4] = {
"./title.jpg",
"./gameover.jpg",
"./titletrans.gif",
"./background.gif"
};

//alpha color yellow (255, 255, 100)
unsigned char *buildAlphaData(Image *img)
{
	//add 4th component to RGB stream...
	int i;
	int a,b,c;
	unsigned char *newdata, *ptr;
	unsigned char *data = (unsigned char *)img->data;
	newdata = (unsigned char *)malloc(img->width * img->height * 4);
	ptr = newdata;
	for (i=0; i<img->width * img->height * 3; i+=3) {
		a = *(data+0);
		b = *(data+1);
		c = *(data+2);
		*(ptr+0) = a;
		*(ptr+1) = b;
		*(ptr+2) = c;
		//-----------------------------------------------
		//get largest color component...
		//*(ptr+3) = (unsigned char)((
		//		(int)*(ptr+0) +
		//		(int)*(ptr+1) +
		//		(int)*(ptr+2)) / 3);
		//d = a;
		//if (b >= a && b >= c) d = b;
		//if (c >= a && c >= b) d = c;
		//*(ptr+3) = d;
		//-----------------------------------------------
		//this code optimizes the commented code above.
		if(a==255 && b==255 && c==100) {
			*(ptr+3) = 0;
		}else {
			*(ptr+3) = 1;
		}
		//-----------------------------------------------
		ptr += 4;
		data += 3;
	}
	return newdata;
}


//
#ifdef USE_SOUND
#include </usr/include/AL/alut.h>
class Openal {
	ALuint alSource[2];
	ALuint alBuffer[2];
public:
	Openal() {
		//Get started right here.
		alutInit(0, NULL);
		if (alGetError() != AL_NO_ERROR) {
			printf("ERROR: starting sound.\n");
		}
		//Clear error state
		alGetError();
		//Setup the listener.
		//Forward and up vectors are used.
		float vec[6] = {0.0f,0.0f,1.0f, 0.0f,1.0f,0.0f};
		alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
		alListenerfv(AL_ORIENTATION, vec);
		alListenerf(AL_GAIN, 1.0f);
		//
		//Buffer holds the sound information.
		alBuffer[0] = alutCreateBufferFromFile("./billiard.wav");
		alBuffer[1] = alutCreateBufferFromFile("./wall.wav");
		//Source refers to the sound stored in buffer.
		for (int i=0; i<2; i++) {
			alGenSources(1, &alSource[i]);
			alSourcei(alSource[i], AL_BUFFER, alBuffer[i]);
			alSourcef(alSource[i], AL_GAIN, 1.0f);
			if (i==0) alSourcef(alSource[i], AL_GAIN, 0.4f);
			alSourcef(alSource[i], AL_PITCH,1.0f);
			alSourcei(alSource[i], AL_LOOPING,  AL_FALSE);
			if (alGetError() != AL_NO_ERROR) {
				printf("ERROR\n");
			}
		}
	}
	void playSound(int i)
	{
		alSourcePlay(alSource[i]);
	}
	~Openal() {
		//First delete the source.
		alDeleteSources(1, &alSource[0]);
		alDeleteSources(1, &alSource[1]);
		//Delete the buffer
		alDeleteBuffers(1, &alBuffer[0]);
		alDeleteBuffers(1, &alBuffer[1]);
		//Close out OpenAL itself.
		//unsigned int alSampleSet;
		ALCcontext *Context;
		ALCdevice *Device;
		//Get active context
		Context=alcGetCurrentContext();
		//Get device for active context
		Device=alcGetContextsDevice(Context);
		//Disable context
		alcMakeContextCurrent(NULL);
		//Release context(s)
		alcDestroyContext(Context);
		//Close device
		alcCloseDevice(Device);
	}
} oal;
#endif

void playSound(int s) {
#ifdef USE_SOUND
	oal.playSound(s);
#else
	if (s) {}
#endif
}

int lbump=0;
int lbumphigh=0;


void init_opengl(void);
void init_balls(void);
void init_boxes(void);
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
void check_keys(XEvent *e);
void physics(void);
void render(void);

int done=0;
int leftButtonDown=0;
Vec leftButtonPos;

class Box {
public:
	Vec pos;
	Vec vel;
	Vec force;
	Vec center;
	Shape s;
	float radius;
	float mass;
	float angle;
	bool split;
	Particle particle[3];
} box[100];

class Ball {
public:
	Vec pos;
	Vec vel;
	Vec force;
	float radius;
	float mass;
	float angle;
	bool split;
	Particle particle[3];
} ball[100];


class Player {
public:
	Vec pos;
	Vec vel;
	Vec force;
	float angle;
	float radius;
	float mass;
	float eye1;
	float eye2;
	float eyeBall1;
	float eyeBall2;
	float mouth;
	float mouthSad;
	float mouthHappy;
	bool lookUp;
	bool lookDown;
	bool lookLeft;
	bool lookRight;
};

enum State {
	STATE_NONE,
	STATE_STARTUP,
	STATE_GAMEPLAY,
	STATE_PAUSE,
	STATE_GAMEOVER

};

const int MAX_BULELTS = 11;

class Game {
    public:
	Player player;
//	Shape box[6];
	Particle particle[MAX_PARTICLES];
	Particle hitPart[MAX_PARTICLES];
	int n;
	int nBullets;
	int nBoxes;
	int maxBullets;
	int nHit;
	int score;
	int level;
	float moveSpeed;
	bool levelUp;
	
	//texures
	GLuint gameOverTexture;
	GLuint titleTexture;
	GLuint titleTextureTrans;
	GLuint backgroundTexture;

	State state;
	bool spawner;
	unsigned char keys[65535];

	Game(){
	    	levelUp = false;
	    	nBoxes = 0;
		level = 0;
		score = 0;
	    	state = STATE_STARTUP;
		spawner = false;
		n = 0;
		nHit = 0;
		moveSpeed = 3;
		maxBullets = 10;
		nBullets = 0;
	}

}
game;

//extern void particleVelocity(Particle *p);


void makeParticle(int x, int y)
{
	if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = x;
	game.particle[game.nBullets].s.center[1] = y;
//	particleVelocity(p);
//	game.particle[game.nBullets].velocity[0] = rnd() * 1.0 -.5; 
//	game.particle[game.nBullets].velocity[1] = rnd() * 1.0 -.5; 
	game.nBullets++;
}

int showOpengl = 0;

//Setup timers
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
struct timespec timeStart, timeCurrent;
struct timespec timePause;
double physicsCountdown=0.0;
double timeSpan=0.0;
double timeDiff(struct timespec *start, struct timespec *end) {
	return (double)(end->tv_sec - start->tv_sec ) +
			(double)(end->tv_nsec - start->tv_nsec) * oobillion;
}
void timeCopy(struct timespec *dest, struct timespec *source) {
	memcpy(dest, source, sizeof(struct timespec));
}

void showFrameRate()
{
        static int count=0;
        static int lastt = xxGetTicks();
        if (++count >= 32) {
                int diff = xxGetTicks() - lastt;
                //32 frames took diff 1/1000 seconds.
                //how much did 1 frame take?
                float secs = (float)diff / 1000.0;
                //frames per second...
                float fps = (float)count / secs;
                //printf("frame rate: %f\n", fps);
                count = 0;
                lastt = xxGetTicks();
        }
}



int main(int argc, char *argv[])
{
	init_opengl();
	init_balls();
	//init_boxes();
	clock_gettime(CLOCK_REALTIME, &timePause);
	clock_gettime(CLOCK_REALTIME, &timeStart);


//	int x=0, y=0;
//	if (argc > 2) {
//		x = atoi(argv[1]);
//		y = atoi(argv[2]);
//	}
//	if (argc > 3)
	//	showOpengl = 1;

	//declar game object
//	Game game;
//	game.n=0;
	gettimeofday(&gamestarttv, NULL);
	
//	initXWindows(800, 600);

	while (!done) {
		while (x11.getPending()) {
			XEvent e;
			x11.getNextEvent(&e);
			check_resize(&e);
			check_mouse(&e);
			check_keys(&e);
		}
		clock_gettime(CLOCK_REALTIME, &timeCurrent);
		timeSpan = timeDiff(&timeStart, &timeCurrent);
		timeCopy(&timeStart, &timeCurrent);
		physicsCountdown += timeSpan;
		while (physicsCountdown >= physicsRate) {
			physics();
			physicsCountdown -= physicsRate;
		}
		render();
		x11.swapBuffers();	
		showFrameRate();
	}
	//cleanup_fonts();
	return 0;
}


void reshape_window(int width, int height)
{
	//window has been resized.
	setup_screen_res(width, height);
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	x11.set_title();
}

//alpha building from walk framework
#define ALPHA 1


void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, xres, yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, xres, 0, yres, -1, 1);
	//Clear the screen
	//set screen background color
	//currently set to black
	glClearColor(0.0, 0.0, 0.0, 1.0);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	//enable fonts
	//initialize_fonts();

	//create opengl texture elements
	glGenTextures(1, &game.titleTexture);	
	glGenTextures(1, &game.gameOverTexture);
	glGenTextures(1, &game.titleTextureTrans);
	glGenTextures(1, &game.backgroundTexture);

	//title image
	glBindTexture(GL_TEXTURE_2D, game.titleTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,
		img[0].width, img[0].height,
		0, GL_RGB, GL_UNSIGNED_BYTE, img[0].data);


	//game over image
	glBindTexture(GL_TEXTURE_2D, game.gameOverTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,
		img[1].width, img[1].height,
		0, GL_RGB, GL_UNSIGNED_BYTE, img[1].data);


	//background image
	glBindTexture(GL_TEXTURE_2D, game.backgroundTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,
		img[3].width, img[3].height,
		0, GL_RGB, GL_UNSIGNED_BYTE, img[3].data);

	//title image transparent
	//

        //must build a new set of data...
	//send to alpha data, then use GL_RBG_a, check walk and rainforest frameworks for examples
	glBindTexture(GL_TEXTURE_2D, game.titleTextureTrans);

       	unsigned char *walkData = buildAlphaData(&img[2]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[2].width, img[2].height, 0,
        	GL_RGBA, GL_UNSIGNED_BYTE, walkData);

	
	//
	/*
	glTexImage2D(GL_TEXTURE_2D, 0, 3,
		img[2].width, img[2].height,
		0, GL_RGB, GL_UNSIGNED_BYTE, img[2].data);
	*/

	delete walkData;

}

#define sphereVolume(r) (r)*(r)*(r)*3.14159265358979*4.0/3.0;


void init_boxes(void)
{

    	box[0].pos[0] = 200;
	box[0].pos[1] = yres-150;
	box[0].vel[0] = -1.0;
	box[0].vel[1] = -1.0;
	box[0].radius = 30.0;
	box[0].center[0] = 300;
	box[0].center[1] = 200;
	game.nBoxes++;


}

void init_balls(void)
{


	ball[0].pos[0] = 100;
	ball[0].pos[1] = yres-150;
	ball[0].vel[0] = 0.0;
	ball[0].vel[1] = 0.0;
	ball[0].force[0] =
	ball[0].force[1] = 0.0;
	ball[0].radius = 25.0;
	ball[0].mass = sphereVolume(ball[0].radius);
	game.n++;
	//
	/*
	ball[1].pos[0] = 400;
	ball[1].pos[1] = yres-150;
	ball[1].vel[0] = 0.0;
	ball[1].vel[1] = 0.0;
	ball[1].force[0] =
	ball[1].force[1] = 0.0;
	ball[1].radius = 25.0;
	ball[1].mass = sphereVolume(ball[1].radius);
	game.n++;
	*/

}

void clearBoxes(void)
{
	for( int i = 0; i < game.nBoxes; i++) {
	//	ball[i].pos[0] = 200;
	//	ball[i].pos[1] = yres-150;
	//	ball[i].vel[0] = 0.0;
	//	ball[i].vel[1] = 0.0;
	//	ball[i].radius = 70.0;
	//	ball[i].mass = sphereVolume(ball[i].radius);
		box[i].s.width = 0;
		box[i].s.height = 0;
		box[i].s.radius = 0;
		//game.particle[i].s.radius = 0;

		game.nBoxes--;
	}


}

void clearBullets(void)
{
	for( int i = 0; i < game.nBullets; i++) {
	//	ball[i].pos[0] = 200;
	//	ball[i].pos[1] = yres-150;
	//	ball[i].vel[0] = 0.0;
	//	ball[i].vel[1] = 0.0;
	//	ball[i].radius = 70.0;
	//	ball[i].mass = sphereVolume(ball[i].radius);
		game.particle[i].s.width = 0;
		game.particle[i].s.height = 0;
		game.particle[i].s.radius = 0;

		game.nBullets--;
	}


}

void clearBalls(void)
{
	//puts balls postions off screen and defaut radius
	for( int i; i < game.n; i++) {
		ball[i].pos[0] = 999;
		ball[i].pos[1] = 999;
		ball[i].vel[0] = 0.0;
		ball[i].vel[1] = 0.0;
		ball[i].radius = 40.0;
		ball[i].mass = sphereVolume(ball[i].radius);
		game.n--;
	}


}

void scenario1(void)
{

	for(int i = 0; i < 2; i++) {

	//top right start 
		ball[i].pos[0] = 900;
		ball[i].pos[1] = 700;
		ball[i].vel[0] = 0.0;
		ball[i].vel[1] = 0.0;
		ball[i].radius = 40.0;
		ball[i].mass = sphereVolume(ball[0].radius);
		ball[i].split = false;
		game.n++;
	}
	game.state = STATE_GAMEPLAY;

	/*
	//top right start 
	ball[0].pos[0] = xres - 50;
	ball[0].pos[1] = yres - 50;
	ball[0].vel[0] = 0.0;
	ball[0].vel[1] = 0.0;
	ball[0].radius = 40.0;
	ball[0].mass = sphereVolume(ball[0].radius);
	ball[0].split = false;
	game.n++;
	//bot left start
	ball[1].pos[0] = 50;
	ball[1].pos[1] = 50;
	ball[1].vel[0] = 0.0;
	ball[1].vel[1] = 0.0;
	ball[1].radius = 40.0;
	ball[1].split = false;
	ball[1].mass = sphereVolume(ball[1].radius);
	game.n++;
	*/
}


void check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != xres || xce.height != yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}

void check_mouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type == ButtonRelease) {
		leftButtonDown=0;
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
			leftButtonDown = 1;
			leftButtonPos[0] = (Flt)e->xbutton.x;
			leftButtonPos[1] = (Flt)(yres - e->xbutton.y);
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		savex = e->xbutton.x;
		savey = e->xbutton.y;
		if (leftButtonDown) {
			leftButtonPos[0] = (Flt)e->xbutton.x;
			leftButtonPos[1] = (Flt)(yres - e->xbutton.y);
		}
	}
}

void check_keys(XEvent *e)
{

	if(e->type == KeyRelease) {
		int key = XLookupKeysym(&e->xkey, 0);
		game.keys[key]=0;
	}
//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		game.keys[key]=1;
		switch(key) {
			case XK_1:
				//scenario
				if(game.state == STATE_STARTUP) { scenario1(); }
				break;
				case XK_Escape:
				game.state = STATE_STARTUP;
				break;
			case XK_p:
				done=1;
				break;
		}
	}
}


#define rnd() (float)rand() / (float)RAND_MAX
void shootUp()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = ((rnd() * .1)+.1)*1.9; 
	game.particle[game.nBullets].velocity[1] = ((rnd() * 2.0)+1)*1.9; 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;

	game.player.lookUp = true;
	game.player.lookLeft = false;
	game.player.lookRight = false;
	game.player.lookDown = false;


}

void shootDown()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -((rnd() * .1)+.1)*2; 
	game.particle[game.nBullets].velocity[1] = -((rnd() * 2.0)+1)*2; 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;


	game.player.lookDown = true;
	game.player.lookUp = false;
	game.player.lookLeft = false;
	game.player.lookRight = false;


}


void shootDownLeft()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -((rnd() * 2.0)+1); 
	game.particle[game.nBullets].velocity[1] = -((rnd() * 2.0)+1); 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;




}

void shootDownRight()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = ((rnd() * 2.0)+1); 
	game.particle[game.nBullets].velocity[1] = -((rnd() * 2.0)+1); 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;



}

void shootUpLeft()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -((rnd() * 2.0)+1); 
	game.particle[game.nBullets].velocity[1] = ((rnd() * 2.0)+1); 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;


}


void shootUpRight()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = ((rnd() * 2.0)+1); 
	game.particle[game.nBullets].velocity[1] = ((rnd() * 2.0)+1); 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;


}


void shootLeft()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -((rnd() * 2.0)+1)*2; 
	game.particle[game.nBullets].velocity[1] = ((rnd() * .1)+.1)*2; 
        game.particle[game.nBullets].count = 0;
	game.nBullets++;

	game.player.lookLeft = true;
	game.player.lookRight = false;
	game.player.lookUp = false;
	game.player.lookDown = false;


}

void shootRight()
{
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = ((rnd() * 2.0)+1)*2; 
	game.particle[game.nBullets].velocity[1] = ((rnd() * .1)+.1)*2;
        game.particle[game.nBullets].count = 0;
	game.nBullets++;


	game.player.lookRight = true;
	game.player.lookUp = false;
	game.player.lookLeft = false;
	game.player.lookDown = false;


}


void moveLeft() 
{ 
	if(game.player.vel[0] > -2 && game.player.vel[0] < 2) {
		game.player.vel[0] -= game.moveSpeed;

	   //Spawn Thrust particle 6
	   game.hitPart[game.nHit].s.center[0] = game.player.pos[0];
	   game.hitPart[game.nHit].s.center[1] = game.player.pos[1];
           game.hitPart[game.nHit].velocity[0] = 5;
	   game.hitPart[game.nHit].velocity[1] = 0;
	   game.hitPart[game.nHit].thrust = true;
	   game.nHit++;
	}
    //game.player.pos[0] -= game.moveSpeed;
    game.player.angle = 270;
}

void moveRight()
{

	if(game.player.vel[0] > -2 && game.player.vel[0] < 2) {
		game.player.vel[0] += game.moveSpeed;

	   //Spawn Thrust particle 6
	   game.hitPart[game.nHit].s.center[0] = game.player.pos[0];
	   game.hitPart[game.nHit].s.center[1] = game.player.pos[1];
           game.hitPart[game.nHit].velocity[0] = -5;
	   game.hitPart[game.nHit].velocity[1] = 0;
	   game.hitPart[game.nHit].thrust = true;
	   game.nHit++;
	}
    //game.player.pos[0] += game.moveSpeed;
    game.player.angle = 90; 
}

void moveUp() 
{

	if(game.player.vel[1] > -2 && game.player.vel[1] < 2) {
    		game.player.vel[1] += game.moveSpeed;

	   //Spawn Thrust particle 6
	   game.hitPart[game.nHit].s.center[0] = game.player.pos[0];
	   game.hitPart[game.nHit].s.center[1] = game.player.pos[1];
           game.hitPart[game.nHit].velocity[0] = 0;
	   game.hitPart[game.nHit].velocity[1] = -5;
	   game.hitPart[game.nHit].thrust = true;
	   game.nHit++;
	} 
    //game.player.pos[1] += game.moveSpeed;
    game.player.angle = 360;


}

void moveDown() 
{ 
	if(game.player.vel[1] > -2 && game.player.vel[1] < 2) {
  	  game.player.vel[1] -= game.moveSpeed;

	   //Spawn Thrust particle 6
	   game.hitPart[game.nHit].s.center[0] = game.player.pos[0];
	   game.hitPart[game.nHit].s.center[1] = game.player.pos[1];
           game.hitPart[game.nHit].velocity[0] = 0;
	   game.hitPart[game.nHit].velocity[1] = 5;
	   game.hitPart[game.nHit].thrust = true;
	   game.nHit++;

	}
    //game.player.pos[1] -= game.moveSpeed;
    game.player.angle = 180;


}


void VecNormalize(Vec v)
{
	Flt dist = v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
	if (dist==0.0)
		return;
	Flt len = 1.0 / sqrt(dist);
	v[0] *= len;
	v[1] *= len;
	v[2] *= len;
}

void VecNormalize2d(Vec v)
{
	Flt dist = v[0]*v[0]+v[1]*v[1];
	if (dist==0.0)
		return;
	Flt len = 1.0 / sqrt(dist);
	v[0] *= len;
	v[1] *= len;
	//v[2] *= len;
}

//called when player dies, will reset player pos and change to game over state
void gameOver(void)
{	
	//playSound(1);
	game.player.pos[0] = xres/2;
	game.player.pos[1] = yres/2;
	game.state = STATE_GAMEOVER;
	game.n = 0;

}

void killBall(void)
{


}

void killEffect(int posx, int posy) 
{
    if(game.nHit < MAX_PARTICLES-20) {
			    //Spawn death particle 1	
			    game.hitPart[game.nHit].s.center[0] = posx;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -3;
			    game.hitPart[game.nHit].velocity[1] = -3;
			    game.nHit++;

			    //Spawn death particle 2
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 3;
			    game.hitPart[game.nHit].velocity[1] = 3;
			    game.nHit++;

			    //Spawn death particle 3
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -2;
			    game.hitPart[game.nHit].velocity[1] = -2;
			    game.nHit++;
			    //Spawn death particle 4
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 2;
			    game.hitPart[game.nHit].velocity[1] = 2;
			    game.nHit++;
			    //Spawn death particle 5
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -5;
			    game.hitPart[game.nHit].velocity[1] = -5;
			    game.nHit++;
			    //Spawn death particle 6
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 5;
			    game.hitPart[game.nHit].velocity[1] = 5;
			    game.nHit++;

			    //Spawn death particle 7
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -6;
			    game.hitPart[game.nHit].velocity[1] = -6;
			    game.nHit++;
			    //Spawn death particle 8
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 6;
			    game.hitPart[game.nHit].velocity[1] = 6;
			    game.nHit++;
			    
			    //start of top left bottom right
			    //Spawn death particle 9	
			    game.hitPart[game.nHit].s.center[0] = posx;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 3;
			    game.hitPart[game.nHit].velocity[1] = -3;
			    game.nHit++;

			    //Spawn death particle 10
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -3;
			    game.hitPart[game.nHit].velocity[1] = 3;
			    game.nHit++;

			    //Spawn death particle 11
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 2;
			    game.hitPart[game.nHit].velocity[1] = -2;
			    game.nHit++;
			    //Spawn death particle 12
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 2;
			    game.hitPart[game.nHit].velocity[1] = -2;
			    game.nHit++;
			    //Spawn death particle 13
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 5;
			    game.hitPart[game.nHit].velocity[1] = -5;
			    game.nHit++;
			    //Spawn death particle 14
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -5;
			    game.hitPart[game.nHit].velocity[1] = 5;
			    game.nHit++;

			    //Spawn death particle 15
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = 6;
			    game.hitPart[game.nHit].velocity[1] = -6;
			    game.nHit++;
			    //Spawn death particle 16
			    game.hitPart[game.nHit].s.center[0] = posx+14;
			    game.hitPart[game.nHit].s.center[1] = posy;
			    game.hitPart[game.nHit].velocity[0] = -6;
			    game.hitPart[game.nHit].velocity[1] = 6;
			    game.nHit++;
    } else {
	game.nHit = 0;
    }


}

void physics(void)
{

	//debugging statments
	//printf("nHits = %i\n", game.nHit);

	//shooting key checks
	if (game.state == STATE_GAMEPLAY) {	
	    if (game.keys[XK_Down] && game.keys[XK_Right]) {
		    shootDownRight();
	    } 
	    if (game.keys[XK_Down] && game.keys[XK_Left]) {
		    shootDownLeft();
	    }
	    if (game.keys[XK_Up] && game.keys[XK_Left]) {
		    shootUpLeft();
	    }
	    if (game.keys[XK_Up] && game.keys[XK_Right]) {
		    shootUpRight();
	    }
	    if (game.keys[XK_Down] ) {
		    shootDown();
	    }
	    if (game.keys[XK_Left] ) {
		    shootLeft();
	    }
	    if (game.keys[XK_Down] ) {
		    shootDown();
	    }
	    if (game.keys[XK_Right] ) {
		    shootRight();
	    }
	    if (game.keys[XK_Up]) {
		    shootUp();
	    }


	    //movement key checks
	    if (game.keys[XK_a]) {
		    moveLeft();	
	    }
	    if (game.keys[XK_d]) {
		    moveRight();	
	    }
	    if (game.keys[XK_w]) {
		    moveUp();	
	    }
	    if (game.keys[XK_s]) {
		    moveDown();	
	    }

	}

	//Update positions mouse ball movement for debuging
	/*
	if (leftButtonDown) {
		//Special physics applied here.
		//Physics employs a penalty system.
		ball[1].vel[0] += ball[1].force[0];
		ball[1].vel[1] += ball[1].force[1];
		ball[1].pos[0] += ball[1].vel[0];
		ball[1].pos[1] += ball[1].vel[1];
		ball[1].force[0] =
		ball[1].force[1] = 0.0;
		ball[0].pos[0] = leftButtonPos[0];
		ball[0].pos[1] = leftButtonPos[1];
		Vec v;
		v[0] = ball[1].pos[0] - ball[0].pos[0];
		v[1] = ball[1].pos[1] - ball[0].pos[1];
		Flt distance = sqrt(v[0]*v[0] + v[1]*v[1]);
		if (distance < (ball[0].radius + ball[1].radius)) {
			ball[1].force[0] = v[0]*0.01;
			ball[1].force[1] = v[1]*0.01;
		}
		ball[1].vel[0] *= 0.99;
		ball[1].vel[1] *= 0.99;
		//
		//bounce balls off window sides...
		if (ball[1].pos[0] < ball[1].radius) {
			ball[1].pos[0] = ball[1].radius + 0.1;
			if (ball[1].vel[0] < 0.0)
				ball[1].vel[0] = -ball[1].vel[0];
			playSound(1);
		}
		if (ball[1].pos[0] >= (Flt)xres-ball[1].radius) {
			ball[1].pos[0] = (Flt)xres-ball[1].radius - 0.1;
			if (ball[1].vel[0] > 0.0)
				ball[1].vel[0] = -ball[1].vel[0];
			playSound(1);
		}
		if (ball[1].pos[1] < ball[1].radius) {
			ball[1].pos[1] = ball[1].radius + 0.1;
			if (ball[1].vel[1] < 0.0)
				ball[1].vel[1] = -ball[1].vel[1];
			playSound(1);
		}
		if (ball[1].pos[1] >= (Flt)yres-ball[1].radius) {
			ball[1].pos[1] = (Flt)yres-ball[1].radius - 0.1;
			if (ball[1].vel[1] > 0.0)
				ball[1].vel[1] = -ball[1].vel[1];
			playSound(1);
		}
		return;
	}
	*/

	//boxes physics
	for (int i=0; i<game.nBoxes; i++) {

		//moves ball via vel
		box[i].pos[0] += box[i].vel[0];
		box[i].pos[1] += box[i].vel[1];
	}

	//ball physics
	for (int i=0; i<game.n; i++) {

		//moves ball via vel
		ball[i].pos[0] += ball[i].vel[0];
		ball[i].pos[1] += ball[i].vel[1];
	}

	//pulls particles down 
	for (int i=0; i<game.nHit; i++) {
		
		game.hitPart[i].s.center[0] += game.hitPart[i].velocity[0];
		game.hitPart[i].s.center[1] += game.hitPart[i].velocity[1];

		game.hitPart[i].count++;
	}
	//moves death particles off screen
	//TODO reuse off screen particles
	for (int i=0; i < game.nHit; i++) {
		if (game.hitPart[i].s.center[1] < -1) {
			game.hitPart[i].s.center[0] = xReserve;
			game.hitPart[i].s.center[1] = yReserve;
			game.hitPart[i].velocity[0] = 0;
			game.hitPart[i].velocity[1] = 0;
		//	printf("moving death particle off screen\n");
		//	game.nHit--;
		}
		if (game.hitPart[i].count > 20) {
			game.hitPart[i].s.center[0] = xReserve;
			game.hitPart[i].s.center[1] = yReserve;
			game.hitPart[i].velocity[0] = 0;
			game.hitPart[i].velocity[1] = 0;
			game.hitPart[i].count = 0;
		}

	}
	

	//player physics applied here...
	game.player.pos[0] += game.player.vel[0];
	game.player.pos[1] += game.player.vel[1];

	float dragAmmount = .8;
	//player movement drag
	if (game.player.vel[0] > 0) {
		game.player.vel[0] = game.player.vel[0] - dragAmmount;
		if (game.player.vel[0] < .01) {
			game.player.vel[0] = 0;
		}
	}
	if (game.player.vel[0] < 0) {
		game.player.vel[0] = game.player.vel[0] + dragAmmount;

		if (game.player.vel[0] > .01) {
			game.player.vel[0] = 0;
		}
	}
	if (game.player.vel[1] > 0) {
		game.player.vel[1] = game.player.vel[1] - dragAmmount;
		
		if (game.player.vel[1] < .01) {
			game.player.vel[1] = 0;
		}
	}
	if (game.player.vel[1] < 0) {
		game.player.vel[1] = game.player.vel[1] + dragAmmount;
		if (game.player.vel[1] > .01) {
			game.player.vel[1] = 0;
		}	
	}
	

	//check for collision here
	Flt distance, speed;
	//Flt distance, pdistance, speed;

	Flt movi[2], movj[2];
	Flt massFactor, massi, massj;
	Vec vcontact[2];
	//Vec pcontact[2];
	Vec vmove[2];
	Flt dot0, dot1;
	for (int i=0; i<game.n-1; i++) {
		for (int j=i+1; j<game.n; j++) {
			//vx = ball[i].pos[0] - ball[j].pos[0];
			//vy = ball[i].pos[1] - ball[j].pos[1];
			vcontact[0][0] = ball[i].pos[0] - ball[j].pos[0];
			vcontact[0][1] = ball[i].pos[1] - ball[j].pos[1];
			//vcontact[0][2] = 0.0;
			distance = sqrt(vcontact[0][0]*vcontact[0][0] +
							vcontact[0][1]*vcontact[0][1]);

			//ball hits ball collision
			if (distance < (ball[i].radius + ball[j].radius)) {
				//We have a collision!
				playSound(0);
				//vector from center to center.
				VecNormalize2d(vcontact[0]);
				VecCopy2d(vcontact[0], vcontact[1]);
				VecNegate2d(vcontact[1]);
				movi[0] = ball[i].vel[0];
				movi[1] = ball[i].vel[1];
				movj[0] = ball[j].vel[0];
				movj[1] = ball[j].vel[1];
				vmove[0][0] = movi[0];
				vmove[0][1] = movi[1];
				VecNormalize2d(vmove[0]);
				vmove[1][0] = movj[0];
				vmove[1][1] = movj[1];
				VecNormalize2d(vmove[1]);
				//Determine how direct the hit was...
				dot0 = VecDot2d(vcontact[0], vmove[0]);
				dot1 = VecDot2d(vcontact[1], vmove[1]);
				//Find the closing (relative) speed of the objects...
				speed =
					sqrtf(movi[0]*movi[0] + movi[1]*movi[1]) * dot0 +
					sqrtf(movj[0]*movj[0] + movj[1]*movj[1]) * dot1;
				if (speed < 0.0) {
					//Normalize the mass of each object...
					massFactor = 2.0 / (ball[i].mass + ball[j].mass);
					massi = ball[i].mass * massFactor;
					massj = ball[j].mass * massFactor;
					ball[j].vel[0] += vcontact[0][0] * speed * massi;
					ball[j].vel[1] += vcontact[0][1] * speed * massi;
					ball[i].vel[0] += vcontact[1][0] * speed * massj;
					ball[i].vel[1] += vcontact[1][1] * speed * massj;
				}
			}
		}
	}

	//check off screen boxes
	/*
	bool hit2 = false;
	for (int i=0; i<game.nBoxes; i++) {
		//BALL COLLIONS WITH WALLS
		//
		//
		//ball[i].pos[0] == 900 && ball[i].pos[1] == 700
		//check balls against window edges
		if (box[i].pos[0] < box[i].radius && box[i].vel[0] <= 0.0) {
			//check of not in reserve pile
			if (box[i].pos[0] != 900 && box[i].pos[1] != 700) {
				box[i].vel[0] = -box[i].vel[0];
				//printf("ball hit edge\n");
				hit2 = true;
				//ball[i] = ball[game.n];
				//	game.n--;
				playSound(1);
			}
		}
		if (box[i].pos[0] >= (Flt)xres-box[i].radius && box[i].vel[0] >= 0.0) {

			if (box[i].pos[0] != 900 && box[i].pos[1] != 700) {
				box[i].vel[0] = -box[i].vel[0];
				//printf("ball hit edge\n");
				hit2 = true;
				//	ball[i] = ball[game.n];
				//	game.n--;
				//printf("ball hit edge\n");
				playSound(1);
			}
		}
		if (box[i].pos[1] < box[i].radius && box[i].vel[1] <= 0.0) {

			if (box[i].pos[1] != 700 && box[i].pos[0] != 900) {
			box[i].vel[1] = -box[i].vel[1];
			//printf("ball hit edge\n");
			hit2 = true;
			//	ball[i] = ball[game.n];
			//	game.n--;
			//printf("ball hit edge\n");
			playSound(1);
			}
		}
		if (box[i].pos[1] >= (Flt)yres-box[i].radius + 100 && box[i].vel[1] >= 0.0) {
		
			if (box[i].pos[1] != 700 && box[i].pos[0] != 900) {
				box[i].vel[1] = -box[i].vel[1];
			//	printf("ball hit edge\n");
				hit2 = true;
			//	ball[i] = ball[game.n];
			//	game.n--;		
			//	printf("ball hit edge\n");
				playSound(1);
		}

		if(hit2) {
		//		ball[i] = ball[game.n-1];
				box[i].pos[0] = 900;
				box[i].pos[1] = 700;
				box[i].vel[0] = 0;
				box[i].vel[1] = 0;
				printf("moving box off screen\n");
				hit2 = false;		
			}

		}
	}
	*/

	bool hit = false;



	//check off screen balls
	for (int i=0; i<game.n; i++) {
		//BALL COLLIONS WITH WALLS
		//
		//
		//ball[i].pos[0] == 900 && ball[i].pos[1] == 700
		//check balls against window edges
		if (ball[i].pos[0] < ball[i].radius && ball[i].vel[0] <= 0.0) {
			//check of not in reserve pile
			if (ball[i].pos[0] != 900 && ball[i].pos[1] != 700) {
				ball[i].vel[0] = -ball[i].vel[0];
				//printf("ball hit edge\n");
				hit = true;
				//ball[i] = ball[game.n];
				//	game.n--;
				playSound(1);
			}
		}
		if (ball[i].pos[0] >= (Flt)xres-ball[i].radius && ball[i].vel[0] >= 0.0) {

			if (ball[i].pos[0] != 900 && ball[i].pos[1] != 700) {
				ball[i].vel[0] = -ball[i].vel[0];
				//printf("ball hit edge\n");
				hit = true;
				//	ball[i] = ball[game.n];
				//	game.n--;
				//printf("ball hit edge\n");
				playSound(1);
			}
		}
		if (ball[i].pos[1] < ball[i].radius && ball[i].vel[1] <= 0.0) {

			if (ball[i].pos[1] != 700 && ball[i].pos[0] != 900) {
			ball[i].vel[1] = -ball[i].vel[1];
			//printf("ball hit edge\n");
			hit = true;
			//	ball[i] = ball[game.n];
			//	game.n--;
			//printf("ball hit edge\n");
			playSound(1);
			}
		}
		if (ball[i].pos[1] >= (Flt)yres-ball[i].radius + 100 && ball[i].vel[1] >= 0.0) {
		
			if (ball[i].pos[1] != 700 && ball[i].pos[0] != 900) {
				ball[i].vel[1] = -ball[i].vel[1];
			//	printf("ball hit edge\n");
				hit = true;
			//	ball[i] = ball[game.n];
			//	game.n--;		
			//	printf("ball hit edge\n");
				playSound(1);
		}

		if(hit) {
		//	ball[i] = ball[game.n-1];
			ball[i].pos[0] = 900;
			ball[i].pos[1] = 700;
			ball[i].vel[0] = 0;
			ball[i].vel[1] = 0;
			printf("moving ball off screen\n");
			hit = false;
			
		}
		

	}
	}




	//render bullet particles
	for (int i=0; i < game.nBullets; i++) { 
		//pulls the bullets down by gravity
	    	//game.particle[i].velocity[1] -= GRAVITY;

		game.particle[i].s.center[0] += game.particle[i].velocity[0] * 5;
		game.particle[i].s.center[1] += game.particle[i].velocity[1] * 5;


		//check off screen bullets
		if(game.particle[i].s.center[1] < 0.0 || game.particle[i].s.center[1] > yres) {
			game.particle[i] = game.particle[game.nBullets-1];
			game.nBullets--;
		}
		if(game.particle[i].s.center[0] < 0.0 || game.particle[i].s.center[0] > xres) {
			game.particle[i] = game.particle[game.nBullets-1];
			game.nBullets--;
		}
		//check for slow bullets no working
		
	
		//BULLET COLLIONS WITH Box
		/*
		for (int i3=0; i3<game.nBoxes; i3++) {

		//
		//finds position of particle postion against the balls
	//	Flt newx = game.particle[i].s.center[0] - box[i3].pos[0];
	//	Flt newy = game.particle[i].s.center[1] - box[i3].pos[1];
		Flt newx = game.particle[i].s.center[0];
		Flt newy = game.particle[i].s.center[1];
		Flt area = newx * newy;
		Flt distance = newx * newx;
		Flt distance2 = newy * newy;
		//
		//BULLET HIT BALL
		//TODO fix bullet collion with box
		//currently broken only clears box when bullet is to the left of it
		if (area < box[i3].pos[0]) {
		//	ball[i2].vel[0] = -ball[i2].vel[0];
			if (area < box[i3].pos[1]) {
			playSound(1);

			//debugging
			//game.score = 35;
			//game.level = 3;

			
			//make sure particle array is not overflown
			//spawn a ton of particles!
			    	killEffect(box[i3].pos[0], box[i3].pos[1]);

			//printf("ball[%i] hit sending off screen \n", i2);
			//remove ball that was hit
			game.score = game.score + 1;

			//printf("ball[%i] hit Score = %i \n", i2, game.score);
			printf("Score: %i \n", game.score);

			box[i3].pos[0] = 900;
			box[i3].pos[1] = 700;
			box[i3].vel[0] = 0;
			box[i3].vel[1] = 0;
		
			//remove bullet that was hit with ball
			game.particle[i] = game.particle[game.nBullets-1];
			game.nBullets--;

			}

			
			}
		}
		*/
	





		//BULLET COLLIONS WITH BALL
		for (int i2=0; i2<game.n; i2++) {

		//
		//finds position of particle postion against the balls
		Flt newx = game.particle[i].s.center[0] - ball[i2].pos[0];
		Flt newy = game.particle[i].s.center[1] - ball[i2].pos[1];
		//
		//BULLET HIT BALL
		if (pow(newx,2) + pow(newy, 2) < pow(ball[i2].radius,2)) {
		//	ball[i2].vel[0] = -ball[i2].vel[0];
			playSound(1);

			//debugging
			//game.score = 35;
			//game.level = 3;

			//split hit ball into 2 new balls
			if (ball[i2].split == false && game.level >= 3 && ball[i2].radius == 40){
				ball[i2].radius = ball[i2].radius/2;
				//game.n++;
				ball[game.n].radius = ball[i2].radius;
				ball[game.n].pos[0] = ball[i2].pos[0] + ball[i2].radius + 10;
				ball[game.n].pos[1] = ball[i2].pos[1] + ball[i2].radius + 10;
				ball[game.n].vel[0] = -ball[i2].vel[0];
				ball[game.n].vel[1] = -ball[i2].vel[1];
				if (game.n >= 10) {
					ball[game.n].split = true;
					ball[i2].split = true;
				}
				game.n++;
				return;
			} else {
			//make sure particle array is not overflown
			//spawn a ton of particles!
			    	killEffect(ball[i2].pos[0], ball[i2].pos[1]);

			}

			
			//printf("ball[%i] hit sending off screen \n", i2);
			//remove ball that was hit
			game.score = game.score + 1;

			//printf("ball[%i] hit Score = %i \n", i2, game.score);
			printf("Score: %i \n", game.score);

			ball[i2].pos[0] = 900;
			ball[i2].pos[1] = 700;
			ball[i2].vel[0] = 0;
			ball[i2].vel[1] = 0;
		
			//remove bullet that was hit with ball
			game.particle[i] = game.particle[game.nBullets-1];
			game.nBullets--;
			}
		}

	


	}

		//PLAYER COLLIONS WITH WALLS
		//
		//check if player is against winow edges show game over when too close
		if (game.player.pos[0] < game.player.radius && game.player.vel[0] <= 0.0) {
		//	ball[i].vel[0] = -ball[i].vel[0];
			game.player.pos[0] = game.player.radius;
			game.player.vel[0] = -game.player.vel[0];
			//gameOver();
		}
		if (game.player.pos[0] >= (Flt)xres-game.player.radius &&
				game.player.vel[0] >= 0.0) {
			game.player.pos[0] = (Flt)xres-game.player.radius;
		//	ball[i].vel[0] = -ball[i].vel[0];
			//gameOver();
		}
		if (game.player.pos[1] < game.player.radius && game.player.vel[1] <= 0.0) {
			//ball[i].vel[1] = -ball[i].vel[1];
			game.player.pos[1] = (Flt)game.player.radius;
			//gameOver();
		}
		if (game.player.pos[1] >= (Flt)yres-game.player.radius &&
				game.player.vel[1] >= 0.0) {
			//ball[i].vel[1] = -ball[i].vel[1];
			game.player.pos[1] = (Flt)yres-game.player.radius;
			//gameOver();
		}


	//Check for collision with window edges with player and balls
	for (int i=0; i<game.n; i++) {
		
		//check balls for player
		//
		//finds position of player postion against the balls
		//TODO fix sphere on sphere collision
		////
		Flt newx = game.player.pos[0] - ball[i].pos[0];
		Flt newy = game.player.pos[1] - ball[i].pos[1];
		if (pow(newx,2) + pow(newy, 2) < pow(ball[i].radius,2)) {	
			gameOver();
		}

	

		//BALL COLLIONS WITH WALLS
		//
		//check balls against window edges
		if (ball[i].pos[0] < ball[i].radius && ball[i].vel[0] <= 0.0) {
			//ball[i].vel[0] = -ball[i].vel[0];
			//printf("ball hit edge removing\n");
			//	ball[i] = ball[game.n];
			//	game.n--;
			playSound(1);
		}
		if (ball[i].pos[0] >= (Flt)xres-ball[i].radius && ball[i].vel[0] >= 0.0) {
		//	ball[i].vel[0] = -ball[i].vel[0];
		//	printf("ball hit edge removing\n");
		//		ball[i] = ball[game.n];
		//		game.n--;
			//printf("ball hit edge\n");
			playSound(1);
		}
		if (ball[i].pos[1] < ball[i].radius && ball[i].vel[1] <= 0.0) {
		//	ball[i].vel[1] = -ball[i].vel[1];
		//	printf("ball hit edge removing\n");
		//		ball[i] = ball[game.n];
		//		game.n--;
			//printf("ball hit edge\n");
			playSound(1);
		}
		if (ball[i].pos[1] >= (Flt)yres-ball[i].radius && ball[i].vel[1] >= 0.0) {
		//	ball[i].vel[1] = -ball[i].vel[1];
		//		printf("ball hit edge removing\n");
		//		ball[i] = ball[game.n];
		//		game.n--;		
		//	printf("ball hit edge\n");
			playSound(1);
		}

		//IF ball is off screen in reserve, send it to the screen
		//check of ball is in off screen reserve
		if (ball[i].pos[0] == 900 && ball[i].pos[1] == 700) {
			
			//shoots reserves to the right middle
		//	printf("repositioned reserved ball\n");
		//
		//gives random number from 0 - 4
		int randomPOS = rand()%6;
		//debugging
		//randomPOS = 4;
		//printf("%d = randomPOS\n", randomPOS);
		ball[i].radius = 40;
		

		switch (randomPOS)
		{
			//spawn top left
			// use minus 10ish to keep away from edges
			case 0:
				ball[i].pos[0] = 10 + ball[i].radius;
				ball[i].pos[1] = yres -10 - ball[i].radius;
				ball[i].vel[0] = (rand()%3)+1 * game.level;
				ball[i].vel[1] = -((rand()%3)+1 * game.level);
			break;
			
			//spawn top right
			case 1:
				ball[i].pos[0] = xres - 10 - ball[i].radius;
				ball[i].pos[1] = yres - 10 - ball[i].radius;
				ball[i].vel[0] = -((rand()%3)+1 * game.level);
				ball[i].vel[1] = -((rand()%3)+1 * game.level);
			break;
			//spawn bottom left
			case 2:
				ball[i].pos[0] = 0 + 10 + ball[i].radius;
				ball[i].pos[1] = 0 + 10 + ball[i].radius;
				ball[i].vel[0] = (rand()%3)+1 * game.level;
				ball[i].vel[1] = ((rand()%3)+1 * game.level);
			break;
			//spawn bottom right
			case 3:
				ball[i].pos[0] = xres - ball[i].radius;
				ball[i].pos[1] = 0 + ball[i].radius;
				ball[i].vel[0] = -((rand()%3)+1 * game.level);
				ball[i].vel[1] = ((rand()%3)+1 * game.level);
			break;
			//spawn bottom right mean
			case 4:
				ball[i].pos[0] = xres - ball[i].radius;
				ball[i].pos[1] = 5 + ball[i].radius;
				ball[i].vel[0] = -((rand()%5)+1 * game.level);
				ball[i].vel[1] = ((rand()%1)+1 * game.level);
			break;
			//spawn bottom right sends to left
			case 5:
				ball[i].pos[0] = xres - ball[i].radius;
				ball[i].pos[1] = 5 + ball[i].radius;
				ball[i].vel[0] = -((rand()%1) * game.level);
				ball[i].vel[1] = (.3);
			break;

		}
			
//			ball[i].pos[0] = 0;
//			ball[i].pos[1] = yres/2;
//			ball[i].vel[0] = 2;
//			ball[i].vel[1] = 0;

		}

		//update ball pos to chase player
		//ball[i].angle = game.player.angle;

		//Flt newposx = game.player.pos[0];
		//Flt newposy = game.player.pos[1];

		//ball[i].pos[0] = newposx;
		//ball[i].pos[1] = newposy;
	

		//speed of the balls
		//cap speed of the balls	
		if (ball[i].vel[0] > 10) {
			ball[i].vel[0] = 10;
		}
		if (ball[i].vel[1] > 10) {
			ball[i].vel[1] = 10;
		}

	
		


	}



}


void drawBox(Flt rad)
{
	int i;
	static int firsttime3=1;
	static Flt verts[32][2];
	static int n=32;


	if (firsttime3) {
		Flt ang=0.0;
		Flt inc = 3.14159 * 2.0 / (Flt)n;
		for (i=0; i<n; i++) {
			verts[i][0] = sin(ang);
			verts[i][1] = cos(ang);
			ang += inc;
		}
		firsttime3=0;
	}
	/*
	glBegin(GL_TRIANGLE_FAN);
		for (i=0; i<n; i++) {
			glVertex2f(verts[i][0]*rad, verts[i][1]*rad);
		}
	*/

	//glDisable(GL_BLEND);
}

void drawBall(Flt rad)
{
	int i;
	static int firsttime=1;
	static Flt verts[32][2];
	static int n=32;
	if (firsttime) {
		Flt ang=0.0;
		Flt inc = 3.14159 * 2.0 / (Flt)n;
		for (i=0; i<n; i++) {
			verts[i][0] = sin(ang);
			verts[i][1] = cos(ang);
			ang += inc;
		}
		firsttime=0;
	}
	glBegin(GL_TRIANGLE_FAN);
		for (i=0; i<n; i++) {
			glVertex2f(verts[i][0]*rad, verts[i][1]*rad);
		}
	glEnd();
}


void drawPlayer(Flt rad)
{
	int i;
	static int firsttime2=1;
	static Flt verts[32][2];
	static Flt vertsEye1[32][2];
	static int n=32;
	if (firsttime2) {
		//draw body
		Flt ang=0.0;
		Flt inc = 3.14159 * 2.0 / (Flt)n;
		for (i=0; i<n; i++) {
			verts[i][0] = sin(ang);
			verts[i][1] = cos(ang);
			ang += inc;
		}
		//draw left eye
		 ang=0.0;
		 inc = 3.14159 * 2.0 / (Flt)n;
		for (i=0; i<n; i++) {
			vertsEye1[i][0] = sin(ang);
			vertsEye1[i][1] = cos(ang);
			ang += inc;
		}
		firsttime2=0;
	}
	//draw the body
	glBegin(GL_TRIANGLE_FAN);
		for (i=0; i<n; i++) {
			glVertex2f(verts[i][0]*rad, verts[i][1]*rad);
		}
	glEnd();
	//draw left eye
	glBegin(GL_TRIANGLE_FAN);
		for (i=0; i<n; i++) {
			glVertex2f(vertsEye1[i][0]*rad/2, vertsEye1[i][1]*rad/2);
		}
	glEnd();

}


void renderBoxes(void)
{

	float h = 25.0;
	float w = 25.0;

	for(int i=0; i < game.nBoxes; i++) {
	//draw balls
	glColor3ub(0,255,0);
	glPushMatrix();
	glTranslatef(box[i].pos[0], box[i].pos[1], box[i].pos[2]);
//	drawBall(box[i].radius);

	glBegin(GL_QUADS);
		glVertex2i(-w, -h);
		glVertex2i(-w, h);
		glVertex2i( w, h);
		glVertex2i( w, -h);
	glEnd();

	glPopMatrix();
	//
	
	}

}

void renderBalls(void)
{

	for(int i=0; i < game.n; i++) {
	//draw balls
	glColor3ub(255,0,0);
	glPushMatrix();
	glTranslatef(ball[i].pos[0], ball[i].pos[1], ball[i].pos[2]);
	drawBall(ball[i].radius);
	glPopMatrix();
	//
	
	}

}

void renderPlayer() {

		
		//draw player
		glColor3ub(255,207,13);
		glPushMatrix();
		glTranslatef(game.player.pos[0], game.player.pos[1], game.player.pos[2]);
		drawPlayer(game.player.radius);
		glPopMatrix();
		//draw player left eye
		glColor3ub(255,225,255);
		glPushMatrix();
		glTranslatef(game.player.pos[0]-8, game.player.pos[1]+5, game.player.pos[2]);
		drawPlayer(game.player.eye1);
		glPopMatrix();
		//draw player right eye
		glColor3ub(255,225,255);
		glPushMatrix();
		glTranslatef(game.player.pos[0]+8, game.player.pos[1]+5, game.player.pos[2]);
		drawPlayer(game.player.eye2);
		glPopMatrix();

		//draw player left eye ball
		glColor3ub(0,0,0);
		glPushMatrix();
		if (game.player.lookUp == true) {

			glTranslatef(game.player.pos[0]+8, game.player.pos[1]+8, game.player.pos[2]);

		}
		else if (game.player.lookDown == true) {

			glTranslatef(game.player.pos[0]+8, game.player.pos[1]-1, game.player.pos[2]);
		}
		else if (game.player.lookLeft == true) {

			glTranslatef(game.player.pos[0]-12, game.player.pos[1]+5, game.player.pos[2]);
		}
		else if (game.player.lookRight == true) {

			glTranslatef(game.player.pos[0], game.player.pos[1]+5, game.player.pos[2]);
		}
	       	else {

			glTranslatef(game.player.pos[0]+8, game.player.pos[1]+5, game.player.pos[2]);
		}
		drawPlayer(game.player.eyeBall1);
		glPopMatrix();
		//
		//draw player right eye ball
		glColor3ub(0,0,0);
		glPushMatrix();
		if (game.player.lookUp == true) {
			
			glTranslatef(game.player.pos[0]-8, game.player.pos[1]+8, game.player.pos[2]);
		}
		else if (game.player.lookDown == true) {

			glTranslatef(game.player.pos[0]-8, game.player.pos[1]-1, game.player.pos[2]);
		}
		else if (game.player.lookLeft == true) {
	
			glTranslatef(game.player.pos[0]+1, game.player.pos[1]+5, game.player.pos[2]);
		}
		else if (game.player.lookRight == true) {

			glTranslatef(game.player.pos[0]+12, game.player.pos[1]+5, game.player.pos[2]);
		}
		else	{

			glTranslatef(game.player.pos[0]-8, game.player.pos[1]+5, game.player.pos[2]);
		}
		drawPlayer(game.player.eyeBall2);

		glPopMatrix();
		//
	

}


void render(void)
{
//	Rect r;
	glClear(GL_COLOR_BUFFER_BIT);

	if(game.state == STATE_GAMEOVER) {

		game.score = 0;

		glColor3ub(255,255,255);

		//ESC key switches back to STATE_STARTUP
		//
		//show gameOver texture
		//

	
		glBindTexture(GL_TEXTURE_2D, game.gameOverTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
			glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
			glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
			glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
		glEnd();



	}

	//
	if(game.state == STATE_GAMEPLAY) {
			    



		//show background space texture
		glEnable(GL_BLEND);

		glBindTexture(GL_TEXTURE_2D, game.backgroundTexture);
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0f);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
			glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
			glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
			glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
		glEnd();
		//be sure to use glBindTeture with 0 to unbind the texture to draw more	
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glPopMatrix();


	    	renderPlayer();
	 	renderBalls();
	//	renderBoxes();

		//game level 
		if (game.score == 0) {
			game.level = 0;
			//game.n++;

			game.levelUp = false;
		}
		if (game.score == 10) {
			game.level = 1;
			if (game.levelUp == false) { 
				ball[game.n].pos[0] = 900;
				ball[game.n].pos[1] = 700;
				ball[game.n].vel[0] = 0.0;
				ball[game.n].vel[1] = 0.0;
				ball[game.n].radius = 40.0;
				ball[game.n].mass = sphereVolume(ball[0].radius);
				ball[game.n].split = false;

				game.n++;
				game.levelUp = true;
			}
		}
		if (game.score == 20) {
			game.level = 2;
			if (game.levelUp == true) {
				ball[game.n].pos[0] = 900;
				ball[game.n].pos[1] = 700;
				ball[game.n].vel[0] = 0.0;
				ball[game.n].vel[1] = 0.0;
				ball[game.n].radius = 40.0;
				ball[game.n].mass = sphereVolume(ball[0].radius);
				ball[game.n].split = false;

				game.n++;
				game.levelUp = false;
			}

		}
		if (game.score == 25) {
			game.level = 3;
			if (game.levelUp == false) {
				ball[game.n].pos[0] = 900;
				ball[game.n].pos[1] = 700;
				ball[game.n].vel[0] = 0.0;
				ball[game.n].vel[1] = 0.0;
				ball[game.n].radius = 40.0;
				ball[game.n].mass = sphereVolume(ball[0].radius);
				ball[game.n].split = false;

				game.n++;
				game.levelUp = true;
			}
		}
		if (game.score == 30) {
			game.level = 4;
			if (game.levelUp == true) {
				ball[game.n].pos[0] = 900;
				ball[game.n].pos[1] = 700;
				ball[game.n].vel[0] = 0.0;
				ball[game.n].vel[1] = 0.0;
				ball[game.n].radius = 40.0;
				ball[game.n].mass = sphereVolume(ball[0].radius);
				ball[game.n].split = false;

				game.n++;
				game.levelUp = false;
			}
		}
		if (game.score == 35) {
			game.level = 5;
			if (game.levelUp == false ) {
				ball[game.n].pos[0] = 900;
				ball[game.n].pos[1] = 700;
				ball[game.n].vel[0] = 0.0;
				ball[game.n].vel[1] = 0.0;
				ball[game.n].radius = 40.0;
				ball[game.n].mass = sphereVolume(ball[0].radius);
				ball[game.n].split = false;

				game.n++;
				game.levelUp = true;
			}
		}
		if (game.score == 45) {
			game.level = 6;
			if (game.levelUp == true ) {
				ball[game.n].pos[0] = 900;
				ball[game.n].pos[1] = 700;
				ball[game.n].vel[0] = 0.0;
				ball[game.n].vel[1] = 0.0;
				ball[game.n].radius = 40.0;
				ball[game.n].mass = sphereVolume(ball[0].radius);
				ball[game.n].split = false;

				game.n++;
				game.levelUp = false;
			}
		}
	
		//render particles
		//still needs some love
		float h, w;
		for (int i = 0; i < game.nBullets; i++) {
			glPushMatrix();
			glColor3ub(18,125,255);
			w = 3;
			h = 3;
			glBegin(GL_QUADS);
			glVertex2i(game.particle[i].s.center[0]-w, game.particle[i].s.center[1]-h);
			glVertex2i(game.particle[i].s.center[0]-w, game.particle[i].s.center[1]+h);
			glVertex2i(game.particle[i].s.center[0]+w, game.particle[i].s.center[1]+h);
			glVertex2i(game.particle[i].s.center[0]+w, game.particle[i].s.center[1]-h);
			glEnd();
			glPopMatrix();

		}


	}
	if(game.state == STATE_STARTUP) {

		
		int randomX = rand()%800;
		int randomY = rand()%600;
		if(game.nHit < MAX_PARTICLES) {
			killEffect(randomX, randomY);
		}else{
			game.nHit = 0;
		}

	    	renderPlayer();

		//clears the rendering of balls
		game.n = 0;		

		//setup player center pos
		game.player.radius = 25;
		game.player.pos[0] = xres/2;
		game.player.pos[1] = yres/2;

		game.player.eye1 = 8;
		game.player.eye2 = 8;
		game.player.eyeBall1 = 2;
		game.player.eyeBall2 = 2;

		clearBullets();
		clearBalls();
		//clearBoxes();

		glColor3ub(255,255,255);




		//show title menu texture
		glEnable(GL_BLEND);

		glBindTexture(GL_TEXTURE_2D, game.titleTextureTrans);
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0f);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
			glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
			glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
			glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
		glEnd();
		//be sure to use glBindTeture with 0 to unbind the texture to draw more	
		glBindTexture(GL_TEXTURE_2D, 0);





		/*
		//show text box
		r.bot = 400;
		r.left = xres/2;
		r.center = xres/2;
		
		ggprint8b(&r, 16, 0x0000000, "x11 Wars");
		ggprint8b(&r, 16, 0x0000000, "WSAD to move");
		ggprint8b(&r, 16, 0x0000000, "ARROWS to shoot");
		ggprint8b(&r, 16, 0x0000000, "1 - Normal");
		ggprint8b(&r, 16, 0x0000000, "ESC - Pause");
		ggprint8b(&r, 16, 0x0000000, "P - Quit");
		*/
	}

		//render hitpart death particles from balls
		//if(game.nHit > 0) {
		    float h, w;
			for (int i = 0; i < game.nHit; i++) {
				glPushMatrix();
			
				
				if (game.hitPart[i].thrust == true) {
					glColor3ub(255,129,79);

				} else {
					glColor3ub(255,255,255);
				}
					
				if (game.hitPart[i].thrust == true) {
					w = 1;
					h = 1;
				} else {
					w = 2;
					h = 2;
				}
				glBegin(GL_QUADS);
				glVertex2i(game.hitPart[i].s.center[0]-w, game.hitPart[i].s.center[1]-h);
				glVertex2i(game.hitPart[i].s.center[0]-w, game.hitPart[i].s.center[1]+h);
				glVertex2i(game.hitPart[i].s.center[0]+w, game.hitPart[i].s.center[1]+h);
				glVertex2i(game.hitPart[i].s.center[0]+w, game.hitPart[i].s.center[1]-h);
				glEnd();
				glPopMatrix();

				glColor3ub(255,255,255);
			}
	//	}

	        //
        if (showOpengl) {
                glClear(GL_COLOR_BUFFER_BIT);
                float wid = 40.0f;
                glColor3ub(30,60,90);
                glPushMatrix();
                glTranslatef(pos[0], pos[1], pos[2]);
                glBegin(GL_QUADS);
                        glVertex2i(-wid,-wid);
                        glVertex2i(-wid, wid);
                        glVertex2i( wid, wid);
                        glVertex2i( wid,-wid);
                glEnd();
                glPopMatrix();
        }



}
