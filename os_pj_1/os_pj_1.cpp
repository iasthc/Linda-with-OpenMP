#include "pch.h"
#include <omp.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <list>
#include <tuple>
#include <utility>
#include <typeinfo>
#include <regex>
#include <iterator>
#include <iostream>
#include <string>
#include <tuple>
#include <fstream>

using namespace std;

/*Global Variable*/
vector<string> vsTuple;

template<class Tuple, size_t N>
struct TupleTexter {
	static string txtTuple(const Tuple& t)
	{
		return TupleTexter<Tuple, N - 1>::txtTuple(t) + "," + get<N - 1>(t);
	}
};

template<class Tuple>
struct TupleTexter<Tuple, 1> {
	static string txtTuple(const Tuple& t)
	{
		return get<0>(t);
	}
};

template<class... Args>
string txtTuple(const tuple<Args...>& t)
{
	return "(" + TupleTexter<decltype(t), sizeof...(Args)>::txtTuple(t) + ")";
}
//end tuple space

template <size_t ... Is>
auto vsToTuple(const vector<string>  &vs, index_sequence<Is...>)
{
	/*if (string(base_match[1 + Is]).rfind("\"", 0) == 0) {
		std::cout << "rfind " << string(base_match[1 + Is]);
	}*/
	return  make_tuple(vs[Is]...);
}

template <size_t N>
auto vsToTuple(const vector<string> &vs)
{
	return vsToTuple(vs, make_index_sequence<N>());
}

static string readFromFile(string client) {
	string content;
	ifstream ifs("./" + client + ".txt");
	getline(ifs, content);
	//std::cout << "readFromFile: " << content << endl;
	return content;
}

static void lindaOut(string client, string text) {
	string content = readFromFile(client);
	if (content.size() > 2) { //()
		content.pop_back();
		content = content.substr(1) + ",";
	}
	else {
		content = "";
	}
	ofstream ofs("./" + client + ".txt");
	ofs << "(" + content + text + ")";
}

static bool lindaRead(string client, string text, vector<string> &seq) {
	bool result = 0;

	if (seq[0] != client)
		return 0;

	string content = readFromFile("server");
	if (content.size() > 2) {
		if (content.find(text) != string::npos) {
			lindaOut(client, text);
			result = 1;
		}
	}
	return result;
}

static bool lindaIn(string client, string text, vector<string> &seq) {
	bool result = 0;
	result = lindaRead(client, text, seq);
	if (result) {
		string content = readFromFile("server");
		if (content.size() > 2) {
			string m = text;
			int i = content.find(m);
			if (i >= 0) {
				if (content.size() == text.size() + 2) {
					content.erase(i, m.length());
				}
				else {
					m = text + ",";
					i = content.find(m);
					if (i >= 0) {
						content.erase(i, m.length());
					}
					else {
						m = ",", text;
						i = content.find(m);
						content.erase(i, m.length());
					}
				}
			}

			ofstream ofs("./server.txt");
			ofs << content;
		}
	}
	return result;
}

auto regexInput(string str) {
	regex exp(R"((["].+?["])|([^\s]+))");
	smatch res;
	while (regex_search(str, res, exp))
	{
		vsTuple.push_back(res[0]);
		str = res.suffix();
	}
	return vsToTuple<2>(vsTuple);
}

