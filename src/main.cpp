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

int g_iWidth = 1280;
int g_iHeight = 720;
int g_iCalibFrameSize =30;
int g_iCalibFrameThick = 3;

unsigned char *g_pRGBBack;
int g_iBackWidth;
int g_iBackHeight;

bool combined=true;
bool back;
bool cam;
bool filtered;

int maskSize=0;		
int mask=1;

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
 /* for(int i=0;i<256;i++)
  {
    for(int j=0;j<256;j++)
    {
      g_pRGBHistogramSample[(i*256+j)*3+0] = ppHist[i][j]*256/iMax;
      g_pRGBHistogramSample[(i*256+j)*3+1] = ppHist[i][j]*256/iMax;
      g_pRGBHistogramSample[(i*256+j)*3+2] = ppHist[i][j]*256/iMax;
    }
  } */

  //Deallokacje Pamieci
  for(int i=0;i<256;i++)
  {
    delete ppHist[i];
  }
  delete ppHist;

}



unsigned char* ReadPpmFromFile(char* szFileName,int &riWidth, int &riHeight)
{
  FILE* hfile = fopen(szFileName,"rb");
  int i,j,k,fb;
  int v[3];
  unsigned char buff[10];
  unsigned char* pRGBBuffer;

  if(hfile!=NULL)
  {
    //Odczytanie nag³ówka
    fread(buff,3,1,hfile);
    if((buff[0]=='P')&&(buff[1]=='6')&&(isspace(buff[2])))
    {
      for(i=0;i<3;i++)
      {
        fread(&buff[0],1,1,hfile);
        if(buff[0]=='#') 
        {
          while((buff[0]!=13)&&(buff[0]!=10)) fread(&buff[0],1,1,hfile);
          fread(&buff[0],1,1,hfile);
        }
        k=0;
        do
        {
          k++;
          fread(&buff[k],1,1,hfile);
        }
        while(!isspace(buff[k]));
        buff[k+1] = 0;
        v[i] = atoi((char*)buff);
      }
      v[2] /= 256; //Pobranie dynamiki obrazu 1 lub 2 bajty na próbkê
      
      //Inicjacja obrazu
      if((v[0]*3)%4!=0)
      {
        fb = 4-(((v[0])*3)%4);
      }
      else
      {
        fb = 0;
      }
      riWidth = v[0]+fb;
      riHeight = v[1];
      pRGBBuffer = new unsigned char [riWidth*riHeight*3]; //Zaalokowanie odpowiedniego buffora obrazu
   
      // Wczytanie sk³adowych
      if(v[2]==0)
      {
        for(i=0;i<v[1];i++)
        {
          for(j=0;j<v[0];j++)
          {
            fread(&pRGBBuffer[((riHeight-i-1)*(riWidth)+j)*3+2],1,1,hfile);
            fread(&pRGBBuffer[((riHeight-i-1)*(riWidth)+j)*3+1],1,1,hfile);
            fread(&pRGBBuffer[((riHeight-i-1)*(riWidth)+j)*3+0],1,1,hfile);
          }
          
          /*
          for(j=0;j<fb;j++)
          {
            pTmp[j*3+0] = 0;
            pTmp[j*3+1] = 0;
            pTmp[j*3+2] = 0;
          }
          pTmp+=fb*3;//*/
        }
      }
      else
      {
        //16 bit samples - Nie wspierane
        /*
        for(i=0;i<img->cmp[0]->dj;i++)
          for(j=0;j<img->cmp[0]->dx;j++)
          {
            x_fread(hfile,&buff,6);
            img->cmp[0]->pel[i][j] = buff[0]<<8 & buff[1];
            img->cmp[1]->pel[i][j] = buff[2]<<8 & buff[3];
            img->cmp[2]->pel[i][j] = buff[4]<<8 & buff[5];
          }
          //*/
        delete pRGBBuffer;
        return NULL;
      }
  }
  return pRGBBuffer;
 }
 return NULL;
}

