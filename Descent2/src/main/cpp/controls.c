//
// Created by devin on 4/21/16.
//

#include <jni.h>
#include <stdlib.h>

#include "game.h"
#include "key.h"
#include "timer.h"

#define NUM_BUTTONS 25
#define MAX_KEYS 2

#define ACCELERATE_BTN 0
#define REVERSE_BTN 1
#define SLIDE_LEFT_BTN 2
#define SLIDE_RIGHT_BTN 3
#define SLIDE_UP_BTN 4
#define SLIDE_DOWN_BTN 5
#define BANK_LEFT_BTN 6
#define BANK_RIGHT_BTN 7
#define FIRE_PRIMARY_BTN 8
#define FIRE_SECONDARY_BTN 9
#define TOGGLE_PRIMARY_BTN 10
#define TOGGLE_SECONDARY_BTN 11
#define FIRE_FLARE_BTN 12
#define ACCELERATE_SLIDE_LEFT_BTN 13
#define ACCELERATE_SLIDE_RIGHT_BTN 14
#define REVERSE_SLIDE_LEFT_BTN 15
#define REVERSE_SLIDE_RIGHT_BTN 16
#define MENU_BTN 17
#define MAP_BTN 18
#define REAR_VIEW_BTN 19
#define TOGGLE_COCKPIT_BTN 20
#define HEADLIGHT_BTN 21
#define ENERGY_TRANSFER_BTN 22
#define PLACE_MARKER 23
#define GUIDE_BOT_MENU 24

#define ACTION_DOWN 0
#define ACTION_UP 1
#define ACTION_MOVE 2
#define ACTION_POINTER_DOWN 5
#define ACTION_POINTER_UP 6

typedef struct GameButton {
	int x, y, w, h;
	unsigned char keys[MAX_KEYS];
	int nKeys;
	bool activateOnHover;
} GameButton;

static GameButton buttons[NUM_BUTTONS];
static int touchButtons[10];
static int trackingTouch;
static long accelerateLastTouched;

extern JavaVM *jvm;
extern jobject Activity;
extern ubyte gr_current_pal[256 * 3];
extern int touch_dx, touch_dy;

extern void key_handler(unsigned char scancode, bool down);

jfloat pt_conv(const char *method_name, jfloat arg) {
	JNIEnv *env;
	jclass clazz;
	jmethodID method;
	jfloat retval;

	(*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6);
	clazz = (*env)->FindClass(env, "tuchsen/descent2/DescentActivity");
	method = (*env)->GetMethodID(env, clazz, method_name, "(F)F");
	retval = (*env)->CallFloatMethod(env, Activity, method, arg);
	(*env)->DeleteLocalRef(env, clazz);
	return retval;
}

jfloat px_to_dp(jfloat px) {
	return pt_conv("pxToDp", px);
}

jfloat dp_to_px(jfloat dp) {
	return pt_conv("dpToPx", dp);
}