int main()
{
	/*
		1 out "abc" 25 "x"
		1,"out","abc",2,"x"
	*/

	string sNum = "";
	getline(cin, sNum);

	//omp variable
	bool exit = false;
	string ci = "";
	string p = "";
	vector<string> vsSeq;
#pragma omp parallel num_threads(stoi(sNum) + 1) 
	{
		if (omp_get_thread_num() > 0) {
			std::cout << "CLIENT: " + to_string(omp_get_thread_num()) + '\n';
			ofstream ofs("./" + to_string(omp_get_thread_num()) + ".txt");
			while (!exit) {
				if (ci != "") {
					if (ci.substr(0, ci.find_first_of(' ')) == to_string(omp_get_thread_num()))
					{
						string tmpCI = ci;
						string tmpP = p;
						ci = "";
						p = "";
						/*
						操作tmpCI
						*/

						auto t = regexInput(tmpP);
						/*cout << txtTuple(t) + '\n';*/
						if (tmpCI[tmpCI.length() - 1] == 'i') {
							std::cout << "Thread_" + to_string(omp_get_thread_num()) + " is waiting for IN\n";
							vsSeq.push_back(to_string(omp_get_thread_num()));
							while (!lindaIn(to_string(omp_get_thread_num()), txtTuple(t), vsSeq));
							vsSeq.erase(vsSeq.begin());
						}
						else if (tmpCI[tmpCI.length() - 1] == 'r') {
							std::cout << "Thread_" + to_string(omp_get_thread_num()) + " is waiting for READ\n";
							vsSeq.push_back(to_string(omp_get_thread_num()));
							while (!lindaRead(to_string(omp_get_thread_num()), txtTuple(t), vsSeq));
							vsSeq.erase(vsSeq.begin());
						}
						else {
							break;
						}


						std::cout << "Thread_" + to_string(omp_get_thread_num()) + " >> " + tmpCI[tmpCI.length() - 1] + " Done!\n";
					}
				}
			}
		}


#pragma omp master

		try {
			std::cout << "SERVER: " + to_string(omp_get_thread_num()) + '\n';
			ofstream ofst("./server.txt");
			string s = "";
			string c = "";
			string i = "";
			/*string p = "";*/
			while (!exit) {
				getline(cin, s);
				if (s == "exit") { exit = true; break; }
				size_t n = count(s.begin(), s.end(), ' ');
				if (n < 2) { cout << "illegl input!!\n"; continue; }
				c = s.substr(0, s.find_first_of(' '));
				if (stoi(c) > stoi(sNum) + 1) { std::cout << "Error: stoi(c) > iNum+1 \n"; exit = true; break; }
				i = s.substr(s.find_first_of(' ') + 1);
				i = i.substr(0, i.find_first_of(' '));
				p = s.substr(c.length() + i.length() + 2);

				auto t = regexInput(p);
				if (i[0] == 'o') {
					std::cout << "Thread_" + to_string(omp_get_thread_num()) + " is waiting for OUT\n";
					lindaOut("server", txtTuple(t));
					std::cout << "Thread_" + to_string(omp_get_thread_num()) + " >> " + i[0] + " Done!\n";
				}
				else if (i[0] == 'r' || i[0] == 'i') {
					ci = c + " " + i[0];
				}
				else { "illegl input!!\n"; continue; }
			}
		}
		catch (exception ex) {
			std::cout << ex.what() + '\n';
		}
	}

	return 0;

	//while (true)
	//{
	//	//if private ==0 input 
	//	//others thread produce txt
	//	string strInput = "";
	//	getline(cin, strInput);
	//	strInput = "3 out \"xxx\" 1";
	//	strInput = strInput.substr(2);
	//	strInput = strInput.substr(strInput.find_first_of(' ') + 1);
	//	std::cout << strInput;

	//	if (strInput != "exit") {
	//		string s("3out\"xxx\"1");
	//		regex e("(3)(\\D*)([\"]\\D*[\"])(\\d+)");
	//		smatch sm;
	//		regex_match(s, sm, e);
	//		//std::cout << "string object with " << sm.size() << " matches\n";
	//		auto tt = vsToTuple<4>(sm);
	//		//std::cout << typeid(tt).name() << endl;
	//		/*string test = txtTuple(tt);
	//		std::cout << test << endl;*/

	//		string client = sm[1];
	//		int instruction = 0;

	//		std::cout << "client: " << client << endl;
	//		std::cout << "instruction: " << instruction << endl;

	//		if (sm[2] == "out") {
	//			instruction = 1;
	//			//client = 0;
	//		}
	//		else if (sm[2] == "in") instruction = 2;
	//		else if (sm[2] == "read") instruction = 3;



	//		switch (instruction) {
	//		case 1: //put tuple into tuple space.
	//			std::cout << "instruction: out" << endl;
	//			lindaOut("server", txtTuple(tt));
	//			while (!lindaIn("3", "(4)"));
	//			break;
	//		case 2: //This tuple in tuple space will be removed.
	//			std::cout << "instruction: in" << endl;
	//			while (!lindaIn(client, txtTuple(tt)));
	//			break;
	//		case 3: //This tuple in tuple space won't be removed
	//			std::cout << "instruction: read" << endl;
	//			while (!lindaRead(client, txtTuple(tt)));
	//			break;
	//		default:
	//			std::cout << "switch (instruction): Fail" << endl;
	//			return 0;
	//		}
	//	}
	//	else {
	//		return 0;
	//	}
	//}

}
















