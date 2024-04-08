/**
*Using a similar concept as question 2, you will now crack passwords using CUDA. As a kernel function 
*cannot use the crypt library, you will be given an encryption function instead which will 
*generate a password for you.  Your program will take in an encrypted password and decrypt it 
*using many threads on the GPU. CUDA allows multidimensional thread configurations so your kernel function 
*(which runs on the GPU) will need to be modified according to how you call your function. 
*
*Creatd by Nirmal Abeykoon Mudiynaslegae - 1811342
*6CS005 - High Performance Computing. 

***********************************************************************************************
					How to Run
*Compile as Normal nvcc - nvcc Password_cracking_cuda2.0.cu


*To run(need for command line arguments [blockx,blocky,threadx,thready] in this oder)
*this works with any number of threads upto 67600
*max blockx and blocky values are 26
*max Threadx and Thready values are 10
*there are some pre encrypted passwords in main that user can uncommnet to test the program

*eg:- ./a.out 26 26 10 10

***********************************************************************************************


*/
// includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Crypt a password
__device__ 
char* CudaCrypt(char* rawPassword){

	char * newPassword = (char *) malloc(sizeof(char) * 11);

	newPassword[0] = rawPassword[0] + 2;
	newPassword[1] = rawPassword[0] - 2;
	newPassword[2] = rawPassword[0] + 1;
	newPassword[3] = rawPassword[1] + 3;
	newPassword[4] = rawPassword[1] - 3;
	newPassword[5] = rawPassword[1] - 1;
	newPassword[6] = rawPassword[2] + 2;
	newPassword[7] = rawPassword[2] - 2;
	newPassword[8] = rawPassword[3] + 4;
	newPassword[9] = rawPassword[3] - 4;
	newPassword[10] = '\0';

	for(int i =0; i<10; i++){
		if(i >= 0 && i < 6){ //checking all lower case letter limits
			if(newPassword[i] > 122){
				newPassword[i] = (newPassword[i] - 122) + 97;
			}else if(newPassword[i] < 97){
				newPassword[i] = (97 - newPassword[i]) + 97;
			}
		}else{ //checking number section
			if(newPassword[i] > 57){
				newPassword[i] = (newPassword[i] - 57) + 48;
			}else if(newPassword[i] < 48){
				newPassword[i] = (48 - newPassword[i]) + 48;
			}
		}
	}
	return newPassword;
}

// cheack if two strings are matching
__device__ 
int isMatching(char* charOne, char* charTwo, int length) {
	int result = 1;
	for (int i = 0; i < length; i++) {
		if (charOne[i] != charTwo[i]) {
			result = 0;
			break;
		}
	}
	return result;
}

//GPU kernal
__global__ 
void crack(char * cryptPassword , char * GpuPPass){

	//close thread if password is already found
	if(*GpuPPass!= NULL){
		return;
	}
	
	int combos = 26*26*100;
	long threadCount = gridDim.x*gridDim.y*blockDim.x*blockDim.y;
	int remainder = combos % threadCount;
	int start;
	int end;
	
	int newCombos = combos-remainder;
	long workperthread = newCombos/threadCount;
	
	//create unique 2D thread ID
	int blockId = blockIdx.x+blockIdx.y*gridDim.x;
	int threadId = blockId*(blockDim.x*blockDim.y)+(threadIdx.y*blockDim.x)+threadIdx.x;
	
	//create unique start and end point for each thread
	if (threadId == 0){
		start = 0;
		
	}else{
		start = threadId*workperthread;
		
	}
	if (threadId == (threadCount-1)){
		end = combos;
		//printf("ThreadID = %d Start=%d End=%d\n",threadId,start,end);
		//printf("%d\n",end);
	}else{
		end = (threadId*workperthread)+workperthread;
	}
	
	//printf("ThreadID = %d Start=%d End=%d\n",threadId,start,end);
	for(int i=start; i<end; i++){
		char  plain[5];
		char * TempcryptPassword;
		
		//close thread if password is already found
		if(*GpuPPass!= NULL){
			return;
		}
		
		//create unique password matching to uniqe thread number and workload
		int letterAndNumber = 26*100;
		int firstIndex = i/letterAndNumber;
		char firstChar = (char)( firstIndex + 'a');
		
		int secondAlphabetIndex = (i / 100) % 26;
		char secondChar = (char) (secondAlphabetIndex + 'a');
		int intNumbers= i % 100;
		char numberOne = intNumbers/10+'0';
		char numberTwo = intNumbers%10+'0';
		
		//printf("%c\n",(intNumbers%10+'0'));

		
		plain[0] = firstChar;
		plain[1] = secondChar;
		plain[2] = numberOne ;
		plain[3] = numberTwo;
			
		//temp crypt password
		//printf("Cheacked PassWord--->%s\n", plain);
		TempcryptPassword = CudaCrypt(plain);
		
		//if password matches save the results
		if ( isMatching(cryptPassword, TempcryptPassword, 11) > 0 )
		{
			//printf("GPU found the password PassWord--->%s\n", plain);
			for(int i=0; i < 4; i++){
				GpuPPass[i]=plain[i];	
			}
			
			//close thread if password is already found
			if(*GpuPPass!= NULL){
				return;
			}
		}
		
	}

}

//main
int main(int argc, char ** argv){

	char* CpuPPass;

	/*Some passwords for testing*/
	//const char cryptPassword [] = "ccbddb2244"; //aa00
	//const char cryptPassword [] = "cxbdwy2745"; //zz99
	const char cryptPassword [] = "plosmo2723"; //np97
	//const char cryptPassword [] = "hdgwqs7380"; //ft54
	
	if (argc < 5) {
		printf("Usage: %s need 4 args\n",argv[0]);
		exit(-1);
	}
	
	
	//thread and block cound from user
	int blockx = atoll(argv[1]);
	int blocky = atoll(argv[2]);
	int threadx = atoll(argv[3]);
	int thready = atoll(argv[4]);
		
	if ((blockx > 26) || (blocky > 26) ){
		printf("Usage: Blockx and Blocky values Must be less than 26\n");
		exit(-1);
	}
	
	if ((threadx > 10) || (thready > 10) ){
		printf("Usage: Threadx and Thready values Must be less than 10\n");
		exit(-1);
	}

	
	char * gpuCryptPassword;
	cudaMalloc((void**)&gpuCryptPassword,sizeof(cryptPassword));
	cudaMemcpy(gpuCryptPassword,cryptPassword,sizeof(cryptPassword),cudaMemcpyHostToDevice);
	
	char *GpuPPass;
	cudaMalloc((void**)&GpuPPass, sizeof(char)*5);

	crack<<< dim3(blockx,blocky,1), dim3(threadx,thready,1) >>>(gpuCryptPassword,GpuPPass);
	cudaDeviceSynchronize();
	
	// Copy password from device to host
	CpuPPass = (char*)malloc( sizeof(char) * 5 );
	cudaMemcpy(CpuPPass, GpuPPass, sizeof(char)*5, cudaMemcpyDeviceToHost);
	
	
	//print if password was found
	if (CpuPPass != NULL && CpuPPass[0] != 0) {
		printf("Encrypted PassWord--->%s\n", cryptPassword);
		printf("Decrypted PassWord--->%s\n", CpuPPass);
	} else {
		printf("Unable to find the password.\n");
	}
	
	
	//free memory
	cudaFree(gpuCryptPassword);
	cudaFree(GpuPPass);
	free(CpuPPass);
	return 0;
}
