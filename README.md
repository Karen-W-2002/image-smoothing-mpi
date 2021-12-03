# Parallel Image Smoothing
Program written in C++ with the use of MPI

### Description 
Image smoothing of a bmp file using parallelisation. This image is smoothed over 1000 times and parallelisation will allow this image to be processed faster. The image smoothing is done by averaging the pixels around the current pixel, and parallelisation is done mostly through MPI's scatter and gather (vector in this case).

### Program Functions
- **main(int argc, char \*argv[])**
  - Main function of the program

- **int readBMP(char \*fileName)**
  - Reads the BMP file (input.bmp) and saves into \*\*BMPReadData

- **int saveBMP(char \*fileName)**
  - Saves the BMP file from \*\* BMPSaveData and stores into BMP file (output.bmp)

- **void swap(RGBTRIPLE \*a, RGBTRIPLE \*b)**
  - Exchange pixel data with temp storge indicators

- **RGBTRIPLE \*\*alloc_memory(int Y, int X)**
  - Dynmically allocates memory

- **void update_data(int my_rank, int comm_sz, RGBTRIPLE \*\*TempTopData, RGBTRIPLE \*\*TempBottomData, RGBTRIPLE \*\*BMPSaveData, BMPINFO bmpInfo, MPI_Datatype MPI_RGBTRIPLE, MPI_Status \*status, int newHeight)**
  - Updates each processor's data with other processors border datas 

### Sections of The Program
*The program can be organized into different sections*
- Initialization of the variable and functions
- Processor 0 reads the file
- Processor 0 broadcasts the height and width information to the rest 
- All processors dynmically allocate memory to temp storage space
- Create a new MPI_Datatype for RGBTRIPLE
- Calculation for scatter
- Process 0 scatters parts of the image to every other processor
- Each processor needs data information from their neighboring processor, so they send and recieve
- Smoothing operations begin
- Process 0 gathers all the information into one array
- Processor 0 saves the file to output.bmp

### List of MPI Functions Used
- MPI_Init()
- MPI_Status()
- MPI_Comm_rank()
- MPI_Comm_Size()
- MPI_Wtime()
- MPI_Send()
- MPI_Recv()
- MPI_Finalize()
- MPI_Barrier()
- MPI_Scatterv()
- MPI_Gatherv()

### Compilation
`mpiicc -no-multibyte-chars -o image_smoothing.out image_smoothing.cpp`
### Execution
`mpiexec -n (# of processors) image_smoothing.out`
## Results
![Image of a graph](https://github.com/Karen-W-2002/image-smoothing-mpi/blob/main/graph.png)

### Analysis on the results
The change is not too significant compared to the other parallel programs, however it is very significant for the first processor to the 4th, in itself. This might be because of how many times each processor needs to send and recieve from eachother to update their data

But the negative slope does mean that the time has definitely increased because of parallelisation

### My thoughts
For parallelizing the image smoothing program, the most tedious part I had to do was debugging a part because I did a calculation wrong for the displacement. However, that was not a difficult part at all.

The most difficult part of this was every loop, every single processor has to pass a part of their program to their neighbors so they can perform smoothing with a newly updated data. I am worried that this can result in making the program much slower than it is supposed to be. Also makes the code a bit messier.
