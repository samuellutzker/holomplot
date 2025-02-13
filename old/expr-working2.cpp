// expr.cpp: unit test for reading and evaluating expressions in variables

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <string>
#include <map>

using namespace std;

template <class T>
class Expr {
    T value;

    enum { SUMS=0, FACTORS=1, POWERS=2, OPERANDS=3, FUNC=4 };
    Expr *left, *right;

    char op;

    string var;

    Expr(istringstream& str, int lv) : right(nullptr), left(nullptr), op(0), var("") {
        const string level_ops[] = {"+-", "*/", "^"};
        char c;
        switch (lv) {
            case SUMS:
            case FACTORS:
            case POWERS:
                left = new Expr(str, lv+1);
                while (level_ops[lv].find(str.peek()) != string::npos) {
                    if (right != nullptr) {
                        left = new Expr(this);
                    }
                    str >> op;
                    right = new Expr(str, lv+1);
                }
                break;

            case OPERANDS:
                c = str.peek();
                if (c == '(') {
                    str >> c;
                    left = new Expr(str, SUMS);
                    if (str.peek() != ')')
                        throw logic_error("Bad input, missing )");
                    str >> c;
                } else if (isdigit(c)) {
                    str >> value;
                } else {
                    left = new Expr(str, FUNC);
                }
                break;

            case FUNC:
                while (isalpha(str.peek())) {
                    str >> c;
                    var.push_back(c);
                }
                if (funcs.find(var) != funcs.end()) {
                    if (str.peek() != '(') {
                        throw logic_error("Bad input, expected (");
                    }
                    left = new Expr(str, OPERANDS);
                }
                break;
        }
    }

public:
    typedef T (*fp)(T);
    inline static map<string, fp> funcs = {}; // Needs to be assigned

    Expr(const string& s) : right(nullptr) {
        istringstream str(s);
        left = new Expr(str, SUMS);
    }

    Expr(Expr* src) {
        value = src->value;
        left = src->left;
        right = src->right;
        op = src->op;
        var = src->var;
    }

    ~Expr() {
        if (left != nullptr)
            delete left;
        if (right != nullptr)
            delete right;
    }

    T eval(map<string, T>& vars) {
        // if (string("+-*/^").find(op) != string::npos) {
        //     cout << "Stacking evaluation of " << op << endl;
        // }
        switch (op) {
            case '+': return left->eval(vars) + right->eval(vars);
            case '-': return left->eval(vars) - right->eval(vars);
            case '*': return left->eval(vars) * right->eval(vars);
            case '/': return left->eval(vars) / right->eval(vars);
            case '^': return pow(left->eval(vars), right->eval(vars));
        }

        if (var != "") {
            if (funcs.find(var) != funcs.end()) {
                return funcs[var](left->eval(vars));
            }

            if (vars.find(var) != vars.end()) {
                return vars[var];
            }

            throw invalid_argument("Variable '"+var+"' was not assigned");
        }
        return (left != nullptr) ? left->eval(vars) : value;
    }
};


int main() {
    string s;
    Expr<double>::funcs = {{"sin",sin},{"cos",cos}};
    while (true) {
        cout << "Enter expression (q=quit): ";
        
        getline(cin, s);
        if (s == "q") break;
        Expr<double> expr(s);
        map<string,double> vars;
        while (true) {
            cout << "Substitute var (Enter=eval, q=new expression): ";
            getline(cin, s);
            if (s == "") {
                try {
                    cout << "Evaluated: " << expr.eval(vars) << endl;
                } catch (invalid_argument& e) {
                    cerr << e.what() << endl;
                }
                continue;
            }
            else if (s == "q")
                break;

            cout << "Value: ";
            cin >> vars[s];
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // remove \n in buffer
        }
    }
    return 0;
}