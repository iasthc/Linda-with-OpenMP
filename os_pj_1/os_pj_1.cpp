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
//struct any {
//
//};

static string readFromFile(int threadCNT) {

	string result = "";
	string content = "";
	ifstream ifs("./server.txt");
	getline(ifs, content);
	result += "readFromFile_0 >>" + content + "\n";

	for (int i = 1; i <= threadCNT; i++) {
		ifstream ifs("./" + to_string(i) + ".txt");
		getline(ifs, content);
		result += "readFromFile_" + to_string(i) + " >>" + content + "\n";
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

static pair<bool, int> vecExist(vector<vector<any>>& vvaTuple, vector<any>& va) {
	int index = 0;
	bool exist = false;
	for (vector<vector<any>>::const_iterator i = vvaTuple.begin(); i != vvaTuple.end(); ++i) {
		if (vec2Str(*i) == vec2Str(va)) {
			exist = true;
			break;
		}
		index++;
	}
	return make_pair(exist, index);
}

int main()
{
	string threadCNT = "";
	getline(cin, threadCNT);

	//omp variable
	bool exit = false;
	vector<any> s2c;
	vector<any> c2s;
	vector<string> vsSeq;
	vector<vector<any>> vvaServer;

#pragma omp parallel num_threads(stoi(threadCNT) + 1) 
	{
		string threadNum = to_string(omp_get_thread_num());

		if (omp_get_thread_num() > 0) {
			vector<vector<any>> vvaTuple;

			cout << "CLIENT: " + threadNum + '\n';
			ofstream ofs("./" + threadNum + ".txt");

			while (!exit) {
				if (s2c.size() > 0) {
					vector<any> vaTmp;
					vaTmp.assign(s2c.begin(), s2c.end());
					if (vaTmp.size() > 0 && any_cast<int>(vaTmp.at(0)) == stoi(threadNum))
					{
						s2c.clear();

						if (any_cast<string>(vaTmp.at(1)) == "in" || any_cast<string>(vaTmp.at(1)) == "read") {
							cout << "Thread_" + threadNum + " is waiting for " + any_cast<string>(vaTmp.at(1)) + "\n";
							vsSeq.push_back(threadNum);

							bool exist = false;
							while (!exist)
							{
								if (stoi(vsSeq[0]) != any_cast<int>(vaTmp.at(0))) continue;
								//cout << vecExist(vvaServer, vaTmp);
								if (get<0>(vecExist(vvaServer, vaTmp))) {
									vvaTuple.push_back(vaTmp); //Linda Read
									if (any_cast<string>(vaTmp.at(1)) == "in") { //Linda In
										c2s.assign(vaTmp.begin(), vaTmp.end());
									}
									exist = true;
								}
							}

							vsSeq.erase(vsSeq.begin());
							save2txt(vvaTuple, threadNum);
							cout << "Thread_" + threadNum + " >> " + any_cast<string>(vaTmp.at(1)) + " done!\n" + readFromFile(stoi(threadCNT));
						}
					}
					vaTmp.clear();
				}
			}
		}


#pragma omp master

		try {
			cout << "SERVER: " + threadNum + '\n';
			ofstream ofst("./server.txt");

			string input;

			while (!exit) {
				
				input = ""; //reset
				getline(cin, input);

				if (input == "exit") { exit = true; break; }

				size_t n = count(input.begin(), input.end(), ' ');
				if (n < 2) { cout << "Error: wrong quantiy of parameters\n"; continue; }

				vector<any> vaInput = input2Vec(input);

				int c = any_cast<int>(vaInput.at(0));
				string i = any_cast<string>(vaInput.at(1));

				if (c > stoi(threadCNT) + 1) { cout << "Error: Not an exist client\n"; continue; }

				if (i == "out") {
					cout << "Thread_" + threadNum + " is waiting for " + i + "\n";
					vvaServer.push_back(vaInput);
					save2txt(vvaServer, "server");
					cout << "Thread_" + threadNum + " >> " + i + " done!\n" + readFromFile(stoi(threadCNT));
				}
				else if (i == "in" || i == "read") {
					s2c.assign(vaInput.begin(), vaInput.end());
				}
				else { "illegl input!!\n"; continue; }

				if (c2s.size() > 0) {
					int it = get<1>(vecExist(vvaServer, c2s));
					c2s.clear();
					//if (it != vvaServer.end()) {*/
					vvaServer.erase(vvaServer.begin() + it);
					save2txt(vvaServer, "server");
					cout << "Thread_" + threadNum + " >> out done!\n" + readFromFile(stoi(threadCNT));
					//}
				}
			}
		}
		catch (exception ex) {
			cout << ex.what() + '\n';
		}
	}
	return 0;
}