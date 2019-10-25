#include "pch.h"
#include <omp.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <typeinfo>
#include <regex>
#include <iterator>
#include <string>
#include <tuple>
#include <fstream>
#include <any>
using namespace std;

static string readFromFile(int threadCNT) {

	string result = "";
	string content = "";
	ifstream ifs("./server.txt");
	getline(ifs, content);
	result += "server.txt >>" + content + "\n";

	for (int i = 1; i <= threadCNT; i++) {
		ifstream ifs("./" + to_string(i) + ".txt");
		getline(ifs, content);
		result += to_string(i) + ".txt >>" + content + "\n";
	}
	return result + "\n";
}

vector<any> input2Vec(string s)
{
	vector<any> vecAny;
	regex exp(R"((["].+?["])|([^\s]+))");
	string input = s;
	smatch res;
	while (regex_search(input, res, exp))
	{
		if (string(res[0])[0] == '"')
			vecAny.push_back(string(res[0]).substr(1, string(res[0]).length() - 2));
		else if (string(res[0]) == "read" || string(res[0]) == "out" || string(res[0]) == "in")
			vecAny.push_back(string(res[0]));
		else
			vecAny.push_back(stoi(res[0]));

		input = res.suffix();
	}
	return vecAny;
}

string vec2Str(vector<any>v)
{
	string result = "";
	vector<any> vecAny = v;
	vecAny.assign(v.begin(), v.end());

	vecAny.erase(vecAny.begin());//Client
	vecAny.erase(vecAny.begin());//Instruction

	while (vecAny.empty() == false)
	{
		if (string(vecAny.front().type().name()) == "int") {
			result += to_string(any_cast<int>(vecAny.front())) + ",";
		}
		else { //string
			result += "\"" + any_cast<string>(vecAny.front()) + "\",";
		}
		vecAny.erase(vecAny.begin());
	}
	result = result.substr(0, result.length() - 1);
	return "(" + result + ")";
}

static void save2txt(vector<vector<any>>& vvaTuple, string fileName) {
	string content;
	for (vector<vector<any>>::const_iterator i = vvaTuple.begin(); i != vvaTuple.end(); ++i) {
		content += vec2Str(*i) + ",";
	}
	if (content.length() > 0) {
		content = content.substr(0, content.length() - 1);
	}
	ofstream ofs("./" + fileName + ".txt");
	ofs << "(" + content + ")";
}

int main()
{
	string threadCNT = "";
	getline(cin, threadCNT);

	//omp variable
	bool exit = false;
	vector<vector<any>> sharedTuple(stoi(threadCNT) + 1);

#pragma omp parallel num_threads(stoi(threadCNT) + 1) 
	{
		string threadNum = to_string(omp_get_thread_num());
		vector<vector<any>> privateTuple;

		if (omp_get_thread_num() > 0) {

			cout << "CLIENT: " + threadNum + '\n';
			ofstream ofs("./" + threadNum + ".txt");
			try {
				while (!exit) {
				#pragma omp critical
					{
						if (!sharedTuple[stoi(threadNum)].empty()) {
							privateTuple.push_back(sharedTuple[stoi(threadNum)]);
							save2txt(privateTuple, threadNum);
							cout << "Thread_" + to_string(any_cast<int>(sharedTuple[stoi(threadNum)].at(0))) + " >> " + any_cast<string>(sharedTuple[stoi(threadNum)].at(1)) + " done!\n" + readFromFile(stoi(threadCNT));
							sharedTuple[stoi(threadNum)].clear();
						}
					}
				}
			}
			catch (exception ex) {
				cout << threadNum + ": " + ex.what() + '\n';
			}
		}

#pragma omp master

		try {
			cout << "SERVER: " + threadNum + '\n';
			ofstream ofst("./server.txt");

			string input;

			vector<vector<any>> vvaSeq;

			while (!exit) {
				input = ""; //reset
				getline(cin, input);

				if (input == "exit") { exit = true; break; }

				size_t n = count(input.begin(), input.end(), ' ');
				if (n < 2) { cout << "Error: wrong quantiy of parameters\n"; continue; }

				vector<any> vaInput;
				regex exp(R"((["].+?["])|([^\s]+))");
				smatch res;
				while (regex_search(input, res, exp))
				{
					if (string(res[0])[0] == '"')
						vaInput.push_back(string(res[0]).substr(1, string(res[0]).length() - 2));
					else if (string(res[0]) == "read" || string(res[0]) == "out" || string(res[0]) == "in")
						vaInput.push_back(string(res[0]));
					else
						vaInput.push_back(stoi(res[0]));

					input = res.suffix();
				}

				int c = any_cast<int>(vaInput.at(0));
				string i = any_cast<string>(vaInput.at(1));

				if (c > stoi(threadCNT) + 1) { cout << "Error: Not an exist client\n"; continue; }
				if (i != "out" && i != "in" && i != "read") { cout << "illegl input!!\n"; continue; }

				bool exist = false;
				for (vector<vector<any>>::const_iterator it = vvaSeq.begin(); it != vvaSeq.end(); ++it)
					if (any_cast<int>((*it).at(0)) == any_cast<int>(vaInput.at(0)))
						exist = true;

				if (!exist) {
					cout << "Thread_" + to_string(c) + " is waiting for " + i + "\n";
					if (i == "out") {
						privateTuple.push_back(vaInput);
						save2txt(privateTuple, "server");
						cout << "Thread_" + to_string(c) + " >> " + i + " done!\n" + readFromFile(stoi(threadCNT));
					}
					else if (i == "in" || i == "read") {
						vvaSeq.push_back(vaInput);
					}
				}
				else {
					cout << "Thread_" + to_string(c) + " is suspended\n";
				}

				exist = false;
				int ii = 0;
				int jj = 0;
				
				for (vector<vector<any>>::const_iterator it_i = privateTuple.begin(); it_i != privateTuple.end(); ++it_i) {
					jj = 0;
					for (vector<vector<any>>::const_iterator it_j = vvaSeq.begin(); it_j != vvaSeq.end(); ++it_j) {
						if (vec2Str(*it_i) == vec2Str(*it_j)) {
							(sharedTuple[any_cast<int>((*it_j).at(0))]).assign((*it_j).begin(), (*it_j).end());
							exist = true;
							break;
						}
						jj++;
					}
					if (exist) {
						break;
					}
					ii++;
				}

				if (exist) {
					if (any_cast<string>((vvaSeq[jj]).at(1)) == "in") {
						privateTuple.erase(privateTuple.begin() + ii);
						save2txt(privateTuple, "server");
					}
					vvaSeq.erase(vvaSeq.begin() + jj);
				}
			}
		}
		catch (exception ex) {
			cout << ex.what() + '\n';
		}
	}
	return 0;
}

