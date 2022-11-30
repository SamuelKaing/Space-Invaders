//
//program: asteroids.cpp
//author:  Gordon Griesel
//date:    2014 - 2021
//mod spring 2015: added constructors
//This program is a game starting point for a 3350 project.
//
//
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include <cmath>
#include <X11/Xlib.h>
//#include <X11/Xutil.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "log.h"
#include "fonts.h"
#include "image.h"
#include "skaing.h"
#include "rverduzcogui.h"
#include "jflanders.h"
#include "jsanchezcasa.h"
//defined types
typedef float Flt;
typedef float Vec[3];
typedef Flt	Matrix[4][4];

//macros
#define rnd() (((Flt)rand())/(Flt)RAND_MAX)
#define random(a) (rand()%a)
#define VecZero(v) (v)[0]=0.0,(v)[1]=0.0,(v)[2]=0.0
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
						(c)[1]=(a)[1]-(b)[1]; \
						(c)[2]=(a)[2]-(b)[2]
//constants
const float timeslice = 1.0f;
const float gravity = -0.2f;
#define PI 3.141592653589793
#define ALPHA 1
//const int MAX_BULLETS = 11;
const Flt MINIMUM_ASTEROID_SIZE = 60.0;

//-----------------------------------------------------------------------------
//Setup timers
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
extern struct timespec timeStart, timeCurrent;
extern struct timespec timePause;
extern double physicsCountdown;
extern double timeSpan;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);
//-----------------------------------------------------------------------------


class Global {
public:
	Texture t, t_boss, t_enemy, t_back;
	int xres, yres;
	char keys[65536];
	unsigned int mouse_cursor;
	unsigned int level, credits, p_screen, help, gameover, start, 
			boss_rush, test_mode, juanfeature, feature_weapons, win_screen;
	int weapon, max_bullets;
	struct timespec boss_bulletTimer;
	Global() {
		level = 1;
		xres = 640;
		yres = 480;
		memset(keys, 0, 65536);
		mouse_cursor = 0;	//Initial mouse state is off
		credits = 0; 		//Credits page initially off
		p_screen = 0;     	//Pause screen initially off
		help = 0;           //Help screen initially off
		gameover = 0;     	//Test for Gameover
		start = 1;	   		//Game start screen on
    	boss_rush = 0;		//Boss Rush mode initially off
		test_mode = 0;
		juanfeature = 0;
		feature_weapons = 0;
		weapon = 0;
		max_bullets = 1;
		win_screen = 0;
	}
} gl;

class Ship {
public:
	Vec pos;
	Vec dir;
	Vec vel;
	Vec acc;
	float angle;
	float color[3];
public:
	Ship() {
		pos[0] = (Flt)(gl.xres/2);
		pos[1] = (Flt)(gl.yres/4);
		pos[2] = 0.0f;
		VecZero(dir);
		VecZero(vel);
		VecZero(acc);
		angle = 0.0;
		color[0] = color[1] = color[2] = 1.0;
	}
};

class Asteroid {
public:
	Vec pos;
	Vec vel;
	int nverts;
	Flt radius;
	Vec vert[8];
	float angle;
	float rotate;
	float color[3];
	struct Asteroid *prev;
	struct Asteroid *next;
public:
	Asteroid() {
		prev = NULL;
		next = NULL;
	}
};

class Game {
public:
	int level[3] = {2, 3, 4};
	Ship ship;
	Asteroid *ahead;
	Bullet *barr;
	int nasteroids;
	int nbullets;
	struct timespec bulletTimer;
	struct timespec mouseThrustTimer;
	bool mouseThrustOn;
public:
	Game() {
		//level[] = {2, 5, 10};
		ahead = NULL;
		barr = new Bullet[gl.max_bullets];
		nasteroids = 0;
		nbullets = 0;
		mouseThrustOn = false;
		//build 10 asteroids...
		for (int j=0; j<level[gl.level]; j++) {
			Asteroid *a = new Asteroid;
			a->nverts = 8;
			a->radius = rnd()*80.0 + 40.0;
			Flt r2 = a->radius / 2.0;
			Flt angle = 0.0f;
			Flt inc = (PI * 2.0) / (Flt)a->nverts;
			for (int i=0; i<a->nverts; i++) {
				a->vert[i][0] = sin(angle) * (r2 + rnd() * a->radius);
				a->vert[i][1] = cos(angle) * (r2 + rnd() * a->radius);
				angle += inc;
			}
			a->pos[0] = (Flt)(rand() % gl.xres);
			a->pos[1] = (Flt)(rand() % gl.yres);
			a->pos[2] = 0.0f;
			a->angle = 0.0;
			a->rotate = rnd() * 4.0 - 2.0;
			a->color[0] = 0.8;
			a->color[1] = 0.8;
			a->color[2] = 0.7;
			a->vel[0] = (Flt)(rnd()*2.0-1.0); //(Flt)(rnd()*2.0-1.0
			a->vel[1] = (Flt)(rnd()*2.0-1.0);
			//std::cout << "asteroid" << std::endl;
			//add to front of linked list
			a->next = ahead;
			if (ahead != NULL)
				ahead->prev = a;
			ahead = a;
			++nasteroids;
		}
        for (int z=0; z<2;z++) {
            make_enemy(gl.xres, gl.yres, gl.t_enemy);
            z++;
        }
		clock_gettime(CLOCK_REALTIME, &bulletTimer);
	}

