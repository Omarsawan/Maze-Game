#include <GL/glut.h>
#include<algorithm>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <random>
#include <stdlib.h>
#include <ctime>
#include <time.h>
#include <windows.h>
#include <math.h> 
#include <mmsystem.h>
#include <dsound.h>

#define HALF_NOTE 1.059463094359 // HALF_NOTE ^ 12 = 2
#define PI 3.14159265358979

using namespace std;

const int cntLanes = 5;
const int laneHeight = 10;
const int cntCoins = 2 + 7;
const int borderHeight = 15;
const double baseSpeed = 5;

const int gameTime = 300;
int timer = gameTime;
double bridgeTime = 10000;

double speed = baseSpeed;

double borderx = 1000, bordery = 725;

int curScore = 0;
double minx = borderHeight, miny = borderHeight, maxx = borderx - borderHeight * 1.0, maxy = bordery - borderHeight * 1.0;
const double lanesSpacing = (maxy - miny - 50) / cntLanes;


double speX = minx + 12.5;
double speY = miny + 25;


double bridges[cntLanes];

int pos;

double powerx[cntCoins], powery[cntCoins], powerRadius[cntCoins];
bool powerAvailable[cntCoins];
int powerLane[cntCoins];
double colr[cntCoins], colb[cntCoins], colg[cntCoins];

bool gameOver = false;
int timeToFinish = -1;

