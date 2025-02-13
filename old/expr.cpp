// expr.cpp: Unit test for parsing and evaluating expressions with variables

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <string>
#include <map>
#include <algorithm>

using namespace std;

template <class T>
class Expr {
    T value;             // Holds a numeric value for leaf nodes
    Expr *left, *right;  // Pointers to sub-expressions (binary tree structure)
    char op;             // Operator (+, -, *, /, ^)
    string name;         // Variable or function name

    enum ParseLevel { SUMS=0, FACTORS, POWERS, OPERANDS, FUNC };

    // Explicitly perform shallow copy
    Expr(Expr* expr) 
        : value(expr->value), left(expr->left), right(expr->right), op(expr->op), name(expr->name) {
    }

    // Recursive constructor to parse expressions
    Expr(istringstream& str, int level) 
        : value(0), left(nullptr), right(nullptr), op(0), name("") {
        const string level_ops[] = { "+-", "*/", "^" };
        char c;

        switch (level) {
            case SUMS:
            case FACTORS:
            case POWERS:
                left = new Expr(str, level + 1);
                while (level_ops[level].find(str.peek()) != string::npos || 
                       (level == FACTORS && (isalnum(str.peek()) || str.peek() == '('))) {
                    // need to fix for x^y^z
                    if (right != nullptr) {
                        left = new Expr(this); // Shallow copy of current node
                    }

                    // Handle omitted '*' for implicit multiplication
                    if (level_ops[level].find(str.peek()) != string::npos) {
                        str >> op;
                    } else {
                        op = '*';
                    }

                    right = new Expr(str, level + 1);
                }
                break;

            case OPERANDS:
                c = str.peek();
                if (c == '(') {
                    str >> c; // Consume '('
                    left = new Expr(str, SUMS);
                    if (str.peek() == ',') {
                        // only needed for 2-arg funcs
                        str >> c;
                        right = new Expr(str, SUMS);
                    }
                    if (str.peek() != ')') {
                        throw invalid_argument("Error: Missing closing ')'");
                    }
                    str >> c; // Consume ')'
                } else if (is_value("", c)) {
                    string val_str;
                    while (is_value(val_str, str.peek()))
                        val_str.push_back(str.get());
                    istringstream(val_str) >> value;
                } else {
                    left = new Expr(str, FUNC);
                }
                break;

            case FUNC:
                while (isalpha(str.peek())) {
                    str >> c;
                    name.push_back(c);
                }
                if (funcs1.find(name) != funcs1.end() || funcs2.find(name) != funcs2.end()) {
                    if (str.peek() != '(') {
                        throw invalid_argument("Error: Function '" + name + "' expects '('");
                    }
                    left = new Expr(str, OPERANDS);
                } 
                break;
        }
    }

public:
    Expr(Expr&&) = delete;         // Disables the move constructor

    Expr& operator=(const Expr& other) {
        if (this != &other) { // Self-assignment check
            // Clean up existing resources
            delete left;
            delete right;

            // Copy new resources
            value = other.value;
            op = other.op;
            name = other.name;

            left = other.left ? new Expr(*other.left) : nullptr;
            right = other.right ? new Expr(*other.right) : nullptr;
        }
        return *this;
    }

    using fp1 = T (*)(T); // 1-arg function pointer type
    using fp2 = T (*)(T, T); // 2-arg function pointer type
    inline static map<string, fp1> funcs1 = {}; // 1-arg user-defined functions
    inline static map<string, fp2> funcs2 = {}; // 2-arg user-defined functions
    // Checks if we are reading a constant of type T.
    // Preset for double and float values. Needs altering for other types.
    inline static bool (*is_value)(const string&, char) = [](const string& prev, char c) {
        if (isdigit(c)) return true;
        if (prev.empty()) return false;
        if (c == '.' && prev.find('.') == string::npos) return true;
        if (tolower(c) == 'e' && prev.find('e') == string::npos && prev.find('E') == string::npos) return true;
        if ((c == '+' || c == '-') && tolower(prev.back()) == 'e') return true;
        return false;
    };

    // Constructor: Initializes from a string and removes spaces
    Expr(string s) : left(nullptr), right(nullptr), op(0) {
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
        istringstream str(s);
        left = new Expr(str, SUMS);
    }

    // Constructor: Deep copy an expression
    Expr(const Expr& expr) 
        : value(expr.value), op(expr.op), name(expr.name) {
        left = expr.left ? new Expr(*expr.left) : nullptr;
        right = expr.right ? new Expr(*expr.right) : nullptr;
    }

    // Destructor: Clean up nodes
    ~Expr() {
        delete left;
        delete right;
    }

    Expr() : left(nullptr), right(nullptr) {
    }

    // Evaluate the expression with given variable substitutions
    T operator()(map<string, T> vars) {
        // Debugging:
        // if (string("+-*/^").find(op) != string::npos) {
        //     cout << "Stacking evaluation of " << op << endl;
        // }

        switch (op) {
            case '+': return (*left)(vars) + (*right)(vars);
            case '-': return (*left)(vars) - (*right)(vars);
            case '*': return (*left)(vars) * (*right)(vars);
            case '/': return (*left)(vars) / (*right)(vars);
            case '^': return pow((*left)(vars), (*right)(vars));
        }

        if (!name.empty()) {
            if (funcs1.find(name) != funcs1.end()) {
                return funcs1[name]((*left->left)(vars));
            }
            if (funcs2.find(name) != funcs2.end()) {
                return funcs2[name]((*left->left)(vars), (*left->right)(vars));
            }
            if (vars.find(name) != vars.end()) {
                return vars[name];
            }
            throw invalid_argument("Error: Variable '" + name + "' is undefined");
        }

        return left ? (*left)(vars) : value;
    }
};

// // Unit test:
// void test_expr() {
//     typedef double MyT;

//     Expr<double>::funcs1 = {
//         {"sin", sin}, 
//         {"cos", cos}, 
//         {"log", log}, 
//         {"ln", log}, 
//         {"exp", exp}
//     };

//     Expr<double>::funcs2 = {
//         {"max", [](double x, double y) { return x > y ? x : y; }}, 
//         {"min", [](double x, double y) { return x < y ? x : y; }}
//     };

//     Expr<MyT> expr;
//     map<string, MyT> vars;

//     while (true) {
//         string s;
//         cout << "Enter expression (q=quit): ";
//         getline(cin, s);
//         s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());

//         if (s == "q") break;

//         size_t split;
//         if ((split = s.find('=')) != string::npos) {
//             MyT value;
//             istringstream(s.substr(split+1, string::npos)) >> value;
//             vars[s.substr(0, split)] = value;
//         } else if (s.empty()) {
//             try {
//                 cout << "Evaluated: " << expr(vars) << endl;
//             } catch (const invalid_argument& e) {
//                 cerr << e.what() << endl;
//             }
//         } else { 
//             try {
//                 expr = Expr<MyT>(s);
//             } catch (const invalid_argument& e) {
//                 cerr << e.what() << endl;
//             }
//         }
//     }
// }

// int main() { test_expr(); return 0; }

