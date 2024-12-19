#include <thread>
#include <chrono>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <string>

using namespace std;
// a class to get more accurate time

class stopwatch{

private:
    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;
    bool timing;

public:
    stopwatch( ): timing( false ) {
        t1 = std::chrono::high_resolution_clock::time_point::min();
        t2 = std::chrono::high_resolution_clock::time_point::min();
    }

    void start( ) {
        if( !timing ) {
            timing = true;
            t1 = std::chrono::high_resolution_clock::now();
        }
    }

    void stop( ) {
        if( timing ) {
            t2 = std::chrono::high_resolution_clock::now();
            timing = false;
        }
    }

    void reset( ) {
        t1 = std::chrono::high_resolution_clock::time_point::min();
        t2 = std::chrono::high_resolution_clock::time_point::min();;
        timing = false;
    }

    // will return the elapsed time in seconds as a double
    double getTime( ) {
        std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2-t1);
        return elapsed.count();
    }
};



// function takes an array pointer, and the number of rows and cols in the array, and
// allocates and intializes the two dimensional array to a bunch of random numbers

void makeRandArray( unsigned int **& data, unsigned int rows, unsigned int cols, unsigned int seed )
{
    // allocate the array
    data = new unsigned int*[ rows ];
    for( unsigned int i = 0; i < rows; i++ )
    {
        data[i] = new unsigned int[ cols ];
    }

    // seed the number generator
    // you should change the seed to get different values
    srand( seed );

    // populate the array

    for( unsigned int i = 0; i < rows; i++ )
        for( unsigned int j = 0; j < cols; j++ )
        {
            data[i][j] = rand() % 10000 + 1; // number between 1 and 10000
        }

}

void getDataFromFile( unsigned int **& data, char fileName[], unsigned int &rows, unsigned int &cols )
{
    ifstream in;
    in.open( fileName );
    if( !in )
    {
        cerr << "error opening file: " << fileName << endl;
        exit( -1 );
    }

    in >> rows >> cols;
    data = new unsigned int*[ rows ];
    for( unsigned int i = 0; i < rows; i++ )
    {
        data[i] = new unsigned int[ cols ];
    }

    // now read in the data

    for( unsigned int i = 0; i < rows; i++ )
        for( unsigned int j = 0; j < cols; j++ )
        {
            in >> data[i][j];
        }

}


int main( int argc, char* argv[] )
{
    if( argc < 3 )
    {
        cerr<<"Usage: " << argv[0] << " [input data file] [num of threads to use] " << endl;

        cerr<<"or" << endl << "Usage: "<< argv[0] << " rand [num of threads to use] [num rows] [num cols] [seed value]" << endl;
        exit( 0 );
    }

    // read in the file
    unsigned int rows, cols, seed;
    unsigned int numThreads;
    unsigned int ** data;
    // convert numThreads to int
    {
        stringstream ss1;
        ss1 << argv[2];
        ss1 >> numThreads;
    }

    string fName( argv[1] );
    if( fName == "rand" )
    {
        {
            stringstream ss1;
            ss1 << argv[3];
            ss1 >> rows;
        }
        {
            stringstream ss1;
            ss1 << argv[4];
            ss1 >> cols;
        }
        {
            stringstream ss1;
            ss1 << argv[5];
            ss1 >> seed;
        }
        makeRandArray( data, rows, cols, seed );
    }
    else
    {
        getDataFromFile( data,  argv[1], rows, cols );
    }

    //UNCOMMENT if you want to print the data array
//	cerr << "data: " << endl;
//   	for( unsigned int i = 0; i < rows; i++ )
//	{
//		for( unsigned int j = 0; j < cols; j++ )
//		{
//			cerr << "i,j,data " << i << ", " << j << ", ";
//			cerr << data[i][j] << " ";
//	}
//		cerr << endl;
//	}
//	cerr<< endl;

    // tell omp how many threads to use
    omp_set_num_threads( numThreads );


    stopwatch S1;
    S1.start();

 double max_average = 0;
    unsigned int max_i = 0, max_j = 0;

#pragma omp parallel
    {
        double local_max_average = 0;
        unsigned int local_max_i = 0, local_max_j = 0;

#pragma omp for schedule(static) nowait
        for (unsigned int i = 0; i < rows - 1; i++) {
            for (unsigned int j = 0; j < cols - 1; j++) {
                double current_sum = data[i][j] + data[i][j + 1] + data[i + 1][j] + data[i + 1][j + 1];
                double current_average = current_sum / 4.0;

                if (current_average > local_max_average) {
                    local_max_average = current_average;
                    local_max_i = i;
                    local_max_j = j;
                }
            }
        }

#pragma omp critical
        {
            if (local_max_average > max_average) {
                max_average = local_max_average;
                max_i = local_max_i;
                max_j = local_max_j;
            }
        }
    }

    S1.stop();

    cerr << "largest average: " << max_average << endl;
    cerr << "found at cell: (" << max_i << ", " << max_j << ")" << endl;
    cerr << "elapsed time: " << S1.getTime() << endl;
}