//this is the method used to print text in OpenGL
//there are three parameters,
//the first two are the coordinates where the text is display,
//the third coordinate is the string containing the text to display
void print(int x, int y, string str)
{
	int len, i;

	//set the position of the text in the window using the x and y coordinates
	glRasterPos2f(x, y);

	//get the length of the string to display
	len = (int)str.size();

	//loop to display character by character
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str[i]);
	}
}
void play_background_music() {
	PlaySound(TEXT("ph.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
}
double angle = 0;
void Anim()
{
	angle++;

	glutPostRedisplay();
}
void disableSpeed(int val)//timer animation function, allows the user to pass an integer valu to the timer function.
{
	speed = baseSpeed;
	glutPostRedisplay();
}

//class for sound
class SoundEffect
{
public:
	SoundEffect()
	{
		m_data = NULL;
	}
	SoundEffect(const int noteInfo[], const int arraySize)
	{
		// Initialize the sound format we will request from sound card
		m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;     // Uncompressed sound format
		m_waveFormat.nChannels = 1;                    // 1 = Mono, 2 = Stereo
		m_waveFormat.wBitsPerSample = 8;               // Bits per sample per channel
		m_waveFormat.nSamplesPerSec = 11025;           // Sample Per Second
		m_waveFormat.nBlockAlign = m_waveFormat.nChannels * m_waveFormat.wBitsPerSample / 8;
		m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
		m_waveFormat.cbSize = 0;

		int dataLength = 0, moment = (m_waveFormat.nSamplesPerSec / 75);
		double period = 2.0 * PI / (double)m_waveFormat.nSamplesPerSec;

		// Calculate how long we need the sound buffer to be
		for (int i = 1; i < arraySize; i += 2)
			dataLength += (noteInfo[i] != 0) ? noteInfo[i] * moment : moment;

		// Allocate the array
		m_data = new char[m_bufferSize = dataLength];

		int placeInData = 0;

		// Make the sound buffer
		for (int i = 0; i < arraySize; i += 2)
		{
			int relativePlaceInData = placeInData;

			while ((relativePlaceInData - placeInData) < ((noteInfo[i + 1] != 0) ? noteInfo[i + 1] * moment : moment))
			{
				// Generate the sound wave (as a sinusoid)
				// - x will have a range of -1 to +1
				double x = sin((relativePlaceInData - placeInData) * 55 * pow(HALF_NOTE, noteInfo[i]) * period);

				// Scale x to a range of 0-255 (signed char) for 8 bit sound reproduction
				m_data[relativePlaceInData] = (char)(127 * x + 128);

				relativePlaceInData++;
			}

			placeInData = relativePlaceInData;
		}
	}
	SoundEffect(SoundEffect& otherInstance)
	{
		m_bufferSize = otherInstance.m_bufferSize;
		m_waveFormat = otherInstance.m_waveFormat;

		if (m_bufferSize > 0)
		{
			m_data = new char[m_bufferSize];

			for (int i = 0; i < otherInstance.m_bufferSize; i++)
				m_data[i] = otherInstance.m_data[i];
		}
	}
	~SoundEffect()
	{
		if (m_bufferSize > 0)
			delete[] m_data;
	}

	SoundEffect& operator=(SoundEffect& otherInstance)
	{
		if (m_bufferSize > 0)
			delete[] m_data;

		m_bufferSize = otherInstance.m_bufferSize;
		m_waveFormat = otherInstance.m_waveFormat;

		if (m_bufferSize > 0)
		{
			m_data = new char[m_bufferSize];

			for (int i = 0; i < otherInstance.m_bufferSize; i++)
				m_data[i] = otherInstance.m_data[i];
		}

		return *this;
	}

	void Play()
	{
		// Create our "Sound is Done" event
		m_done = CreateEvent(0, FALSE, FALSE, 0);

		// Open the audio device
		if (waveOutOpen(&m_waveOut, 0, &m_waveFormat, (DWORD)m_done, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
		{
			cout << "Sound card cannot be opened." << endl;
			return;
		}

		// Create the wave header for our sound buffer
		m_waveHeader.lpData = m_data;
		m_waveHeader.dwBufferLength = m_bufferSize;
		m_waveHeader.dwFlags = 0;
		m_waveHeader.dwLoops = 0;

		// Prepare the header for playback on sound card
		if (waveOutPrepareHeader(m_waveOut, &m_waveHeader, sizeof(m_waveHeader)) != MMSYSERR_NOERROR)
		{
			cout << "Error preparing Header!" << endl;
			return;
		}

		// Play the sound!
		ResetEvent(m_done); // Reset our Event so it is non-signaled, it will be signaled again with buffer finished

		if (waveOutWrite(m_waveOut, &m_waveHeader, sizeof(m_waveHeader)) != MMSYSERR_NOERROR)
		{
			cout << "Error writing to sound card!" << endl;
			return;
		}

		// Wait until sound finishes playing
		if (WaitForSingleObject(m_done, INFINITE) != WAIT_OBJECT_0)
		{
			cout << "Error waiting for sound to finish" << endl;
			return;
		}

		// Unprepare our wav header
		if (waveOutUnprepareHeader(m_waveOut, &m_waveHeader, sizeof(m_waveHeader)) != MMSYSERR_NOERROR)
		{
			cout << "Error unpreparing header!" << endl;
			return;
		}

		// Close the wav device
		if (waveOutClose(m_waveOut) != MMSYSERR_NOERROR)
		{
			cout << "Sound card cannot be closed!" << endl;
			return;
		}

		// Release our event handle
		CloseHandle(m_done);
	}

private:
	HWAVEOUT m_waveOut; // Handle to sound card output
	WAVEFORMATEX m_waveFormat; // The sound format
	WAVEHDR m_waveHeader; // WAVE header for our sound data
	HANDLE m_done; // Event Handle that tells us the sound has finished being played.
				   // This is a very efficient way to put the program to sleep
				   // while the sound card is processing the sound buffer

	char* m_data; // Sound data buffer
	int m_bufferSize; // Size of sound data buffer
};
const int gameFinishedarr[] = { 37 ,4 ,41 ,4 ,44 ,4 ,49 ,4 ,53 ,4 ,56, 4 ,61 ,4 ,65 ,4 ,68 ,4 ,73, 4 };
SoundEffect gameFinish(gameFinishedarr, 20);

const int bridgeOpenarr[] = { 56 ,8 ,61 ,8 ,65 ,8 ,61 ,8 ,63 ,8 ,68 ,8 };
SoundEffect bridgeOpen(bridgeOpenarr, 12);

const int speedPowerUparr[] = { 37 ,4 ,44 ,4 ,49 ,4 ,38 ,4 ,45 ,4 ,50 ,4 ,39 ,4 ,46 ,4 ,51 ,4 };
SoundEffect speedPowerUp(speedPowerUparr, 18);

const int coinarr[] = { 66 ,1 };
SoundEffect coin(coinarr, 2);

const int gameLostarr[] = { 25 ,3 ,13 ,4, 1, 6 };
SoundEffect gameLost(gameLostarr, 6);

void Display() {
	if (gameOver || timeToFinish != -1) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		if (gameOver) {
			glPushAttrib(GL_CURRENT_BIT);
			glColor3f(1, 1, 1);

			print(-50 + (minx + maxx) / 2, (miny + maxy) / 2, "Game Over !!");
			print(-150 + (minx + maxx) / 2, -50 + (miny + maxy) / 2, "You did not reach the goal in time");

			glPopAttrib();
		}
		else {
			glPushAttrib(GL_CURRENT_BIT);
			glColor3f(1, 1, 1);
			print(-50 + (minx + maxx) / 2, 100 + (miny + maxy) / 2, "You Won !!!!");
			print(-75 + (minx + maxx) / 2, 50 + (miny + maxy) / 2, "Congratulations !!");

			string cur = "Your score:";
			cur.append(to_string(curScore));
			print(-40 + (minx + maxx) / 2, (miny + maxy) / 2, cur);

			string curT = "You finished the game in ";
			curT.append(to_string(timeToFinish));
			curT.append(" seconds");
			print(-150 + (minx + maxx) / 2, -50 + (miny + maxy) / 2, curT);

			glPopAttrib();
		}


		glFlush();
		return;
	}
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//the border :
	glBegin(GL_LINE_LOOP); // GL_LINE_STRIP, GL_LINE_LOOP
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, bordery, 0.0f);
	glVertex3f(minx, bordery, 0.0f);
	glVertex3f(minx, 0.0f, 0.0f);
	glEnd();

	glBegin(GL_LINE_LOOP); // GL_LINE_STRIP, GL_LINE_LOOP
	glVertex3f(maxx, 0.0f, 0.0f);
	glVertex3f(maxx, bordery, 0.0f);
	glVertex3f(borderx, bordery, 0.0f);
	glVertex3f(borderx, 0.0f, 0.0f);
	glEnd();

	glBegin(GL_LINE_LOOP); // GL_LINE_STRIP, GL_LINE_LOOP
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, miny, 0.0f);
	glVertex3f(borderx, miny, 0.0f);
	glVertex3f(borderx, 0.0f, 0.0f);
	glEnd();

	glBegin(GL_LINE_LOOP); // GL_LINE_STRIP, GL_LINE_LOOP
	glVertex3f(0.0f, bordery, 0.0f);
	glVertex3f(borderx, bordery, 0.0f);
	glVertex3f(borderx, maxy, 0.0f);
	glVertex3f(0.0f, maxy, 0.0f);
	glEnd();


	glBegin(GL_QUADS); // GL_QUAD_STRIP
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, bordery, 0.0f);
	glVertex3f(minx, bordery, 0.0f);
	glVertex3f(minx, 0.0f, 0.0f);

	glVertex3f(maxx, 0.0f, 0.0f);
	glVertex3f(maxx, bordery, 0.0f);
	glVertex3f(borderx, bordery, 0.0f);
	glVertex3f(borderx, 0.0f, 0.0f);

	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, miny, 0.0f);
	glVertex3f(borderx, miny, 0.0f);
	glVertex3f(borderx, 0.0f, 0.0f);

	glVertex3f(0.0f, bordery, 0.0f);
	glVertex3f(borderx, bordery, 0.0f);
	glVertex3f(borderx, maxy, 0.0f);
	glVertex3f(0.0f, maxy, 0.0f);

	glEnd();


	//lane borders:
	int cnt = 0;
	for (double yBorder = miny - laneHeight + lanesSpacing; cnt < cntLanes; cnt++, yBorder += lanesSpacing) {
		double mid = bridges[cnt];
		glBegin(GL_QUADS); // GL_QUAD_STRIP
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(minx, yBorder, 0.0f);
		glVertex3f(minx, yBorder + laneHeight, 0.0f);
		glVertex3f(mid - 25, yBorder + laneHeight, 0.0f);
		glVertex3f(mid - 25, yBorder, 0.0f);
		glEnd();

		glBegin(GL_LINES); // GL_LINE_STRIP, GL_LINE_LOOP
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex3f(minx, yBorder + laneHeight * 1.0 / 2, 0.0f);
		glVertex3f(mid - 25, yBorder + laneHeight * 1.0 / 2, 0.0f);
		glEnd();


		glBegin(GL_QUADS); // GL_QUAD_STRIP
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(mid + 25, yBorder, 0.0f);
		glVertex3f(mid + 25, yBorder + laneHeight, 0.0f);
		glVertex3f(maxx, yBorder + laneHeight, 0.0f);
		glVertex3f(maxx, yBorder, 0.0f);
		glEnd();

		glBegin(GL_LINES); // GL_LINE_STRIP, GL_LINE_LOOP
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex3f(mid + 25, yBorder + laneHeight * 1.0 / 2, 0.0f);
		glVertex3f(maxx, yBorder + laneHeight * 1.0 / 2, 0.0f);
		glEnd();
	}
	//goal
	print(pos, maxy - 20, "END");

	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(pos - 5, maxy - 30, 0.0f);
	glVertex3f(pos - 5, maxy, 0.0f);

	glVertex3f(pos - 5, maxy, 0.0f);
	glVertex3f(pos + 55, maxy, 0.0f);

	glVertex3f(pos + 55, maxy, 0.0f);
	glVertex3f(pos + 55, maxy - 30, 0.0f);

	glVertex3f(pos + 55, maxy - 30, 0.0f);
	glVertex3f(pos - 5, maxy - 30, 0.0f);

	glEnd();

	glBegin(GL_TRIANGLES);
	glVertex3f(pos - 20, maxy - 25, 0.0f);
	glVertex3f(pos - 10, maxy - 15, 0.0f);
	glVertex3f(pos - 20, maxy - 5, 0.0f);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(pos - 20, maxy - 15, 0.0f);
	glVertex3f(pos - 40, maxy - 15, 0.0f);
	glEnd();


	glBegin(GL_TRIANGLES);
	glVertex3f(pos + 70, maxy - 25, 0.0f);
	glVertex3f(pos + 60, maxy - 15, 0.0f);
	glVertex3f(pos + 70, maxy - 5, 0.0f);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(pos + 70, maxy - 15, 0.0f);
	glVertex3f(pos + 90, maxy - 15, 0.0f);
	glEnd();

	//power ups and coins

	for (int i = 0; i < cntCoins; i++) {
		if (powerAvailable[i]) {
			glPushMatrix();

			glTranslated(powerx[i], powery[i], 0);
			glRotated(angle, 0, 0, 1); // rotation
			glTranslated(-powerx[i], -powery[i], 0);
			for (double x = powerx[i] - powerRadius[i]; x <= powerx[i] + powerRadius[i]; x += 0.1) {
				double dx = x - powerx[i];
				double C = dx * dx + powery[i] * powery[i] - powerRadius[i] * powerRadius[i];
				double B = -2 * powery[i];

				double y1 = (-B + sqrt(B * B - 4 * C)) / 2;
				double y2 = (-B - sqrt(B * B - 4 * C)) / 2;

				glColor3f(colr[i], colg[i], colb[i]);
				glBegin(GL_LINES); // GL_LINE_STRIP, GL_LINE_LOOP
				glVertex3f(x, y1, 0.0f);
				glVertex3f(x, y2, 0.0f);
				glEnd();
			}



			glBegin(GL_TRIANGLES); // GL_LINE_STRIP, GL_LINE_LOOP
			glColor3f(colr[i], colg[i], colb[i]);
			glVertex3f(powerx[i], powery[i] + 5, 0.0f);
			glVertex3f(powerx[i] - 5, powery[i] - 5, 0.0f);
			glVertex3f(powerx[i] + 5, powery[i] - 5, 0.0f);
			glEnd();

			glBegin(GL_LINE_LOOP); // GL_LINE_STRIP, GL_LINE_LOOP
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex3f(powerx[i], powery[i] + 5, 0.0f);
			glVertex3f(powerx[i] - 5, powery[i] - 5, 0.0f);
			glVertex3f(powerx[i] + 5, powery[i] - 5, 0.0f);
			glEnd();

			glPopMatrix();
		}
	}

	//player:

	//body
	glColor3f(0, 0, 1);
	glPointSize(25);				//change the point size to be 25
	glBegin(GL_POINTS);
	glVertex3d(speX, speY, 0);
	glEnd();

	glBegin(GL_LINES); // GL_LINE_STRIP, GL_LINE_LOOP
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(speX - 6, speY + 12.5, 0.0f);
	glVertex3f(speX - 6, speY - 12.5, 0.0f);
	glEnd();

	glBegin(GL_LINES); // GL_LINE_STRIP, GL_LINE_LOOP
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(speX + 6, speY + 12.5, 0.0f);
	glVertex3f(speX + 6, speY - 12.5, 0.0f);
	glEnd();

	//head
	for (double x = speX - 5; x <= speX + 5; x += 0.1) {
		double dx = x - speX;
		double C = dx * dx + (speY + 17.5) * (speY + 17.5) - 5.0 * 5;
		double B = -2 * (speY + 17.5);

		double y1 = (-B + sqrt(B * B - 4 * C)) / 2;
		double y2 = (-B - sqrt(B * B - 4 * C)) / 2;

		glColor3f(0.0f, 0.0f, 0.0f);
		glBegin(GL_LINES); // GL_LINE_STRIP, GL_LINE_LOOP
		glVertex3f(x, y1, 0.0f);
		glVertex3f(x, y2, 0.0f);
		glEnd();
	}


	//legs 

	glBegin(GL_LINE_LOOP);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(speX - 10, speY - 25, 0.0f);
	glVertex3f(speX - 10, speY - 12.5, 0.0f);
	glVertex3f(speX - 5, speY - 12.5, 0.0f);
	glVertex3f(speX - 5, speY - 25, 0.0f);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(speX + 5, speY - 25, 0.0f);
	glVertex3f(speX + 5, speY - 12.5, 0.0f);
	glVertex3f(speX + 10, speY - 12.5, 0.0f);
	glVertex3f(speX + 10, speY - 25, 0.0f);
	glEnd();

	glBegin(GL_POLYGON);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(speX - 10, speY - 25, 0.0f);
	glVertex3f(speX - 10, speY - 12.5, 0.0f);
	glVertex3f(speX - 5, speY - 12.5, 0.0f);
	glVertex3f(speX - 5, speY - 25, 0.0f);
	glEnd();

	glBegin(GL_POLYGON);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(speX + 5, speY - 25, 0.0f);
	glVertex3f(speX + 5, speY - 12.5, 0.0f);
	glVertex3f(speX + 10, speY - 12.5, 0.0f);
	glVertex3f(speX + 10, speY - 25, 0.0f);
	glEnd();


	//score
	string cur = "score : ";
	cur.append(to_string(curScore));
	print((minx + maxx) / 2 + 200, bordery + 5, cur);

	string curT = "timer : ";
	curT.append(to_string(timer));
	print((minx + maxx) / 2 - 100, bordery + 5, curT);

	glFlush();
}
double EPS = 1e-9;
//check if a point is contained in a rectangle
bool contains(double ll1x, double ll1y, double ur1x, double ur1y, double px, double py) {
	return px <= ur1x + EPS && px + EPS >= ll1x && py <= ur1y + EPS && py + EPS >= ll1y;
}
//check if 2 rectangles intersect
bool intersect(double ll1x, double ll1y, double ur1x, double ur1y,
	double ll2x, double ll2y, double ur2x, double ur2y) {
	double llx = max(ll1x, ll2x), lly = max(ll1y, ll2y);
	double urx = min(ur1x, ur2x), ury = min(ur1y, ur2y);
	return (abs(urx - llx) > EPS && abs(ury - lly) > EPS && contains(ll1x, ll1y, ur1x, ur1y, llx, lly) &&
		contains(ll1x, ll1y, ur1x, ur1y, urx, ury) && contains(ll2x, ll2y, ur2x, ur2y, llx, lly)
		&& contains(ll2x, ll2y, ur2x, ur2y, urx, ury));
}