unsigned char* ReadBmpFromFile(char* szFileName,int &riWidth, int &riHeight)
{
	BITMAPFILEHEADER     bfh;
	BITMAPINFOHEADER     bih;

	int                i,j,h,v,lev,l,ls;
	unsigned char*     buff = NULL;  

  unsigned char* p_palette = NULL;
  unsigned short n_colors = 0;

  unsigned char* pRGBBuffer;

  FILE* hfile = fopen(szFileName,"rb");

	if(hfile!=NULL)
	{
    fread(&bfh,sizeof(bfh),1,hfile);
		if(!(bfh.bfType != 0x4d42 || (bfh.bfOffBits < (sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)))) )
		{
      fread(&bih,sizeof(bih),1,hfile);
			v   = bih.biWidth;
			h   = bih.biHeight;
			lev = bih.biBitCount;
			
      riWidth = v;
      riHeight = h;
      pRGBBuffer = new unsigned char [riWidth*riHeight*3]; //Zaalokowanie odpowiedniego buffora obrazu
      
      //Za³aduj Palete barw jesli jest
      if((lev==1)||(lev==4)||(lev==8))
      {
        n_colors = 1<<lev;
        p_palette = new unsigned char[4*n_colors];
        fread(p_palette,4*n_colors,1,hfile);
      }

      fseek(hfile,bfh.bfOffBits,SEEK_SET);

			buff = new unsigned char[v*4];

			switch(lev)
			{
				case 1:
				//Nie obs³ugiwane
				break;
        case 4:
        //nie Obs³ugiwane
        break;
        case 8: //Skala szaroœci
        ls = (v+3)&0xFFFFFFFC;
				for(j=(h-1);j>=0;j--)
				{
          fread(buff,ls,1,hfile);
          for(i=0,l=0;i<v;i++) 
          {
            pRGBBuffer[((j*riWidth)+i)*3+2] = p_palette[(buff[i]<<2)+2];//R
            pRGBBuffer[((j*riWidth)+i)*3+1] = p_palette[(buff[i]<<2)+1];//G
            pRGBBuffer[((j*riWidth)+i)*3+0] = p_palette[(buff[i]<<2)+0];//B
          }
				};
				break;
				case 24:
				//bitmapa RGB
				ls = (v*3+3)&0xFFFFFFFC;
				for(j=(h-1);j>=0;j--)
				{
					//x_fread(hfile,buff,ls);
          fread(buff,ls,1,hfile);
					for(i=0,l=0;i<v;i++,l+=3) 
					{
						pRGBBuffer[((j*riWidth)+i)*3+0] = buff[l+0];
						pRGBBuffer[((j*riWidth)+i)*3+1] = buff[l+1];
						pRGBBuffer[((j*riWidth)+i)*3+2] = buff[l+2];
					};
				};
				break;
				case 32:
			  // RGBA bitmap 
				for(j=(h-1);j>=0;j--)
				{
          fread(buff,v*4,1,hfile);
					for(i=0,l=0;i<v;i++,l+=4) 
					{
						pRGBBuffer[((j*riWidth)+i)*3+0] = buff[l+0];
						pRGBBuffer[((j*riWidth)+i)*3+1] = buff[l+1];
						pRGBBuffer[((j*riWidth)+i)*3+2] = buff[l+2];
					}
				};
				break;
			};
			delete buff; 
   if(p_palette) delete p_palette;

		}
	}
	return pRGBBuffer;
}

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