	void reset(int newLevel){
		ahead = NULL;
		barr = new Bullet[gl.max_bullets];
		nasteroids = 0;
		nbullets = 0;
		mouseThrustOn = false;
		//build Level[l] amount of asteroids...
		for (int j=0; j<level[newLevel-1]; j++) {
			Asteroid *a = new Asteroid;
			a->nverts = 8;
			a->radius = rnd()*80.0 + 40.0;
			Flt r2 = a->radius / 2.0;
			Flt angle = 0.0f;
			Flt inc = (PI * 2.0) / (Flt)a->nverts;
			for (int i=0; i<a->nverts; i++) {
				a->vert[i][0] = sin(angle) * (r2 + rnd() * a->radius);
				a->vert[i][1] = cos(angle) * (r2 + rnd() * a->radius);
				angle += inc;
			}
			a->pos[0] = (Flt)(rand() % gl.xres);
			a->pos[1] = (Flt)(rand() % gl.yres);
			a->pos[2] = 0.0f;
			a->angle = 0.0;
			a->rotate = rnd() * 4.0 - 2.0;
			a->color[0] = 0.8;
			a->color[1] = 0.8;
			a->color[2] = 0.7;
			a->vel[0] = (Flt)(rnd()*2.0-1.0); //(Flt)(rnd()*2.0-1.0
			a->vel[1] = (Flt)(rnd()*2.0-1.0);
			//std::cout << "asteroid" << std::endl;
			//add to front of linked list
			a->next = ahead;
			if (ahead != NULL)
				ahead->prev = a;
			ahead = a;
			++nasteroids;
	
		}
	}
	~Game() {extern unsigned int manage_feature_weapons_state(unsigned int w);

//extern void show_feature_weapons(int xres, int yres, int weapon1);

		delete [] barr;
	}
};

//Image Class
Image image[4] = {"SInvaders.jpeg", "images/boss.png", 
		  "images/s_enemy.jpeg", "images/background.png"};

//X Windows variables
class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	X11_wrapper() { }
	X11_wrapper(int w, int h) {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
		XSetWindowAttributes swa;
		setup_screen_res(gl.xres, gl.yres);
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			std::cout << "\n\tcannot connect to X server" << std::endl;
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XWindowAttributes getWinAttr;
		XGetWindowAttributes(dpy, root, &getWinAttr);
		int fullscreen = 0;
		gl.xres = w;
		gl.yres = h;
		if (!w && !h) {
			//Go to fullscreen.
			gl.xres = getWinAttr.width;
			gl.yres = getWinAttr.height;
			//When window is fullscreen, there is no client window
			//so keystrokes are linked to the root window.
			XGrabKeyboard(dpy, root, False,
				GrabModeAsync, GrabModeAsync, CurrentTime);
			fullscreen=1;
		}
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			std::cout << "\n\tno appropriate visual found\n" << std::endl;
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		swa.colormap = cmap;
		swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
			PointerMotionMask | MotionNotify | ButtonPress | ButtonRelease |
			StructureNotifyMask | SubstructureNotifyMask;
		unsigned int winops = CWBorderPixel|CWColormap|CWEventMask;
		if (fullscreen) {
			winops |= CWOverrideRedirect;
			swa.override_redirect = True;
		}
		win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
			vi->depth, InputOutput, vi->visual, winops, &swa);
		//win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
		//vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
		show_mouse_cursor(0);
	}
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "Space Invaders");
	}
	void check_resize(XEvent *e) {
		//The ConfigureNotify is sent by the
		//server if the window is resized.
		if (e->type != ConfigureNotify)
			return;
		XConfigureEvent xce = e->xconfigure;
		if (xce.width != gl.xres || xce.height != gl.yres) {
			//Window size did change.
			reshape_window(xce.width, xce.height);
		}
	}
	void reshape_window(int width, int height) {
		//window has been resized.
		setup_screen_res(width, height);
		glViewport(0, 0, (GLint)width, (GLint)height);
		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW); glLoadIdentity();
		glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
		set_title();
	}
	void setup_screen_res(const int w, const int h) {
		gl.xres = w;
		gl.yres = h;
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
	bool getXPending() {
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void set_mouse_position(int x, int y) {
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, x, y);
	}
	Display* getDisplay(){
		return dpy;
	}
	Window getWindow(){
		return win;
	}
	void show_mouse_cursor(const int onoff) {
		if (onoff) {
			//this removes our own blank cursor.
			XUndefineCursor(dpy, win);
			return;
		}
		//vars to make blank cursor
		Pixmap blank;
		XColor dummy;
		char data[1] = {0};
		Cursor cursor;
		//make a blank cursor
		blank = XCreateBitmapFromData (dpy, win, data, 1, 1);
		if (blank == None)
			std::cout << "error: out of memory." << std::endl;
		cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
		XFreePixmap(dpy, blank);
		//this makes the cursor. then set it using this function
		XDefineCursor(dpy, win, cursor);
		//after you do not need the cursor anymore use this function.
		//it will undo the last change done by XDefineCursor
		//(thus do only use ONCE XDefineCursor and then XUndefineCursor):
	}
} x11(gl.xres, gl.yres);
// ---> for fullscreen x11(0, 0);

