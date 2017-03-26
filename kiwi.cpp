/**
 * Unsubmited solution for travellingsalesman.cz contest
 * Probably not best solution in contest, but this is first version and with competition in mind.
 *
 * @author Michal Horák horakmx@gmail.com
 * @version 0.1 - 2017-03-26 19:33 CEST
 * @licence open
 *
 * how to compile: # gcc -c -O3 kiwi.cpp && gcc -pthread -o kiwi -O3 *.o
 * how to run:     # cat data_300.txt | ./kiwi
 *
 * @todo: 1. Make make file.
 * @todo: 2. pthreads are done, now make windows threads happen. :)
 * @todo: 3. Fix small memory leakage.
 * @todo: 4. Add some heuritics into shuffler!!
 * @todo: 5. Test more data sets (only largest data set was tested).
 * @todo: 6. Cheapest first function with backtracking for missing paths.
 * @todo: 7. ???
 * @todo: 8. profit
 */
#include "kiwi.h"

// Time limit defined by travellingsalesman.cz contest (30 s) 
#define TIME_LIMIT 29700
// DEBUG = false ... output is usable for travellingsalesman.cz
// DEBUG = true  ... info-stream output - usable for online watching of current best solution found
#define DEBUG false

int main(int argc, char *argv[]) {
    unsigned short numOfThreads = getNumOfThreads();
    unsigned long time = timer();
    FILE *fd;
    GlobalData *globalData = (GlobalData *)malloc(sizeof(GlobalData));
    Trip *trip = (Trip *)malloc(sizeof(Trip));

    if (argc > 1) {
        fd = fopen(argv[1], "r");
        if (!fd) {
            printf("Unable to open '%s' for input!\n", argv[1]);
            return(1);
        }
    }
    else fd = stdin;

    // STEP #0 loading Global Data
    if (!loadGlobalData(fd, globalData)) {
        printf("ERROR - No input data\n");
        return 1;
    }
    trip->globalData = globalData;
    trip->time = time;
    // STEP #1 initialize first Hamiltonian path
    if (!initializeTrip(trip)) {
        printf("ERROR - Unable to initialize trip\n");
        return 2;
    }
    // STEP #2 optimize moving sequence size 6 and swap every 2
    unsigned long a = timer();
    unsigned char permutationSize = 6; // OPTIMAL VALUE 6
    optimizeTrip(trip, permutationSize);
    if (DEBUG) printf("Price after first optimization = %lu\n", trip->totalPrice);
    // STEP #3 shuffling and reoptimizing -- breaking local optimum
    // prepare multiple alternative trips
    Trip *trips[100];
    for (int i = 0; i < 100; i++) {
        trips[i] = (Trip *)malloc(sizeof(Trip));
    }
    srand(timer());
    int besTrip;

    if (DEBUG) printf("Spawning %d threads.\n", numOfThreads);
    WorkerData *workerData = (WorkerData *)malloc(sizeof(WorkerData) * numOfThreads);
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * numOfThreads);
    int i;
    for (i = 0; i < numOfThreads; i++) {
        workerData[i].newData = false;
        workerData[i].trip = trips[i];
        workerData[i].sourceTrip = trip;
        workerData[i].permutationSize = permutationSize;
        pthread_create(&threads[i], NULL, threadWorker, (void *)&workerData[i]);
    }
    //pthread_t *pth = (pthread_t *)malloc(sizeof(pthread_t) * 4);
    while (1) {
        if (timer() - trip->time > TIME_LIMIT) {
            break;
        }
        besTrip = -1;
        unsigned long bestAlternativePrice = trip->totalPrice;
        for (i = 0; i < numOfThreads; i++) {
            workerData[i].newData = true;
        }
        while (1) {
            usleep(10);
            if (timer() - trip->time > TIME_LIMIT) {
                break;
            }
            for (i = 0; i < numOfThreads; i++) {
                if (workerData[i].newData) {
                    break;
                }
            }
            if (i == numOfThreads) {
                break;
            }
        }
        for (i = 0; i < numOfThreads; i++) {
            if (bestAlternativePrice > trips[i]->totalPrice) {
                bestAlternativePrice = trips[i]->totalPrice;
                besTrip = i;
            }
        }
        if (besTrip != -1) {
            if (DEBUG) printf("Best alternative price = %lu .. %lu ms\n", bestAlternativePrice, timer() - a);
            copyTrip(trip, trips[besTrip]);
        }
    }
    if (DEBUG) printf("%lu ms\n", timer() - a);
    if (DEBUG) validateTrip(trip);

    printTrip(trip);
    time = timer() - time;
    if (DEBUG) printf("Total time: %lu", time);
    if (DEBUG) winStall();
    return 0;
}

