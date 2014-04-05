#include "main.h"
#include <videoInput.h>


HINSTANCE hInst;

videoInput VI;
unsigned char *g_pRGBOriginalSample;
unsigned char *g_pRGBProcesedSample;
unsigned char *g_last;			
unsigned char *g_temp;


unsigned int ***g_colorDetection;
bool g_bIsCalibrating;
bool g_bIsGetFrame;

int g_iWidth = 1280;				// rozdzielczosc kamery
int g_iHeight = 720;				// rozdzielczosc kamery
int g_iCalibFrameSize =30;			//rozmiar ramki pobieraj¹cej próbki
int g_iCalibFrameThick = 3;
int bonusPix=1;						//dodatkowe pixele, pod ktore jest podstawiane tlo

unsigned char *g_pRGBBack;
int g_iBackWidth;
int g_iBackHeight;

bool combined=true;
bool back;
bool cam;
bool filtered;
bool normal_mode=true;
bool delete_pattern=false;
bool add_patter=true;

int maskSize=0;		
int mask=1;


int mouseX;
int mouseY;

//przyciski
int butW=120;		//szerokosc przycisku
int butH=30;	//wysokosc przycisku

void DrawSq(int iWidth, int iHeight);		//funkcja rysujaca kwadrat w miejscu pobierania probek
void maskF(int i, int j, int iWidth, int iHeight, unsigned char* pRGBDsrSample );

//! wczytywanie plikow BMP i PPM jako tla
unsigned char* ReadBmpFromFile(char* szFileName,int &riWidth, int &riHeight);
unsigned char* ReadPpmFromFile(char* szFileName,int &riWidth, int &riHeight);



//! przetwarzanie obrazu
void DoSomeThingWithSample(unsigned char* pRGBSrcSample,unsigned char* pRGBDsrSample,int iWidth, int iHeight)
{



	for(int y=0;y<iHeight;y++) //Pêtla po wszystkich wierszach obrazu
	{
		for(int x=0;x<iWidth;x++) //Pêtla po wszystkich kolumnach obrazu
		{
			pRGBDsrSample[(y*iWidth+x)*3+0] = pRGBSrcSample[(y*iWidth+x)*3+0]; //Przepisanie s³adowej B
			pRGBDsrSample[(y*iWidth+x)*3+1] = pRGBSrcSample[(y*iWidth+x)*3+1]; //Przepisanie s³adowej G
			pRGBDsrSample[(y*iWidth+x)*3+2] = pRGBSrcSample[(y*iWidth+x)*3+2]; //Przepisanie s³adowej R

		}
	}

	for(int i=0; i<iHeight;i++)
	{
		for(int j=0; j<iWidth;j++)
		{

			g_last[(i*iWidth+j)*3+0] = pRGBDsrSample[(i*iWidth+j)*3+0];
			g_last[(i*iWidth+j)*3+1] = pRGBDsrSample[(i*iWidth+j)*3+1];
			g_last[(i*iWidth+j)*3+2] = pRGBDsrSample[(i*iWidth+j)*3+2];


			//maska
			if(maskSize && i>maskSize && j>maskSize && j<iWidth-maskSize && i<iHeight-maskSize)
				maskF(i,j,iWidth,iHeight,pRGBDsrSample);



			int R = g_last[(i*iWidth+j)*3+2];
			int G = g_last[(i*iWidth+j)*3+1];
			int B = g_last[(i*iWidth+j)*3+0];

			float Y = 0.299f*R+0.587f*G+0.114f*B;
			float U = -0.147f*R-0.289f*G+0.437f*B;
			float V = 0.615f*R-0.515f*G+0.100f*B;





			if(g_bIsCalibrating)
			{


				if(g_bIsGetFrame)
				{

					if(j>=mouseX-g_iCalibFrameSize && j<=mouseX+g_iCalibFrameSize && i>=mouseY-g_iCalibFrameSize && i<=mouseY+g_iCalibFrameSize)
						if( j>0 && j<iWidth && i>0 && i<iHeight)
							if(delete_pattern)
							{
								for(double Y1=Y-0.2; Y1<Y+0.2; Y1=Y1+0.01)
									for(double U1=U-0.05; U1<U+0.05;U1=U1+0.01)
										for(double V1=V-0.05; V1<V+0.05;V1=V1+0.01)	
											g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] =0;
							}
							else
							{
								for(double Y1=Y-0.2; Y1<Y+0.2; Y1=Y1+0.01)
									for(double U1=U-0.05; U1<U+0.05;U1=U1+0.01)
										for(double V1=V-0.05; V1<V+0.05;V1=V1+0.01)	
											g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] += 1;
							}
							
				}

		//	}
		//	else
		//	{
							
								
									

				if(normal_mode==true)		//tryb normal
				{
					if(g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V]>1 )

						for(int g=i-bonusPix; g<i+bonusPix; g++)
							for(int h=j-bonusPix; h<j+bonusPix; h++)
							{
								if(h>0 && h<iWidth && g>0 && g<iHeight)
								{
									g_last[(g*iWidth+h)*3+0] = g_pRGBBack[(g*g_iBackWidth+h)*3+0]; //0;
									g_last[(g*iWidth+h)*3+1] = g_pRGBBack[(g*g_iBackWidth+h)*3+1]; // 0;
									g_last[(g*iWidth+h)*3+2] = g_pRGBBack[(g*g_iBackWidth+h)*3+2]; //0;
								}
							}
				}
				else            //tryb reverse
				{
					if(g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V]<1 )
						for(int g=i-bonusPix; g<i+bonusPix; g++)
							for(int h=j-bonusPix; h<j+bonusPix; h++)
							{
								if(h>0 && h<iWidth && g>0 && g<iHeight)
								{
									g_last[(g*iWidth+h)*3+0] = g_pRGBBack[(g*g_iBackWidth+h)*3+0]; //0;
									g_last[(g*iWidth+h)*3+1] = g_pRGBBack[(g*g_iBackWidth+h)*3+1]; // 0;
									g_last[(g*iWidth+h)*3+2] = g_pRGBBack[(g*g_iBackWidth+h)*3+2]; //0;
								}
							}

				}

				

			}



			
		}  //koniec for
	}		// koniec for
	

	if(g_bIsGetFrame)
		g_bIsGetFrame=false;
}