void init_buttons(jint w, jint h) {
	int i;
	float menuSpacing;

	w = (jint) px_to_dp(w);
	h = (jint) px_to_dp(h);
	menuSpacing = (w - 250) / 7;

	// Define buttons
	buttons[ACCELERATE_BTN] = (struct GameButton) {120, h - 185, 55, 55, {KEY_A}, 1, true};
	buttons[REVERSE_BTN] = (struct GameButton) {120, h - 75, 55, 55, {KEY_Z}, 1, true};
	buttons[SLIDE_LEFT_BTN] = (struct GameButton) {65, h - 130, 55, 55, {KEY_PAD1}, 1, true};
	buttons[SLIDE_RIGHT_BTN] = (struct GameButton) {175, h - 130, 55, 55, {KEY_PAD3}, 1, true};
	buttons[SLIDE_UP_BTN] = (struct GameButton) {25, h - 185, 35, 80, {KEY_PADMINUS}, 1, true};
	buttons[SLIDE_DOWN_BTN] = (struct GameButton) {25, h - 100, 35, 80, {KEY_PADPLUS}, 1, true};
	buttons[BANK_LEFT_BTN] = (struct GameButton) {65, h - 225, 80, 35, {KEY_Q}, 1, true};
	buttons[BANK_RIGHT_BTN] = (struct GameButton) {150, h - 225, 80, 35, {KEY_E}, 1, true};
	buttons[ACCELERATE_SLIDE_LEFT_BTN] = (struct GameButton) {65, h - 185, 55, 55,
															  {KEY_A, KEY_PAD1}, 2, true};
	buttons[ACCELERATE_SLIDE_RIGHT_BTN] = (struct GameButton) {175, h - 185, 55, 55,
															   {KEY_A, KEY_PAD3}, 2, true};
	buttons[REVERSE_SLIDE_LEFT_BTN] = (struct GameButton) {65, h - 75, 55, 55, {KEY_Z, KEY_PAD1}, 2,
														   true};
	buttons[REVERSE_SLIDE_RIGHT_BTN] = (struct GameButton) {175, h - 75, 55, 55, {KEY_Z, KEY_PAD3},
															2, true};
	buttons[FIRE_PRIMARY_BTN] = (struct GameButton) {w - 95, h - 170, 70, 70, {KEY_LCTRL}, 1,
													 false};
	buttons[FIRE_SECONDARY_BTN] = (struct GameButton) {w - 175, h - 90, 70, 70, {KEY_SPACEBAR}, 1,
													   false};
	buttons[FIRE_FLARE_BTN] = (struct GameButton) {w - 85, h - 80, 50, 50, {KEY_F}, 1, false};
	buttons[TOGGLE_PRIMARY_BTN] = (struct GameButton) {w - 95, h - 225, 70, 40, {KEY_1}, 1, false};
	buttons[TOGGLE_SECONDARY_BTN] = (struct GameButton) {w - 230, h - 90, 40, 70, {KEY_6}, 1,
														 false};
	buttons[MENU_BTN] = (struct GameButton) {25, 20, 25, 25, {KEY_ESC}, 1, false};
	buttons[TOGGLE_COCKPIT_BTN] = (struct GameButton) {50 + (int) menuSpacing, 20, 25, 25,
													   {KEY_F3}, 1, false};
	buttons[REAR_VIEW_BTN] = (struct GameButton) {75 + (int) (menuSpacing * 2), 20, 25, 25,
												  {KEY_R}, 1, false};
	buttons[HEADLIGHT_BTN] = (struct GameButton) {100 + (int) (menuSpacing * 3), 20, 25, 25,
												  {KEY_H}, 1, false};
	buttons[ENERGY_TRANSFER_BTN] = (struct GameButton) {125 + (int) (menuSpacing * 4), 20, 25, 25,
														{KEY_T}, 1, false};
	buttons[PLACE_MARKER] = (struct GameButton) {150 + (int) (menuSpacing * 5), 20, 25, 25,
												 {KEY_F4}, 1, false};
	buttons[GUIDE_BOT_MENU] = (struct GameButton) {175 + (int) (menuSpacing * 6), 20, 25, 25,
												   {KEY_LSHIFT, KEY_F4}, 2, false};
	buttons[MAP_BTN] = (struct GameButton) {200 + (int) (menuSpacing * 7), 20, 25, 25, {KEY_TAB}, 1,
											false};

	for (i = 0; i < NUM_BUTTONS; ++i) {
		buttons[i].x = (int) dp_to_px(buttons[i].x);
		buttons[i].y = (int) dp_to_px(buttons[i].y);
		buttons[i].w = (int) dp_to_px(buttons[i].w);
		buttons[i].h = (int) dp_to_px(buttons[i].h);
	}
	accelerateLastTouched = 0;
}

void draw_buttons() {
	int i;
	grs_canvas *save_canv;
	ubyte save_pal[3];

	if (Game_mode == GM_NORMAL && !In_screen) {
		save_canv = grd_curcanv;
		gr_set_current_canvas(NULL);
		memcpy(save_pal, gr_current_pal, sizeof(ubyte) * 3);
		memset(gr_current_pal, 255, sizeof(ubyte) * 3);
		gr_setcolor(0);
		Gr_scanline_darkening_level = 24;
		for (i = 0; i < NUM_BUTTONS; ++i) {
			if (i == 13) {
				Gr_scanline_darkening_level = 31;
			}
			if (i == 17) {
				Gr_scanline_darkening_level = 24;
			}
			gr_rect(buttons[i].x, buttons[i].y, buttons[i].x + buttons[i].w,
					buttons[i].y + buttons[i].h);
		}
		memcpy(gr_current_pal, save_pal, sizeof(ubyte) * 3);
		Gr_scanline_darkening_level = GR_FADE_LEVELS;
		gr_set_current_canvas(save_canv);
	}
}

bool point_in_button(jfloat x, jfloat y, const GameButton *button) {
	return x >= button->x && y >= button->y && x <= button->x + button->w && y <= button->y
																				  + button->h;
}

void handle_down(jint pointerId, jfloat x, jfloat y) {
	fix accelerateTime;

	for (int i = 0; i < NUM_BUTTONS; ++i) {
		if (point_in_button(x, y, &buttons[i])) {
			touchButtons[pointerId] = i;
			for (int j = 0; j < buttons[i].nKeys; ++j) {

				// Double-tapping accelerate triggers afterburner
				if (i == ACCELERATE_BTN) {
					accelerateTime = timer_get_fixed_seconds();
					if (accelerateTime - accelerateLastTouched < fl2f(0.5)) {
						key_handler(KEY_S, true);
					}
					accelerateLastTouched = timer_get_fixed_seconds();
				}

				key_handler(buttons[i].keys[j], true);
			}
		}
	}
}

