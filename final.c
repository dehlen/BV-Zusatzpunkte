#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

int Debug=0;

#define	Abs(x)	((x) < 0 ? -(x) : (x))

void	Usage	(char *Name)
{
  fprintf(stderr,"Benutzung: %s file.jpg [file.jpg file.jpg...]\n",Name);
}

int	ReadJpegMarker	(FILE *Fin)
{
  int B1,B2;
  long Pos;
  long Len;

  Pos = ftell(Fin); Len=0;
ReadAgainB1:
  B1 = fgetc(Fin); Len++;
  while(!feof(Fin) && (B1 != 0xff))
    {
    B1 = fgetc(Fin); Len++;
    }
  if (feof(Fin)) return(0);
ReadAgainB2:
  B2 = fgetc(Fin); Len++;
  if (B2 == 0xff)  goto ReadAgainB2;
  if (B2 == 0x00)  goto ReadAgainB1;
  return(B1*256+B2);
}


int	ProcessJPEG	(FILE *Fin)
{
  int Header[15];
  int i;
  int Type;
  int Length;
  float Diff=0; 
  float QualityAvg[3] = {0,0,0}; 
  float QualityF; 
  int QualityI;
  float Total;
  float TotalNum;
  int Precision,Index;
	
  for(i=0; i<2; i++)
    {
    Header[i] = fgetc(Fin);
    }

  if ((Header[0] != 0xFF) || (Header[1] != 0xD8))
	{
	printf("ERROR: Kein unterstütztes JPEG Format\n");
	return(1);
	}

  while(!feof(Fin))
    {
   
    Type = ReadJpegMarker(Fin);
    if (Type==0)	return(0);
    
    Length = fgetc(Fin) * 256 + fgetc(Fin);
    Length = Length - 2;
    if (Length < 0) Length=0;

    if (Type != 0xffdb)
	{
	
	for(i=0; i<Length; i++) fgetc(Fin);
	continue;
	}

    if (Length%65 != 0)
	{
	printf("ERROR: Falsche Größe der Quantisierungstabelle -- beinhaltet %d bytes (%d bytes zu kurz oder %d bytes zu lang)\n",Length,65-Length%65,Length%65);
	}

	printf("\nQuantisierungstabelle\n");

    while(Length > 0)
	{
	Precision = fgetc(Fin);
	Length--;
	Index = Precision & 0x0f;
	Precision = (Precision & 0xf0) / 16;
	printf("Genauigkeit=%d; Tabellenindex=%d (%s)\n",Precision,Index,Index ? "Chrominanz":"Luminanz");


	Total=0;
	TotalNum=0;
	while((Length > 0) && (TotalNum<64))
	    {
	    i = fgetc(Fin);
	    if (TotalNum!=0) Total += i;
	    Length--;
	    if (((int)TotalNum%8) == 0) printf("    ");
	    printf("%4d",i);
	    if (((int)TotalNum%8) == 7) printf("\n");
	    TotalNum++;
	    }
	TotalNum--; 
	if (Index < 3) 
	    {
	    QualityAvg[Index] = 100.0-Total/TotalNum;
	    printf("Geschätzte Qualität = %5.2f%%\n",QualityAvg[Index]);
	    if (QualityAvg[Index] <= 0)
		printf("Qualität zu gering. Schätzung kann falsch sein.\n");
	    for(i=Index+1; i<3; i++) QualityAvg[i]=QualityAvg[Index];
	    }

	
	if (Index > 0)
	    {
			Diff  = Abs(QualityAvg[0]-QualityAvg[1]) * 0.49;
	    Diff += Abs(QualityAvg[0]-QualityAvg[2]) * 0.49;
	  
	    QualityF = (QualityAvg[0]+QualityAvg[1]+QualityAvg[2])/3.0 + Diff;
	    QualityI = (QualityF+0.5);
	    printf("Qualität (Durchschnitt): %5.2f%% (%d%%)\n",QualityF,QualityI);
	    } 
	  } 
    } 
  return(0);
} 

int	main	(int argc, char *argv[])
{
  int c;
  FILE *Fin;
  int rc;


  opterr=0;
  while ((c=getopt(argc,argv,"")) != -1)
    {
    switch (c)
      {
      default:
      	Usage(argv[0]);
	exit(-1);
      }
    }
		if (argc - optind < 1)
  	{
	Usage(argv[0]);
	exit(-1);
	}

  for( ; optind < argc; optind++)
    {
    Fin = fopen(argv[optind],"rb");
    if (!Fin)
      {
      fprintf(stderr,"ERROR: Datei konnte nicht geöffnet werden: %s\n",argv[optind]);
      continue;
      }
    printf("#Datei: %s\n",argv[optind]);
    rc=ProcessJPEG(Fin);
    fclose(Fin);
    if (optind+1 < argc) printf("\n");
    }
  return(0);
} 