bool canMove() {
	bool collision = false;
	if (speX - 12.5 < minx) {
		collision = true;
	}
	if (speX + 12.5 > maxx) {
		collision = true;
	}
	if (speY - 25 < miny) {
		collision = true;
	}
	if (speY + 22.5 > maxy) {
		collision = true;
	}
	int cnt = 0;
	for (double yBorder = miny - laneHeight + lanesSpacing; cnt < cntLanes && !collision; cnt++, yBorder += lanesSpacing) {
		double mid = bridges[cnt];
		collision |= (intersect(speX - 12.5, speY - 25, speX + 12.5, speY + 22.5
			, minx, yBorder, mid - 25, yBorder + laneHeight));

		collision |= (intersect(speX - 12.5, speY - 25, speX + 12.5, speY + 22.5
			, mid + 25, yBorder, maxx, yBorder + laneHeight));
	}
	return !collision;
}
void spe(int k, int x, int y)// keyboard special key function takes 3 parameters
							// int k: is the special key pressed such as the keyboard arrows the f1,2,3 and so on
{
	if (k == GLUT_KEY_RIGHT)//if the right arrow is pressed, then the object will be translated in the x axis by 10. (moving right)
		speX += speed;
	if (k == GLUT_KEY_LEFT)//if the left arrow is pressed, then the object will be translated in the x axis by -10. (moving left)
		speX -= speed;
	if (k == GLUT_KEY_UP)//if the up arrow is pressed, then the object will be translated in the y axis by 10. (moving upwords)
		speY += speed;
	if (k == GLUT_KEY_DOWN)//if the down arrow is pressed, then the object will be translated in the y axis by -10. (moving downwords)
		speY -= speed;


	if (!canMove()) {
		while (!canMove()) {
			if (k == GLUT_KEY_RIGHT)//if the right arrow is pressed, then the object will be translated in the x axis by 10. (moving right)
				speX -= 1;
			if (k == GLUT_KEY_LEFT)//if the left arrow is pressed, then the object will be translated in the x axis by -10. (moving left)
				speX += 1;
			if (k == GLUT_KEY_UP)//if the up arrow is pressed, then the object will be translated in the y axis by 10. (moving upwords)
				speY -= 1;
			if (k == GLUT_KEY_DOWN)//if the down arrow is pressed, then the object will be translated in the y axis by -10. (moving downwords)
				speY += 1;
		}
		Beep(623, 7);
	}
	else {
		if (intersect(speX - 12.5, speY - 25, speX + 12.5, speY + 22.5,
			pos * 1.0 - 5, maxy - 30, pos * 1.0 + 55, maxy)) {
			timeToFinish = gameTime - timer;
			gameFinish.Play();
		}
		if (powerAvailable[0] && intersect(speX - 12.5, speY - 25, speX + 12.5, speY + 22.5,
			powerx[0] - powerRadius[0], powery[0] - powerRadius[0], powerx[0] + powerRadius[0], powery[0] + powerRadius[0])) {

			powerAvailable[0] = false;
			bridges[powerLane[0]] = speX;
			//sound

			bridgeOpen.Play();
		}
		if (powerAvailable[1] && intersect(speX - 12.5, speY - 25, speX + 12.5, speY + 22.5,
			powerx[1] - powerRadius[1], powery[1] - powerRadius[1], powerx[1] + powerRadius[1], powery[1] + powerRadius[1])) {

			powerAvailable[1] = false;
			speed += 5;
			glutTimerFunc(5000, disableSpeed, 0);

			//sound

			speedPowerUp.Play();
		}
		for (int i = 2; i < cntCoins; i++) {
			if (powerAvailable[i] && intersect(speX - 12.5, speY - 25, speX + 12.5, speY + 22.5,
				powerx[i] - powerRadius[i], powery[i] - powerRadius[i], powerx[i] + powerRadius[i], powery[i] + powerRadius[i])) {

				powerAvailable[i] = false;
				curScore += 5;

				//sound

				coin.Play();
			}
		}
	}

	//if (k == GLUT_KEY_F1)//if the F1 key is pressed, then the object color will be changed
		//speC = 1;
	glutPostRedisplay();//redisplay to update the screen with the changes
}

