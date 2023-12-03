/*
Starios Dense Emulator
*/
//some random commands:
		//window->getSize().x;
		//window->getSize().y;
		//MouseBlackHole.setFillColor(sf::Color(rand() % 255, rand() % 255, rand() % 255));
		//mousePos = (sf::Vector2f) sf::Mouse::getPosition(*window).x;
		//std::cout << "number of Balls:" << numBalls << "\n"; //Send data to the terminal window


#include <iostream>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <thread>
#include <Windows.h>    
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <RS232Comm.h>
#include <cmath>
#include "Hardware.h"
#include "ws2812.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define PIXEL_WIDTH 100
#define PIXEL_HEIGHT 100

#define M_PI 3.141592653589793

//-----------------------------------Global Variables--------------------------------------

//Graphics
sf::RenderWindow* window = nullptr;

//Make Pixels array
//std::vector<std::vector<sf::RectangleShape> > Pixels(NUMROWS,std::vector<sf::RectangleShape>(NUMCOLS)); // cant figure out how to make 2d object vector
std::vector<sf::RectangleShape> Pixels;
ws2812 panel(NUMLEDS);


//Serial
uint32_t numSamples = 0; //needed?

char charArray[256] = "Empty"; //graphics text buffer sprintf

//holds the 8bit audio values from the serial port
#define HIGHT 10
#define HOFFSET 8 //minum value for hightThress (really should be equation (highThresh[i]-lowThresh > HOFFSET)
struct {
	int freq[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	int highThresh[16] = { HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT,HIGHT };
	int lowThresh[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
} audio;
char incomingData[256] = ""; //Serial Rx Buffer
int dataLength = 255; //number of bytes to read on the serial port
int readResult = 0; //error code from serial ReadData()



//--------------------------------------User Function Definitions--------------------------------------

bool isMouseOverRect(sf::Vector2f mousePosition, sf::RectangleShape rectangleShape) {
	if (mousePosition.x > rectangleShape.getPosition().x && mousePosition.x < rectangleShape.getPosition().x + rectangleShape.getSize().x) {
		if (mousePosition.y > rectangleShape.getPosition().y && mousePosition.y < rectangleShape.getPosition().y + rectangleShape.getSize().y) {
			return true;
		}
	}
		return false;
}

//adds two vectors
sf::Vector2f operator+ (sf::Vector2f v1, sf::Vector2f v2) {
	return sf::Vector2f(v1.x + v2.x, v1.y + v2.y);
}

//subratcts two vectors
sf::Vector2f operator- (sf::Vector2f v1, sf::Vector2f v2) {
	return sf::Vector2f(v1.x - v2.x, v1.y - v2.y);
}

//returns the dot-product scaler from 2 vectors
float dot(sf::Vector2f v1, sf::Vector2f v2) {
	return (v1.x*v2.x) + (v1.y*v2.y);
}

//magnitude for v2-v1
float mag(sf::Vector2f v1, sf::Vector2f v2) {
	return float(abs(sqrt((v2.x - v1.x)*(v2.x - v1.x) + (v2.y - v1.y)*(v2.y - v1.y))));
}

float sq(float s) {
	return s * s;
}

sf::Vector2f vMultiply(sf::Vector2f v1, sf::Vector2f v2) {
	return sf::Vector2f(v1.x * v2.x, v1.y * v2.y);
}

sf::Vector2f sMultiply(float s, sf::Vector2f v) {
	return sf::Vector2f(s * v.x, s * v.y);
}

//Trail right function curvy
void starios(int delayTime) {   //Shift columns right 1 place
#define ANGLE_STEP 20
#define DECAY_STEP 5
//volatile uint32_t ArrayBuffer[8] = {0}; //holds 8 colors
	volatile int baseColorAngle = Rand(0, 360);
	volatile int dir;
	volatile uint8_t brightnessTemp;
	volatile uint32_t colorTemp;
	volatile uint32_t colorbufferTemp;
	if (!Rand(0, 2)) {
		dir = 1;
	}
	else {
		dir = -1;
	}
	panel.updateColorVars(panel.wheelColor(baseColorAngle, panel.getBrightness()));
	brightnessTemp = panel.getBrightness();
	volatile int y_start = Rand(0, NUMROWS); //random y position
	for (int i = 0; i < NUMCOLS; i++) { //move across the entire display
		volatile int y_delta = Rand(-1, 2); //random y position offset (-1 inclusive, 2 exclusive) so -1,0,1
		volatile int y = constrain(y_start + y_delta, 0, NUMROWS - 1); //pixel y position
		panel.FillColumn(0, 0x000000); //blank the left most column
		panel.setPixelColor(grid[0][y], panel.Forecolor); //feed column zero
		panel.show();
		for (int s = NUMCOLS - 2; s >= 0; s--) { //loop columns. this will move all the pixels 1 PLACE to the right
			for (int b = 0; b <= NUMROWS - 1; b++) { //loop bulbs
				colorbufferTemp = panel.getPixelColor(grid[s][b]); //store the color into buffer
				if (colorbufferTemp != 0) colorTemp = colorbufferTemp;
				else colorTemp = 0;
				panel.setPixelColor(grid[s + 1][b], colorTemp); //put those stored colors in place one to the right
			}
		}
		//_delay_ms(delayTime + 5); //sets the update rate
		//std::this_thread::sleep_for(std::chrono::milliseconds(delayTime+5));

		//turn the color angle by ANGLE_STEP degrees
		baseColorAngle += dir * ANGLE_STEP;
		if (baseColorAngle > 360) {
			baseColorAngle -= 360;
		}
		else if (baseColorAngle < 0) {
			baseColorAngle = +360;
		}

		//dim the color trails
		if (brightnessTemp > 0) brightnessTemp--;
		//panel.updateColorVars(panel.wheelColor(baseColorAngle, brightnessTemp));
		
	}
}

//grow shrink square eyes
void BeyeSquare(int delayTime) {  //makes both eyes grow with random colors
	//panel.getrand(randSelect);
	panel.updateColorVars(panel.wheelColor(RAND_AREA)); //wheelArea
	//add colors
	for (int i = 0; i < 8; i++) { //index
		panel.wheelAngle += 20;
		if (panel.wheelAngle >= 360) panel.wheelAngle = 0;
		panel.updateColorVars(panel.wheelColor(panel.wheelAngle, panel.getBrightness())); //wheelArea
		for (int idx = 0; idx < 8; idx++) {
			panel.setPixelColor(grid[i][idx], panel.Forecolor); //LEFT EYE
			panel.setPixelColor(grid[idx][i], panel.Forecolor);
			panel.setPixelColor(grid[NUMCOLS - 1 - i][idx], panel.Forecolor);//RIGHT EYE
			panel.setPixelColor(grid[NUMCOLS - 1 - idx][i], panel.Forecolor);
		}
		for (int i = 0; i < NUMLEDS; i++) window->draw(Pixels[i]);
		window->display();
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
	}

	//remove colors
	for (int i = 7; i >= 0; i--) { //index steps
		for (int idx = 7; idx >= i; idx--) { //index rows and columns
			panel.setPixelColor(grid[i][idx], 0x000000);//LEFT EYE
			panel.setPixelColor(grid[idx][i], 0x000000);
			panel.setPixelColor(grid[NUMCOLS - 1 - i][idx], 0x000000);//RIGHT EYE
			panel.setPixelColor(grid[NUMCOLS - 1 - idx][i], 0x000000);
		}
		for (int i = 0; i < NUMLEDS; i++) window->draw(Pixels[i]);
		window->display();
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
	}
}

//must use volatile in Debug Build because they get optimized out :(. do not need volatile in Release Build
void nGrid(int _len) {
#define NUMBALLS 5 //using define because or else I have to malloc to dynamically allocate memory
	volatile int Headx[NUMBALLS]; //head position holder
	volatile int Heady[NUMBALLS]; //head position holder
	volatile int Dirx[NUMBALLS]; //-1 or 1
	volatile int Diry[NUMBALLS]; //-1 or 1
	volatile int Tailx[NUMBALLS]; //tail x potion. is not really 'seen'. exists to reduce "flickering"
	volatile int Taily[NUMBALLS]; //tail y position
	volatile int myGreen[NUMBALLS];
	volatile int myRed[NUMBALLS];
	volatile int myBlue[NUMBALLS];
	for (int c = 0; c < NUMBALLS; c++) { //loop through all arrays to set initial params
		panel.updateColorVars(panel.wheelColor(RAND_PERIMETER)); //wheelArea
		myGreen[c] = panel.green; //n number if random colors
		myRed[c] = panel.red;
		myBlue[c] = panel.blue;
		Headx[c] = Rand(0, NUMCOLS); //pick n number of random x start points
		Heady[c] = Rand(0, NUMROWS); //pick n number of random y start points
		if (Rand(0, 2)) {
			Dirx[c] = 1;//pow(-1,random(1,2));
			Diry[c] = 1;//pow(-1,random(1,2));
		}
		else {
			Dirx[c] = -1;//pow(-1,random(1,2));
			Diry[c] = -1;//pow(-1,random(1,2));
		}
	}
	for (int i = 0; i <= _len; i++) {
		for (int c = 0; c < NUMBALLS; c++) {
			if (Headx[c] == 0) Dirx[c] = 1;
			if (Headx[c] == NUMCOLS - 1) Dirx[c] = -1;
			if (Heady[c] == 0) Diry[c] = 1;
			if (Heady[c] == NUMROWS - 1) Diry[c] = -1;

			Tailx[c] = Headx[c];
			Taily[c] = Heady[c];
			if (c % 2 == 0) {
				Headx[c] += Dirx[c];
			}
			else {
				Heady[c] += Diry[c];
			}
			panel.setPixelColor(grid[Headx[c]][(NUMROWS - 1) - Heady[c]], panel.Color(myGreen[c] / (NUMBALLS - c), myRed[c] / (NUMBALLS - c), myBlue[c] / (NUMBALLS - c))); //set the pixel
			panel.setPixelColor(grid[Tailx[c]][(NUMROWS - 1) - Taily[c]], 0x000000); //clear the pixel (causing flashing?
			panel.show();
		}
		
	}
	if (Rand(0, 1000) == 500) {
		panel.printStringWithShiftL(" STARIOS", 80); //Send Left scrolling Text (send car array)
	}

}

#define CHANNELS 8
//eq bars. color is from wheel perimeter but then the r,g,and b are scaled by the audio levels (this leads to dimmer effect)
void AudioPattern1() {
	int yDist; //dY holds value for the hight value for one y graduated division
	int rDist, gDist, bDist;

	panel.wheelAngle++;
	if (panel.wheelAngle >= 360) panel.wheelAngle = 0;
	panel.updateColorVars(panel.wheelColor(panel.wheelAngle, BMAX));

	for (int x = 0; x < CHANNELS; x++) { //MSGEQ7 channel 0 to 7
		if (audio.freq[x]) {
			int temp = audio.highThresh[x] - audio.lowThresh[x];
			yDist = constrain(((audio.freq[x] * NUMROWS) / (audio.highThresh[x] - audio.lowThresh[x])), 0, NUMROWS - 1); //math to get valS[x] proportional to nLength
			rDist = (audio.freq[x] * panel.red) / (audio.highThresh[x] - audio.lowThresh[x]);
			gDist = (audio.freq[x] * panel.green) / (audio.highThresh[x] - audio.lowThresh[x]);
			bDist = (audio.freq[x] * panel.blue) / (audio.highThresh[x] - audio.lowThresh[x]);
			for (int y = 0; y <= yDist; y++) {
				panel.setPixelColor(grid[x][(NUMROWS - 1) - y], panel.Color(rDist, gDist, bDist)); //even columns 0 to 8
				panel.setPixelColor(grid[(NUMCOLS - 1) - x][(NUMROWS - 1) - y], panel.Color(rDist, gDist, bDist)); //green part //even columns 0 to 8
			}
		}
	}
	panel.show();
	//shrink columns .
	for (int x = 0; x < CHANNELS; x++) { //MSGEQ7 channel 0 to 7
		panel.setPixelColor(grid[x][0], 0x000000); //blank top pixel in current column
		panel.setPixelColor(grid[(NUMCOLS - 1) - x][0], 0x000000); //blank top pixel in current column
		for (int y = 0; y < NUMROWS - 1; y++) {
			panel.setPixelColor(grid[x][(NUMROWS - 1) - y], panel.getPixelColor(grid[x][(NUMROWS - 2) - y])); //move each bulb down 1
			panel.setPixelColor(grid[(NUMCOLS - 1) - x][(NUMROWS - 1) - y], panel.getPixelColor(grid[(NUMCOLS - 1) - x][(NUMROWS - 2) - y])); //move each bulb down 1
		}
	}

	if (audio.freq[1] > (int)audio.highThresh[1] * .7) { //if bass reading is > 70% of max bass
		panel.updateColorVars(panel.wheelColor(RAND_PERIMETER)); //wheelPerimeter
	}
}

int main()
{	

	//sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, 1080), "SFML Playground");
	window = new sf::RenderWindow (sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML Serial Matrix");
	sf::Vector2f mousePos = (sf::Vector2f) sf::Mouse::getPosition(*window);
	window->setMouseCursorVisible(true);
	window->setFramerateLimit(200);
	sf::Font font;
	if (!font.loadFromFile("../res/Pumpkin_Pancakes.ttf")) {
		printf("Error loading Font");
		system("pause");
	}


	//Push Pixels onto vector array
	for (int i = 0; i < NUMLEDS; i++) {
		Pixels.push_back(sf::RectangleShape(sf::Vector2f(PIXEL_WIDTH, PIXEL_HEIGHT)));
	}

	panel.setBrightness(BMAX);

	//Construct the Matrix of Pixels
	#define DELTA_X WINDOW_WIDTH/NUMCOLS
	#define DELTA_Y WINDOW_HEIGHT/NUMROWS
	#define X_OFFSET 10	
	#define Y_OFFSET 15
	float xPos = X_OFFSET;
	float yPos = PIXEL_HEIGHT + Y_OFFSET;
	for (int y = 0; y < NUMROWS; y++) {
		for (int x = 0; x < NUMCOLS; x++) {
			Pixels[(y*NUMCOLS) + x].setPosition(sf::Vector2f(xPos, yPos));
			Pixels[(y*NUMCOLS) + x].setOutlineThickness(1);
			Pixels[(y*NUMCOLS) + x].setOutlineColor(sf::Color(0x80,0x80,0x80));
			//Pixels[(x*NUM_ROWS)+y].setFillColor(sf::Color(0x00, 0x00, 0xFF));
			panel.setPixelColor(grid[x][y], 0x000000);
			xPos += DELTA_X;			
		}
		xPos = X_OFFSET;
		yPos += (DELTA_Y - Y_OFFSET);
	}

	//GUI Objects
	sf::Text DebugText;
	DebugText.setFont(font);
	DebugText.setString(charArray); 
	DebugText.setCharacterSize(50);
	DebugText.setFillColor(sf::Color::Magenta);
	DebugText.setPosition(sf::Vector2f(100, 20)); //just hard coded a text position
	
	//mouse crosshair can be drawn
	sf::RectangleShape xMouseCross(sf::Vector2f(WINDOW_WIDTH, 2)); //x mouse crosshair
	xMouseCross.setPosition(sf::Vector2f(0, (window->getSize().y / 2) - 1));
	xMouseCross.setFillColor(sf::Color::Magenta);

	sf::RectangleShape yMouseCross(sf::Vector2f(2, WINDOW_HEIGHT)); //y mouse crosshair
	yMouseCross.setPosition(sf::Vector2f((window->getSize().x / 2) - 1, 0));
	yMouseCross.setFillColor(sf::Color::Magenta);

	
	sf::RectangleShape Button_1(sf::Vector2f(100,50)); //create button with wideth,height
	Button_1.setPosition(sf::Vector2f(WINDOW_WIDTH-Button_1.getSize().x, WINDOW_HEIGHT - Button_1.getSize().y));
	Button_1.setFillColor(sf::Color::Blue);

	

	Serial* SP = new Serial("\\\\.\\COM11");    // adjust as needed \\\\.\\COM10
	if (SP->IsConnected()) printf("We're connected");

	//Super Loop Begin
	while (window->isOpen())
	{
		//I need a way to have the serial and graphics on seperate threads. (so I can use a delay/sleep function)
		//std::thread th1(BeyeSquare, 3);
		
		//BeyeSquare(1);
		//panel.printStringWithShiftL(" STARIOS     ", 80); //Send Left scrolling Text (send car array)
		//for (int i = 0; i < 1000; i++) starios(15);
		//for(int i=0; i<1000; i++) nGrid(4);

		



		//-----------------------------------------------------Get Mouse & Keyboard inputs----------------------------------------------------------------|
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
			//Do Stuff	
		}
		if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
			//Do Stuff
		}

		//mouse wheel input only? not sure how this 'event' works
		sf::Event event;
		while (window->pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window->close();
				SP->~Serial(); //deconstruct
			}
			else {
				if (event.type == sf::Event::MouseWheelMoved) {
					//Mouse Wheel UP
					if (event.mouseWheel.delta > 0) {
						//Do Stuff
					}
					if (event.mouseWheel.delta < 0) {
						//Do Stuff
					}
					//for (int i = 0; i < WINDOW_WIDTH-1; i++) dot[i].setPosition(sf::Vector2f(i, (float) ( ( (dot[i].getPosition().y - (window.getSize().y / 2)) * scaler ) + (window.getSize().y / 2))) );
				}
			}
		}
		
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
			//Do Stuff
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
			//Do Stuff
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) DebugText.move(10,0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) DebugText.move(-10, 0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) DebugText.move(0, -10);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) DebugText.move(0, 10);


		//Handle Serial
		if (SP->IsConnected()) {
			//std::thread th1(&Serial::ReadData,incomingData, &dataLength); //how do i get the return value?
			readResult = SP->ReadData(incomingData, dataLength);
			// printf("Bytes read: (0 means no data available) %i\n",readResult);
			//printf("%s", incomingData);
			//incomingData[readResult] = '\0'; //?
			if (readResult > 0) {
				readResult = 0; //needed?
				//extract the values from the serial ascii string
				sscanf_s(incomingData, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r", 
					&audio.freq[0], &audio.freq[1], &audio.freq[2], &audio.freq[3], &audio.freq[4], &audio.freq[5], &audio.freq[6], &audio.freq[7],
					&audio.freq[8], &audio.freq[9], &audio.freq[10], &audio.freq[11], &audio.freq[12], &audio.freq[13], &audio.freq[14], &audio.freq[15]); // ascii to bin
				

				//decrease highThresh after countermax by 5%
				#define THRESHCOUNTERMAX 50
				static int threshCounter = THRESHCOUNTERMAX;
				threshCounter--;
				if (threshCounter <= 0) {
					threshCounter = THRESHCOUNTERMAX;
					for (int i = 0; i < NUMCOLS; i++) {
						if (audio.highThresh[i] > HOFFSET) audio.highThresh[i] *= 0.95f; //this is slowed down with a counter
					}
				}
				
				//adjust vals (increase highThresh on data and decrease slowly on loop
				for (int i = 0; i < NUMCOLS; i++) {
					if (audio.freq[i] > audio.highThresh[i]) audio.highThresh[i] = audio.freq[i];
				}

				//error check for debug
				for (int x = 0; x < 16; x++) { //dumb error check fo a glitch I have fixed. remove this code
					if (audio.highThresh[x] < HOFFSET) {
						printf("error");
					}
				}
				//print the audio data
				sprintf_s(charArray, "Audio Data: %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i\r\n",
					audio.freq[0], audio.freq[1], audio.freq[2], audio.freq[3], audio.freq[4], audio.freq[5], audio.freq[6], audio.freq[7],
					audio.freq[8], audio.freq[9], audio.freq[10], audio.freq[11], audio.freq[12], audio.freq[13], audio.freq[14], audio.freq[15]); // ascii to bin
				printf(charArray);
				DebugText.setString(charArray);	


				sprintf_s(charArray, "hightThresg: %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i\r\n\r\n",
					audio.highThresh[0], audio.highThresh[1], audio.highThresh[2], audio.highThresh[3], audio.highThresh[4], audio.highThresh[5], audio.highThresh[6], audio.highThresh[7],
					audio.highThresh[8], audio.highThresh[9], audio.highThresh[10], audio.highThresh[11], audio.highThresh[12], audio.highThresh[13], audio.highThresh[14], audio.highThresh[15]); // ascii to bin
				printf(charArray);
			}
			//grash the Audio data for the serial port
			AudioPattern1();
		}
		//--------------------------------------------------- Update Shapes with inputs -------------------------------------------------------------|
				
		//update crosshair position
		xMouseCross.setPosition(sf::Vector2f(0, sf::Mouse::getPosition(*window).y));
		yMouseCross.setPosition(sf::Vector2f(sf::Mouse::getPosition(*window).x, 0));
	


		//create the bar graph effect here
		
		
		//the mouse shit needs to be removed
		/*
		//GUI mouse over pixels
		sf::Vector2f mousePosRaw = (sf::Vector2f) sf::Mouse::getPosition(*window);
		for (int x = 0; x < NUMCOLS; x++) {
			for (int y = 0; y < NUMROWS; y++) {
				if (isMouseOverRect(mousePosRaw, Pixels[(x*NUMROWS)+y])) {
					panel.setPixelColor(grid[x][y], 0xF000F0);
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
						if (sf::Mouse::isButtonPressed(sf::Mouse::Left)); //wait for button release
						panel.setPixelColor(grid[x][y], 0x00F0F0);
					}
				} else {
					panel.setPixelColor(grid[x][y], 0x2F2F00);
				}
			}
		}
		*/
		// display\draw all the shapes
		//window->clear(sf::Color::Black);
		
		//window->draw(Button_1);
		//for (int i = 0; i < NUMLEDS; i++) window->draw(Pixels[i]);
		window->draw(DebugText);
		//window->draw(xMouseCross);
		//window->draw(yMouseCross);
		
		//window->display();
	}//end update loop

	return 0;
}