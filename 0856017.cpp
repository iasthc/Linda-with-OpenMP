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

bool debug = false;
bool readFiles = true;

void split(const string& s, vector<string>& sv, const char delim = ' ') {

	sv.clear();
	istringstream iss(s);
	string temp;

	while (getline(iss, temp, delim)) {
		sv.emplace_back(move(temp));
	}

	return;
}

/*
static string readFromFile(int threadCNT) {

	string result = "==========\n";
	string content = "";
	ifstream ifs("./server.txt");
	getline(ifs, content);
	result += "server.txt >>" + content + "\n";

	for (int i = 1; i <= threadCNT; i++) {
		ifstream ifs("./" + to_string(i) + ".txt");
		getline(ifs, content);
		result += to_string(i) + ".txt >>" + content + "\n";
	}
	return result + "==========\n";
}
*/

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
			if (debug) cout << "vec2Str" << vecAny.front().type().name() << endl;
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

 	ofstream ofs;
  	ofs.open("./" + fileName + ".txt", std::ios_base::app);
	ofs << "(" + content + ")\n"; 
}

static void save2txt(vector<any>& vaTuple, string fileName) { //client

	string content = vec2Str(vaTuple);
 	ofstream ofs;
  	ofs.open("./" + fileName + ".txt", std::ios_base::app);
	ofs << content + "\n"; 
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

			if (debug) cout << "CLIENT: " + threadNum + '\n';
			ofstream ofs("./" + threadNum + ".txt");
			try {
				while (!exit) {
#pragma omp critical
					{
						if (!sharedTuple[stoi(threadNum)].empty()) {
							try {
								save2txt(sharedTuple[stoi(threadNum)], threadNum);
								if (debug) cout << "Thread_" + to_string(any_cast<int>(sharedTuple[stoi(threadNum)].at(0))) + " >> " + any_cast<string>(sharedTuple[stoi(threadNum)].at(1)) + " done!\n";
								sharedTuple[stoi(threadNum)].clear();
								//if (readFiles) cout << readFromFile(stoi(threadCNT));
							}
							catch (exception ex) {
								if (debug) cout << "Client Exception:" + (string)ex.what() + '\n';
							}

						}
					}
				}
			}
			catch (exception ex) {
				if (debug) cout << threadNum + ": " + ex.what() + '\n';
			}
		}

#pragma omp master

		try {
			vector<vector<any>> privateTuple;

			if (debug) cout << "SERVER: " + threadNum + '\n';
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
				if (n < 2) { if (debug) cout << "Error: wrong quantiy of parameters\n"; continue; }

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
						if (debug) cout << "badcast!!\n";
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
					if (debug) cout << "Thread_" + to_string(c) + " is waiting for " + i + "\n";
					if (i == "out") {
						privateTuple.push_back(vaInput);
						//save2txt(privateTuple, "server");
						if (debug) cout << "Thread_" + to_string(c) + " >> " + i + " done!\n";
						//if (readFiles) cout << readFromFile(stoi(threadCNT));
					}
					else if (i == "in" || i == "read") {
						vvaSeq.push_back(vaInput);
					}
				}
				else {
					cout << "Thread_" + to_string(c) + " is suspended\n";
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
						//if(debug) cout << (*it_i).size() << "|" << (*it_j).size() << endl;

						for (int k = 2; k < (*it_j).size(); k++) {
							//if(debug) cout << k << "\n";
							if ((*it_i).size() == (*it_j).size()) {
								//if (debug) cout << (*it_j).at(k).type().name() << endl;
								if ((*it_j).at(k).type().name() == typeid(string).name()) {
									if (any_cast<string>((*it_j).at(k))[0] == '?') {
										if (debug) cout << "continue\n";
										continue;
									}
								}
								if ((*it_i).at(k).type().name() == (*it_j).at(k).type().name()) {
									if ((*it_i).at(k).type().name() == typeid(string).name()) {
										//if(debug) cout << any_cast<string>((*it_i).at(k)) << "|" << any_cast<string>((*it_j).at(k)) << endl;
										if (any_cast<string>((*it_i).at(k)) != any_cast<string>((*it_j).at(k))) {
											equal = false;
											break;
										}
									}
									else if ((*it_i).at(k).type().name() == typeid(int).name()) {
										//if(debug) cout << any_cast<int>((*it_i).at(k)) << "|" << any_cast<int>((*it_j).at(k)) << endl;
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
							if (debug) cout << "equal" << endl;
							break;

						}
						jj++;
					}
					if (equal) {
						break;
					}
					ii++;
				}

				if (debug) cout << "privateTuple.size()" << privateTuple.size() << "vvaSeq.size()" << vvaSeq.size() << endl;
				if (privateTuple.size() > 0 && vvaSeq.size() > 0 && equal) {
					for (int i = 2; i < privateTuple[ii].size(); i++) {
						if (debug) cout << vvaSeq[jj].at(i).type().name() << endl;
						if (vvaSeq[jj].at(i).type().name() == typeid(string).name()) {
							if (debug) cout << any_cast<string>(vvaSeq[jj].at(i))[0] << endl;
							if (any_cast<string>(vvaSeq[jj].at(i))[0] = '?') {
								if (debug) cout << any_cast<string>(vvaSeq[jj].at(i)).substr(1) << endl;
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
					if (debug) cout << "ServerTuple: " << vec2Str(*it_i) << (*it_i).size() << ii << endl;
					jj = 0;
					for (vector<vector<any>>::const_iterator it_j = vvaSeq.begin(); it_j != vvaSeq.end(); ++it_j) {
						if (debug) cout << "vvaSeq: " << vec2Str(*it_j) << (*it_j).size() << jj << endl;
						if (vec2Str(*it_i) == vec2Str(*it_j)) {
							(sharedTuple[any_cast<int>((*it_j).at(0))]).assign((*it_j).begin(), (*it_j).end());
							exist = true;
							if (debug) cout << "exist: " << vec2Str(*it_j) << (*it_j).size() << jj << endl;
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
						if (debug) cout << "exist" << ii << jj << endl;
						privateTuple.erase(privateTuple.begin() + ii);
						//save2txt(privateTuple, "server");
					}
					vvaSeq.erase(vvaSeq.begin() + jj);
				}

				if(( exist || i=="out" ) && i!="read"){
					save2txt(privateTuple, "server");
				}
			}
		}
		catch (exception ex) {
			if (debug) cout << ex.what() + '\n';
		}
	}
	return 0;
}