void DoSomeThingWithSample(unsigned char* pRGBSrcSample,unsigned char* pRGBDsrSample,int iWidth, int iHeight)
{
  for(int y=0;y<iHeight;y++) //Pêtla po wszystkich wierszach obrazu
  {
    for(int x=0;x<iWidth;x++) //Pêtla po wszystkich kolumnach obrazu
    {
      pRGBDsrSample[(y*iWidth+x)*3+0] = pRGBSrcSample[(y*iWidth+x)*3+0]; //Przepisanie s³adowej B
      pRGBDsrSample[(y*iWidth+x)*3+1] = pRGBSrcSample[(y*iWidth+x)*3+1]; //Przepisanie s³adowej G
      pRGBDsrSample[(y*iWidth+x)*3+2] = pRGBSrcSample[(y*iWidth+x)*3+2]; //Przepisanie s³adowej R
	  //g_temp=pRGBDsrSample;
/*
	  g_last[(y*iWidth+x)*3+0] = pRGBSrcSample[(y*iWidth+x)*3+0]; //Przepisanie s³adowej B
	  g_last[(y*iWidth+x)*3+1] = pRGBSrcSample[(y*iWidth+x)*3+1]; //Przepisanie s³adowej G
	  g_last[(y*iWidth+x)*3+2] = pRGBSrcSample[(y*iWidth+x)*3+2]; //Przepisanie s³adowej R
*/
	}
  }

   for(int i=0; i<iHeight;i++)
	  {
		for(int j=0; j<iWidth;j++)
		{

			g_temp[(i*iWidth+j)*3+0]= pRGBDsrSample[(i*iWidth+j)*3+0];
			g_temp[(i*iWidth+j)*3+1]= pRGBDsrSample[(i*iWidth+j)*3+1];
			g_temp[(i*iWidth+j)*3+2]= pRGBDsrSample[(i*iWidth+j)*3+2];

			//mask
			if(maskSize && i>maskSize && j>maskSize && j<iWidth-maskSize && i<iHeight-maskSize)
			{
				g_temp[(i*iWidth+j)*3+0]=g_temp[(i*iWidth+j)*3+0]/mask;
				g_temp[(i*iWidth+j)*3+1]=g_temp[(i*iWidth+j)*3+1]/mask;
				g_temp[(i*iWidth+j)*3+2]=g_temp[(i*iWidth+j)*3+2]/mask;
				for (int ii=-maskSize;ii<=maskSize;ii++)
				{
					for (int jj=-maskSize;jj<=maskSize;jj++)
					{
						if (ii || jj)
						{
						g_temp[(i*iWidth+j)*3+0]=g_temp[(i*iWidth+j)*3+0]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+0]/mask;
						g_temp[(i*iWidth+j)*3+1]=g_temp[(i*iWidth+j)*3+1]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+1]/mask;
						g_temp[(i*iWidth+j)*3+2]=g_temp[(i*iWidth+j)*3+2]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+2]/mask;
						}
					}
				}

			}
							int R = g_temp[(i*iWidth+j)*3+2];
							int G = g_temp[(i*iWidth+j)*3+1];
							int B = g_temp[(i*iWidth+j)*3+0];

							float Y = 0.299f*R+0.587f*G+0.114f*B;
							float U = -0.147f*R-0.289f*G+0.437f*B;
							float V = 0.615f*R-0.515f*G+0.100f*B;
				   /*
					if((U<0)&&(V<0))
					{
					
					g_last[(i*iWidth+j)*3+0] = g_pRGBBack[(i*g_iBackWidth+j)*3+0]; //0;
					g_last[(i*iWidth+j)*3+1] = g_pRGBBack[(i*g_iBackWidth+j)*3+1]; // 0;
					g_last[(i*iWidth+j)*3+2] = g_pRGBBack[(i*g_iBackWidth+j)*3+2]; //0;

					}
					else
					{
					
					g_last[(i*iWidth+j)*3+0] = pRGBDsrSample[(i*iWidth+j)*3+0];
					g_last[(i*iWidth+j)*3+1] = pRGBDsrSample[(i*iWidth+j)*3+1];
					g_last[(i*iWidth+j)*3+2] =pRGBDsrSample[(i*iWidth+j)*3+2];
					} 
					
				  */

					
					g_last[(i*iWidth+j)*3+0] = pRGBDsrSample[(i*iWidth+j)*3+0];
					g_last[(i*iWidth+j)*3+1] = pRGBDsrSample[(i*iWidth+j)*3+1];
					g_last[(i*iWidth+j)*3+2] = pRGBDsrSample[(i*iWidth+j)*3+2];
					
					
					
					 if(g_bIsCalibrating)
					 {
							//Przetwarzaj œrodek ekranu
							if((i>=g_iWidth/2-g_iCalibFrameSize)&&(i<=g_iWidth/2+g_iCalibFrameSize)&&(j>=g_iHeight/2-g_iCalibFrameSize)&&(j<=g_iHeight/2+g_iCalibFrameSize))
							{
							  //Rysuj ramkê na œrodku ekranu
							  if(!((i>=g_iWidth/2-g_iCalibFrameSize+g_iCalibFrameThick)&&(i<=g_iWidth/2+g_iCalibFrameSize-g_iCalibFrameThick)&&(j>=g_iHeight/2-g_iCalibFrameSize+g_iCalibFrameThick)&&(j<=g_iHeight/2+g_iCalibFrameSize-g_iCalibFrameThick)))
							  {
								g_last[(i*iWidth+j)*3+0] = 0;
								g_last[(i*iWidth+j)*3+1] = 0;
								g_last[(i*iWidth+j)*3+2] =255;
							  }
							  //Dodaj ramke do naszego histogramu
							  if(g_bIsGetFrame)
							  {
								g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] += 1;
							  }
							}
						  }
						  else
						  {
							if(g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V]>1)
							{
								g_last[(i*iWidth+j)*3+0] = g_pRGBBack[(i*g_iBackWidth+j)*3+0]; //0;
								g_last[(i*iWidth+j)*3+1] = g_pRGBBack[(i*g_iBackWidth+j)*3+1]; // 0;
								g_last[(i*iWidth+j)*3+2] = g_pRGBBack[(i*g_iBackWidth+j)*3+2]; //0;
							}
		
						}

						}
					 }
	if(g_bIsGetFrame)
    g_bIsGetFrame = false;
 }

