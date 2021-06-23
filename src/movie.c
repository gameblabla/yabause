/*  
    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file movie.c
    \brief Movie recording functions.
*/

#include "peripheral.h"
#include "scsp.h"
#include "movie.h"
#include "cs2.h"
#include "vdp2.h"  // for DisplayMessage() prototype
#include "yabause.h"
#include "error.h"

//Counting
int framecounter;
int lagframecounter;
int LagFrameFlag;
int FrameAdvanceVariable=0;

/*
static void SetInputDisplayCharacters(void) {

	int x;

	strcpy(str, "");

	for (x = 0; x < 8; x++) {

		if(PORTDATA1.data[2] & (1 << x)) {
         size_t spaces_len = strlen(Spaces[x]);
         if (spaces_len >= 40)
            return;
			strcat(str, Spaces[x]);	
		}
      else
      {
         size_t buttons_len = strlen(Buttons[x]);
         if (buttons_len >= 40)
            return;
         strcat(str, Buttons[x]);
      }

	}

	for (x = 0; x < 8; x++) {

		if(PORTDATA1.data[3] & (1 << x)) {
         size_t spaces2_len = strlen(Spaces2[x]);
         if (spaces2_len >= 40)
            return;
			strcat(str, Spaces2[x]);	
		}
      else
      {
         size_t buttons2_len = strlen(Buttons2[x]);
         if (buttons2_len >= 40)
            return;
         strcat(str, Buttons2[x]);
      }

	}

	strcpy(InputDisplayString, str);
}
*/
//////////////////////////////////////////////////////////////////////////////

static void IncrementLagAndFrameCounter(void)
{
	if(LagFrameFlag == 1)
		lagframecounter++;

	framecounter++;
}

//////////////////////////////////////////////////////////////////////////////

void DoMovie(void) {
	
	IncrementLagAndFrameCounter();
	
/*
	int x;
   size_t num_read = 0;

	if (Movie.Status == 0)
		return;

	IncrementLagAndFrameCounter();
	LagFrameFlag=1;
	SetInputDisplayCharacters();

	//Read/Write Controller Data
	if(Movie.Status == Recording) {
		for (x = 0; x < 8; x++) {
			fwrite(&PORTDATA1.data[x], 1, 1, Movie.fp);
		}
		for (x = 0; x < 8; x++) {
			fwrite(&PORTDATA2.data[x], 1, 1, Movie.fp);
		}
	}

	if(Movie.Status == Playback) {
		for (x = 0; x < 8; x++) {
         num_read = fread(&PORTDATA1.data[x], 1, 1, Movie.fp);
		}
		for (x = 0; x < 8; x++) {
         num_read = fread(&PORTDATA2.data[x], 1, 1, Movie.fp);
		}

		//if we get to the end of the movie
		if(((ftell(Movie.fp)-headersize)/framelength) >= Movie.Frames) {
			fclose(Movie.fp);
			PlaybackFileOpened=0;
			Movie.Status = Stopped;
			ClearInput();
			strcpy(MovieStatus, "Playback Stopped");
		}
	}

	//Stop Recording/Playback
	if(Movie.Status != Recording && RecordingFileOpened) {
		fclose(Movie.fp);
		RecordingFileOpened=0;
		Movie.Status = Stopped;
		strcpy(MovieStatus, "Recording Stopped");
	}

	if(Movie.Status != Playback && PlaybackFileOpened && Movie.ReadOnly != 0) {
		fclose(Movie.fp);
		PlaybackFileOpened=0;
		Movie.Status = Stopped;
		strcpy(MovieStatus, "Playback Stopped");
	}*/
}

//////////////////////////////////////////////////////////////////////////////

void MovieLoadState(void) {

/*
	if (Movie.ReadOnly == 1 && Movie.Status == Playback)  {
		//Movie.Status = Playback;
		fseek (Movie.fp,headersize+(framecounter * framelength),SEEK_SET);
	}

	if(Movie.Status == Recording) {
		fseek (Movie.fp,headersize+(framecounter * framelength),SEEK_SET);
		Movie.Rerecords++;
	}

	if(Movie.Status == Playback && Movie.ReadOnly == 0) {
		Movie.Status = Recording;
		RecordingFileOpened=1;
		strcpy(MovieStatus, "Recording Resumed");
		TruncateMovie(Movie);
		fseek (Movie.fp,headersize+(framecounter * framelength),SEEK_SET);
		Movie.Rerecords++;
	}*/
}

//////////////////////////////////////////////////////////////////////////////

void TruncateMovie(struct MovieStruct Movie) {

	//when we resume recording, shorten the movie so that there isn't 
	//potential garbage data at the end

/*//TODO
	struct MovieBufferStruct tempbuffer;
	fseek(Movie.fp,0,SEEK_SET);
	tempbuffer=ReadMovieIntoABuffer(Movie.fp);
	fclose(Movie.fp);

	//clear the file and write it again
	Movie.fp=fopen(Movie.filename,"wb");
	fwrite(tempbuffer.data,framelength,framecounter,Movie.fp);
	fclose(Movie.fp);

	Movie.fp=fopen(Movie.filename,"r+b");
*/
}

