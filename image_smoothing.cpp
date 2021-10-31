#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

// # of smoothing operations
#define NSmooth 1000

BMPHEADER bmpHeader;
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;
RGBTRIPLE **BMPData = NULL;
RGBTRIPLE **BMPReadData = NULL;

RGBTRIPLE **tempTopData = NULL;
RGBTRIPLE **tempBottomData = NULL;

int readBMP( char *fileName); //read file
int saveBMP( char *fileName); //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X ); //allocate memory

int main(int argc,char *argv[])
{
	char *infileName = "input.bmp";
        char *outfileName = "output.bmp";
	double startwtime = 0.0, endwtime=0;

	int my_rank, comm_sz;
	int i, j, count;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Status status;

	// Processor 0 starts time
	if(my_rank == 0)
		startwtime = MPI_Wtime();

	// Processor 0 reads the file
	if(my_rank == 0)
	{
        	if (readBMP(infileName))
                	cout << "Read file successfully!!" << endl;
       		else
         	       cout << "Read file fails!!" << endl;
	}

	MPI_Bcast(&bmpInfo.biHeight, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&bmpInfo.biWidth, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// Dynamically allocate memory to temporary storage space
	tempTopData = alloc_memory(1, bmpInfo.biWidth);
	tempBottomData = alloc_memory(1, bmpInfo.biWidth);
	BMPData = alloc_memory(bmpInfo.biHeight, bmpInfo.biWidth);
	BMPSaveData = alloc_memory(bmpInfo.biHeight, bmpInfo.biWidth);
	if(my_rank != 0) 
		BMPReadData = alloc_memory(bmpInfo.biHeight, bmpInfo.biWidth);

	// Create MPI_Datatype for struct
	const int items = 3;
	int blocklengths[3] = {1, 1, 1};
	MPI_Datatype types[3] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
	MPI_Datatype MPI_RGBTRIPLE;
	MPI_Aint offsets[3];
	
	offsets[0] = offsetof(tagRGBTRIPLE, rgbBlue);
	offsets[1] = offsetof(tagRGBTRIPLE, rgbGreen);
	offsets[2] = offsetof(tagRGBTRIPLE, rgbRed);

	MPI_Type_create_struct(items, blocklengths, offsets, types, &MPI_RGBTRIPLE);
	MPI_Type_commit(&MPI_RGBTRIPLE);

	// Some information
	int slice = (bmpInfo.biHeight * bmpInfo.biWidth) / comm_sz;
	int *displ = (int*)malloc(sizeof(int) * comm_sz);
	int *sendcounts = (int*)malloc(sizeof(int) * comm_sz);
	
	for(i = 0; i < comm_sz; i++)
	{
		sendcounts[i] = slice;
		displ[i] = slice * i;
	}
	// Process 0 scatters the data
	MPI_Scatterv(*BMPReadData, sendcounts, displ, MPI_RGBTRIPLE, *BMPSaveData, slice, MPI_RGBTRIPLE, 0, MPI_COMM_WORLD);
	int newHeight = bmpInfo.biHeight/comm_sz;

	// Each process gets their data of the other processors
	if(comm_sz > 1)
	{
		if(my_rank == 0 && comm_sz)
		{
			if(comm_sz > 2) 
			{
				MPI_Send(BMPSaveData[0], bmpInfo.biWidth, MPI_RGBTRIPLE, comm_sz - 1, 0, MPI_COMM_WORLD);
				MPI_Send(BMPSaveData[newHeight-1], bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank + 1, 0, MPI_COMM_WORLD);
			} else
			{
				MPI_Send(BMPSaveData[newHeight-1], bmpInfo.biWidth, MPI_RGBTRIPLE, comm_sz - 1, 0, MPI_COMM_WORLD);
				MPI_Send(BMPSaveData[0], bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank + 1, 0, MPI_COMM_WORLD);
			}
			MPI_Recv(*tempTopData, bmpInfo.biWidth, MPI_RGBTRIPLE, comm_sz - 1, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(*tempBottomData, bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank + 1, 0, MPI_COMM_WORLD, &status);
		}
		else if(my_rank == comm_sz - 1)
		{
			if(comm_sz > 2)
			{
				MPI_Send(BMPSaveData[0], bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank - 1, 0, MPI_COMM_WORLD);
				MPI_Send(BMPSaveData[newHeight-1], bmpInfo.biWidth, MPI_RGBTRIPLE, 0, 0, MPI_COMM_WORLD);
			} else
			{
				MPI_Send(BMPSaveData[newHeight-1], bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank - 1, 0, MPI_COMM_WORLD);
				MPI_Send(BMPSaveData[0], bmpInfo.biWidth, MPI_RGBTRIPLE, 0, 0, MPI_COMM_WORLD);
			}
			MPI_Recv(*tempTopData, bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank - 1, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(*tempBottomData, bmpInfo.biWidth, MPI_RGBTRIPLE, 0, 0, MPI_COMM_WORLD, &status);
		}
		else 
		{
			MPI_Send(BMPSaveData[0], bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank - 1, 0, MPI_COMM_WORLD);
			MPI_Send(BMPSaveData[newHeight-1], bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank + 1, 0, MPI_COMM_WORLD);
			MPI_Recv(*tempTopData, bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank - 1, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(*tempBottomData, bmpInfo.biWidth, MPI_RGBTRIPLE, my_rank + 1, 0, MPI_COMM_WORLD, &status);
		}
	}

        // Smoothing operations
	for(count = 0; count < NSmooth; count ++){
		// exchange pixel data with temporary storage indicators
		swap(BMPSaveData,BMPData);
			
		// the smoothing operation
		for(i = 0; i < newHeight; i++)
		{
			for(j = 0; j < bmpInfo.biWidth; j++) 
			{
				// sets the directional position of the pixels
				int Top = i>0 ? i-1 : newHeight-1;
				int Down = i<newHeight-1 ? i+1 : 0;
				int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;

				// Average on the pixels (all directions), then rounds up
				if(i == 0)
				{
					// top, i = 0
					BMPSaveData[i][j].rgbBlue = (double) (BMPData[i][j].rgbBlue+tempTopData[0][j].rgbBlue+tempTopData[0][Left].rgbBlue+tempTopData[0][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;
					BMPSaveData[i][j].rgbGreen = (double) (BMPData[i][j].rgbGreen+tempTopData[0][j].rgbGreen+tempTopData[0][Left].rgbGreen+tempTopData[0][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
					BMPSaveData[i][j].rgbRed = (double) (BMPData[i][j].rgbRed+tempTopData[0][j].rgbRed+tempTopData[0][Left].rgbRed+tempTopData[0][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;
				
				}
				else if(i == (newHeight -1))
				{
					// bottom, i = newHeight - 1		
					BMPSaveData[i][j].rgbBlue = (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+tempBottomData[0][j].rgbBlue+tempBottomData[0][Left].rgbBlue+tempBottomData[0][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;	
					BMPSaveData[i][j].rgbGreen = (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+tempBottomData[0][j].rgbGreen+tempBottomData[0][Left].rgbGreen+tempBottomData[0][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
					BMPSaveData[i][j].rgbRed = (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+tempBottomData[0][j].rgbRed+tempBottomData[0][Left].rgbRed+tempBottomData[0][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;

				}
				else
				{
					BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;
					BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
					BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;	
				}
			}
		}	
	}

	MPI_Barrier(MPI_COMM_WORLD);
	// Processor 0 gathers the data
	MPI_Gatherv(*BMPSaveData, slice, MPI_RGBTRIPLE, *BMPSaveData, sendcounts, displ, MPI_RGBTRIPLE, 0, MPI_COMM_WORLD);

 	// Processor 0 saves the file
	if(my_rank == 0)
	{
        	if (saveBMP(outfileName))
                	cout << "Save file successfully!!" << endl;
        	else
                	cout << "Save file fails!!" << endl;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	// Processor 0 prints the execution time
	if(my_rank == 0)
	{
        	endwtime = MPI_Wtime();
    		cout << "The execution time = "<< endwtime-startwtime <<endl ;
	}

	free(sendcounts);
	free(displ);
	free(BMPData[0]);
 	free(BMPSaveData[0]);
	free(BMPReadData[0]);
 	free(BMPData);
 	free(BMPSaveData);	
	free(BMPReadData);
	free(tempTopData);
	free(tempBottomData);
 	MPI_Finalize();

    return 0;
}

int readBMP(char *fileName)
{
	// creates input file object
        ifstream bmpFile( fileName, ios::in | ios::binary );

        // file error
        if (!bmpFile){
                cout << "It can't open file!!" << endl;
                return 0;
        }

        // reads the header data of the BMP image file
    	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

        // determines whether it is a BMP file
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }

        // reads the information on BMP
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

        // checks whether bit depth is 24 bits
        if ( bmpInfo.biBitCount != 24 ){
                cout << "The file is not 24 bits!!" << endl;
                return 0;
        }

        // Corrects the width of the picture to be a multiple of 4
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        // Dynamically allocates memory
	BMPReadData = alloc_memory(bmpInfo.biHeight, bmpInfo.biWidth);

        // Read pixel data
	bmpFile.read( (char* )BMPReadData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

        // Close file
        bmpFile.close();

        return 1;
}

int saveBMP( char *fileName)
{
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }

        ofstream newFile( fileName,  ios:: out | ios::binary );

        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }

        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        newFile.close();

        return 1;

}

RGBTRIPLE **alloc_memory(int Y, int X )
{
        RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	    RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
        memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
        memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

        for( int i = 0; i < Y; i++){
                temp[ i ] = &temp2[i*X];
        }

        return temp;
}

void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}