//function prototypes
void init_opengl(void);
void check_mouse(XEvent *e, Game *g);
int check_keys(XEvent *e,Game *g);
void physics(Game *g);
void render(Game *g);
void deleteAsteroid(Game *g, Asteroid *node);
extern void menu(int xres, int yres);
extern void show_credits(Texture t, int xres, int yres);

//extern unsigned int manage_state(unsigned int s);
//==========================================================================
// M A I N
//==========================================================================
int main()
{
	Game *g = new Game();
	logOpen();
	init_opengl();
	srand(time(NULL));
	clock_gettime(CLOCK_REALTIME, &timePause);
	clock_gettime(CLOCK_REALTIME, &timeStart);
	x11.set_mouse_position(100,100);
	int done=0;
	while (!done) {
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			check_mouse(&e, g);
			done = check_keys(&e, g);
		}
		clock_gettime(CLOCK_REALTIME, &timeCurrent);
		timeSpan = timeDiff(&timeStart, &timeCurrent);
		timeCopy(&timeStart, &timeCurrent);
		physicsCountdown += timeSpan;
		while (physicsCountdown >= physicsRate) {
			physics(g);
			physicsCountdown -= physicsRate;
		}
		render(g);
		x11.swapBuffers();
	}
	cleanup_fonts();
	logClose();
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, gl.xres, gl.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	//
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	gl.t.img = &image[0];
	gl.t_boss.img = &image[1];
	gl.t_enemy.img = &image[2];
	gl.t_back.img = &image[3];
	//
	//Clear the screen to black
	glClearColor(0.0, 0.0, 0.0, 1.0);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
	
	//Init background texture
	glGenTextures(1, &gl.t_back.backText);
	int width = gl.t_back.img->w;
	int height = gl.t_back.img->h;
	glBindTexture(GL_TEXTURE_2D, gl.t_back.backText);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0,
						GL_RGB, GL_UNSIGNED_BYTE, gl.t_back.img->data);
}

void normalize2d(Vec v)
{
	Flt len = v[0]*v[0] + v[1]*v[1];
	if (len == 0.0f) {
		v[0] = 1.0;
		v[1] = 0.0;
		return;
	}
	len = 1.0f / sqrt(len);
	v[0] *= len;
	v[1] *= len;
}

void check_mouse(XEvent *e, Game *g)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	static int ct=0;
	//std::cout << "m" << std::endl << std::flush;
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//printf("X: %3i, Y: %3i\n", e->xbutton.x, e->xbutton.y);
			int xcent = gl.xres / 2;
			int ycent = gl.yres / 2;
			//Check if mouse is over resume or quit buttons.
			
			if (gl.mouse_cursor && e->xbutton.x > xcent-100 && e->xbutton.x < xcent+100){
				//Resume
				if (e->xbutton.y > (ycent/2) && e->xbutton.y < (ycent *.8))
					gl.mouse_cursor ^= 1;
				//Help Screen
				else if (e->xbutton.y > (ycent/1.1) && e->xbutton.y > (ycent * .625)){
					gl.help ^= 1;
					//printf("Success!");
					//show_controls(gl.xres, gl.yres);
				//Quit
				} else if (e->xbutton.y > (ycent*1.35) && e->xbutton.y < (ycent *1.65))
					XDestroyWindow(x11.getDisplay(), x11.getWindow());
			}


			if (gl.credits || gl.mouse_cursor)
			    return;
			if(gl.p_screen)
			    return;
			if (gl.help)
				return;
		    
		    	//Left button is down
			//a little time between each bullet
			struct timespec bt;
			clock_gettime(CLOCK_REALTIME, &bt);
			double ts = timeDiff(&g->bulletTimer, &bt);
			if (ts > 0.1) {
				timeCopy(&g->bulletTimer, &bt);
				//shoot a bullet...

				if (g->nbullets < gl.max_bullets) {
					Bullet *b = &g->barr[g->nbullets];

					timeCopy(&b->time, &bt);
					b->pos[0] = g->ship.pos[0];
					b->pos[1] = g->ship.pos[1];
					b->vel[0] = g->ship.vel[0];
					b->vel[1] = g->ship.vel[1];
					//convert ship angle to radians
					Flt rad = ((g->ship.angle+90.0) / 360.0f) * PI * 2.0;
					//convert angle to a vector
					Flt xdir = cos(rad);
					Flt ydir = sin(rad);
					b->pos[0] += xdir*20.0f;
					b->pos[1] += ydir*20.0f;
					b->vel[0] += xdir*6.0f + rnd()*0.1;
					b->vel[1] += ydir*6.0f + rnd()*0.1;
					b->color[0] = 1.0f;
					b->color[1] = 1.0f;
					b->color[2] = 1.0f;
					++g->nbullets;
				}
			}
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	//keys[XK_Up] = 0;
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		//check mouse state
		if (gl.mouse_cursor)
		    return;
		if (gl.credits)
		    return;
		if(gl.p_screen)
		    return;
		if (gl.help)
		    return;
		if(gl.win_screen)
		    return;

		//
		int xdiff = savex - e->xbutton.x;
		int ydiff = savey - e->xbutton.y;
		if (++ct < 10)
			return;		
		//std::cout << "savex: " << savex << std::endl << std::flush;
		//std::cout << "e->xbutton.x: " << e->xbutton.x << std::endl <<
		//std::flush;
		if (xdiff > 0) {
			//std::cout << "xdiff: " << xdiff << std::endl << std::flush;
			//g->ship.angle += 0.05f * (float)xdiff;
			//if (g->ship.angle >= 360.0f)
			//	g->ship.angle -= 360.0f;
			g->ship.pos[0] -= 3;
		}
		else if (xdiff < 0) {
			//std::cout << "xdiff: " << xdiff << std::endl << std::flush;
			//g->ship.angle += 0.05f * (float)xdiff;
			//if (g->ship.angle < 0.0f)
			//	g->ship.angle += 360.0f;
			g->ship.pos[0] += 3;
		}
		if (ydiff > 0) {
		    
			//apply thrust
			//convert ship angle to radians
			//Flt rad = ((g->ship.angle+90.0) / 360.0f) * PI * 2.0;
			////convert angle to a vector
			//Flt xdir = cos(rad);
			//Flt ydir = sin(rad);
			////g->ship.vel[0] += xdir * (float)ydiff * 0.01f;
			//g->ship.vel[1] += ydir * (float)ydiff * 0.01f;
			//Flt speed = sqrt(g->ship.vel[0]*g->ship.vel[0]+
			//									g->ship.vel[1]*g->ship.vel[1]);
			//if (speed > 10.0f) {
			//	speed = 10.0f;
			//	normalize2d(g->ship.vel);
			//	g->ship.vel[0] *= speed;
			//	g->ship.vel[1] *= speed;
			//}
			//g->mouseThrustOn = true;
			//clock_gettime(CLOCK_REALTIME, &g->mouseThrustTimer);
			
		    g->ship.pos[1] += 3;

		}
		if (ydiff < 0) {
		    g->ship.pos[1] -= 3;
		}

		x11.set_mouse_position(100,100);
		savex = 100;
		savey = 100;
	} else {
	    //g->ship.vel[0] = 0;
	    //g->ship.vel[1] = 0;
	}
}


