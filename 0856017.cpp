/*
	g++ -std=c++17 0856017.cpp -fopenmp
*/
#include <omp.h>
#include <vector>
#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <sstream>
#include <any>
#include <map>
using namespace std;

string vec2Str(vector<any>v) {

	string result = "";
	vector<any> vecAny = v;
	vecAny.assign(v.begin(), v.end());

	if (vecAny.size() > 2) {
		vecAny.erase(vecAny.begin());//Client
		vecAny.erase(vecAny.begin());//Instruction
	}

	while (vecAny.empty() == false)
	{
		if (vecAny.front().type().name() == typeid(int).name()) {
			result += to_string(any_cast<int>(vecAny.front())) + ",";
		}
		else if (vecAny.front().type().name() == typeid(string).name()) {
			//result += "\"" + any_cast<string>(vecAny.front()) + "\",";
			result += any_cast<string>(vecAny.front()) + ",";
		}
		else {
			cout << "vec2Str" + (string)vecAny.front().type().name() + '\n';
		}

		vecAny.erase(vecAny.begin());
	}

	result = result.substr(0, result.length() - 1);
	return "(" + result + ")";
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

		if (omp_get_thread_num() > 0) {

			ofstream ofs("./" + threadNum + ".txt");
			try {
				while (!exit) {
					{
						if (!sharedTuple[stoi(threadNum)].empty()) {
							try {
								string content = vec2Str(sharedTuple[stoi(threadNum)]);
								ofstream ofs;
								ofs.open("./" + threadNum + ".txt", std::ios_base::app);
								ofs << content + "\n"; 
								sharedTuple[stoi(threadNum)].clear();
							}
							catch (exception ex) {
								cout << "Client Exception of ofs:" + (string)ex.what() + '\n';
							}

						}
					}
				}
			}
			catch (exception ex) {
				cout << "Client Exception of while:" + threadNum + ": " + ex.what() + '\n';
			}
		}

#pragma omp master

		try {
			vector<vector<any>> privateTuple;
			ofstream ofst("./server.txt");
			string input;
			vector<vector<any>> vvaSeq;
			map<string, any> mapSA;

			while (!exit) {
				input = ""; //reset
				getline(cin, input);

				if (input == "exit") { 
					while(!exit){
						int cnt = 0;
						for (vector<vector<any>>::const_iterator i = sharedTuple.begin(); i != sharedTuple.end(); ++i) {
							if ((*i).empty()){
								cnt++;	
							}
						}
						if(cnt==sharedTuple.size()){
							exit = true; 
						}
					}
					break; 
				}

				size_t n = count(input.begin(), input.end(), ' ');
				if (n < 2) { cout << "Error: wrong quantiy of parameters\n"; continue; }

				vector<any> vaInput;
				regex exp(R"((["].+?["])|([^\s]+))");
				smatch res;
				while (regex_search(input, res, exp))
				{
					try {
						if (string(res[0])[0] == '"') {
							vaInput.push_back(string(res[0]));
							//vaInput.push_back(string(res[0]).substr(1, string(res[0]).length() - 2));
						}
						else if (string(res[0])[0] == '?') {
							if (any_cast<string>(vaInput.at(1)) == "out") {
								cout << "Error: Parameters are not in accordance with the rules\n";
								exit = true; break;
							}
							mapSA[string(res[0]).substr(1)] = string(res[0]); //mapSA["i"] = "?i";
							vaInput.push_back((mapSA[string(res[0]).substr(1)]));
						}
						else if (string(res[0]) == "read" || string(res[0]) == "out" || string(res[0]) == "in")
							vaInput.push_back(string(res[0]));
						else
							vaInput.push_back(stoi(res[0]));
					}
					catch (invalid_argument ia) {
						vaInput.push_back((mapSA[string(res[0])])); //vaInput.push_back((mapSA["i"]));
						//cout << "badcast!!\n";
					}

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
					if (i == "out") {
						privateTuple.push_back(vaInput);
					}
					else if (i == "in" || i == "read") {
						vvaSeq.push_back(vaInput);
					}
				}
				else {
					//cout << "Thread_" + to_string(c) + " is suspended\n";
				}
				int ii, jj;
				ii = 0;
				jj = 0;
				bool equal = true;
				for (vector<vector<any>>::const_iterator it_i = privateTuple.begin(); it_i != privateTuple.end(); ++it_i) {
					jj = 0;

					for (vector<vector<any>>::const_iterator it_j = vvaSeq.begin(); it_j != vvaSeq.end(); ++it_j) {
						equal = true;
						vector<any> v;

						for (int k = 2; k < (*it_j).size(); k++) {
							if ((*it_i).size() == (*it_j).size()) {
								if ((*it_j).at(k).type().name() == typeid(string).name()) {
									if (any_cast<string>((*it_j).at(k))[0] == '?') {
										continue;
									}
								}
								if ((*it_i).at(k).type().name() == (*it_j).at(k).type().name()) {
									if ((*it_i).at(k).type().name() == typeid(string).name()) {
										if (any_cast<string>((*it_i).at(k)) != any_cast<string>((*it_j).at(k))) {
											equal = false;
											break;
										}
									}
									else if ((*it_i).at(k).type().name() == typeid(int).name()) {
										if (any_cast<int>((*it_i).at(k)) != any_cast<int>((*it_j).at(k))) {
											equal = false;
											break;
										}
									}
								}
								else {
									equal = false;
									break;
								}
							}
							else {
								equal = false;
								break;
							}
						}
						if (equal) {
							break;
						}
						jj++;
					}
					if (equal) {
						break;
					}
					ii++;
				}

				if (privateTuple.size() > 0 && vvaSeq.size() > 0 && equal) {
					for (int i = 2; i < privateTuple[ii].size(); i++) {
						if (vvaSeq[jj].at(i).type().name() == typeid(string).name()) {
							if (any_cast<string>(vvaSeq[jj].at(i))[0] = '?') {
								mapSA[any_cast<string>(vvaSeq[jj].at(i)).substr(1)] = privateTuple[ii].at(i);
							}
						}
					}

					auto c = vvaSeq[jj].at(0);
					auto i = vvaSeq[jj].at(1);
					vvaSeq[jj].assign(privateTuple[ii].begin(), privateTuple[ii].end());
					vvaSeq[jj].at(0) = c;
					vvaSeq[jj].at(1) = i;
				}


				exist = false;
				ii = 0;
				jj = 0;

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
					}
					vvaSeq.erase(vvaSeq.begin() + jj);
				}

				if(( exist || i=="out" ) && i!="read"){
					string content = "";
					for (vector<vector<any>>::const_iterator i = privateTuple.begin(); i != privateTuple.end(); ++i) {
						content += vec2Str(*i) + ",";
					}
					if (content.length() > 0) {
						content = content.substr(0, content.length() - 1);
					}

					ofstream ofs;
					ofs.open("./server.txt", std::ios_base::app);
					ofs << "(" + content + ")\n"; 
				}
			}
		}
		catch (exception ex) {
			cout << "Server Exception of while:" + threadNum + ": " + ex.what() + '\n';
		}
	}
	return 0;
}

