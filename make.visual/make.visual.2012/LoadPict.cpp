#include <windows.h>
#include <string>
#include <stdlib.h>


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
				//for(j=(h-1);j>=0;j--)
				for(j=0;j<=(h-1);j++)
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