int check_keys(XEvent *e, Game *g)
{
	static int shift=0;
	if (e->type != KeyRelease && e->type != KeyPress) {
		//not a keyboard event
		return 0;
	}
	int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
	//Log("key: %i\n", key);
	if (e->type == KeyRelease) {
		gl.keys[key] = 0;
		if (key == XK_Shift_L || key == XK_Shift_R)
			shift = 0;
		return 0;
	}
	if (e->type == KeyPress) {
	    	if (!gl.mouse_cursor){
		    //std::cout << "press" << std::endl;
		    gl.keys[key]=1;
		    if (key == XK_Shift_L || key == XK_Shift_R) {
			shift = 1;
			return 0;
		    }
		}
	}
	(void)shift;
	switch (key) {
		case XK_Escape:
			return 1;
		case XK_F1:
			//Toggle help menu
			gl.help = manage_help_state(gl.help);
			break;

		case XK_b:
			gl.boss_rush = boss_rush_state(gl.boss_rush);
			//make boss
			init_boss(gl.xres, gl.yres, gl.t_boss, gl.level);
			//start timer for behavior
			clock_gettime(CLOCK_REALTIME, &gl.boss_bulletTimer);
			break;
		case XK_m:
			//Toggles mouse cursor state
			gl.mouse_cursor ^= 1;
			x11.show_mouse_cursor(gl.mouse_cursor);
			break;
		case XK_c:
			//Toggle credits screen on/off
			gl.credits = manage_state(gl.credits);
			break;
		case XK_x:
			//Toggle Jacob's feature mode
			gl.test_mode = manage_mode(gl.test_mode);
			make_enemy(gl.xres, gl.yres, gl.t_enemy);
			break;
		case XK_g:
			break;
		case XK_h:
			gl.juanfeature = manage_state(gl.juanfeature);
			break;
		case XK_s:
			gl.start = 0;
			break;
		case XK_p:
			gl.p_screen = manage_state(gl.p_screen);
			break;
		case XK_Down:
			break;
		case XK_equal:
			gl.gameover = manage_gameover_state(gl.gameover);
			break;
		case XK_w:
			gl.feature_weapons = manage_feature_weapons_state
                                             (gl.feature_weapons);
                        if (gl.feature_weapons == 1) {
                              gl.weapon = rand()% 3 + 1;
                         }

			break;
		case XK_l:
			if (gl.boss_rush) {
				gl.boss_rush = 0;
				if (gl.level < 3)
					g->reset(++gl.level);

				else
					gl.win_screen = 1;
			}
			else {
				//Delete all asteroids
				Asteroid *a = g->ahead;
				while (a) {
					Asteroid *savea = a->next;
					deleteAsteroid(g, a);
					a = savea;
				}
				g->nasteroids = 0;

			}
			break;
		case XK_r:
			g->reset(gl.level);
			gl.gameover = 0;
			gl.boss_rush = 0;
	}
	return 0;
}

