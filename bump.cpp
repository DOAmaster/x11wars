//Modified by: Derrick Alden
//program: bump.cpp
//github: https://github.com/doamaster/x11wars
//author:  Gordon Griesel
//date:    2018
//Depending on your Linux distribution,
//may have to install these packages:
// libx11-dev
// libglew1.6
// libglew1.6-dev
//
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
#define MAX_PARTICLES 1000
#define GRAVITY 0.1
#define PI 3.141592653589793

//might use for future ai 
const float gravity = 0.2f;
const int MAX_BULLETS = 11;

static int xres=800, yres=600;
void setup_screen_res(const int w, const int h);


struct Shape {

	float width, height;
	float radius;
	Vec center;

};

struct Particle {
	Shape s;
	Vec velocity;
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
Image img[2] = {
"./title.jpg",
"./gameover.jpg" };

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
		*(ptr+3) = (a|b|c);
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
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
void check_keys(XEvent *e);
void physics(void);
void render(void);

int done=0;
int leftButtonDown=0;
Vec leftButtonPos;
class Ball {
public:
	Vec pos;
	Vec vel;
	Vec force;
	float radius;
	float mass;
} ball[15];


class Player {
public:
	Vec pos;
	Vec vel;
	Vec force;
	float angle;
	float radius;
	float mass;
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
	int n;
	int nBullets;
	int maxBullets;
	int score;
	float moveSpeed;
	
	//texures
	GLuint gameOverTexture;
	GLuint titleTexture;

	State state;
	bool spawner;
	unsigned char keys[65535];

	Game(){
		score = 0;
	    	state = STATE_STARTUP;
		spawner = false;
		n = 0;
		moveSpeed = 5;
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
//-----------------------------------------------------------------------------


int main(void)
{
	init_opengl();
	init_balls();
	clock_gettime(CLOCK_REALTIME, &timePause);
	clock_gettime(CLOCK_REALTIME, &timeStart);

	//declar game object
//	Game game;
//	game.n=0;

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
	}
	//cleanup_fonts();
	return 0;
}

void setup_screen_res(const int w, const int h)
{
	xres = w;
	yres = h;
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

}

#define sphereVolume(r) (r)*(r)*(r)*3.14159265358979*4.0/3.0;

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
	ball[1].pos[0] = 400;
	ball[1].pos[1] = yres-150;
	ball[1].vel[0] = 0.0;
	ball[1].vel[1] = 0.0;
	ball[1].force[0] =
	ball[1].force[1] = 0.0;
	ball[1].radius = 25.0;
	ball[1].mass = sphereVolume(ball[1].radius);
	game.n++;

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
	for( int i; i < game.n; i++) {
		ball[i].pos[0] = 200;
		ball[i].pos[1] = yres-150;
		ball[i].vel[0] = 0.0;
		ball[i].vel[1] = 0.0;
		ball[i].radius = 70.0;
		ball[i].mass = sphereVolume(ball[i].radius);
		game.n--;
	}


}

void scenario1(void)
{
	game.state = STATE_GAMEPLAY;
	//top right start 
	ball[0].pos[0] = xres - 50;
	ball[0].pos[1] = yres - 50;
	ball[0].vel[0] = 0.0;
	ball[0].vel[1] = 0.0;
	ball[0].radius = 40.0;
	ball[0].mass = sphereVolume(ball[0].radius);
	game.n++;
	//bot left start
	ball[1].pos[0] = 50;
	ball[1].pos[1] = 50;
	ball[1].vel[0] = 0.0;
	ball[1].vel[1] = 0.0;
	ball[1].radius = 40.0;
	ball[1].mass = sphereVolume(ball[1].radius);
	game.n++;
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
			case XK_o:
				ball[0].radius -= 1.0;
				ball[0].mass = sphereVolume(ball[0].radius);
				break;
			case XK_i:
				ball[0].radius += 1.0;
				ball[0].mass = sphereVolume(ball[0].radius);
				break;
			case XK_k:
				ball[1].radius -= 1.0;
				ball[1].mass = sphereVolume(ball[1].radius);
				break;
			case XK_l:
				ball[1].radius += 1.0;
				ball[1].mass = sphereVolume(ball[1].radius);
				break;
			case XK_m:
				//press s to slow the balls
				ball[0].vel[0] *= 0.5;
				ball[0].vel[1] *= 0.5;
				ball[1].vel[0] *= 0.5;
				ball[1].vel[1] *= 0.5;
				break;
			case XK_Escape:
				game.state = STATE_STARTUP;
				//done=1;
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
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = rnd() * .1; 
	game.particle[game.nBullets].velocity[1] = rnd() * 2.0; 
	game.nBullets++;


}

void shootDown()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -(rnd() * .1); 
	game.particle[game.nBullets].velocity[1] = -(rnd() * 2.0); 
	game.nBullets++;


}


