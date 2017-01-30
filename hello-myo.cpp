// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <stdio.h>

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>
#pragma comment(lib,"SDL2")
#pragma comment(lib,"SDL2_image")

#include<SDL.h>
#include<SDL_image.h>

#undef main

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Event event;

SDL_Texture *background;
SDL_Texture *sprite;
SDL_Texture *sprite2;
SDL_Texture *sprite3;

SDL_Texture *sprite_cur1;
SDL_Texture *sprite_cur2;

SDL_Texture* LoadIMG(char *path)
{
	return IMG_LoadTexture(renderer, path);
}

void Init()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("Byung Gon Song Proj2: SMI-MYO communication", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1599, 800, 0);
	renderer = SDL_CreateRenderer(window, 0, -1);
}

void ImageInit()
{
	background = LoadIMG("background.png");
	sprite = LoadIMG("target1.png");
	sprite2 = LoadIMG("target2.png");
	sprite3 = LoadIMG("target3.png");
}

void DrawImage(SDL_Texture *texture, double x, double y)
{
	SDL_Rect src, dst;
	SDL_QueryTexture(texture, NULL, NULL, &src.w, &src.h);
	src.x = 0;
	src.y = 0;
	dst.x = x;
	dst.y = y;
	dst.w = src.w;
	dst.h = src.h;
	SDL_RenderCopy(renderer, texture, &src, &dst);
}

void gotoxy(int x, int y)
{
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// Classes that inherit from myo::DeviceListener can be used to receive events from Myo devices. DeviceListener
// provides several virtual functions for handling different kinds of events. If you do not override an event, the
// default behavior is to do nothing.
class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: onArm(false), isUnlocked(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
	{
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		roll_w = 0;
		pitch_w = 0;
		yaw_w = 0;
		onArm = false;
		isUnlocked = false;
	}

	// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
	// as a unit quaternion.
	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
	{
		using std::atan2;
		using std::asin;
		using std::sqrt;
		using std::max;
		using std::min;

		// Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
		float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
			1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
		float pitch = asin(max(-1.0f, min(1.0f, 2.0f * (quat.w() * quat.y() - quat.z() * quat.x()))));
		float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
			1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));

		// Convert the floating point angles in radians to a scale from 0 to 18.
		roll_w = static_cast<int>((roll + (float)M_PI) / (M_PI * 2.0f) * 18);
		pitch_w = static_cast<int>((pitch + (float)M_PI / 2.0f) / M_PI * 18);
		yaw_w = static_cast<int>((yaw + (float)M_PI) / (M_PI * 2.0f) * 18);
	}

	// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
	// making a fist, or not making a fist anymore.
	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
	{
		currentPose = pose;

		if (pose != myo::Pose::unknown && pose != myo::Pose::rest) {
			// Tell the Myo to stay unlocked until told otherwise. We do that here so you can hold the poses without the
			// Myo becoming locked.
			myo->unlock(myo::Myo::unlockHold);

			// Notify the Myo that the pose has resulted in an action, in this case changing
			// the text on the screen. The Myo will vibrate.
			myo->notifyUserAction();
		}
		else {
			// Tell the Myo to stay unlocked only for a short period. This allows the Myo to stay unlocked while poses
			// are being performed, but lock after inactivity.
			myo->unlock(myo::Myo::unlockTimed);
		}
	}

	// onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
	// arm. This lets Myo know which arm it's on and which way it's facing.
	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
		myo::WarmupState warmupState)
	{
		onArm = true;
		whichArm = arm;
	}

	// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
	// when Myo is moved around on the arm.
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
	{
		onArm = false;
	}

	// onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
	void onUnlock(myo::Myo* myo, uint64_t timestamp)
	{
		isUnlocked = true;
	}

	// onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
	void onLock(myo::Myo* myo, uint64_t timestamp)
	{
		isUnlocked = false;
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	void print()
	{
		// Clear the current line
		std::cout << '\r';

		// Print out the orientation. Orientation data is always available, even if no arm is currently recognized.
		std::cout << '[' << std::string(roll_w, '*') << std::string(18 - roll_w, ' ') << ']'
			<< '[' << std::string(pitch_w, '*') << std::string(18 - pitch_w, ' ') << ']'
			<< '[' << std::string(yaw_w, '*') << std::string(18 - yaw_w, ' ') << ']';

		if (onArm) {
			// Print out the lock state, the currently recognized pose, and which arm Myo is being worn on.

			// Pose::toString() provides the human-readable name of a pose. We can also output a Pose directly to an
			// output stream (e.g. std::cout << currentPose;). In this case we want to get the pose name's length so
			// that we can fill the rest of the field with spaces below, so we obtain it as a string using toString().
			std::string poseString = currentPose.toString();

			std::cout << '[' << (isUnlocked ? "unlocked" : "locked  ") << ']'
				<< '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
				<< '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
		}
		else {
			// Print out a placeholder for the arm and pose when Myo doesn't currently know which arm it's on.
			std::cout << '[' << std::string(8, ' ') << ']' << "[?]" << '[' << std::string(14, ' ') << ']';
		}

		std::cout << std::flush;
	}

	// These values are set by onArmSync() and onArmUnsync() above.
	bool onArm;
	myo::Arm whichArm;

	// This is set by onUnlocked() and onLocked() above.
	bool isUnlocked;

	// These values are set by onOrientationData() and onPose() above.
	int roll_w, pitch_w, yaw_w;
	myo::Pose currentPose;


};