/*

decltype (cusTuple()) xxx = cusTuple();

tuple<char> tc{ '*' };
auto tic = tupleAppend(tc, 42);
for (int i = 0; i < 1; i++) {
	auto tic = tupleAppend(tc, "sdfsf");
	std::cout << typeid(tic).name() << endl;
}
std::cout << typeid(tic).name() << endl;
print(tic);
return 0;

/*
auto t1 = make_tuple(1,2);
auto t2 = make_tuple("1", "2");
auto t3 = t1 + t1;
std::cout << get<0>(t3) << endl;
std::cout << get<1>(t3) << endl;

return 0;

auto q = make_tuple("1", 2);
auto r = cusTuple<int, char,char>();
std::cout << typeid(r).name() << endl;

auto r1 = cusTuple<int, char>();
auto r2 = tuple_cat(q, r1);
std::cout << typeid(r2).name() << endl;

auto t = make_tuple("1", 2);
auto t2 = tuple_cat(t, make_tuple("1", 2));
std::cout << typeid(t2).name() << endl;
*/


//auto t3 = cusTuple(t, t);
/*
return 0;
*/



//auto tpInputsdss = make_tuple(12312,"213123");
//std::cout << "ttt:" << tuple_size<decltype(tpInputsdss)>::value << endl;

//tuple<typename T> ttt;
//auto ttt = parse(getline(cin, strInput));
//auto tpInput = parse(getline(cin, strInput));
//std::cout << "ttt:" << tuple_size<decltype(tpInput)>::value << endl;

//print(ttt);

//string client = "";
//string instruction = "";
//if (strInput != "") {

//	vector<string> vecInput;
//	istringstream f(strInput);
//	string s;
//	/*tuple<int, string> t;
//	tuple<int, string> tTmp;*/
//	int cnt = 0;

//	vector<string> vecStr;
//	vector<string> vecInt;
//	vector<int> vecSeq; // 0 for string, 1 for integer 

//	while (getline(f, s, ' ')) {
//		if (cnt == 0) {
//			client = s;
//			std::cout << cnt << "_client:";
//		}
//		else if (cnt == 1) {
//			instruction = s;
//			std::cout << cnt << "_instruction:";
//		}
//		else {
//			if (s.at(0) == '"') {
//				s.erase(remove(s.begin(), s.end(), '"'), s.end());
//				vecStr.push_back(s);
//				vecSeq.push_back(0);

//				std::cout << cnt << "_str:";
//			}
//			else {
//				vecInt.push_back(s);
//				vecSeq.push_back(1);
//				std::cout << cnt << "_int:";
//			}
//		}
//		std::cout << s << endl;
//		vecInput.push_back(s);

//		cnt++;
//	}

	/*testFunc(vecSeq, vecStr, vecInt);*/

	/*tuple<int, int> tuInt;
	for (vector<string>::const_iterator i = vecStr.begin(); i != vecStr.end(); ++i)
		std::cout << '"' << *i << '"' << ' ';
	for (vector<string>::const_iterator i = vecInt.begin(); i != vecInt.end(); ++i)
		std::cout << *i << ' ';
	for (vector<int>::const_iterator i = vecSeq.begin(); i != vecSeq.end(); ++i) {
		std::cout << *i << ' ';
	}


	tuple<int, double, const char*> t = make_tuple(1, 84.1, "YOO");
	//std::cout << get<0>(t) << ", "
	//	<< get<1>(t) << ", "
	//	<< get<2>(t) << '\n';

	vector<double> v{ 1.1, 2.2, 3.3, 689.2, 9.2 };
	auto t1 = make_tuple(3, string("AABBCCDD"), v);
	string ss = get<1>(t1);
	auto myvector = get<2>(t1);
	auto integer = get<0>(t1);

	std::cout << ss << '\n' << integer << '\n'
		<< myvector.size() << '\n';
	/*auto t = make_tuple("1", 2);
	auto t2 = tuple_cat(t,make_tuple("1", 2));*/
	//print(t2);
	//auto t = tuple_cat(t, make_tuple("Foo", "bar"), t1, tie(n));
	//auto tpInputsdss = make_tuple(12312, "213123");
/*}
else {
	std::cout << "Input?" << endl;
}*/


//return 0;






/*
tuple<int, string, float> t1(10, "Test", 3.14);
int n = 7;
auto t2 = tuple_cat(t1, make_tuple("Foo", "bar"), t1, tie(n));
n = 10;
print(t2);

string s("abc:def:ghi");
vector<string> sv;

split(s, sv, ':');

for (const auto& s : sv) {
	std::cout << s << endl;
}
*/



//list < tuple < int, string > >lists;

//int numClient = 0; //the number of clients
//string input = "";

//cin >> numClient;

//while (true) {
	//cin >> input;
	//auto tpInputsdss = make_tuple(12312,"213123");
	//int client = get<0>(tpInput);

	//auto tpInput = parse(cin);
	//test(tpInput);
	//std::cout << "client: " << get<0>(tpInput);
//}