void handle_up(jint pointerId) {
	int buttonNumber;
	if (trackingTouch == pointerId) {
		trackingTouch = -1;
		touch_dx = touch_dy = 0;
	}
	buttonNumber = touchButtons[pointerId];
	if (buttonNumber == -1) {
		return;
	}
	for (int i = 0; i < buttons[buttonNumber].nKeys; ++i) {
		if (buttonNumber == ACCELERATE_BTN) {
			key_handler(KEY_S, false);
		}
		key_handler(buttons[buttonNumber].keys[i], false);
	}
	touchButtons[pointerId] = -1;
}

void handle_move(jint pointerId, jfloat x, jfloat y, jfloat prevX, jfloat prevY) {
	int currentButtonIndex, previousButtonIndex;
	int i, j;
	int nSharedKeys = 0;
	unsigned char sharedKeys[MAX_KEYS];
	bool ignoreKey;

	int prev;

	currentButtonIndex = previousButtonIndex = -1;

	// Track the touch for ship orientation
	// (go outside the movement pad so the player doesn't accedentally spin the ship)
	if (trackingTouch == -1 && x > buttons[BANK_RIGHT_BTN].x + buttons[BANK_RIGHT_BTN].w + 50) {
		trackingTouch = pointerId;
	} else if (trackingTouch == -1) {
		touch_dx = touch_dy = 0;
	}
	if (trackingTouch == pointerId) {
		touch_dx = (int) (x - prevX);
		touch_dy = (int) (prevY - y);
	}

	// Get the button the touch is currently in
	for (i = 0; i < NUM_BUTTONS; ++i) {
		if (point_in_button(x, y, &buttons[i])) {
			currentButtonIndex = i;
			break;
		}
	}

	// Find the button the touch was previously in
	prev = touchButtons[pointerId];
	if (prev != -1) {
		previousButtonIndex = prev;
	}

	// Might press button if we hover over it
	if (currentButtonIndex > -1 && previousButtonIndex == -1 &&
		buttons[currentButtonIndex].activateOnHover) {
		touchButtons[pointerId] = -1;
		touchButtons[pointerId] = i;
		for (i = 0; i < buttons[currentButtonIndex].nKeys; ++i) {
			key_handler(buttons[currentButtonIndex].keys[i], true);
		}
	}

		// We changed buttons. Let the fun begin!
	else if (currentButtonIndex > -1 && previousButtonIndex > -1 &&
			 currentButtonIndex != previousButtonIndex) {
		touchButtons[pointerId] = -1;
		touchButtons[pointerId] = currentButtonIndex;

		// Find the keys the two buttons have in common
		for (i = 0; i < buttons[currentButtonIndex].nKeys; ++i) {
			for (j = 0; j < buttons[previousButtonIndex].nKeys; ++j) {
				if (buttons[currentButtonIndex].keys[i] == buttons[previousButtonIndex].keys[j]) {
					sharedKeys[nSharedKeys] = buttons[currentButtonIndex].keys[i];
					++nSharedKeys;
				}
			}
		}

		// Press all the keys that are in the new button that weren't in the old one
		for (i = 0; i < buttons[currentButtonIndex].nKeys; ++i) {
			ignoreKey = false;
			for (j = 0; j < nSharedKeys; ++j) {
				if (buttons[currentButtonIndex].keys[i] == sharedKeys[j]) {
					ignoreKey = true;
					break;
				}
			}
			if (!ignoreKey) {
				key_handler(buttons[currentButtonIndex].keys[i], true);
			}
		}

		// Release all the keys that were in the old button that aren't in the new one
		for (i = 0; i < buttons[previousButtonIndex].nKeys; ++i) {
			ignoreKey = false;
			for (j = 0; j < nSharedKeys; ++j) {
				if (buttons[previousButtonIndex].keys[i] == sharedKeys[j]) {
					ignoreKey = true;
					break;
				}
			}
			if (!ignoreKey) {
				key_handler(buttons[previousButtonIndex].keys[i], false);
			}
		}
	}
}

JNIEXPORT jboolean JNICALL
Java_tuchsen_descent2_DescentView_touchHandler(JNIEnv *env, jclass clazz, jint action, jint pointer_id, jfloat x,
											   jfloat y, jfloat prev_x, jfloat prev_y) {
	if (Game_mode == GM_NORMAL && !In_screen) {
		switch (action) {
			case ACTION_DOWN:
			case ACTION_POINTER_DOWN:
				handle_down(pointer_id, x, y);
				break;
			case ACTION_UP:
			case ACTION_POINTER_UP:
				handle_up(pointer_id);
				break;
			case ACTION_MOVE:
				handle_move(pointer_id, x, y, prev_x, prev_y);
				break;
			default:
				return false;
		}
		return true;
	} else {
		return false;
	}
}