void timerFunc(int val)//timer animation function, allows the user to pass an integer valu to the timer function.
{

	for (int i = 0; i < cntLanes; i++) {
		int range = maxx - minx - 50;
		bridges[i] = (rand() % range) + minx + 25;
	}
	
	while (!canMove()) {
		speY -= speed;
	}
	for (int i = 0; i < 2; i++) {
		powerAvailable[i] = true;
	}
	for (int i = 1; i >= 0; i--) {
		int range = maxx - minx - 50;
		powerx[i] = (rand() % range) + minx + 25;
		powerLane[i] = (rand() % cntLanes);
		powery[i] = ((double)powerLane[i] * lanesSpacing) + (1.0 * lanesSpacing / 2) + miny;

		bool collide = false;
		for (int j = i + 1; j < cntCoins; j++) {
			if (!powerAvailable[j])continue;
			collide |= intersect(powerx[i] - powerRadius[i], powery[i] - powerRadius[i], powerx[i] + powerRadius[i], powery[i] + powerRadius[i],
				powerx[j] - powerRadius[j], powery[j] - powerRadius[j], powerx[j] + powerRadius[j], powery[j] + powerRadius[j]);
		}

		while (collide) {
			powerx[i] = (rand() % range) + minx + 25;
			powerLane[i] = (rand() % cntLanes);
			powery[i] = ((double)powerLane[i] * lanesSpacing) + (1.0 * lanesSpacing / 2) + miny;

			collide = false;
			for (int j = i + 1; j < cntCoins; j++) {
				if (!powerAvailable[j])continue;
				collide |= intersect(powerx[i] - powerRadius[i], powery[i] - powerRadius[i], powerx[i] + powerRadius[i], powery[i] + powerRadius[i],
					powerx[j] - powerRadius[j], powery[j] - powerRadius[j], powerx[j] + powerRadius[j], powery[j] + powerRadius[j]);
			}
		}
	}

	glutPostRedisplay();						// redraw 	
	if (!gameOver && timeToFinish == -1) {
		glutTimerFunc(bridgeTime, timerFunc, 0);
	}
}
void time2(int val)//timer animation function, allows the user to pass an integer valu to the timer function.
{
	timer--;
	if (timer == 0) {

		gameLost.Play();
		gameOver = true;
	}
	glutPostRedisplay();						// redraw 	
	if (!gameOver && timeToFinish == -1) {
		glutTimerFunc(1000, time2, 0);
	}
}