void xInitCamera(int iDevice, int iWidth, int iHeight)
{
  int width,height,size;
  VI.setupDevice(iDevice,iWidth,iHeight); // 320 x 240 resolution

  width = VI.getWidth(iDevice);
  height = VI.getHeight(iDevice);
  size = VI.getSize(iDevice); // size to initialize `
  //VI.setUseCallback(true);
}

void xGetFrame(unsigned char* pRGBSample)
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

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
HWND hWndButton;
  switch (message)
  {
  case WM_CREATE:

	hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Calibrate Mode"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,40,120,30,hwnd,(HMENU)GRINBOX_CALIB_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
    hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Detect Mode"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,80,120,30,hwnd,(HMENU)GRINBOX_DETECT_BUTTON,GetModuleHandle(NULL),NULL);   ////////////
    hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Get Frame"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,120,120,30,hwnd,(HMENU)GRINBOX_GETFRAME_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
	hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Reset Calibration"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,160,120,30,hwnd,(HMENU)GRINBOX_RESET_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
	 hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Exit"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,g_iHeight-60,120,30,hwnd,(HMENU)GRINBOX_EXIT_BUTTON,GetModuleHandle(NULL),NULL);  ///////////

    xInitCamera(0,g_iWidth,g_iHeight); //Aktjwacja pierwszej kamery do pobierania obrazu o rozdzielczoœci 320x240

	g_pRGBOriginalSample = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na originalne próbki obrazu
    g_pRGBProcesedSample = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu
	g_last = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu
	g_temp = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu
	g_pRGBBack = ReadPpmFromFile("blank.ppm",g_iBackWidth, g_iBackHeight); //Wczyt obrazu z pliku
	
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
  //  xDisplayBmpOnWindow(hwnd,0,0,g_pRGBOriginalSample,g_iWidth,g_iHeight); //Narysowanie naszego buffora próbek obrazu na okienku
    xDisplayBmpOnWindow(hwnd,0,0,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Narysowanie naszego buffora próbek obrazu na okienku
      break;
    break;
  case WM_TIMER:
    switch(wParam)
    {
    case GRINBOX_ID_TIMER_GET_FRAME:
      xGetFrame(g_pRGBOriginalSample);  //Pobranie 1 ramki obrazu z kamery

      DoSomeThingWithSample(g_pRGBOriginalSample,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Wywo³anie procedury przetwarzaj¹cej obraz
    //  xDisplayBmpOnWindow(hwnd,0,0,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Wyœwitlenie 1 ramki obrazu na okienku
	  if (combined) xDisplayBmpOnWindow(hwnd,0,0,g_last,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
	  if (back)xDisplayBmpOnWindow(hwnd,0,0,g_pRGBBack,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
	  if (cam) xDisplayBmpOnWindow(hwnd,0,0,g_pRGBOriginalSample,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
	  if (filtered) xDisplayBmpOnWindow(hwnd,0,0,g_temp,g_iWidth,g_iHeight); //Wyœwitlenie zmodyfikowanej ramki obrazu na okienku w innym miejscu
      break;
    }
    break;

  case WM_COMMAND:
	  if(HIWORD(wParam)==0)
		  switch(LOWORD(wParam))
	  {

			case GRINBOX_CALIB_BUTTON:
				g_bIsCalibrating = true;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				combined=true;
				filtered=cam=back=false;
				break;
			case GRINBOX_DETECT_BUTTON:
				xEndCalibrate();
				g_bIsCalibrating = false;
				break;
			case GRINBOX_GETFRAME_BUTTON:
				g_bIsGetFrame = true;
				break;
			case GRINBOX_RESET_BUTTON:
				ResetHistogram(480,640);
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
				CheckMenuItem(GetMenu(hwnd),ID_MASK_5,MF_UNCHECKED);
				break;
			case ID_MASK_3:
				maskSize=1;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_5,MF_UNCHECKED);
				break;
			case ID_MASK_5:
				maskSize=2;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_5,MF_CHECKED);
				break;
		  case ID_MENU_EXIT:
			  PostQuitMessage(0);
			  break;
		   case GRINBOX_EXIT_BUTTON:
			   PostQuitMessage(0);
			  break;

		  case ID_MENU_ABOUT:
			  MessageBox(0,TEXT("Coded by\nKamil Czempiñski and Marcin No¿yñski\nunder Multimedia Scientific Circle\n\nversion 0.88.1 beta"),TEXT("About"),MB_OK);
			  break;
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

					if(!strstr(ofn.lpstrFile,".bmp")&&!strstr(ofn.lpstrFile,".ppm"))
						MessageBox(0,TEXT("Why ya clickin if not openin, dude?"),TEXT("WTF?"),MB_OK);

					

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
    WS_OVERLAPPEDWINDOW,
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