/**
 * Function for initial loading - reading price list in format "ABC DEF 123 45678\n"
 */
unsigned short loadGlobalData(FILE *fd, GlobalData *globalData) {
    char buffer[20];
    globalData->prices      = (unsigned short *)calloc(MAX_CITIES * MAX_CITIES * MAX_CITIES, sizeof(unsigned short)); // 54 MB RAM
    unsigned short *cityMap = (unsigned short *)calloc(256 * 256 * 256, sizeof(unsigned short)); // 32 MB RAM (every combination of 3 chars)
    unsigned char last;
    unsigned short hit;
    unsigned short cityCount = 0;
    unsigned short cityFromId, cityToId, day, price;
    // reading first row
    if (fgets(buffer, 20, fd) == NULL) {
        return 0;
    }
    cityMap[ (buffer[0] << 16) + (buffer[1] << 8) + buffer[2] ] = ++cityCount;
    globalData->cities[cityCount - 1][0] = buffer[0];
    globalData->cities[cityCount - 1][1] = buffer[1];
    globalData->cities[cityCount - 1][2] = buffer[2];
    globalData->startCityId = cityCount - 1;
    // reading rest of input (2..N rows)
    while (fgets(buffer, 20, fd) != NULL) {
        hit = cityMap[(buffer[0] << 16) + (buffer[1] << 8) + buffer[2]];
        if (hit) {
            cityFromId = hit;
        }
        else {
            cityFromId = ++cityCount;
            cityMap[(buffer[0] << 16) + (buffer[1] << 8) + buffer[2]] = cityCount;
            globalData->cities[cityCount - 1][0] = buffer[0];
            globalData->cities[cityCount - 1][1] = buffer[1];
            globalData->cities[cityCount - 1][2] = buffer[2];
        }
        hit = cityMap[(buffer[4] << 16) + (buffer[5] << 8) + buffer[6]];
        if (hit) {
            cityToId = hit;
        }
        else {
            cityToId = ++cityCount;
            cityMap[(buffer[4] << 16) + (buffer[5] << 8) + buffer[6]] = cityCount;
            globalData->cities[cityCount - 1][0] = buffer[4];
            globalData->cities[cityCount - 1][1] = buffer[5];
            globalData->cities[cityCount - 1][2] = buffer[6];
        }
        day = 0;
        last = 8;
        price = 0;
        while (buffer[last] != ' ') {
            day *= 10;
            day += buffer[last++] - '0';
        }
        last++;
        while (1) {
            if (buffer[last] == '\n' || buffer[last] == ' ') {
                break;
            }
            price *= 10;
            price += buffer[last++] - '0';
        }
        globalData->prices[PRICE(cityFromId - 1, day, cityToId - 1)] = price;
    }
    fclose(fd);
    globalData->cityCount = cityCount;
    return cityCount;
}

/**
 * Cheapest first approach
 * Possible problem - missing backtracking
 */
unsigned char initializeTrip(Trip * trip) {
    unsigned short startCityStep;
    int totalPrice = 0;
    unsigned short currentPrice, minPrice;
    unsigned short bestDestinationId;

    startCityStep = trip->globalData->startCityId;
    char visitedCities[300] = { 0 };
    visitedCities[0] = 1;

    trip->pairs = (TripPair *)malloc(sizeof(TripPair) * (trip->globalData->cityCount + 1));

    for (int currentDay = 0; currentDay < trip->globalData->cityCount; currentDay++) {
        if (currentDay == trip->globalData->cityCount - 1) {
            visitedCities[0] = 0;
        }
        minPrice = -1;
        for (int destinationCityId = 0; destinationCityId < 300; destinationCityId++) {
            if (destinationCityId == startCityStep || visitedCities[destinationCityId]) {
                continue;
            }
            currentPrice = trip->globalData->prices[PRICE(startCityStep, currentDay, destinationCityId)];
            if (minPrice == -1 || (currentPrice != 0 && minPrice > currentPrice)) {
                minPrice = currentPrice;
                bestDestinationId = destinationCityId;
            }
        }
        totalPrice += minPrice;

        trip->pairs[currentDay].cityId = startCityStep;
        trip->pairs[currentDay].price = minPrice;
        visitedCities[bestDestinationId] = 1;
        startCityStep = bestDestinationId;
    }
    trip->pairs[trip->globalData->cityCount].cityId = bestDestinationId;
    trip->length = trip->globalData->cityCount;
    trip->totalPrice = totalPrice;
    return totalPrice;
}