void main(int argc, char** argr) {
	srand(time(0));
	glutInit(&argc, argr);

	glutInitWindowSize(borderx, bordery + 25);
	glutInitWindowPosition(50, 0);

	glutCreateWindow("Game");
	glutDisplayFunc(Display);
	glutSpecialFunc(spe);			//call the keyboard special keys function
	glutTimerFunc(bridgeTime, timerFunc, 0);		//call the timer function
	glutTimerFunc(1000, time2, 0);
	glutIdleFunc(Anim);
	glPointSize(25);				//change the point size to be 25
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gluOrtho2D(0.0, borderx, 0.0, bordery + 25);

	for (int i = 0; i < cntCoins; i++) {
		if (i >= 2) {
			colr[i] = 212.0 / 255;
			colg[i] = 175.0 / 255;
			colb[i] = 55.0 / 255;
		}
		powerAvailable[i] = true;
		powerRadius[i] = 15;
		int range = maxx - minx - 50;
		powerx[i] = (rand() % range) + minx + 25;
		powerLane[i] = (rand() % cntLanes);
		powery[i] = ((double)powerLane[i] * lanesSpacing) + (1.0 * lanesSpacing / 2) + miny;

		bool collide = false;
		for (int j = i - 1; j >= 0; j--) {
			collide |= intersect(powerx[i] - powerRadius[i], powery[i] - powerRadius[i], powerx[i] + powerRadius[i], powery[i] + powerRadius[i],
				powerx[j] - powerRadius[j], powery[j] - powerRadius[j], powerx[j] + powerRadius[j], powery[j] + powerRadius[j]);
		}

		while (collide) {
			powerx[i] = (rand() % range) + minx + 25;
			powerLane[i] = (rand() % cntLanes);
			powery[i] = ((double)powerLane[i] * lanesSpacing) + (1.0 * lanesSpacing / 2) + miny;

			collide = false;
			for (int j = i - 1; j >= 0; j--) {
				collide |= intersect(powerx[i] - powerRadius[i], powery[i] - powerRadius[i], powerx[i] + powerRadius[i], powery[i] + powerRadius[i],
					powerx[j] - powerRadius[j], powery[j] - powerRadius[j], powerx[j] + powerRadius[j], powery[j] + powerRadius[j]);
			}
		}
	}
	colr[0] = 1; colg[0] = 0; colb[0] = 1;
	colr[1] = 0; colg[1] = 1; colb[1] = 1;

	for (int i = 0; i < cntLanes; i++) {
		int range = maxx - minx - 50;
		bridges[i] = (rand() % range) + minx + 25;
	}
	int range = maxx - minx - 25;
	speX = (rand() % range) + 15.0 + minx;

	//goal
	range = maxx - minx - 500;
	pos = (rand() % range) + 250 + minx;

	play_background_music();

	glutMainLoop();//don't call any method after this line as it will not be reached.

}