void shootDownLeft()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -(rnd() * 2.0); 
	game.particle[game.nBullets].velocity[1] = -(rnd() * 2.0); 
	game.nBullets++;


}

void shootDownRight()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = rnd() * 2.0; 
	game.particle[game.nBullets].velocity[1] = -(rnd() * 2.0); 
	game.nBullets++;


}

void shootUpLeft()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -(rnd() * 2.0); 
	game.particle[game.nBullets].velocity[1] = (rnd() * 2.0); 
	game.nBullets++;


}


void shootUpRight()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = (rnd() * 2.0); 
	game.particle[game.nBullets].velocity[1] = (rnd() * 2.0); 
	game.nBullets++;


}


void shootLeft()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = -(rnd() * 2.0); 
	game.particle[game.nBullets].velocity[1] = (rnd() * .1); 
	game.nBullets++;


}

void shootRight()
{
//	makeParticle(game.player.pos[0], game.player.pos[1]);
//	game.particle[game.nBullets].velocity[0] = 0; 
//	game.particle[game.nBullets].velocity[1] = 5; 
		if (game.nBullets >= game.maxBullets)
		return;
	//Particle p = game.particle[game.nBullets];
	game.particle[game.nBullets].s.center[0] = game.player.pos[0];
	game.particle[game.nBullets].s.center[1] = game.player.pos[1];
	game.particle[game.nBullets].velocity[0] = rnd() * 2.0; 
	game.particle[game.nBullets].velocity[1] = rnd() * .1; 
	game.nBullets++;


}

void moveLeft() { game.player.pos[0] -= game.moveSpeed; }

void moveRight() { game.player.pos[0] += game.moveSpeed; }

void moveUp() { game.player.pos[1] += game.moveSpeed; }

void moveDown() { game.player.pos[1] -= game.moveSpeed; }


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

}

void killBall(void)
{


}

