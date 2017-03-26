#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CITIES 300
#define CITY_NAME_LENGTH 3
#define PRICE(cityFrom, day, cityTo) (cityFrom) * MAX_CITIES * MAX_CITIES + (day) * MAX_CITIES + (cityTo)

typedef struct {
	unsigned short cityCount;
	unsigned short *prices;
	char cities[MAX_CITIES][CITY_NAME_LENGTH];
	unsigned short startCityId;
} GlobalData;

typedef struct {
	unsigned short cityId;
	unsigned short price;
} TripPair;

typedef struct {
	GlobalData *globalData;
	TripPair *pairs;
	unsigned short length;
	unsigned long totalPrice;
	unsigned long time;
} Trip;

typedef struct {
	Trip *trip;
	unsigned short length;
	unsigned short *days;
} SubTrip;

typedef struct {
	Trip *trip;                       // readonly - puvodni trip
	unsigned short *dayMap;            // readonly - seznam dnu ve kterych permutujeme
	unsigned short *bestPermutation;   // aktualne nejlepsi nalezena permutace - meni se pri rekurzi
	unsigned long bestPermutationPrice;     // aktualne nejlepsi cena menene casti tripu
} PermutationData;

typedef struct {
	Trip *trip;
	Trip *sourceTrip;
	unsigned char permutationSize;
	bool newData;
} WorkerData;

unsigned short loadGlobalData(FILE *fd, GlobalData *globalData);
unsigned char initializeTrip(Trip * trip);
bool firstPermutationOptimize(Trip *trip, unsigned char permutationSize, unsigned short *changedDays);
bool sequencePermutationForDay(Trip *trip, unsigned char permutationSize, unsigned short dayFrom, unsigned short dayTo, unsigned short *changedDays);
unsigned long getSubTripPrice(SubTrip *subTrip);
void updateTripBySubtrip(SubTrip *subTrip, unsigned short *bestPermutation, unsigned short *changedDays);
bool changedDayOptimize(Trip *trip, unsigned char permutationSize, unsigned short *changedDays);
bool tryAllPairs(Trip *trip, unsigned char permutationSize, unsigned short *changedDays);
void optimizeTrip(Trip *trip, unsigned char permutationSize);
void reoptimizeTrip(Trip *trip, unsigned char permutationSize, unsigned short * changedDays);
void copyTrip(Trip *destinationTrip, Trip *sourceTrip);
void shuffleTrip(Trip *trip, unsigned char permutationSize, unsigned short * changedDays);
void *threadWorker(void *threadarg);
void subTripPermuteSwap(unsigned short *x, unsigned short *y);
void subTripPermute(unsigned short *subTrip, unsigned char l, unsigned char r, PermutationData *data);
bool validateTrip(Trip *trip);
void printTrip(Trip *trip);
unsigned short getNumOfThreads();
static unsigned long timer(void);
void winStall();