void deleteAsteroid(Game *g, Asteroid *node)
{
	//Remove a node from doubly-linked list
	//Must look at 4 special cases below.
	if (node->prev == NULL) {
		if (node->next == NULL) {
			//only 1 item in list.
			g->ahead = NULL;
		} else {
			//at beginning of list.
			node->next->prev = NULL;
			g->ahead = node->next;
		}
	} else {
		if (node->next == NULL) {
			//at end of list.
			node->prev->next = NULL;
		} else {
			//in middle of list.
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
	}
	delete node;
	node = NULL;
}

void buildAsteroidFragment(Asteroid *ta, Asteroid *a)
{
	//build ta from a
	ta->nverts = 8;
	ta->radius = a->radius / 2.0;
	Flt r2 = ta->radius / 2.0;
	Flt angle = 0.0f;
	Flt inc = (PI * 2.0) / (Flt)ta->nverts;
	for (int i=0; i<ta->nverts; i++) {
		ta->vert[i][0] = sin(angle) * (r2 + rnd() * ta->radius);
		ta->vert[i][1] = cos(angle) * (r2 + rnd() * ta->radius);
		angle += inc;
	}
	ta->pos[0] = a->pos[0] + rnd()*10.0-5.0;
	ta->pos[1] = a->pos[1] + rnd()*10.0-5.0;
	ta->pos[2] = 0.0f;
	ta->angle = 0.0;
	ta->rotate = a->rotate + (rnd() * 4.0 - 2.0);
	ta->color[0] = 0.8;
	ta->color[1] = 0.8;
	ta->color[2] = 0.7;
	ta->vel[0] = a->vel[0] + (rnd()*2.0-1.0);
	ta->vel[1] = a->vel[1] + (rnd()*2.0-1.0);
	//std::cout << "frag" << std::endl;
}

void physics(Game *g)
{
	if (gl.credits || gl.mouse_cursor)
	    return;
	if (gl.p_screen)
	    return;
	if (gl.help)
	    return;
    if (gl.gameover)
	    return;
	if(gl.win_screen)
	   return;
	Flt d0,d1,dist;
	//Update ship position
	g->ship.pos[0] += g->ship.vel[0];
	g->ship.pos[1] += g->ship.vel[1];
	//Check for collision with window edges

	if (g->ship.pos[0] < 8.0) {
		g->ship.pos[0] = 8;
		//g->ship.pos[0] += (float)gl.xres;
	}
	else if (g->ship.pos[0] > ((float)gl.xres - 8)) {
		g->ship.pos[0] = (float)gl.xres - 8;
		//g->ship.pos[0] -= (float)gl.xres;
	}
	else if (g->ship.pos[1] < 12.0) {
		g->ship.pos[1] = 12;
		//g->ship.pos[1] += (float)gl.yres;
	}
	else if (g->ship.pos[1] > ((float)gl.yres - 12)) {
		g->ship.pos[1] = (float)gl.yres - 12;

	}
	//
	//
	//Update bullet positions
	struct timespec bt;
	clock_gettime(CLOCK_REALTIME, &bt);
	int i = 0;
	while (i < g->nbullets) {
		Bullet *b = &g->barr[i];
		//How long has bullet been alive?
		double ts = timeDiff(&b->time, &bt);
		if (ts > 2.5) {
			//time to delete the bullet.
			memcpy(&g->barr[i], &g->barr[g->nbullets-1],
				sizeof(Bullet));
			g->nbullets--;
			//do not increment i.
			continue;
		}
		//move the bullet
		b->pos[0] += b->vel[0];
		b->pos[1] += b->vel[1];
		//Check for collision with window edges
		if (b->pos[0] < 0.0) {
			b->pos[0] += (float)gl.xres;
		}
		else if (b->pos[0] > (float)gl.xres) {
			b->pos[0] -= (float)gl.xres;
		}
		else if (b->pos[1] < 0.0) {
			b->pos[1] += (float)gl.yres;
		}
		else if (b->pos[1] > (float)gl.yres) {
			//b->pos[1] -= (float)gl.yres;

			memcpy(&g->barr[i], &g->barr[g->nbullets-1],
				sizeof(Bullet));
			g->nbullets--;

		}
		++i;
	}
	
	//Update asteroid positions
	Asteroid *a = g->ahead;
	while (a) {
		a->pos[0] += a->vel[0];
		a->pos[1] += a->vel[1];
		//Check for collision with window edges
		if (a->pos[0] < -100.0) {
			a->pos[0] += (float)gl.xres+200;
		}
		else if (a->pos[0] > (float)gl.xres+100) {
			a->pos[0] -= (float)gl.xres+200;
		}
		else if (a->pos[1] < -100.0) {
			a->pos[1] += (float)gl.yres+200;
		}
		else if (a->pos[1] > (float)gl.yres+100) {
			a->pos[1] -= (float)gl.yres+200;
		}
		a->angle += a->rotate;
		a = a->next;
	}
	//
	//Asteroid collision with bullets?
	//If collision detected:
	//     1. delete the bullet
	//     2. break the asteroid into pieces
	//        if asteroid small, delete it
	a = g->ahead;
	while (a) {
		//is there a bullet within its radius?
		int i=0;
		while (i < g->nbullets) {
			Bullet *b = &g->barr[i];
			d0 = b->pos[0] - a->pos[0];
			d1 = b->pos[1] - a->pos[1];
			dist = (d0*d0 + d1*d1);
			if (dist < (a->radius*a->radius)) {
				//std::cout << "asteroid hit." << std::endl;
				//this asteroid is hit.
				if (a->radius > MINIMUM_ASTEROID_SIZE) {
					//break it into pieces.
					Asteroid *ta = a;
					buildAsteroidFragment(ta, a);
					int r = rand()%10+5;
					for (int k=0; k<r; k++) {
						//get the next asteroid position in the array
						Asteroid *ta = new Asteroid;
						buildAsteroidFragment(ta, a);
						//add to front of asteroid linked list
						ta->next = g->ahead;
						if (g->ahead != NULL)
							g->ahead->prev = ta;
						g->ahead = ta;
						g->nasteroids++;
					}
				} else {
					a->color[0] = 1.0;
					a->color[1] = 0.1;
					a->color[2] = 0.1;
					//asteroid is too small to break up
					//delete the asteroid and bullet
					Asteroid *savea = a->next;
					deleteAsteroid(g, a);
					a = savea;
					g->nasteroids--;
					if (g->nasteroids == 0 && gl.level < 4){
					    //gl.level++;
						//Game g = new Game(gl.level);
						//replenishAsteroids(*g, g->nasteroids, gl.level);
					}
				}
				//delete the bullet...
				memcpy(&g->barr[i], &g->barr[g->nbullets-1], sizeof(Bullet));
				g->nbullets--;
				//if (g->nasteroids == 0) {

				//	gl.boss_rush = 1;
				//	//make boss
				//	init_boss(gl.xres, gl.yres, gl.t_boss);
				//	//start timer for behavior
				//	clock_gettime(CLOCK_REALTIME, &gl.boss_bulletTimer);

				//	g->nasteroids--;

				//}
				if (a == NULL)
					break;
			}
			i++;
		}
		if (a == NULL)
			break;
		a = a->next;
	}

	if (g->nasteroids == 0) {
		gl.boss_rush = 1;
		//make boss
		init_boss(gl.xres, gl.yres, gl.t_boss, gl.level);
		//start timer for behavior
		clock_gettime(CLOCK_REALTIME, &gl.boss_bulletTimer);

		g->nasteroids--;

		sleep(1);
	}

	
	if (gl.boss_rush) {
		//Update boss bullet positions
		boss_bulletPhysics();

		//Checks collision of bullet with boss
		int i = 0; 

		while (i < g->nbullets) {
			Bullet *b = &g->barr[i];
			//pass b into function
			if (boss_hit(b)) {
				memcpy(&g->barr[i], &g->barr[g->nbullets-1], sizeof(Bullet));
				g->nbullets--;

			}
			i++;
		}

		//Check ship collision with boss bullet

		if (player_hit(g->ship.pos)) {
			//std::cout << "Player ship hit" << std::endl;
			gl.gameover = 1;
		}
	}
	
	//damage to enemy
    if (gl.test_mode) {
        int j = 0;
        while (j < g->nbullets) {
            Bullet *b = &g->barr[j];
            if (enemy_hit(b)) {
                memcpy(&g->barr[j], &g->barr[g->nbullets-1], sizeof(Bullet));
                g->nbullets--;
            }
            j++;
        }
    }

	//COLLISION MODE
//	if(gl.juanfeature){
		a = g->ahead;
		while(a){
			Flt test1,test2;
			test1 = g->ship.pos[0] - a->pos[0];
			test2 = g->ship.pos[1] - a->pos[1];
			dist = (test1*test1 + test2*test2);
			if (dist < (a->radius*a->radius)){
				gl.gameover = 1;
			}
			//gl.gameover = collision_mode(a->radius, g->ship.pos[1],g->ship.pos[0], a->pos[1], a->pos[0]);
			if (a == NULL)
				break;
			a = a ->next;
		}
//	}
//      Weapon switch

    if ((gl.weapon ==1) || (gl.weapon == 3))  {
        if (gl.feature_weapons) {
            if (weapon_switch(g->ship.pos)) {
               gl.max_bullets = 11;
            }
        }
    }
    else if (gl.weapon == 2) {
        if (gl.feature_weapons) {
            if (weapon_switch2(g->ship.pos)) {
               gl.max_bullets = 0;
            }
        }
    }
  /* 
    else {
        if (gl.feature_weapons) {
           if (weapon_switch3(g->ship.pos)) {
               gl.max_bullets = 2;
        }
       }
    }
*/



	//---------------------------------------------------
	//check keys pressed now
	if (gl.keys[XK_Left]) {

		//g->ship.angle += 4.0;
		//if (g->ship.angle >= 360.0f)
		//	g->ship.angle -= 360.0f;
		if (g->ship.pos[0] > 15)
			g->ship.pos[0] -= 8;
	}
	if (gl.keys[XK_Right]) {
		//g->ship.angle -= 4.0;
		//if (g->ship.angle < 0.0f)
		//	g->ship.angle += 360.0f;
		if (g->ship.pos[0] < ((float)gl.xres - 15))
			g->ship.pos[0] += 8;
	}
	if (gl.keys[XK_Up]) {
		//apply thrust
		//convert ship angle to radians
		//Flt rad = ((g->ship.angle+90.0) / 360.0f) * PI * 2.0;
		//convert angle to a vector
		//Flt xdir = cos(rad);
		//Flt ydir = sin(rad);
		//g->ship.vel[0] += xdir*0.05f;
		//g->ship.vel[1] += ydir*0.05f;
		Flt speed = sqrt(g->ship.vel[0]*g->ship.vel[0]+
				g->ship.vel[1]*g->ship.vel[1]);
		if (speed > 10.0f) {
			speed = 10.0f;
			normalize2d(g->ship.vel);
			//g->ship.vel[0] *= speed;
			//g->ship.vel[1] *= speed;
		}
		

		if (g->ship.pos[1] < ((float)gl.yres - 12))
			g->ship.pos[1] += 8;
	}
	if (gl.keys[XK_Down]) {
		if (g->ship.pos[1] > 12)
			g->ship.pos[1] -= 8;
		//g->ship.vel[1] -= 0.08;
	}
	if (gl.keys[XK_space]) {
		//a little time between each bullet
		struct timespec bt;
		clock_gettime(CLOCK_REALTIME, &bt);

		double ts = timeDiff(&g->bulletTimer, &bt);
		if (ts > 0.3) {
			timeCopy(&g->bulletTimer, &bt);
			if (g->nbullets < gl.max_bullets) {

				//shoot a bullet...
				//Bullet *b = new Bullet;
				Bullet *b = &g->barr[g->nbullets];
				timeCopy(&b->time, &bt);
				b->pos[0] = g->ship.pos[0];
				b->pos[1] = g->ship.pos[1];
				b->vel[0] = g->ship.vel[0];
				b->vel[1] = g->ship.vel[1];
				//convert ship angle to radians
				Flt rad = ((g->ship.angle+90.0) / 360.0f) * PI * 2.0;
				//convert angle to a vector
				Flt xdir = cos(rad);
				Flt ydir = sin(rad);
				b->pos[0] += xdir*20.0f;
				b->pos[1] += ydir*20.0f;
				b->vel[0] += xdir*6.0f + rnd()*0.1;
				b->vel[1] += ydir*6.0f + rnd()*0.1;
				b->color[0] = 1.0f;
				b->color[1] = 1.0f;
				b->color[2] = 1.0f;
				g->nbullets++;
			}
		}
	}
	if (g->mouseThrustOn) {
		//should thrust be turned off
		struct timespec mtt;
		clock_gettime(CLOCK_REALTIME, &mtt);
		double tdif = timeDiff(&mtt, &g->mouseThrustTimer);
		//std::cout << "tdif: " << tdif << std::endl;
		if (tdif < -0.3)
			g->mouseThrustOn = false;
	}
}

void start_screen()
{
	Rect r3;
	r3.left = gl.xres /2.5;
	r3.bot = gl.yres/2.0;
	r3.center = 0;
	int xcent = gl.xres / 2;
	int ycent = gl.yres / 2;
	int w = 600;
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f( xcent-w, ycent-w);
        glVertex2f( xcent-w, ycent+w);
        glVertex2f( xcent+w, ycent+w);
        glVertex2f( xcent+w, ycent-w);
    glEnd();
    ggprint8b(&r3, 50, 0x2e281 , "Space Invaders");
    ggprint8b(&r3, 30, 0x2e281 , "Press 'S' to start");
	glDisable(GL_BLEND);
}

void render(Game *g)
{
	glClear(GL_COLOR_BUFFER_BIT);
	//Draw background
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.t_back.backText);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex2i(0, 0);
		glTexCoord2f(0, 0); glVertex2i(0, gl.yres);
		glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
		glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
	glEnd();
	
	if (gl.start) {
		start_screen();
		return;
	}
	Rect r;
	r.bot = gl.yres - 20;
	r.left = 10;
	r.center = 0;
	ggprint8b(&r, 16, 0x00ff0000, "3350 - SPACE-INVADERS");
	ggprint8b(&r, 16, 0x00ffff00, "n bullets: %i", g->nbullets);
	ggprint8b(&r, 16, 0x00ffff00, "n asteroids: %i", g->nasteroids);
	ggprint8b(&r, 16, 0x00fffff0, "Press \"M\" for menu");
	ggprint8b(&r, 16, 0x00fffff0, "Level: %i", gl.level);
	//-------------------------------------------------------------------------
	//Draw the ship
	glColor3fv(g->ship.color);
	glPushMatrix();
	glTranslatef(g->ship.pos[0], g->ship.pos[1], g->ship.pos[2]);
	//float angle = atan2(ship.dir[1], ship.dir[0]);
	glRotatef(g->ship.angle, 0.0f, 0.0f, 1.0f);
	glBegin(GL_TRIANGLES);
	//glVertex2f(-10.0f, -10.0f);
	//glVertex2f(  0.0f, 20.0f);
	//glVertex2f( 10.0f, -10.0f);
	glVertex2f(-12.0f, -10.0f);
	glVertex2f(  0.0f,  20.0f);
	glVertex2f(  0.0f,  -6.0f);
	glVertex2f(  0.0f,  -6.0f);
	glVertex2f(  0.0f,  20.0f);
	glVertex2f( 12.0f, -10.0f);
	glEnd();
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	glVertex2f(0.0f, 0.0f);
	glEnd();
	glPopMatrix();
	if(gl.p_screen == 0 || gl.mouse_cursor == 0){
	    if (gl.keys[XK_Up] || g->mouseThrustOn) {
		int i;
		//draw thrust
		Flt rad = ((g->ship.angle+90.0) / 360.0f) * PI * 2.0;
		//convert angle to a vector
		Flt xdir = cos(rad);
		Flt ydir = sin(rad);
		Flt xs,ys,xe,ye,r;
		glBegin(GL_LINES);
		for (i=0; i<16; i++) {
		    xs = -xdir * 11.0f + rnd() * 4.0 - 2.0;
		    ys = -ydir * 11.0f + rnd() * 4.0 - 2.0;
		    r = rnd()*40.0+40.0;
		    xe = -xdir * r + rnd() * 18.0 - 9.0;
		    ye = -ydir * r + rnd() * 18.0 - 9.0;
		    glColor3f(rnd()*.3+.7, rnd()*.3+.7, 0);
		    glVertex2f(g->ship.pos[0]+xs,g->ship.pos[1]+ys);
		    glVertex2f(g->ship.pos[0]+xe,g->ship.pos[1]+ye);
		}
		glEnd();
	    }
	}
	//-------------------------------------------------------------------------
	//Draw the asteroids
	{
		Asteroid *a = g->ahead;
		while (a) {
			//Log("draw asteroid...\n");
			glColor3fv(a->color);
			glPushMatrix();
			glTranslatef(a->pos[0], a->pos[1], a->pos[2]);
			glRotatef(a->angle, 0.0f, 0.0f, 1.0f);
			glBegin(GL_LINE_LOOP);
			//Log("%i verts\n",a->nverts);
			for (int j=0; j<a->nverts; j++) {
				glVertex2f(a->vert[j][0], a->vert[j][1]);
			}
			glEnd();
			//glBegin(GL_LINES);
			//	glVertex2f(0,   0);
			//	glVertex2f(a->radius, 0);
			//glEnd();
			glPopMatrix();
			glColor3f(1.0f, 0.0f, 0.0f);
			glBegin(GL_POINTS);
			glVertex2f(a->pos[0], a->pos[1]);
			glEnd();
			a = a->next;
		}
	}
	//-------------------------------------------------------------------------
	//Draw the bullets
	for (int i=0; i<g->nbullets; i++) {
		Bullet *b = &g->barr[i];
		//Log("draw bullet...\n");
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_POINTS);
		glVertex2f(b->pos[0],      b->pos[1]);
		glVertex2f(b->pos[0]-1.0f, b->pos[1]);
		glVertex2f(b->pos[0]+1.0f, b->pos[1]);
		glVertex2f(b->pos[0],      b->pos[1]-1.0f);
		glVertex2f(b->pos[0],      b->pos[1]+1.0f);
		glColor3f(0.8, 0.8, 0.8);
		glVertex2f(b->pos[0]-1.0f, b->pos[1]-1.0f);
		glVertex2f(b->pos[0]-1.0f, b->pos[1]+1.0f);
		glVertex2f(b->pos[0]+1.0f, b->pos[1]-1.0f);
		glVertex2f(b->pos[0]+1.0f, b->pos[1]+1.0f);
		glEnd();
	}
	
	if (gl.credits) {
		//show credits
		show_credits(gl.t, gl.xres, gl.yres);
		return;
	}
	if (gl.help){
		show_controls(gl.xres, gl.yres);
		return;
	}

	if (gl.mouse_cursor){
	    menu(gl.xres, gl.yres);
	    return;
	}
	if (gl.p_screen){
	    //show pause screen
	   	show_pause(gl.xres, gl.yres);
		return;
	}

	if(gl.test_mode) {
	    //show Jacob's feature mode
	    tester_mode(gl.xres, gl.yres);

		enemy_movement(gl.yres);
        if (!enemy_isAlive()) {
            gl.test_mode = 0;
        }
	    return;
	}

	//gameover screen
	if (gl.gameover) {
	    show_gameover_screen(gl.xres, gl.yres);
	    return;
	}

	if (g->nasteroids == 0) {

		gl.boss_rush = 1;
		init_boss(gl.xres, gl.yres, gl.t_boss, gl.level);
	}

	if (gl.boss_rush) {
		if (g->nasteroids != -1)
			start_boss_rush(gl.xres);
		spawn_boss();
		boss_movement(gl.xres, gl.level);
		boss_behavior(gl.boss_bulletTimer, gl.level);
		boss_drawBullets();
		if (!boss_isAlive()) {
			gl.boss_rush = 0;

			if (g->nasteroids == -1 && gl.level < 3)
				g->reset(++gl.level);
			else if (g->nasteroids == -1 && gl.level == 3)

				gl.win_screen = 1;
		}
		return;
	}

	if(gl.juanfeature) {
	    juanfeature(gl.xres,gl.yres);
	}

	if (gl.feature_weapons) {
		show_feature_weapons(gl.xres, gl.yres, gl.weapon, 
				     gl.max_bullets, g->ship.pos);
		return;
	}
	
	if(gl.win_screen){
		win_screen(gl.xres, gl.yres);
	}


}



