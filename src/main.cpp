#include "main.h"
#include <videoInput.h>

#pragma optimize("", on)

HINSTANCE hInst;

videoInput VI;
unsigned char *g_pRGBOriginalSample;
unsigned char *g_pRGBProcesedSample;
unsigned char *g_last;			
unsigned char *g_temp;


unsigned int ***g_colorDetection;
bool g_bIsCalibrating;
bool g_bIsGetFrame;

int g_iWidth;						// rozdzielczosc kamery
int g_iHeight;						// rozdzielczosc kamery
int g_iCalibFrameSize =30;			//rozmiar ramki pobieraj¹cej próbki
int bonusPix=0;						//dodatkowe pixele, pod ktore jest podstawiane tlo
int combobox_index;
int inc_range=3;
bool RGB_range_active = false;
bool YUV_range_active = false;


int R_low, G_low, B_low, R_up, G_up, B_up;
double Y_low, U_low, V_low, Y_up, U_up, V_up;



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
HBRUSH brush = CreateSolidBrush(RGB(255,0,0));


//przyciski
int butW=120;		//szerokosc przycisku
int butH=30;	//wysokosc przycisku

//void DrawSq(HWND hwnd, int iWidth, int iHeight);		//funkcja rysujaca kwadrat w miejscu pobierania probek
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
							if(delete_pattern)   //usuwanie probek
							{   
								for(float Y1=Y-inc_range; Y1<=Y+inc_range; Y1=Y1++)
									for(float U1=U-inc_range; U1<=U+inc_range;U1=U1++)
										for(float V1=V-inc_range; V1<=V+inc_range;V1=V1++)	
											if(Y1>0)
											g_colorDetection[(unsigned char)Y1][(unsigned char)U1][(unsigned char)V1] =0;
							}
							else    //dodawanie probek
							{   
								for(float Y1=Y-inc_range; Y1<=Y+inc_range; Y1=Y1++)
									for(float U1=U-inc_range; U1<=U+inc_range;U1=U1++)
										for(float V1=V-inc_range; V1<=V+inc_range;V1=V1++)	
											if(Y1>0)
											g_colorDetection[(unsigned char)Y1][(unsigned char)U1][(unsigned char)V1] += 1;
							}
							
				}
				
				if(RGB_range_active)
				{
					for(int r=R_low; r<=R_up; r++)
						for(int g=G_low; g<=G_up; g++)
							for(int b=B_low; b<=B_up; b++)
							{
								float Y = 0.299f*r+0.587f*g+0.114f*b;
								float U = -0.147f*r-0.289f*g+0.437f*b;
								float V = 0.615f*r-0.515f*g+0.100f*b;
								g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] += 1;
							}

							RGB_range_active=false;
				}

				if(YUV_range_active)
				{
					for(double y=Y_low; y<=Y_up; y+=0.2)
						for(double u=U_low; u<=U_up; u+=0.2)
							for(double v=V_low; v<=V_up; v+=0.2)
								g_colorDetection[(unsigned char)y][(unsigned char)u][(unsigned char)v] += 1;


					YUV_range_active=false;
				}

									
									

				if(normal_mode==true)		//tryb normal
				{
					if(g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V]>1 )

						for(int g=i-bonusPix; g<=i+bonusPix; g++)
							for(int h=j-bonusPix; h<=j+bonusPix; h++)
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
						for(int g=i-bonusPix; g<=i+bonusPix; g++)
							for(int h=j-bonusPix; h<=j+bonusPix; h++)
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

			
			RECT rect = {xL,iHeight-yU, xR, iHeight- yD};
			FrameRect(hDC,&rect,brush);

			//Rectangle(hDC, xL,iHeight-yU, xR, iHeight- yD);

		}

}




//! zamaian Int na LPCSTR





void xInitCamera( int iDevice, int &width, int &height)
{
	//int width,height,
	int size;
	VI.setupDevice(iDevice); // 

	width = VI.getWidth(iDevice);
	height = VI.getHeight(iDevice);
	
	size = VI.getSize(iDevice); // size to initialize `

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

	BitBlt(hDC,iX,iY,iWidth, iHeight,hDCofBmp,0,0,SRCCOPY);

	SelectObject(hDCofBmp,0);
	DeleteObject(hBmp);
	DeleteDC(hDCofBmp);

	ReleaseDC(hWnd,hDC);
}