/**
 * Iterate from real end to real start: consecutive sequence of days and try to permute this short sequence into every state
 */
bool firstPermutationOptimize(Trip *trip, unsigned char permutationSize, unsigned short *changedDays) {
    return sequencePermutationForDay(trip, permutationSize, 1, trip->length - permutationSize, changedDays);
}

/**
 * Universal consecutive sequence testing function. Iterates from defined dayTo (end) -> dayFrom (start)
 */
bool sequencePermutationForDay(Trip *trip, unsigned char permutationSize, unsigned short dayFrom, unsigned short dayTo, unsigned short *changedDays) {
    if (dayFrom < 1) {
        dayFrom = 1;
    }
    if (dayTo > trip->length - permutationSize) {
        dayTo = trip->length - permutationSize;
    }
    PermutationData *data = (PermutationData *)malloc(sizeof(PermutationData));
    // one time allocation - per thread
    data->bestPermutation = (unsigned short *)malloc(sizeof(unsigned short) * permutationSize);
    data->dayMap = (unsigned short *)malloc(sizeof(unsigned short) * permutationSize);
    data->trip = trip;
    unsigned short *entryPermutation = (unsigned short *)malloc(sizeof(unsigned short) * permutationSize);
    unsigned long currentSubTripPrice;
    // SubTrip initialization
    SubTrip *subTrip = (SubTrip *)malloc(sizeof(SubTrip));
    subTrip->length = permutationSize;
    subTrip->trip = trip;
    subTrip->days = (unsigned short*)malloc(sizeof(unsigned short) * permutationSize);
    bool change = false;
    for (int x = dayTo; x >= dayFrom; x--) {
        for (int i = 0; i < permutationSize; i++) {
            data->dayMap[i] = i + x;
            subTrip->days[i] = i + x;
            entryPermutation[i] = trip->pairs[i + x].cityId;
        }
        currentSubTripPrice = getSubTripPrice(subTrip);
        data->bestPermutationPrice = currentSubTripPrice;
        subTripPermute(entryPermutation, 0, permutationSize - 1, data);
        if (data->bestPermutationPrice < currentSubTripPrice) {
            updateTripBySubtrip(subTrip, data->bestPermutation, changedDays);
            change = true;
        }
        if (timer() - trip->time > TIME_LIMIT) {
            return change;
        }
    }
    free(subTrip->days);
    free(subTrip);
    free(entryPermutation);
    free(data->dayMap);
    free(data->bestPermutation);
    free(data);
    return change;
}

unsigned long getSubTripPrice(SubTrip *subTrip) {
    unsigned long price = 0;
    unsigned short day;
    for (int i = 0; i < subTrip->length; i++) {
        day = subTrip->days[i];
        price += subTrip->trip->pairs[day - 1].price;
        if (i + 1 == subTrip->length || day + 1 < subTrip->days[i + 1]) {
            price += subTrip->trip->pairs[day].price;
        }
    }
    return price;
}

void updateTripBySubtrip(SubTrip *subTrip, unsigned short *bestPermutation, unsigned short *changedDays) {
    unsigned short day, cityFrom, cityTo;
    unsigned long priceBefore = subTrip->trip->totalPrice;
    for (int i = 0; i < subTrip->length; i++) {
        day = subTrip->days[i];
        cityFrom = subTrip->trip->pairs[day - 1].cityId;
        cityTo   = subTrip->trip->pairs[day].cityId;
        subTrip->trip->totalPrice -= subTrip->trip->pairs[day - 1].price;
        subTrip->trip->pairs[day - 1].price = subTrip->trip->globalData->prices[PRICE(cityFrom, day - 1, bestPermutation[i])];
        subTrip->trip->totalPrice += subTrip->trip->pairs[day - 1].price;
        if (i + 1 == subTrip->length || day + 1 < subTrip->days[i + 1]) {
            cityFrom = subTrip->trip->pairs[day].cityId;
            cityTo = subTrip->trip->pairs[day + 1].cityId;
            subTrip->trip->totalPrice -= subTrip->trip->pairs[day].price;
            subTrip->trip->pairs[day].price = subTrip->trip->globalData->prices[PRICE(bestPermutation[i], day, cityTo)];
            subTrip->trip->totalPrice += subTrip->trip->pairs[day].price;
        }
        // change detected
        if (subTrip->trip->pairs[day].cityId != bestPermutation[i]) {
            subTrip->trip->pairs[day].cityId = bestPermutation[i];
            changedDays[day] = true;
        }
    }
    if (priceBefore < subTrip->trip->totalPrice) {
        printf("WARNING - downgrading trip %lu => %lu\n", priceBefore, subTrip->trip->totalPrice);
    }
}