void xEndCalibrate()
{
	unsigned int **ppHist;
	int iMax = 0;

	//Allokacje Pamieci
	ppHist = new unsigned int*[256];
	for(int i=0;i<256;i++)
	{
		ppHist[i] = new unsigned int[256];
	}

	//Normalize Histogram
	for(int i=0;i<256;i++)
	{
		for(int j=0;j<256;j++)
		{
			ppHist[i][j] = 0;
			for(int k=0;k<256;k++)
			{
				ppHist[i][j] += g_colorDetection[k][i][j];
			}
			if(iMax<ppHist[i][j]) iMax = ppHist[i][j];
		}
	}

	//Create Histogram for display
	/*for(int i=0;i<256;i++)
	{
	for(int j=0;j<256;j++)
	{
	g_pRGBHistogramSample[(i*256+j)*3+0] = ppHist[i][j]*256/iMax;
	g_pRGBHistogramSample[(i*256+j)*3+1] = ppHist[i][j]*256/iMax;
	g_pRGBHistogramSample[(i*256+j)*3+2] = ppHist[i][j]*256/iMax;
	}
	} 
	*/
	//Deallokacje Pamieci
	for(int i=0;i<256;i++)
	{
		delete ppHist[i];
	}
	delete ppHist;

}


//! reset pobranych probek, umozliwia pobranie nowych
void ResetHistogram(int iHeight, int iWidth)	
{
	for(double Y=0; Y<255;Y++)
	{
		for(int U=0; U<256;U++)
		{
			for(int V=0; V<256;V++)					
				g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] = 0;
		}
	}
}	 

//! maska usredniajaca
void maskF(int i, int j, int iWidth, int iHeight, unsigned char* pRGBDsrSample )	
{
	if(maskSize && i>maskSize && j>maskSize && j<iWidth-maskSize && i<iHeight-maskSize)
	{
		g_temp[(i*iWidth+j)*3+0]=g_last[(i*iWidth+j)*3+0] /mask;
		g_temp[(i*iWidth+j)*3+1]=g_last[(i*iWidth+j)*3+1] /mask;
		g_temp[(i*iWidth+j)*3+2]=g_last[(i*iWidth+j)*3+2] /mask;

		for (int ii=-maskSize;ii<=maskSize;ii++)
		{
			for (int jj=-maskSize;jj<=maskSize;jj++)
			{
				if (ii || jj)
				{
					g_temp[(i*iWidth+j)*3+0]=g_temp[(i*iWidth+j)*3+0]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+0]  /mask;
					g_temp[(i*iWidth+j)*3+1]=g_temp[(i*iWidth+j)*3+1]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+1] /mask;
					g_temp[(i*iWidth+j)*3+2]=g_temp[(i*iWidth+j)*3+2]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+2] /mask;
				}
			}
		}



	}
}