void IntToLpcstr(int liczba, char *cache)	//zamiana int na lpcstr
{
	wsprintf (cache, "%d", liczba);
}

void DoubleToLpcstr(double liczba,  char *cache)
{
	sprintf (cache, "%f", liczba);
}

//! wysyla znaki to texboxa
void setTextBoxINT(int num, HWND textbox)	
	
{
		char number[256];					//wpisane wartosi do textboxa	
		IntToLpcstr(num , number);		
		SetWindowText(textbox, number);	
}

void setTextBoxDOUBLE(double num, HWND textbox)	
	
{
		char number[256];					//wpisane wartosi do textboxa	
		DoubleToLpcstr(num , number);		
		SetWindowText(textbox, number);	
}


//! interfejs 
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndButton;
	static HWND textbox;
    static HWND textbox1;
	static HWND textbox_msg;
	HWND text;
	HWND res_combobox;
	HWND radioButton;

	POINT punkt;
	GetCursorPos(&punkt);
	ScreenToClient(hwnd,&punkt);
	short wheel_delta(0);

	static HWND R1,G1,B1,R2,G2,B2;
	static HWND Y1,U1,V1,Y2,U2,V2;






	switch (message)
	{
	case WM_CREATE:

			//  ==================================================== poczatek interfejsu ========================================================//


		
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Add pattern"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,20,butW,butH,hwnd,(HMENU)GRINBOX_ADD_PATTERN_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
		//hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Detect Mode"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,55,butW,butH,hwnd,(HMENU)GRINBOX_DETECT_BUTTON,GetModuleHandle(NULL),NULL);   ////////////
		//hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Get Frame"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,120,butW,butH,hwnd,(HMENU)GRINBOX_GETFRAME_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Reset Calibration"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,90,butW,butH,hwnd,(HMENU)GRINBOX_RESET_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Delete pattern"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,55,butW,butH,hwnd,(HMENU)GRINBOX_DELETE_PATTERN_BUTON,GetModuleHandle(NULL),NULL);  ///////////

		textbox_msg =  CreateWindow("static","Mode:", WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER ,g_iWidth+20,155,120,40,hwnd,(HMENU)TEXTBOX_MSG,GetModuleHandle(NULL),NULL);

		text=		CreateWindow(TEXT("static"),TEXT("Tools:\n~~~~~~~~~~~~~~~~~~~~~"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,210,butW,30,hwnd,0,0,0);

		text=		CreateWindow(TEXT("static"),TEXT("Frame size"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,250,butW,20,hwnd,0,0,0);
		textbox =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER  ,g_iWidth+20,272,butW,20,hwnd,(HMENU)TEXTBOX_FRAMESIZE,GetModuleHandle(NULL),NULL);

		text=		CreateWindow(TEXT("static"),TEXT("Increase range"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,295,butW,20,hwnd,0,0,0);
		textbox1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER  ,g_iWidth+20,317,butW,20,hwnd,(HMENU)TEXTBOX_RANGE,GetModuleHandle(NULL),NULL);

		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Save changes"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,344, butW,butH,hwnd,(HMENU)GRINBOX_SAVE,GetModuleHandle(NULL),NULL);  ///////////



		text=		CreateWindow(TEXT("static"),TEXT("Display Mode:"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,390,butW,20,hwnd,0,0,0);
		radioButton = CreateWindow(TEXT("BUTTON"),TEXT("Normal"),BS_AUTORADIOBUTTON | WS_GROUP |BS_FLAT | WS_VISIBLE | WS_CHILD  ,g_iWidth+20,410,butW,butH,hwnd,(HMENU)NORMAL_MODE,GetModuleHandle(NULL),NULL);  ///////////		
		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Reverse"),BS_AUTORADIOBUTTON | BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,440,butW,butH,hwnd,(HMENU)REVERSE_MODE,GetModuleHandle(NULL),NULL);  ///////////
		SendMessage(radioButton, BM_SETCHECK, BST_CHECKED, 0);

		 text=		CreateWindow(TEXT("static"),TEXT("RGB range:"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,475,butW,20,hwnd,0,0,0);

		 R1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+20,500,38,20,hwnd,(HMENU)R1_EDIT,GetModuleHandle(NULL),NULL);
		 G1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+60,500,38,20,hwnd,(HMENU)G1_EDIT,GetModuleHandle(NULL),NULL);
		 B1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER  ,g_iWidth+100,500,38,20,hwnd,(HMENU)B1_EDIT,GetModuleHandle(NULL),NULL);

		 R2 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+20,525,38,20,hwnd,(HMENU)R2_EDIT,GetModuleHandle(NULL),NULL);
		 G2 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+60,525,38,20,hwnd,(HMENU)G2_EDIT,GetModuleHandle(NULL),NULL);
		 B2 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+100,525,38,20,hwnd,(HMENU)B2_EDIT,GetModuleHandle(NULL),NULL);

		 hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Save RGB range"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,550, butW,butH,hwnd,(HMENU)RGB_RANGE_SAVE_BUTTON,GetModuleHandle(NULL),NULL);  ///////////

		 text=		CreateWindow(TEXT("static"),TEXT("YUV range:"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+160,475,butW,20,hwnd,0,0,0);

		 Y1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+160,500,50,20,hwnd,(HMENU)Y1_EDIT,GetModuleHandle(NULL),NULL);
		 U1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+212,500,50,20,hwnd,(HMENU)U1_EDIT,GetModuleHandle(NULL),NULL);
		 V1 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER  ,g_iWidth+264,500,50,20,hwnd,(HMENU)V1_EDIT,GetModuleHandle(NULL),NULL);

		 Y2 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+160,525,50,20,hwnd,(HMENU)Y2_EDIT,GetModuleHandle(NULL),NULL);
		 U2 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+212,525,50,20,hwnd,(HMENU)U2_EDIT,GetModuleHandle(NULL),NULL);
		 V2 =    CreateWindow("EDIT","", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER ,g_iWidth+264,525,50,20,hwnd,(HMENU)V2_EDIT,GetModuleHandle(NULL),NULL);

		 hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Save YUV range"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+160,550, butW,butH,hwnd,(HMENU)YUV_RANGE_SAVE_BUTTON,GetModuleHandle(NULL),NULL);  ///////////

		//text=		CreateWindow(TEXT("static"),TEXT("Settings:\n~~~~~~~~~~~~~~~~~~~~~"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,g_iHeight-220,butW,30,hwnd,0,0,0);
	//	text=		CreateWindow(TEXT("static"),TEXT("Camera resolution"), SS_CENTER |WS_CHILD | WS_VISIBLE, g_iWidth+20,g_iHeight-180,butW,18,hwnd,0,0,0);
		//res_combobox = CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","Resolution",WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,g_iWidth+20,g_iHeight-160,butW,80,hwnd,(HMENU)RES_LIST,GetModuleHandle(NULL),NULL);
		

		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Load background"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,g_iHeight-100,butW,butH,hwnd,(HMENU)LOADBACK_BUTTON,GetModuleHandle(NULL),NULL);  ///////////

		hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Exit"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,g_iHeight-60,butW,butH,hwnd,(HMENU)GRINBOX_EXIT_BUTTON,GetModuleHandle(NULL),NULL);  ///////////



		//i dodajemy kilka pozycji

		//SendMessage(res_combobox, CB_ADDSTRING,0, (LPARAM)"640x480");		//0
		//SendMessage(res_combobox, CB_ADDSTRING,0, (LPARAM)"1280x720");		//1
		//SendMessage(res_combobox, CB_ADDSTRING,0, (LPARAM)"1920x1080");		//2

		//SendMessage(res_combobox,CB_SETCURSEL,(WPARAM)1,0);			// domyslnie 1280x720
		//combobox_index=SendMessage(res_combobox,CB_GETCURSEL,0,0);
	
		setTextBoxINT(g_iCalibFrameSize, textbox);		//wpisane wartosi do textboxa	
		setTextBoxINT(inc_range, textbox1);	

		/*	
			SetWindowText(R1, "R low");	
			SetWindowText(G1, "G low");	
			SetWindowText(B1, "B low");	
			SetWindowText(R2, "R up");	
			SetWindowText(G2, "G up");	
			SetWindowText(B2, "B up");	
			*/

		//  ==================================================== koniec interfejsu ========================================================//





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
			if(g_bIsCalibrating==true)
			{			
				mouseX=punkt.x;
				mouseY=g_iHeight - punkt.y;   //vertical change
				g_bIsGetFrame = true;
				DrawSq(hwnd, g_iWidth,g_iHeight);
			}

		}
		break;


	case WM_MOUSEMOVE:
		{
			if(g_bIsCalibrating==true)					
			{
				mouseX=punkt.x;
				mouseY=g_iHeight - punkt.y;   //vertical change
				DrawSq(hwnd, g_iWidth,g_iHeight);					
			}
		}
		break;

	case WM_MOUSEWHEEL:
				
			if(GET_WHEEL_DELTA_WPARAM(wParam)<0)
			{
				g_iCalibFrameSize-=5;
				setTextBoxINT(g_iCalibFrameSize, textbox);		//wpisane wartosci do textboxa	
			}
			else
			{
				g_iCalibFrameSize+=5;
				setTextBoxINT(g_iCalibFrameSize, textbox);		//wpisane wartosci do textboxa	
			}
			DrawSq(hwnd, g_iWidth,g_iHeight);

		break;

	case WM_COMMAND:
		if(HIWORD(wParam)==0)
			switch(LOWORD(wParam))	
		{

			case GRINBOX_SAVE:

				char szInput[10];					// czytanie z textbox
				GetWindowText(GetDlgItem(hwnd, TEXTBOX_FRAMESIZE), szInput, 10);
				g_iCalibFrameSize=atoi(szInput);
					
				GetWindowText(GetDlgItem(hwnd, TEXTBOX_RANGE), szInput, 10);
				inc_range=atoi(szInput);

				break;

			case RGB_RANGE_SAVE_BUTTON:

				char szInput1[10];	// czytanie z textbox
				GetWindowText(GetDlgItem(hwnd, R1_EDIT), szInput1, 10);
				R_low=atoi(szInput1);
				if(R_low<0)
				{
					R_low=0;
					setTextBoxINT(R_low,R1);	
				}
				if(R_low>255)
				{
					R_low=255;
					setTextBoxINT(R_low,R1);	
				}
			
					
				GetWindowText(GetDlgItem(hwnd, R2_EDIT), szInput1, 10);
				R_up=atoi(szInput1);
				if(R_up<0)
				{
					R_up=0;
					setTextBoxINT(R_up,R2);
				}
				if(R_up>255)
				{
					R_up=255;
					setTextBoxINT(R_up,R2);
				}
			

				GetWindowText(GetDlgItem(hwnd, G1_EDIT), szInput1, 10);
				G_low=atoi(szInput1);
				if(G_low<0)
				{
					G_low=0;
					setTextBoxINT(G_low,G1);	
				}
				if(G_low>255)
				{
					G_low=255;
					setTextBoxINT(G_low,G1);	
				}
				
					
				GetWindowText(GetDlgItem(hwnd, G2_EDIT), szInput1, 10);
				G_up=atoi(szInput1);
				if(G_up<0)
				{
					G_up=0;
					setTextBoxINT(G_up,G2);
				}
				if(G_up>255)
				{
					G_up=255;
					setTextBoxINT(G_up,G2);
				}
		

				GetWindowText(GetDlgItem(hwnd, B1_EDIT), szInput1, 10);
				B_low=atoi(szInput1);
				if(B_low<0)
				{
					B_low=0;
					setTextBoxINT(B_low,B1);	
				}
				if(B_low>255)
				{
					B_low=255;
					setTextBoxINT(B_low,B1);	
				}
				
					
				GetWindowText(GetDlgItem(hwnd, B2_EDIT), szInput1, 10);
				B_up=atoi(szInput1);
				if(B_up<0)
				{
					B_up=0;
					setTextBoxINT(B_up,B2);
				}
				if(B_up>255)
				{
					B_up=255;
					setTextBoxINT(B_up,B2);
				}
			

				g_bIsCalibrating=true;
				RGB_range_active=true;



				

				setTextBoxDOUBLE( 0.299f*R_low+0.587f*G_low+0.114f*B_low , Y1 );
				setTextBoxDOUBLE( -0.147f*R_low-0.289f*G_low+0.437f*B_low , U1 );
				setTextBoxDOUBLE( 0.615f*R_low-0.515f*G_low+0.100f*B_low, V1 );

				setTextBoxDOUBLE( 0.299f*R_up+0.587f*G_up+0.114f*B_up , Y2 );
				setTextBoxDOUBLE( -0.147f*R_up-0.289f*G_up+0.437f*B_up , U2 );
				setTextBoxDOUBLE( 0.615f*R_up-0.515f*G_up+0.100f*B_up, V2 );

				break;

			case YUV_RANGE_SAVE_BUTTON:

				char szInput2[10];	// czytanie z textbox

				GetWindowText(GetDlgItem(hwnd, Y1_EDIT), szInput2, 10);
				Y_low=atof(szInput2);
				if(Y_low<0)
				{
					Y_low=0;
					setTextBoxDOUBLE(Y_low,Y1);
				}
				if(Y_low>255)
				{
					Y_low=255;
					setTextBoxDOUBLE(Y_low,Y1);
				}

					
				GetWindowText(GetDlgItem(hwnd, Y2_EDIT), szInput2, 10);
				Y_up=atof(szInput2);
				if(Y_up<0)
				{
					Y_up=0;
					setTextBoxDOUBLE(Y_up,Y2);
				}
				if(Y_up>255)
				{
					Y_up=255;
					setTextBoxDOUBLE(Y_up,Y2);
				}

				GetWindowText(GetDlgItem(hwnd, U1_EDIT), szInput2, 10);
				U_low=atof(szInput2);
				if(U_low<-128)
				{
					U_low=-128;
					setTextBoxDOUBLE(U_low,U1);
				}
				if(U_low>127)
				{
					U_low=127;
					setTextBoxDOUBLE(U_low,U1);
				}
					
				GetWindowText(GetDlgItem(hwnd, U2_EDIT), szInput2, 10);
				U_up=atof(szInput2);
				if(U_up<-128)
				{
					U_up=-128;
					setTextBoxDOUBLE(U_up,U2);
				}
				if(U_up>127)
				{
					U_up=127;
					setTextBoxDOUBLE(U_up,U2);
				}

				GetWindowText(GetDlgItem(hwnd, V1_EDIT), szInput2, 10);
				V_low=atof(szInput2);
				if(V_low<-128)
				{
					V_low=-128;
					setTextBoxDOUBLE(V_low,V1);
				}
				if(V_low>127)
				{
					V_low=127;
					setTextBoxDOUBLE(U_low,V1);
				}
					
				GetWindowText(GetDlgItem(hwnd, V2_EDIT), szInput2, 10);
				V_up=atof(szInput2);
				if(V_up<-128)
				{
					V_up=-128;
					setTextBoxDOUBLE(V_up,V2);
				}
				if(V_up>127)
				{
					V_up=127;
					setTextBoxDOUBLE(V_up,V2);
				}


				g_bIsCalibrating=true;
				YUV_range_active=true;

				setTextBoxINT(Y_low + (V_low/0.877), R1);
				setTextBoxINT(Y_low- 0.395*U_low - 0.581 * V_low, G1);
				setTextBoxINT(Y_low + (U_low/0.492), B1);

				setTextBoxINT(Y_up + (V_up/0.877), R2);
				setTextBoxINT(Y_up- 0.395*U_up - 0.581 * V_up, G2);
				setTextBoxINT(Y_up + (U_up/0.492), B2);


				break;

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

			case GRINBOX_DELETE_PATTERN_BUTON:
				SetWindowText(textbox_msg, "Mode:\r\nDelete Pattern");
				delete_pattern=true;
				break;


			case GRINBOX_RESET_BUTTON:
				ResetHistogram(g_iHeight,g_iWidth);
				g_bIsCalibrating = false;
				normal_mode=true;
				RGB_range_active=false;
				YUV_range_active=false;
				SetWindowText(textbox_msg, "Mode:\r\nNone");
				break;



			case NORMAL_MODE:
				normal_mode=true;
				break;

			case REVERSE_MODE:
				normal_mode=false;
				break;

			case RES_LIST:

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

	xInitCamera(0,g_iWidth,g_iHeight); //Aktjwacja pierwszej kamery do pobierania obrazu


	// Tworzenie g³ównego okna aplikacji
	HWND hWnd = CreateWindow(
		GRINBOX_APP_CLASS_NAME,
		GRINBOX_APP_WINDOW_NAME,
		WS_OVERLAPPEDWINDOW ,
		0,
		0,
		g_iWidth+360,
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
