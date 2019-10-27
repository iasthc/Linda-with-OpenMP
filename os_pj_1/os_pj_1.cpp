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

string vec2Str(vector<any>v, bool compare)
{
	try {


		string result = "";
		vector<any> vecAny = v;
		vecAny.assign(v.begin(), v.end());
		if (vecAny.size() > 2) {
			vecAny.erase(vecAny.begin());//Client
			vecAny.erase(vecAny.begin());//Instruction
		}


		while (vecAny.empty() == false)
		{
			if (compare) {
				if (vecAny.front().type().name() == typeid(pair<string, tuple<any &> >).name()) {
					result += get<0>(std::any_cast<pair<string, tuple<any &> >>(vecAny.front())) + ",";
				}
			}
			else {
				if (vecAny.front().type().name() == typeid(int).name()) {
					result += to_string(std::any_cast<int>(vecAny.front())) + ",";
				}
				else if (vecAny.front().type().name() == typeid(string).name()) {
					result += "\"" + std::any_cast<string>(vecAny.front()) + "\",";
				}
				else if (vecAny.front().type().name() == typeid(pair<string, tuple<any &> >).name()) {
					/*if ((get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair<string, tuple<any &> >>(vecAny.front()))))).type().name() == typeid(int).name()) {
						result += to_string(std::any_cast<int>((get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair<string, tuple<any &> >>(vecAny.front()))))))) + ",";
					}
					else if ((get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair<string, tuple<any &> >>(vecAny.front()))))).type().name() == typeid(string).name()) {
						result += "\"" + std::any_cast<string>(get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair<string, tuple<any &> >>(vecAny.front()))))) + "\",";
					}*/
				}
				else {
					if (debug) cout << vecAny.front().type().name() << endl;
				}
			}

			vecAny.erase(vecAny.begin());
		}
		result = result.substr(0, result.length() - 1);
		if (compare) {
			return result;
		}
		return "(" + result + ")";
	}
	catch (exception ex) {
		if (debug) cout << ex.what() + '\n';
	}
}