bool changedDayOptimize(Trip *trip, unsigned char permutationSize, unsigned short *changedDays) {
    bool change = true, hit = false;
    while (change) {
        change = false;
        for (int i = 0; i < trip->length; i++) {
            if (changedDays[i] == true) {
                changedDays[i] = false;
                if (sequencePermutationForDay(trip, permutationSize, i - permutationSize, i, changedDays)) {
                    hit = true;
                    change = true;
                    break;
                }
            }
        }
    }
    return hit;
}

bool tryAllPairs(Trip *trip, unsigned char permutationSize, unsigned short *changedDays) {
    unsigned short i, d, temp;
    unsigned long priceBefore, priceAfter1, priceAfter2, priceAfter3, priceAfter4;
    bool change = false;
    for (i = 1; i < trip->length - permutationSize; i++) {
        for (d = i + permutationSize; d < trip->length; d++) {
            priceAfter1 = trip->globalData->prices[PRICE(trip->pairs[i - 1].cityId, i - 1, trip->pairs[d].cityId)];
            priceAfter2 = trip->globalData->prices[PRICE(trip->pairs[d].cityId, i, trip->pairs[i + 1].cityId)];
            priceAfter3 = trip->globalData->prices[PRICE(trip->pairs[d - 1].cityId, d - 1, trip->pairs[i].cityId)];
            priceAfter4 = trip->globalData->prices[PRICE(trip->pairs[i].cityId, d, trip->pairs[d + 1].cityId)];
            // unpossible swap - missing price(s) for this trip
            if (priceAfter1 == 0 || priceAfter2 == 0 || priceAfter3 == 0 || priceAfter4 == 0) {
                continue;
            }
            priceBefore =
                trip->pairs[i - 1].price +
                trip->pairs[i].price +
                trip->pairs[d - 1].price +
                trip->pairs[d].price;
            if (priceBefore > priceAfter1 + priceAfter2 + priceAfter3 + priceAfter4) {
                trip->pairs[i - 1].price = priceAfter1;
                trip->pairs[i].price     = priceAfter2;
                trip->pairs[d - 1].price = priceAfter3;
                trip->pairs[d].price     = priceAfter4;
                temp = trip->pairs[i].cityId;
                trip->pairs[i].cityId = trip->pairs[d].cityId;
                trip->pairs[d].cityId = temp;
                trip->totalPrice += priceAfter1 + priceAfter2 + priceAfter3 + priceAfter4 - priceBefore;
                changedDays[i] = true;
                changedDays[d] = true;
                change = true;
            }
        }
    }
    return change;
}

void optimizeTrip(Trip *trip, unsigned char permutationSize) {
    unsigned short * changedDays = (unsigned short*)calloc(trip->length, sizeof(unsigned short));
    firstPermutationOptimize(trip, permutationSize, changedDays);
    reoptimizeTrip(trip, permutationSize, changedDays);
    free(changedDays);
}

void reoptimizeTrip(Trip *trip, unsigned char permutationSize, unsigned short * changedDays) {
    bool change = true;
    while (change) {
        change = false;
        if (changedDayOptimize(trip, permutationSize, changedDays)) {
            change = true;
        }
        if (tryAllPairs(trip, permutationSize, changedDays)) {
            change = true;
        }
    }
}

/**
 * Because of nested pointers we need this function to copy not only Trip structure but TripPair inside
 */
void copyTrip(Trip *destinationTrip, Trip *sourceTrip) {
    if (destinationTrip != NULL && destinationTrip->pairs != NULL) {
        // toto: Memory Leak
        //free(destinationTrip->pairs);
    }
    memcpy(destinationTrip, sourceTrip, sizeof(Trip));
    destinationTrip->pairs = (TripPair *)malloc(sizeof(TripPair) * (sourceTrip->globalData->cityCount + 1));
    memcpy(destinationTrip->pairs, sourceTrip->pairs, sizeof(TripPair) * (sourceTrip->globalData->cityCount + 1));
}

/**
 * Simple trip shuffler - swapping few pairs which must be at least 130 % distance size of permutationSize
 * @todo: Add some heuristics better than randomness
 */