void DrawSq(HWND hwnd, int iWidth, int iHeight)		
{  
		int xL=mouseX-g_iCalibFrameSize;
		int xR=mouseX+g_iCalibFrameSize;
		int yD=mouseY-g_iCalibFrameSize;
		int yU=mouseY+g_iCalibFrameSize;

		HDC hDC = GetDC(hwnd);

		if(xL<iWidth)
		{
		
			if(xL<0)
				xL=0;
			if(xR>iWidth)
				xR=iWidth;
			if(yD<0)
				yD=0;
			if(yU>iHeight)
				yU=iHeight;



			//&& mouseX+g_iCalibFrameSize<iWidth && mouseY-g_iCalibFrameSize>0 && mouseY+g_iCalibFrameSize<iHeight)

			Rectangle(hDC, xL,iHeight-yU, xR, iHeight- yD);
		}

		


//	 for(int i=mouseY-g_iCalibFrameSize; i<mouseY+g_iCalibFrameSize; i++)
//		{
//		for(int j=mouseX-g_iCalibFrameSize; j<mouseX+g_iCalibFrameSize; j++)
//		{
//			//if((j>mouseX-g_iCalibFrameSize-g_iCalibFrameThick) &&	( j<mouseX - g_iCalibFrameSize + g_iCalibFrameThick) && (j>mouseX+g_iCalibFrameSize-g_iCalibFrameThick) &&	( j<mouseX + g_iCalibFrameSize + g_iCalibFrameThick) && (i > mouseY-g_iCalibFrameSize-g_iCalibFrameThick) &&	( i <mouseY - g_iCalibFrameSize + g_iCalibFrameThick) && (i>mouseY+g_iCalibFrameSize-g_iCalibFrameThick) &&	( i<mouseY + g_iCalibFrameSize + g_iCalibFrameThick));
//			
//			
//					g_last[(i*iWidth+j)*3+0] = 0;
//					g_last[(i*iWidth+j)*3+1] = 255;
//					g_last[(i*iWidth+j)*3+2] =255;
//				
//
//		}
//
//	}
//
//
//
}




//! zamaian Int na LPCSTR
void IntToLpcstr(int liczba, char *cache)  
{
	wsprintf (cache, "%d", liczba);
}




void xInitCamera( int iDevice, int iWidth, int iHeight)
{
	int width,height,size;
	VI.setupDevice(iDevice,iWidth,iHeight); // 320 x 240 resolution

	width = VI.getWidth(iDevice);
	height = VI.getHeight(iDevice);
	size = VI.getSize(iDevice); // size to initialize `
	//VI.setUseCallback(true);
}

void xGetFrame( unsigned char* pRGBSample)
{
	int device;
	device = 0;
	if(VI.isFrameNew(device))
	{
		VI.getPixels(device,pRGBSample, false, false);
	}
}


void xDisplayBmpOnWindow(HWND hWnd,int iX, int iY, unsigned char* pRGBSample, int iWidth, int iHeight)
{
	HDC hDC = GetDC(hWnd);
	HDC hDCofBmp = CreateCompatibleDC(hDC);

	HBITMAP hBmp = CreateCompatibleBitmap(hDC,iWidth,iHeight);

	SelectObject(hDCofBmp,hBmp);

	BITMAPINFOHEADER biBmpInfoHeader;

	biBmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	biBmpInfoHeader.biWidth = iWidth;
	biBmpInfoHeader.biHeight = iHeight;
	biBmpInfoHeader.biPlanes = 1;
	biBmpInfoHeader.biBitCount = 24;//32
	biBmpInfoHeader.biCompression = BI_RGB;
	biBmpInfoHeader.biSizeImage = 0;
	biBmpInfoHeader.biXPelsPerMeter = 0;
	biBmpInfoHeader.biYPelsPerMeter = 0;
	biBmpInfoHeader.biClrUsed = 0;
	biBmpInfoHeader.biClrImportant = 0;

	SetDIBits(hDCofBmp,hBmp,0,iWidth*iHeight,pRGBSample,(BITMAPINFO*)&biBmpInfoHeader,DIB_RGB_COLORS);

	BitBlt(hDC,iX,iY,iWidth,iHeight,hDCofBmp,0,0,SRCCOPY);

	SelectObject(hDCofBmp,0);
	DeleteObject(hBmp);
	DeleteDC(hDCofBmp);

	ReleaseDC(hWnd,hDC);
}