static void save2txt(vector<vector<any>>& vvaTuple, string fileName) {
	string content;
	for (vector<vector<any>>::const_iterator i = vvaTuple.begin(); i != vvaTuple.end(); ++i) {
		content += vec2Str(*i, false) + ",";
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

			if (debug) cout << "CLIENT: " + threadNum + '\n';
			ofstream ofs("./" + threadNum + ".txt");
			try {
				while (!exit) {
#pragma omp critical
					{
						if (!sharedTuple[stoi(threadNum)].empty()) {
							try {
								privateTuple.push_back(sharedTuple[stoi(threadNum)]);
								save2txt(privateTuple, threadNum);
								if (debug) cout << "Thread_" + to_string(std::any_cast<int>(sharedTuple[stoi(threadNum)].at(0))) + " >> " + std::any_cast<string>(sharedTuple[stoi(threadNum)].at(1)) + " done!\n";
								sharedTuple[stoi(threadNum)].clear();
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
			if (debug) cout << "SERVER: " + threadNum + '\n';
			ofstream ofst("./server.txt");

			string input;

			vector<vector<any>> vvaSeq;
			map<string, any> mapSA;

			while (!exit) {
				if (readFiles) cout << readFromFile(stoi(threadCNT));
				input = ""; //reset
				getline(cin, input);

				if (input == "exit") { exit = true; break; }

				size_t n = count(input.begin(), input.end(), ' ');
				if (n < 2) { if (debug) cout << "Error: wrong quantiy of parameters\n"; continue; }

				vector<any> vaInput;
				regex exp(R"((["].+?["])|([^\s]+))");
				smatch res;
				while (regex_search(input, res, exp))
				{
					try {
						if (string(res[0])[0] == '"')
							vaInput.push_back(string(res[0]).substr(1, string(res[0]).length() - 2));
						else if (string(res[0])[0] == '?') {
							vaInput.push_back(make_pair(string(res[0]).substr(1), tie(mapSA[string(res[0]).substr(1)])));
						}
						else if (string(res[0]) == "read" || string(res[0]) == "out" || string(res[0]) == "in")
							vaInput.push_back(string(res[0]));
						else
							vaInput.push_back(stoi(res[0]));
					}
					catch (invalid_argument ia) {
						vaInput.push_back(make_pair(string(res[0]), tie(mapSA[string(res[0])])));
						if (debug) cout << "badcast!!\n";
					}

					input = res.suffix();
				}
				
				int c = std::any_cast<int>(vaInput.at(0));
				string i = std::any_cast<string>(vaInput.at(1));

				if (c > stoi(threadCNT) + 1) { if (debug) cout << "Error: Not an exist client\n"; continue; }
				if (i != "out" && i != "in" && i != "read") { if (debug) cout << "illegl input!!\n"; continue; }

				bool exist = false;
				for (vector<vector<any>>::const_iterator it = vvaSeq.begin(); it != vvaSeq.end(); ++it)
					if (std::any_cast<int>((*it).at(0)) == std::any_cast<int>(vaInput.at(0)))
						exist = true;

				if (!exist) {
					if (debug) cout << "Thread_" + to_string(c) + " is waiting for " + i + "\n";
					if (i == "out") {
						privateTuple.push_back(vaInput);
						save2txt(privateTuple, "server");
						if (debug) cout << "Thread_" + to_string(c) + " >> " + i + " done!\n";
					}
					else if (i == "in" || i == "read") {
						vvaSeq.push_back(vaInput);
					}
				}
				else {
					if (debug) cout << "Thread_" + to_string(c) + " is suspended\n";
				}

				bool equal = true;
				for (vector<vector<any>>::const_iterator it_i = privateTuple.begin(); it_i != privateTuple.end(); ++it_i) {
					for (vector<vector<any>>::const_iterator it_j = vvaSeq.begin(); it_j != vvaSeq.end(); ++it_j) {
						equal = true;
						vector<any> v;
						//if(debug) cout << (*it_i).size() << "|" << (*it_j).size() << endl;

						for (int k = 2; k < (*it_j).size(); k++) {
							//if(debug) cout << k << "\n";
							if ((*it_i).size() == (*it_j).size()) {
								if ((*it_j).at(k).type().name() == typeid(pair <string, tuple<any &>>).name()) {
									if (debug) cout << "continue\n";
									if ((*it_i).at(k).type().name() == typeid(pair <string, tuple<any &>>).name()) {
										/*if (get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair <string, tuple<any &>>>((*it_i).at(k))))).type().name() == typeid(int).name()) {
											v.push_back(std::any_cast<int>(get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair <string, tuple<any &>>>((*it_i).at(k)))))));
										}
										else if (get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair <string, tuple<any &>>>((*it_i).at(k))))).type().name() == typeid(string).name()) {
											v.push_back(std::any_cast<string>(get<0>(std::any_cast<tuple<any &>>(get<1>(std::any_cast<pair <string, tuple<any &>>>((*it_i).at(k)))))));
										}*/
									}
									else { //string, int
										v.push_back((*it_i).at(k));
									}
									continue;
								}
								//if(debug) cout << (*it_i).at(k).type().name() << "|" << (*it_j).at(k).type().name() << endl;
								if ((*it_i).at(k).type().name() == (*it_j).at(k).type().name()) {
									if ((*it_i).at(k).type().name() == typeid(string).name()) {
										//if(debug) cout << std::any_cast<string>((*it_i).at(k)) << "|" << std::any_cast<string>((*it_j).at(k)) << endl;
										if (std::any_cast<string>((*it_i).at(k)) != std::any_cast<string>((*it_j).at(k))) {
											equal = false;
											break;
										}
									}
									else if ((*it_i).at(k).type().name() == typeid(int).name()) {
										//if(debug) cout << std::any_cast<int>((*it_i).at(k)) << "|" << std::any_cast<int>((*it_j).at(k)) << endl;
										if (std::any_cast<int>((*it_i).at(k)) != std::any_cast<int>((*it_j).at(k))) {
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
								//if(debug) cout << "\nSIZE!=\n" << endl;
								break;
							}
						}
						if (equal) {
							string s = vec2Str(*it_j, true);
							if (debug) cout << s << endl;
							vector<string> sv;
							split(s, sv, ',');

							if (debug) cout << sv.size() << endl;
							if (debug) cout << v.size() << endl;

							if (!sv.empty()) {
								string* a = &sv[0];
								int aa = 0;
								for (vector<any>::const_iterator iv = v.begin(); iv != v.end(); ++iv) {
									//if(debug) cout << std::any_cast<string>(a[aa]) << endl;
									//if(debug) cout << (*iv).type().name() << endl;
									//try {
									//	if(debug) cout << std::any_cast<string>(*iv) << endl;
									//}
									//catch (exception ex) {
									//	try {
									//		if(debug) cout << std::any_cast<int>(*iv) << endl;
									//	}
									//	catch (exception ex) {
									//		//if(debug) cout << std::any_cast<void>(*iv) << endl;
									//	}
									//}
									mapSA[std::any_cast<string>(a[aa])] = *iv;
									aa++;
								}
							}
							if (debug) cout << "END equal" << endl;
						}
					}
				}

				exist = false;
				int ii, jj;
				ii = 0;
				jj = 0;

				for (vector<vector<any>>::const_iterator it_i = privateTuple.begin(); it_i != privateTuple.end(); ++it_i) {
					if (debug) cout << "ServerTuple: " << vec2Str(*it_i, false) << (*it_i).size() << ii << endl;
					jj = 0;
					for (vector<vector<any>>::const_iterator it_j = vvaSeq.begin(); it_j != vvaSeq.end(); ++it_j) {
						if (debug) cout << "vvaSeq: " << vec2Str(*it_j, false) << (*it_j).size() << jj << endl;
						if (vec2Str(*it_i, false) == vec2Str(*it_j, false)) {
							(sharedTuple[std::any_cast<int>((*it_j).at(0))]).assign((*it_j).begin(), (*it_j).end());
							exist = true;
							if (debug) cout << "exist: " << vec2Str(*it_j, false) << (*it_j).size() << jj << endl;
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
					if (std::any_cast<string>((vvaSeq[jj]).at(1)) == "in") {
						if (debug) cout << "exist" << ii << jj << endl;
						privateTuple.erase(privateTuple.begin() + ii);
						save2txt(privateTuple, "server");
					}
					vvaSeq.erase(vvaSeq.begin() + jj);
				}
			}
		}
		catch (exception ex) {
			if (debug) cout << ex.what() + '\n';
		}
	}
	return 0;
}