void shuffleTrip(Trip *trip, unsigned char permutationSize, unsigned short * changedDays) {
    unsigned short i, a, b, c, temp;
    unsigned long priceBefore, priceAfter1, priceAfter2, priceAfter3, priceAfter4;
    // randomly swap few pairs
    for (i = 0; i < 3; i++) {
        a = (rand() % (trip->length - 1)) + 1;
        b = (rand() % (trip->length - 1)) + 1;
        c = a - b;
        if (c < 0) {
            c *= -1;
        }
        // we are looking only for swapping bigger than 130 % of permutationSize
        if (c < (unsigned char) ((float) permutationSize * 1.3) || a < 1 || b < 1 || a >= trip->length || b >= trip->length) {
            i--;
            continue;
        }
        priceAfter1 = trip->globalData->prices[PRICE(trip->pairs[a - 1].cityId, a - 1, trip->pairs[b].cityId)];
        priceAfter2 = trip->globalData->prices[PRICE(trip->pairs[b].cityId, a, trip->pairs[a + 1].cityId)];
        priceAfter3 = trip->globalData->prices[PRICE(trip->pairs[b - 1].cityId, b - 1, trip->pairs[a].cityId)];
        priceAfter4 = trip->globalData->prices[PRICE(trip->pairs[a].cityId, b, trip->pairs[b + 1].cityId)];
        // unpossible swap - missing price(s) for this trip - try again
        if (priceAfter1 == 0 || priceAfter2 == 0 || priceAfter3 == 0 || priceAfter4 == 0) {
            i--;
            continue;
        }
        changedDays[a] = true;
        changedDays[b] = true;
        priceBefore =
            trip->pairs[a - 1].price +
            trip->pairs[a].price +
            trip->pairs[b - 1].price +
            trip->pairs[b].price;

        trip->pairs[a - 1].price = priceAfter1;
        trip->pairs[a].price = priceAfter2;
        trip->pairs[b - 1].price = priceAfter3;
        trip->pairs[b].price = priceAfter4;
        temp = trip->pairs[a].cityId;
        trip->pairs[a].cityId = trip->pairs[b].cityId;
        trip->pairs[b].cityId = temp;
        trip->totalPrice += priceAfter1 + priceAfter2 + priceAfter3 + priceAfter4 - priceBefore;
    }
}

/**
 * Simple worker waiting for trigger (workeData->newdata = true)
 * After trigger is pulled -> shuffle + reoptimize (parent function will check results)   
 */
void *threadWorker(void *threadarg) {
    WorkerData *workerData = (WorkerData *) threadarg;
    unsigned short * changedDays = (unsigned short*)calloc(workerData->sourceTrip->length, sizeof(unsigned short));
    while (1) {
        usleep(10);
        if (workerData->newData) {
            copyTrip(workerData->trip, workerData->sourceTrip);
            shuffleTrip(workerData->trip, workerData->permutationSize, changedDays);
            reoptimizeTrip(workerData->trip, workerData->permutationSize, changedDays);
            workerData->newData = false;
        }
    }
}

/**
 * Swapping function for subTripPermute
 */
void subTripPermuteSwap(unsigned short *x, unsigned short *y) {
    unsigned short temp;
    temp = *x;
    *x = *y;
    *y = temp;
}

/**
 * Permute subTrip recursively in every possible state
 * For each final permuted state check subTrip price
 * and if better price is found, set it as current best permutation state
 */
void subTripPermute(unsigned short *subTrip, unsigned char l, unsigned char r, PermutationData *data) {
    unsigned char i;
    if (l == r) {
        unsigned long subTripPrice = 0;
        bool firstDayInRow = true;
        unsigned short startCityId, endCityId, day, price;
        for (i = 0; i <= r; i++) {
            day = data->dayMap[i];
              if (firstDayInRow) {
                firstDayInRow = false;
                startCityId = data->trip->pairs[day - 1].cityId;
            }
            else {
                startCityId = subTrip[i-1];
            }
            endCityId = subTrip[i];
            price = data->trip->globalData->prices[PRICE(startCityId, day - 1, endCityId)];
            if (!price) {
                return;
            }
            subTripPrice += price;
            if (i == r || day+1 < data->dayMap[i+1]) {
                firstDayInRow = true;
                unsigned short finalCityId = data->trip->pairs[day + 1].cityId;
                price = data->trip->globalData->prices[PRICE(endCityId, day, finalCityId)];
                if (!price) {
                    return;
                }
                subTripPrice += price;
            }
        }
        if (data->bestPermutationPrice > subTripPrice) {
            data->bestPermutationPrice = subTripPrice;
            memcpy(data->bestPermutation, subTrip, sizeof(unsigned short) * (r + 1));
        }
    }
    else {
        for (i = l; i <= r; i++) {
            subTripPermuteSwap((subTrip + l), (subTrip + i));
            subTripPermute(subTrip, l + 1, r, data);
            subTripPermuteSwap((subTrip + l), (subTrip + i)); //backtrack
        }
    }
}