//! interfejs 
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndButton;
	static HWND textbox;
	static HWND textbox_msg;
	HWND text;
	HWND res_combobox;
	switch (message)
	{
	case WM_CREATE:


		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Add pattern"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,20,butW,butH,hwnd,(HMENU)GRINBOX_ADD_PATTERN_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
		//hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Detect Mode"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,55,butW,butH,hwnd,(HMENU)GRINBOX_DETECT_BUTTON,GetModuleHandle(NULL),NULL);   ////////////
		//hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Get Frame"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,120,butW,butH,hwnd,(HMENU)GRINBOX_GETFRAME_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Reset Calibration"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,90,butW,butH,hwnd,(HMENU)GRINBOX_RESET_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Delete pattern"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,55,butW,butH,hwnd,(HMENU)GRINBOX_DELETE_PATTERN_BUTON,GetModuleHandle(NULL),NULL);  ///////////

		textbox_msg =  CreateWindow("static","Mode:", WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER ,g_iWidth+20,175,120,40,hwnd,(HMENU)TEXTBOX_MSG,GetModuleHandle(NULL),NULL);

		text=		CreateWindow(TEXT("static"),TEXT("Tools:\n~~~~~~~~~~~~~~~~~~~~~"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,230,butW,30,hwnd,0,0,0);

		text=		CreateWindow(TEXT("static"),TEXT("Calibration Frame\nSize"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,270,butW,30,hwnd,0,0,0);
		textbox =  CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER  ,g_iWidth+20,302,butW,butH+10,hwnd,(HMENU)TEXTBOX_FRAMESIZE,GetModuleHandle(NULL),NULL);
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Save changes"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,344, butW,butH,hwnd,(HMENU)GRINBOX_SAVE,GetModuleHandle(NULL),NULL);  ///////////



		text=		CreateWindow(TEXT("static"),TEXT("Display Mode:"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,390,butW,20,hwnd,0,0,0);
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Normal"),BS_AUTORADIOBUTTON | WS_GROUP |BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,410,butW,butH,hwnd,(HMENU)NORMAL_MODE,GetModuleHandle(NULL),NULL);  ///////////		
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Reverse"),BS_AUTORADIOBUTTON | BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,440,butW,butH,hwnd,(HMENU)REVERSE_MODE,GetModuleHandle(NULL),NULL);  ///////////


		text=		CreateWindow(TEXT("static"),TEXT("Settings:\n~~~~~~~~~~~~~~~~~~~~~"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,g_iHeight-220,butW,30,hwnd,0,0,0);
		text=		CreateWindow(TEXT("static"),TEXT("Camera resolution"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,g_iHeight-180,butW,18,hwnd,0,0,0);
		res_combobox = CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","Resolution",WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,g_iWidth+20,g_iHeight-160,butW,80,hwnd,(HMENU)RES_LIST,GetModuleHandle(NULL),NULL);

		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Load background"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,g_iHeight-120,butW,butH,hwnd,(HMENU)LOADBACK_BUTTON,GetModuleHandle(NULL),NULL);  ///////////

		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Exit"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,g_iHeight-60,butW,butH,hwnd,(HMENU)GRINBOX_EXIT_BUTTON,GetModuleHandle(NULL),NULL);  ///////////



		//i dodajemy kilka pozycji

		SendMessage(res_combobox, CB_ADDSTRING,0, (LPARAM)"640x480");
		//pozycja_3 = SendMessage(cb_handle, CB_ADDSTRING,0, (LPARAM)"3");
		SendMessage(res_combobox, CB_ADDSTRING,0, (LPARAM)"1280x720");
		SendMessage(res_combobox, CB_ADDSTRING,0, (LPARAM)"1920x1080");


		char detectionSize[256];					//wpisane wartosi do textboxa	
		IntToLpcstr(g_iCalibFrameSize , detectionSize);		
		SetWindowText(textbox, detectionSize);			

		xInitCamera(0,g_iWidth,g_iHeight); //Aktjwacja pierwszej kamery do pobierania obrazu

		g_pRGBOriginalSample = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na originalne próbki obrazu
		g_pRGBProcesedSample = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu
		g_last = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu - combo
		g_temp = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu - po filtracji

		g_colorDetection = new unsigned int** [256]; //Allokacja buffora pamiêci na histogram obrazu
		for(int i=0;i<256;i++)
		{
			g_colorDetection[i] = new unsigned int* [256];
			for(int j=0;j<256;j++)
			{
				g_colorDetection[i][j] = new unsigned int [256];
				for(int k=0;k<256;k++)
				{
					g_colorDetection[i][j][k] = 0;
				}
			}
		}

		SetTimer(hwnd,GRINBOX_ID_TIMER_GET_FRAME,40,NULL); //Ustawienie minutnika na co 40 milisekund

		g_bIsCalibrating = false;
		g_bIsGetFrame = false;

		break;
	case WM_PAINT:

		xDisplayBmpOnWindow(hwnd,0,0,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Narysowanie naszego buffora próbek obrazu na okienku
		g_pRGBBack = ReadBmpFromFile("..\\..\\back\\white.bmp",g_iBackWidth, g_iBackHeight); //Wczyt domyslnego t³a obrazu z pliku
		SetWindowText(textbox_msg, "Mode:\r\nNone");
		break;
		break;
	case WM_TIMER:
		switch(wParam)
		{
		case GRINBOX_ID_TIMER_GET_FRAME:
			xGetFrame( g_pRGBOriginalSample);  //Pobranie 1 ramki obrazu z kamery

			DoSomeThingWithSample(g_pRGBOriginalSample,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Wywo³anie procedury przetwarzaj¹cej obraz

			//  xDisplayBmpOnWindow(hwnd,0,0,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Wyœwitlenie 1 ramki obrazu na okienku
			if (combined) xDisplayBmpOnWindow(hwnd,0,0,g_last,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
			if (back)xDisplayBmpOnWindow(hwnd,0,0,g_pRGBBack,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
			if (cam) xDisplayBmpOnWindow(hwnd,0,0,g_pRGBOriginalSample,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
			if (filtered) xDisplayBmpOnWindow(hwnd,0,0,g_temp,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
			break;
		}
		break;


	case WM_LBUTTONDOWN:
		{
			POINT punkt;
			GetCursorPos(&punkt);
			ScreenToClient(hwnd,&punkt);
			mouseX=punkt.x;
			mouseY=g_iHeight-   punkt.y;   //vertical change
			g_bIsGetFrame = true;
			DrawSq(hwnd, g_iWidth,g_iHeight);

		}
		break;





	case WM_COMMAND:
		if(HIWORD(wParam)==0)
			switch(LOWORD(wParam))	
		{

			case GRINBOX_SAVE:

				char szInput[10];					// czytanie z textbox
				GetWindowText(GetDlgItem(hwnd, TEXTBOX_FRAMESIZE), szInput, 10);
				g_iCalibFrameSize=atoi(szInput);

				break;



				/*case WM_MOUSEMOVE   :
				{
				POINT punkt;
				GetCursorPos(&punkt);
				ScreenToClient(hwnd,&punkt);
				mouseX=punkt.x;
				mouseY=punkt.y;
				DrawSq(hwnd, g_iHeight,g_iWidth);
				}
				break;*/


			case GRINBOX_ADD_PATTERN_BUTTON:
				//MessageBox(0,TEXT("Before calibrate load a background image!"),TEXT("Load image"),MB_OK);
				g_bIsCalibrating = true;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				combined=true;
				filtered=cam=back=false;
				delete_pattern=false;

				SetWindowText(textbox_msg, "Mode:\r\nAdd pattern");
				break;



			case GRINBOX_DETECT_BUTTON:
				//xEndCalibrate();
				g_bIsCalibrating = false;
				delete_pattern=false;
				SetWindowText(textbox_msg, "Mode:\r\nDetect");

				break;
			case GRINBOX_GETFRAME_BUTTON:
				//g_bIsGetFrame = true;
				break;

			case NORMAL_MODE:
				normal_mode=true;
				break;

			case REVERSE_MODE:
				normal_mode=false;
				break;

			case RES_LIST:

				break;

			case GRINBOX_DELETE_PATTERN_BUTON:
				SetWindowText(textbox_msg, "Mode:\r\nDelete Pattern");
				delete_pattern=true;
				break;


			case GRINBOX_RESET_BUTTON:
				ResetHistogram(g_iHeight,g_iWidth);
				g_bIsCalibrating = false;
				normal_mode=true;
				SetWindowText(textbox_msg, "Mode:\r\nNone");
				break;

			case ID_BACK:
				back=true;
				filtered=cam=combined=false;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				break;
			case ID_CAM:
				cam=true;
				filtered=combined=back=false;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				break;
			case ID_COMB:
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				combined=true;
				filtered=cam=back=false;
				break;
			case ID_FILTERED:
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_CHECKED);
				filtered=true;
				combined=cam=back=false;
				break;
			case ID_MASK_OFF:
				maskSize=0;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_UNCHECKED);
				break;
			case ID_MASK_3:
				maskSize=1;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_CHECKED);
				break;

			case ID_MENU_EXIT:
				PostQuitMessage(0);
				break;
			case GRINBOX_EXIT_BUTTON:
				PostQuitMessage(0);
				break;

			case ID_MENU_ABOUT:
				MessageBox(0,TEXT("Coded by\nMarcin No¿yñski \nversion 0.95.1023-2 RC"),TEXT("About"),MB_OK);
				break;

			case LOADBACK_BUTTON:
			case ID_MENU_LOADBACKGROUND:
				{
					OPENFILENAME ofn;       // common dialog box structure
					char szFile[260];       // buffer for file name
					HANDLE hf;              // file handle

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwnd;
					ofn.lpstrFile = szFile;
					// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
					// use the contents of szFile to initialize itself.
					ofn.lpstrFile[0] = '\0';
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "BMP Files\0*.bmp\0PPM Files\0*.ppm\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					// Display the Open dialog box. 

					if (GetOpenFileName(&ofn)==TRUE) 
						hf = CreateFile(ofn.lpstrFile, 
						GENERIC_READ,
						0,
						(LPSECURITY_ATTRIBUTES) NULL,
						CREATE_NEW,
						FILE_ATTRIBUTE_NORMAL,
						(HANDLE) NULL);


					if(strstr(ofn.lpstrFile,".ppm"))
						g_pRGBBack = ReadPpmFromFile(ofn.lpstrFile,g_iBackWidth, g_iBackHeight);

					if(strstr(ofn.lpstrFile,".bmp"))	  
						g_pRGBBack = ReadBmpFromFile(ofn.lpstrFile,g_iBackWidth, g_iBackHeight);

					//					if(!strstr(ofn.lpstrFile,".bmp")&&!strstr(ofn.lpstrFile,".ppm"))
					//					MessageBox(0,TEXT("Why ya clickin if not openin, dude?"),TEXT("WTF?"),MB_OK);



					break;
				}
		}
		break;

	case WM_DESTROY:
		//DeleteObject(hBitmap);

		delete g_pRGBOriginalSample; //Deallokacja buffora pamieci na originalne próbki obrazu 
		delete g_pRGBProcesedSample; //Deallokacja buffora pamieci na przetworzone próbki obrazu 


		//Deallokacja buffora pamiêci na histogram obrazu
		for(int i=0;i<256;i++)
		{
			for(int j=0;j<256;j++)
			{
				delete g_colorDetection[i][j];
			}
			delete g_colorDetection[i];
		}
		delete g_colorDetection;




		PostQuitMessage(0);
		break;
	case WM_SIZE:
		//rozmiar okna
		break;
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	//Definicja klasy okna
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(GRINBOX_MAIN_ICON));
	wc.hCursor = NULL;//LoadCursor(NULL, IDC_HAND);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName = MAKEINTRESOURCE(GRINBOX_MAIN_MENU);
	wc.lpszClassName = GRINBOX_APP_CLASS_NAME;

	//Rejestracja klasy okna
	if ( !RegisterClass( &wc ) ) return( FALSE );

	// Tworzenie g³ównego okna aplikacji
	HWND hWnd = CreateWindow(
		GRINBOX_APP_CLASS_NAME,
		GRINBOX_APP_WINDOW_NAME,
		WS_OVERLAPPEDWINDOW ,
		0,
		0,
		g_iWidth+180,
		g_iHeight+60,
		NULL,
		NULL,
		hInstance,
		NULL);

	// Sprawdzenie czy okienko siê utworzy³o
	if ( hWnd == NULL ) return( FALSE );

	// Pokazanie okinka na ekranie
	ShowWindow(hWnd,iCmdShow);

	MSG msg;

	// G³ówna pêtla komunikatów
	while( GetMessage( &msg, NULL, 0, 0) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