//////////////////////////////////////////////////////////////////////////////

static int MovieGetSize(FILE* fp) {
	/*int size;
	int fpos;

	fpos = ftell(fp);//save current pos

   if (fpos < 0)
   {
      YabSetError(YAB_ERR_OTHER, "MovieGetSize fpos is negative");
      return 0;
   }

	fseek (fp,0,SEEK_END);
	size=ftell(fp);

	Movie.Frames=(size-headersize)/ framelength;

	fseek(fp, fpos, SEEK_SET); //reset back to correct pos
	return(size);*/
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void MovieToggleReadOnly(void) {

	/*if(Movie.Status == Playback) {

		if(Movie.ReadOnly == 1) 
		{
			Movie.ReadOnly=0;
			//DisplayMessage("Movie is now read+write.");
		}
		else 
		{
			Movie.ReadOnly=1;
			//DisplayMessage("Movie is now read only.");
		}
	}*/
}

//////////////////////////////////////////////////////////////////////////////

void StopMovie(void) {
/*
	if(Movie.Status == Recording && RecordingFileOpened) {
		WriteHeader(Movie.fp);
		fclose(Movie.fp);
		RecordingFileOpened=0;
		Movie.Status = Stopped;
		ClearInput();
		strcpy(MovieStatus, "Recording Stopped");
	}

	if(Movie.Status == Playback && PlaybackFileOpened && Movie.ReadOnly != 0) {
		fclose(Movie.fp);
		PlaybackFileOpened=0;
		Movie.Status = Stopped;
		ClearInput();
		strcpy(MovieStatus, "Playback Stopped");
	}*/
}

//////////////////////////////////////////////////////////////////////////////

int SaveMovie(const char *filename) {

	/*char* str=malloc(1024);

	if(Movie.Status == Playback)
		StopMovie();

	if ((Movie.fp = fopen(filename, "w+b")) == NULL)
	{
		free(str);
		return -1;
	}

	strcpy(str, filename);
	Movie.filename=str;
	RecordingFileOpened=1;
	framecounter=0;
	Movie.Status=Recording;
	strcpy(MovieStatus, "Recording Started");
	Movie.Rerecords=0;
	WriteHeader(Movie.fp);
	YabauseReset();*/
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int PlayMovie(const char *filename) {
/*
	char* str=malloc(1024);

	if(Movie.Status == Recording)
		StopMovie();


	if ((Movie.fp = fopen(filename, "r+b")) == NULL)
	{
		free(str);
		return -1;
	}

	strcpy(str, filename);
	Movie.filename=str;
	PlaybackFileOpened=1;
	framecounter=0;
	Movie.ReadOnly = 1;
	Movie.Status=Playback;
	Movie.Size = MovieGetSize(Movie.fp);
	strcpy(MovieStatus, "Playback Started");
	ReadHeader(Movie.fp);
	YabauseReset();*/
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SaveMovieInState(FILE* fp, IOCheck_struct check) {
/*
	struct MovieBufferStruct tempbuffer;

	fseek(fp, 0, SEEK_END);

	if(Movie.Status == Recording || Movie.Status == Playback) {
		tempbuffer=ReadMovieIntoABuffer(Movie.fp);

		fwrite(&tempbuffer.size, 4, 1, fp);
		fwrite(tempbuffer.data, tempbuffer.size, 1, fp);
	}*/
}

//////////////////////////////////////////////////////////////////////////////

void MovieReadState(FILE* fp) {
/*
	ReadMovieInState(fp);
	MovieLoadState();//file pointer and truncation
*/
}

void ReadMovieInState(FILE* fp) {
/*
	struct MovieBufferStruct tempbuffer;
	int fpos;
   size_t num_read = 0;

	//overwrite the main movie on disk if we are recording or read+write playback
	if(Movie.Status == Recording || (Movie.Status == Playback && Movie.ReadOnly == 0)) {

		fpos=ftell(fp);//where we are in the savestate

      if (fpos < 0)
      {
         YabSetError(YAB_ERR_OTHER, "ReadMovieInState fpos is negative");
         return;
      }

      num_read = fread(&tempbuffer.size, 4, 1, fp);//size
		if ((tempbuffer.data = (char *)malloc(tempbuffer.size)) == NULL)
		{
			return;
		}
      num_read = fread(tempbuffer.data, 1, tempbuffer.size, fp);//movie
		fseek(fp, fpos, SEEK_SET);//reset savestate position

		rewind(Movie.fp);
		fwrite(tempbuffer.data, 1, tempbuffer.size, Movie.fp);
		rewind(Movie.fp);
	}*/
}

//////////////////////////////////////////////////////////////////////////////