void physics(void)
{	
	//shooting key checks
	
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

	//Update positions mouse ball movement for debuging
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
	//Different physics applied here...
	//100% elastic collisions. 
	for (int i=0; i<game.n; i++) {
		ball[i].pos[0] += ball[i].vel[0];
		ball[i].pos[1] += ball[i].vel[1];
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
		/*
		if(game.particle[i].velocity[0] < .1 && game.particle[i].velocity[1] < .1) {	
			game.particle[i] = game.particle[game.nBullets-1];
			game.nBullets--;
		}
		*/


		//BULLET COLLIONS WITH BALL
		for (int i2=0; i2<game.n; i2++) {

		//
		//finds position of particle postion against the balls
		Flt newx = game.particle[i].s.center[0] - ball[i2].pos[0];
		Flt newy = game.particle[i].s.center[1] - ball[i2].pos[1];
		if (pow(newx,2) + pow(newy, 2) < pow(ball[i2].radius,2)) {
		//	ball[i2].vel[0] = -ball[i2].vel[0];
			playSound(1);
			//printf("ball[%i] hit \n", i2);
			//remove ball that was hit
			if (game.n > 0) {
				ball[i2] = ball[game.n-1];
				game.n--;
			}
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
			
			//playSound(1);
		//	printf("Player hit ball\n");
			gameOver();

			/*
			if (game.n > 0) {
				ball[i2] = ball[game.n-1];
				game.n--;
			}
			*/
		}

	

		//BALL COLLIONS WITH WALLS
		//
		//check balls against window edges
		if (ball[i].pos[0] < ball[i].radius && ball[i].vel[0] <= 0.0) {
			ball[i].vel[0] = -ball[i].vel[0];
			playSound(1);
		}
		if (ball[i].pos[0] >= (Flt)xres-ball[i].radius &&
				ball[i].vel[0] >= 0.0) {
			ball[i].vel[0] = -ball[i].vel[0];
			playSound(1);
		}
		if (ball[i].pos[1] < ball[i].radius && ball[i].vel[1] <= 0.0) {
			ball[i].vel[1] = -ball[i].vel[1];
			playSound(1);
		}
		if (ball[i].pos[1] >= (Flt)yres-ball[i].radius &&
				ball[i].vel[1] >= 0.0) {
			ball[i].vel[1] = -ball[i].vel[1];
			playSound(1);
		}
	}
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
	static int n=32;
	if (firsttime2) {
		Flt ang=0.0;
		Flt inc = 3.14159 * 2.0 / (Flt)n;
		for (i=0; i<n; i++) {
			verts[i][0] = sin(ang);
			verts[i][1] = cos(ang);
			ang += inc;
		}
		firsttime2=0;
	}
	glBegin(GL_TRIANGLE_FAN);
		for (i=0; i<n; i++) {
			glVertex2f(verts[i][0]*rad, verts[i][1]*rad);
		}
	glEnd();
}

void renderBalls(void)
{

	//draw balls
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[0].pos[0], ball[0].pos[1], ball[0].pos[2]);
	drawBall(ball[0].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[1].pos[0], ball[1].pos[1], ball[1].pos[2]);
	drawBall(ball[1].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[3].pos[0], ball[3].pos[1], ball[3].pos[2]);
	drawBall(ball[3].radius);
	glPopMatrix();
	//
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[4].pos[0], ball[4].pos[1], ball[4].pos[2]);
	drawBall(ball[4].radius);
	glPopMatrix();
	//
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[5].pos[0], ball[5].pos[1], ball[5].pos[2]);
	drawBall(ball[5].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[6].pos[0], ball[6].pos[1], ball[6].pos[2]);
	drawBall(ball[6].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[7].pos[0], ball[7].pos[1], ball[7].pos[2]);
	drawBall(ball[7].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[8].pos[0], ball[8].pos[1], ball[8].pos[2]);
	drawBall(ball[8].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[9].pos[0], ball[9].pos[1], ball[9].pos[2]);
	drawBall(ball[9].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[10].pos[0], ball[10].pos[1], ball[10].pos[2]);
	drawBall(ball[10].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[11].pos[0], ball[11].pos[1], ball[11].pos[2]);
	drawBall(ball[11].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[12].pos[0], ball[12].pos[1], ball[12].pos[2]);
	drawBall(ball[12].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[13].pos[0], ball[13].pos[1], ball[13].pos[2]);
	drawBall(ball[13].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[14].pos[0], ball[14].pos[1], ball[14].pos[2]);
	drawBall(ball[14].radius);
	glPopMatrix();
	//
	glColor3ub(17,219,17);
	glPushMatrix();
	glTranslatef(ball[15].pos[0], ball[15].pos[1], ball[15].pos[2]);
	drawBall(ball[15].radius);
	glPopMatrix();
	//
	//
}


void render(void)
{
//	Rect r;
	glClear(GL_COLOR_BUFFER_BIT);

	if(game.state == STATE_GAMEOVER) {


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

		//draw player
		glColor3ub(255,255,255);
		glPushMatrix();
		glTranslatef(game.player.pos[0], game.player.pos[1], game.player.pos[2]);
		drawPlayer(game.player.radius);
		glPopMatrix();
		//
		//
 		renderBalls();


	
		//render particles
		//still needs some love
		float h, w;
		for (int i = 0; i < game.nBullets; i++) {
		glPushMatrix();
		glColor3ub(150,160,220);
		w = 2;
		h = 2;
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

		//setup player center pos
		game.player.radius = 25;
		game.player.pos[0] = xres/2;
		game.player.pos[1] = yres/2;

		glColor3ub(255,255,255);
		//show title menu texture
		glBindTexture(GL_TEXTURE_2D, game.titleTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
			glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
			glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
			glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
		glEnd();
	

		clearBullets();
		clearBalls();

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


}



