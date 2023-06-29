#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <sstream>
#include <iostream>
#include "hw2_output.h"
#include "main.h"

using namespace std;

unsigned row1;
unsigned col1;
unsigned row2;
unsigned col2;

unsigned* finishedCols; // size = col2, used for counting how much of a column is finished being computing

int* A; // size = row1 * col1
int* B; // size = row1 * col1
int* C; // size = row2 * col2
int* D; // size = row2 * col2

int* J; // size = row1 * col1, J = A + B
int* L; // size = row2 * col2, L = C + D

int* K; // size = row1 * col2, K = J * L

unsigned* iterter; // used for passing the values for threads. size = max(row1, col1, row2, col2), iterter[i] = i.

sem_t* semJ; // size = row1, used to lock access to rows of J
sem_t* semL; // size = col2, used to lock access to cols of L

sem_t semCol; // used to lock access to finishedCols.

pthread_t* tJ; // size = row1, threads for computing J
pthread_t* tL; // size = row2, threads for computing L
pthread_t* tK; // size = row1, threads for computing K


int getA(unsigned row, unsigned col){ return *(A + row * col1 + col); } 
int getB(unsigned row, unsigned col){ return B[row*col1 + col]; }
int getC(unsigned row, unsigned col){ return C[row*col2 + col]; }
int getD(unsigned row, unsigned col){ return D[row*col2 + col]; }
int getJ(unsigned row, unsigned col){ return J[row*col1 + col]; }
int getK(unsigned row, unsigned col){ return K[row*col2 + col]; }
int getL(unsigned row, unsigned col){ return L[row*col2 + col]; }

void setJ(unsigned row, unsigned col, int val){
	J[row * col1 + col] = val;
	hw2_write_output(0, row + 1, col + 1, val);
}

void setK(unsigned row, unsigned col, int val){
	K[row * col2 + col] = val;
	hw2_write_output(2, row + 1, col + 1, val);
}

void setL(unsigned row, unsigned col, int val){
	L[row * col2 + col] = val;
	hw2_write_output(1, row + 1, col + 1, val);
}

void *sum1(void* r){
	unsigned row = *(unsigned*)r;
	for(unsigned i = 0; i < col1; i++){
		setJ(row, i, getA(row, i) + getB(row, i));
	}
	sem_post(semJ + row);
	
	pthread_exit(NULL);
}

void *sum2(void* r){
	unsigned row = *(unsigned*)r;

	for(unsigned i = 0; i < col2; i++){
		setL(row, i, getC(row, i) + getD(row, i));
		sem_wait(&semCol);
		finishedCols[i]++;
		if(finishedCols[i] == row2){
			sem_post(semL + i);
		}
		sem_post(&semCol);
	}
	pthread_exit(NULL);
}

void *mult(void* r){
	unsigned row = *(unsigned*)r;
	sem_wait(semJ + row);
	
	for(unsigned i = 0; i < col2; i++){
		sem_wait(semL + i);
		sem_post(semL + i);
		int sum = 0;
		for(unsigned j = 0; j < row2; j++){
			sum += getJ(row, j) * getL(j, i);
			
		}
		setK(row, i, sum);
		
		
	}
	pthread_exit(NULL);
}

void readMatrix(int* arr, unsigned int row, unsigned int col) {
	string line;
	for (unsigned i = 0; i < row; i++) {
		getline(cin, line);
		stringstream str(line);
		for (unsigned j = 0; j < col; j++) {
			str >> arr[i * col + j];
		}
	}
}

int main(){
	hw2_init_output(); //Initialize the timer.

	//Start reading A
	string line;
    getline(cin, line);
	stringstream str1(line);
	str1 >> row1 >> col1;
	A = new int[row1 * col1];
	readMatrix(A, row1, col1);

	//Start reading B
	getline(cin, line);
	stringstream str2(line);
	str2 >> row1 >> col1;
	B = new int[row1 * col1];
	readMatrix(B, row1, col1);

	//Start reading C
	getline(cin, line);
	stringstream str3(line);
	str3 >> row2 >> col2;
	C = new int[row2 * col2];
	readMatrix(C, row2, col2);

	//Start reading D
	getline(cin, line);
	stringstream str4(line);
	str4 >> row2 >> col2;
	D = new int[row2 * col2];
	readMatrix(D, row2, col2);


	// Compute iterter
	unsigned m = max(row1, row2);
	iterter = new unsigned int[m];
	for(unsigned i = 0; i < m; i++) *(iterter + i) = i;


	// Initialize J, K, L
	J = new int[row1 * col1];
	L = new int[row2 * col2];
	
	K = new int[row1 * col2];
	
	// Initialize semaphores.

	finishedCols = new unsigned[col2];
	for(unsigned i = 0; i < col2; i++) finishedCols[i] = 0;
	semJ = new sem_t[row1];
	semL = new sem_t[col2];
	
	sem_init(&semCol, 0, 1);
	
	for(unsigned i = 0; i < row1; i++) sem_init(semJ + i, 0, 0);
	for(unsigned i = 0; i < col2; i++) sem_init(semL + i, 0, 0);

	
	// Create threads 
	tJ = new pthread_t[row1];
	tK = new pthread_t[row1];
	tL = new pthread_t[row2];

	for(unsigned i = 0; i < row1; i++) pthread_create(tJ + i, NULL, sum1, iterter + i);
	for(unsigned i = 0; i < row2; i++) pthread_create(tL + i, NULL, sum2, iterter + i);
	for(unsigned i = 0; i < row1; i++) pthread_create(tK + i, NULL, mult, iterter + i);

	// Join threads
	for(unsigned i = 0; i < row1; i++) pthread_join(tJ[i], NULL);
	for(unsigned i = 0; i < row2; i++) pthread_join(tL[i], NULL);
	for(unsigned i = 0; i < row1; i++) pthread_join(tK[i], NULL);
	
	// Destroy semaphores.
	for(unsigned i = 0; i < row1; i++) sem_destroy(semJ + i);
	for(unsigned i = 0; i < col2; i++) sem_destroy(semL + i);
	
	sem_destroy(&semCol);
	
	// Print the final calculation
	for(unsigned i = 0; i < row1; i++){
		for(unsigned j = 0; j < col2; j++){
			cout << getK(i, j) << " ";
		}
		cout << endl;
	}
	
	// Delete Arrays
	delete[] A;
	delete[] B;
	delete[] C;
	delete[] D;
	delete[] J;
	delete[] K;
	delete[] L;
	delete[] tJ;
	delete[] tK;
	delete[] tL;
	delete[] iterter;
	delete[] semJ;
	delete[] semL;
}