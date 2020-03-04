#include <vector>
#include <string>
#include <random>
#include <stdio.h>
#include <math.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <execution>
#include <fstream>
#include <sstream>

#include "../include/ONMS_decoder.h"
#include "../include/BF_decoder.h"
#include "../include/SP_decoder.h"

using nano_s = std::chrono::nanoseconds;
using micro_s = std::chrono::microseconds;
using milli_s = std::chrono::milliseconds;
using seconds = std::chrono::seconds;
using minutes = std::chrono::minutes;
using hours = std::chrono::hours;


std::vector<std::vector<int>> readAsRowSparseMatrix(std::string filename) {
	std::vector<std::vector<int>> matrix;
	std::string line;
	std::ifstream myFile(filename);
	
	int val;

	while (std::getline(myFile, line))
	{
		std::stringstream ss(line);

		std::vector<int> temp;
		int columnIndex = 0;
		while (ss >> val) {
			if (val)
				temp.push_back(columnIndex);

			if (ss.peek() == ',')
				ss.ignore();

			columnIndex++;
		}

		matrix.push_back(temp);
	}

	return matrix;
}


void simulate(int maxTests,
	std::vector<double> snr_array,
	int rejection_count,
	std::vector<double>& fer_array,
	std::vector<double>& sigma_array,
	std::vector<int>& tests_count_array) {
	
    std::vector<std::vector<int>> H = readAsRowSparseMatrix("./data/H_389.csv");
	/*for (size_t i = 0; i < H.size(); i++)
	{
		for (size_t j = 0; j < H[i].size(); j++)
		{
			std::cout << H[i][j] << ",";
		}
		std::cout << std::endl;
	}*/

	ONMS_decoder * decoder = new ONMS_decoder(H, 20);
	// size of sparse Matrix
	int n = 0;
	for (size_t i = 0; i < H.size(); i++)
	{
		int max = *max_element(H[i].begin(), H[i].end());
		if (max > n)
			n = max;
	}
	n++;

	std::random_device randomDevice;

	std::vector<int> codeword(n, 0);
	std::vector<double> llrs(n, 0);

    for (size_t ii = 0; ii < snr_array.size(); ii++) {
    	double snr = snr_array[ii];
    	double sigma = sqrt(pow(10, -snr / 10) / 2); 
    	int tests = 0;
        int wrong_dec = 0;
        int errors = 0;
		bool isFailed = false;
		std::normal_distribution<double> distribution(0, sigma);

        while ((tests < maxTests) && (wrong_dec < rejection_count)) {
            tests = ++tests;
            
            for (int i = 0; i < n; i++) {
				llrs[i] = -2 * (2 * codeword[i] - 1 + distribution(randomDevice)) / (sigma * sigma);
				//std::cout << llrs[i] << "|";
            }
			            
			decoder->Decode(llrs, &isFailed);
			if (isFailed) 
			    printf("Error! %d\n", wrong_dec);
            wrong_dec += isFailed;
        }

		sigma_array[ii] = sigma;
		fer_array[ii] = (double)wrong_dec / tests;
		tests_count_array[ii] = tests;	
    }
}


int main() {
	std::vector<double> snr_array{ -4, -3, -2, -1};
	std::vector<double> fer_array(snr_array.size(), 0);
	std::vector<double> sigma_array(snr_array.size(), 0);
	std::vector<int> tests_count_array(snr_array.size(), 0);

	auto t1 = std::chrono::steady_clock::now();
	simulate(100000, snr_array, 20, fer_array, sigma_array, tests_count_array);
	auto t2 = std::chrono::steady_clock::now();

	auto d_s = std::chrono::duration_cast<seconds>(t2 - t1).count();
	for (size_t i = 0; i < snr_array.size(); i++) {
		printf("\nsigma:%f,\tfer: %f,\ttests numbers: %d\n", sigma_array[i], fer_array[i], tests_count_array[i]);
	}
	
	std::cout << "Seconds: " << d_s << std::endl;
	return 0;
}