/**
 * Fast trip validator (validates everything and relatively fast)
 */
bool validateTrip(Trip *trip) {
    unsigned long totalPrice = 0;
    unsigned short price = 0;
    unsigned short bug = 0;
    bool cityExists[MAX_CITIES] = { false };
    if (trip->pairs[0].cityId != trip->globalData->startCityId) {
        bug++;
        printf("Wrong starting city (should be %.3s (%d) and is %.3s (%d))\n",
            trip->globalData->cities[trip->globalData->startCityId],
            trip->globalData->startCityId,
            trip->globalData->cities[trip->pairs[0].cityId],
            trip->pairs[0].cityId
        );
    }
    if (trip->pairs[trip->length].cityId != trip->globalData->startCityId) {
        bug++;
        printf("Wrong final city (should be %.3s (%d) and is %.3s (%d))\n",
            trip->globalData->cities[trip->globalData->startCityId],
            trip->globalData->startCityId,
            trip->globalData->cities[trip->pairs[trip->length].cityId],
            trip->pairs[trip->length].cityId
        );
    }
    for (int i = 0; i < trip->length; i++) {
        if (!cityExists[trip->pairs[i].cityId]) {
            cityExists[trip->pairs[i].cityId] = true;
        }
        else {
            bug++;
            printf("BUG - duplicate city %.3s (%d)\n",
                trip->globalData->cities[trip->pairs[i].cityId],
                trip->pairs[i].cityId
            );
        }
        price = trip->globalData->prices[PRICE(trip->pairs[i].cityId, i, trip->pairs[i + 1].cityId)];
        if (price == 0) {
            bug++;
            printf("BUG - impossible flight @ day %d (%.3s (%d) -> %.3s (%d))\n", i,
                trip->globalData->cities[trip->pairs[i].cityId],
                trip->pairs[i].cityId,
                trip->globalData->cities[trip->pairs[i+1].cityId],
                trip->pairs[i+1].cityId
            );
            continue;
        }
        if (price != trip->pairs[i].price) {
            bug++;
            printf("BUG - wrong trip price @ day %d (%d vs real %d)\n", i, trip->pairs[i].price, price);
        }
        totalPrice += price;
    }
    if (trip->totalPrice != totalPrice) {
        bug++;
        printf("BUG - total price is worng (%lu vs %lu)\n", trip->totalPrice, totalPrice);
    }
    if (bug) {
        printf("%d BUGS FOUND\n", bug);
    }
    else {
        printf("Trip is valid!\n");
    }
    return bug;
}

/**
 * Trip printer in desired format (defined by travellingsalesman.cz contest)
 */
void printTrip(Trip *trip) {
    unsigned short cityFrom, cityTo, price;
    printf("%lu\n", trip->totalPrice);
    for (unsigned short i = 0; i < trip->length; i++) {
        cityFrom = trip->pairs[i].cityId;
        cityTo   = trip->pairs[i+1].cityId;
        price    = trip->pairs[i].price;
        printf("%.3s %.3s %d %d\n",
            trip->globalData->cities[cityFrom],
            trip->globalData->cities[cityTo],
            i,
            price
        );
    }
}

/**
 * Returns number of threads available at current platform
 */
unsigned short getNumOfThreads() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

/**
 * Unified time measuring function for WIN and POSIX
 * Returns time state in milliseconds
 */
#if defined(_WIN32) || defined(_WINDOWS)
static unsigned long timer(void) {
    return (unsigned long)(clock() / (double)CLOCKS_PER_SEC * 1000);
}
#else // posix
static unsigned long timer(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (unsigned long)(time.tv_sec) * 1.0e3 + (unsigned long)(time.tv_nsec) / 1.0e6;
}
#endif

/**
 * Only for VisualStudio testing/debug purposes
 * Stops for input
 */
void winStall() {
#if defined(_WIN32) || defined(_WINDOWS)
    char buffer[20];
    fgets(buffer, 20, stdin);
#endif
}