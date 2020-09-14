// TestVsDeck.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "VisageDeckWritter.h"
#include "VisageDeckSimulationOptions.h"


int main()
{

	VisageDeckSimulationOptions options;
	options->path() = "D:\\GPMTESTS\\";
	options->model_name() = "TestEleFiles";

	float **surfaces = new float*[2];
	float *x = new float[4];
	float *y = new float[4];
	surfaces[0] = new float[2 * 2];
	surfaces[1] = new float[2 * 2];
	surfaces[2] = new float[2 * 2];

	float spacing[3] = { 100,200,300 }; 

	//for (int nk = 0; nk < 2; nk++)
	//{
	//	int n = 0; 
	//	int nxy = 0;
	//	for (int nj = 0; nj < 2; nj++)
	//	{
	//		for (int ni = 0; ni < 2; ni++)
	//		{
	//			x[n] = spacing[0] * ni;
	//			y[n] = spacing[1] * nj;
	//			surfaces[nk][node] = (10-nk);

	//			nxy += 1;
	//			n += 1;

	//		}

	//	}

	//}



	options->set_geometry_size(2, 2, 3);


	VisageDeckWritter::write_ele_files( &options );


    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