/*
Author: Byung Gon Song
Comment: enum for States and MODE
*/
// States for FSM -> Pose
enum States {
	REST,
	FIST,
	FINGERSSPREAD,
	WAVEIN,
	WAVEOUT,
	CUSTOM1
};
enum MODE {
	UP,
	DOWN,
	CENTER,
	INWARD,
	OUTWARD
};

/*
Author: Byung Gon Song
Comment: This can be modified for ANYTHING! Not just printing out A, B, C, D, E...etc
*/
// Current action is a print statement but in future, it can do anything.
// This is typical Moore FSM. It prints out based on the "state" rather than transition. If I want to give meaning to 
// tranistions, it has more choice.
// For now, I am printing a hand sign for military(my own).
void action(int mode, int state)
{
}

/*
Author: Byung Gon Song
Comment: Coded from scratch. Finite state machine and statechart
*/
void detection_mode(DataCollector *collector, myo::Hub *hub)
{
	int state = REST;
	int mode = CENTER;
	myo::Pose pose;
	const int PITCH_UP = 13;
	const int PITCH_DOWN = 2;
	const int YAW_INWARD = 3;
	const int YAW_OUTWARD = 10;


	POINT a;
	HWND hWnd;
	int width = 0;
	RECT window_size;
	a.x = 0;
	a.y = 0;

	// 0 not moving, 1 up, 2 right, 3, down, 4 left
	int direction = 0;
	int locked = 0;
	
	Init();
	ImageInit();
	int x_target1 = 30;
	int y_target1 = 30;
	int x_target2 = 500;
	int y_target2 = 500;

	sprite_cur1 = sprite2;
	sprite_cur2 = sprite2;
	//pose != myo::Pose::unknown && pose != myo::Pose::rest
	// Comment: Switch statement is better than if statement. Use switch statement with Hot encoding!
	while (1)
	{

		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				break;
		}
		if (GetAsyncKeyState(VK_RIGHT))
		{
			x_target1 += 3;
		}
		if (GetAsyncKeyState(VK_RIGHT))
		{
			x_target1 -= 3;
		}
		if (GetAsyncKeyState(VK_RIGHT))
		{
			y_target1 -= 3;
		}
		if (GetAsyncKeyState('A'))
		{
			y_target1 += 3;
		}

		SDL_RenderCopy(renderer, background, NULL, NULL);
		DrawImage(sprite_cur1, x_target1, y_target1);
		DrawImage(sprite_cur2, x_target2, y_target2);
		SDL_RenderPresent(renderer);
		SDL_Delay(15);


		// Get mouse pointer
		GetCursorPos(&a);
		hWnd = WindowFromPoint(a);
		ScreenToClient(hWnd, &a);
		GetWindowRect(hWnd, &window_size);
		width = (window_size.right - window_size.left);


		printf("X point = %d, Y point = %d\n\r", a.x, a.y);
		
		// Get Myo data
		hub->run(1000 / 20);

		if (locked > 0)
		{
			if (collector->currentPose == myo::Pose::fingersSpread)
			{
				locked = false;
			}
			else if (collector->currentPose == myo::Pose::waveIn)
			{
				if (locked == 1)
				{
					sprite_cur1 = sprite2;
				}
				else
				{
					sprite_cur2 = sprite2;
				}
			}
			else if (collector->currentPose == myo::Pose::waveIn)
			{
				if (locked == 1)
				{
					sprite_cur1 = sprite3;
				}
				else
				{
					sprite_cur2 = sprite3;
				}
			}
			// Transition between modes
			else if (collector->pitch_w > PITCH_UP)
			{
				direction = 1;
				if (locked == 1)
				{
					y_target1 += 3;
				}
				else
				{
					y_target2 += 3;
				}
			}
			else if (collector->pitch_w < PITCH_UP)
			{
				direction = 0;
			}
			else if (collector->pitch_w < PITCH_DOWN)
			{
				if (locked == 1)
				{
					y_target1 -= 3;
				}
				else
				{
					y_target2 -= 3;
				}
			}
			else if (collector->pitch_w > PITCH_DOWN)
			{
				direction = 0;
			}

			if (collector->yaw_w < YAW_INWARD)
			{
				if (locked == 1)
				{
					x_target1 += 3;
				}
				else
				{
					x_target2 += 3;
				}
				//printf("INWARD\n");
			}
			if ( collector->yaw_w > YAW_INWARD)
			{

				//printf("CENTER\n");
			}
			else if (collector->yaw_w > YAW_OUTWARD)
			{
				if (locked == 1)
				{
					x_target1 -= 3;
				}
				else
				{
					x_target2 -= 3;
				}
				//printf("OUTWARD\n");
			}
			else if (collector->yaw_w < YAW_OUTWARD)
			{
				//printf("CENTER\n");
			}
		}
		else
		{
			if ( collector->currentPose == myo::Pose::fist)
			{
				/*
				// if I am looking at the first object
				if (a.x && a.y)
				{
					locked = true;
				}
				// else if I am looking at the second object
				else if (a.x && a.y)
				{
					locked = true;
				}
				*/
			}
		}




		// Transition for state, REST

		if (state == REST && collector->currentPose == myo::Pose::waveIn)
		{
			state = WAVEIN;
			action(mode, state);
		}
		else if (state == REST && collector->currentPose == myo::Pose::fingersSpread)
		{
			state = FINGERSSPREAD;
			action(mode, state);
		}
		else if (state == REST && collector->currentPose == myo::Pose::waveOut)
		{
			state = WAVEOUT;
			action(mode, state);
		}

		// Transition for state, WAVEIN
		// No transition to rest is required because all transition basically goes to 
		else if (state == WAVEIN && collector->currentPose == myo::Pose::fist)
		{
			state = FIST;
			action(mode, state);
		}
		else if (state == WAVEIN && collector->currentPose == myo::Pose::fingersSpread)
		{
			state = FINGERSSPREAD;
			action(mode, state);
		}
		else if (state == WAVEIN && collector->currentPose == myo::Pose::waveOut)
		{
			state = WAVEOUT;
			action(mode, state);
		}


		else if (state == WAVEOUT && collector->currentPose == myo::Pose::fist)
		{
			state = FIST;
			action(mode, state);
		}
		else if (state == WAVEOUT && collector->currentPose == myo::Pose::fingersSpread)
		{
			state = FINGERSSPREAD;
			action(mode, state);
		}
		else if (state == WAVEOUT && collector->currentPose == myo::Pose::waveIn)
		{
			state = WAVEIN;
			action(mode, state);
		}


		else if (state == FIST && collector->currentPose == myo::Pose::waveIn)
		{
			state = WAVEIN;
			action(mode, state);
		}
		else if (state == FIST && collector->currentPose == myo::Pose::fingersSpread)
		{
			state = FINGERSSPREAD;
			action(mode, state);
		}
		else if (state == FIST && collector->currentPose == myo::Pose::waveOut)
		{
			state = WAVEOUT;
			action(mode, state);
		}



		else if (state == FINGERSSPREAD && collector->currentPose == myo::Pose::waveIn)
		{
			state = WAVEIN;
			action(mode, state);
		}
		else if (state == FINGERSSPREAD && collector->currentPose == myo::Pose::fist)
		{
			state = FIST;
			action(mode, state);
		}
		else if (state == FINGERSSPREAD && collector->currentPose == myo::Pose::waveOut)
		{
			state = WAVEOUT;
			action(mode, state);
		}
	}
}
int main(int argc, char** argv)
{
	// We catch any exceptions that might occur below -- see the catch statement for more details.


	try {
		// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
		// publishing your application. The Hub provides access to one or more Myos.
		myo::Hub hub("com.example.hello-myo");
		std::cout << "Attempting to find a Myo..." << std::endl;
		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
		// immediately.
		// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
		// if that fails, the function will return a null pointer.
		

		// We've found a Myo.
		std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
		DataCollector collector;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners.
		hub.addListener(&collector);

		/*
		Author: Byung Gon Song
		Comment: User interface. User choose what they want to do with Myo
		*/



		char choice[2];
		// Finally we enter our main loop.
		while (1) {
			// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
			// In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
			printf("Options \n"
				"a: Start to scan raw emg data for 5 sec and save it to csv \n"
				"b: Start to scan low pass filtered emg data for 5 sec and save it to csv \n"
				"c: Start to scan moving average filtered emg data for 5 sec and save it to csv\n"
				"d: Detection mode\n"
				"e: Print EMG data on screen\n"
				"f: Print Pose\n"
				"g: analyzing_mode\n"
				"h: Exit\n"
				"Please select option: ");
			scanf("%s", choice); // blocked

			// Value doesn't change until this statement executes
			switch (choice[0])
			{

			case 'd': // Detection mode
				detection_mode(&collector, &hub);
				break;
			default:
				printf("Invalid choice - please enter again\n");
				break;

			}
			printf("\n\n\n");

		}
		// If a standard exception occurred, we print out its message and exit.
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
}
