/*
 * File: expr-test.cpp
 * -------------------
 *
 * Provides a console unit test for the expression parser.
 *
 * Usage:
 * - Enter any expression in variables
 * - Define the expression variables e.g. by entering z=(3,4) for 3+4i
 * - Evaluate the expression by pressing enter
 */

#include <complex>
#include "expr.hpp"

using namespace std;

typedef complex<double> MyT;

template class Expr<MyT>;

int main()
{
    Expr<MyT>::funcs1 = {
        {   "sin", [](MyT x) { return sin(x); } },
        {   "cos", [](MyT x) { return cos(x); } },
        {   "log", [](MyT x) { return log(x); } },
        {    "ln", [](MyT x) { return log(x); } },
        {   "exp", [](MyT x) { return exp(x); } },
        {  "sqrt", [](MyT x) { return sqrt(x); } },
        {   "tan", [](MyT x) { return tan(x); } },
        {  "atan", [](MyT x) { return atan(x); } },
        {  "asin", [](MyT x) { return asin(x); } },
        {  "acos", [](MyT x) { return acos(x); } },
        {   "abs", [](MyT x) { return (MyT) abs(x); } },
        {    "re", [](MyT x) { return (MyT) x.real(); } },
        {    "im", [](MyT x) { return (MyT) x.imag(); } },
        {  "conj", [](MyT x) { return conj(x); } },
    };

    Expr<MyT>::funcs2 = {
        // {"max", [](MyT x, MyT y) { return x > y ? x : y; }},
        // {"min", [](MyT x, MyT y) { return x < y ? x : y; }}
    };

    Expr<MyT> expr;
    map<string, MyT> vars;

    while (true) {
        string s;
        cout << "Enter expression (q=quit): ";
        getline(cin, s);
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());

        if (s == "q") break;

        size_t split;
        if ((split = s.find('=')) != string::npos) {
            MyT value;
            istringstream(s.substr(split+1, string::npos)) >> value;
            vars[s.substr(0, split)] = value;
        } else if (s.empty()) {
            try {
                vars["i"] = complex<double>(0, 1.0);
                vars["I"] = complex<double>(0, 1.0);
                cout << "Evaluated: " << expr(vars) << endl;
            } catch (const invalid_argument& e) {
                cerr << e.what() << endl;
            }
        } else {
            try {
                expr = Expr<MyT>(s);
            } catch (const invalid_argument& e) {
                cerr << e.what() << endl;
            }
        }
    }

    return 0;
}