/* 
 * File: expr.hpp
 * --------------
 *
 * Defines a template class for parsing and evaluating mathematical expressions 
 * involving variables and functions. May be instantiated with complex<T>.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <string>
#include <map>
#include <algorithm>

template <class T>
class Expr {
    T value;             // Holds a numeric value for leaf nodes
    Expr *left, *right;  // Pointers to sub-expressions (binary tree structure)
    char op;             // Operator (+, -, *, /, ^)
    std::string name;    // Variable or function name

    enum ParseLevel { SUMS=0, FACTORS, POWERS, OPERANDS, FUNC };

    // Explicitly perform shallow copy
    Expr(Expr* expr) 
        : value(expr->value), left(expr->left), right(expr->right), op(expr->op), name(expr->name) {}

    // Recursive constructor to parse expressions
    Expr(std::istringstream& str, int level) 
        : value(0), left(nullptr), right(nullptr), op(0), name("") {
        const std::string level_ops[] = { "+-", "*/", "^" };
        char c;

        switch (level) {
            case SUMS:
            case FACTORS:
            case POWERS:
                left = new Expr(str, level + 1);
                while (level_ops[level].find(str.peek()) != std::string::npos || 
                       (level == FACTORS && (isalnum(str.peek()) || str.peek() == '('))) {
                    
                    if (right != nullptr) {
                        left = new Expr(this); // Shallow copy of the current node
                    }

                    // Handle omitted '*' for implicit multiplication
                    if (level_ops[level].find(str.peek()) != std::string::npos) {
                        str >> op;
                    } else {
                        op = '*';
                    }

                    right = new Expr(str, level == POWERS ? level : level + 1); // fix for x^y^z...
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
                        throw std::invalid_argument("Error: Missing closing ')'.");
                    }
                    str >> c; // Consume ')'
                } else if (is_value("", c)) {
                    std::string val_str;
                    while (is_value(val_str, str.peek()))
                        val_str.push_back(str.get());
                    std::istringstream(val_str) >> value;
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
                        throw std::invalid_argument("Error: Function '" + name + "' expects '('.");
                    }
                    left = new Expr(str, OPERANDS);
                } 
                break;
        }
    }

public:
    using fp1 = T (*)(T);    // 1-arg function pointer type
    using fp2 = T (*)(T, T); // 2-arg function pointer type

    inline static std::map<std::string, fp1> funcs1 = {}; // 1-arg user-defined functions
    inline static std::map<std::string, fp2> funcs2 = {}; // 2-arg user-defined functions

    // This function prototype checks if the currently examined char of a string
    // belongs to a constant of type T.
    // Basic preset for double and float values in floating point notation.
    inline static bool (*is_value)(const std::string&, char) = [](const std::string& prev, char c) {
        if (isdigit(c)) return true;
        if (c == '.' && prev.find('.') == std::string::npos) return true;
        return false;
    };
    /* 
    // The following version allows exponent notation
    // but creates problems if e is defined as a constant:
    inline static bool (*is_value)(const std::string&, char) = [](const std::string& prev, char c) {
         if (isdigit(c)) return true;
        if (prev.empty()) return false;
        if (c == '.' && prev.find('.') == std::string::npos) return true;
        if (tolower(c) == 'e' && prev.find('e') == std::string::npos && prev.find('E') == std::string::npos) return true;
        if ((c == '+' || c == '-') && tolower(prev.back()) == 'e') return true;
        return false;
    };
    */

    Expr(Expr&&) = delete; // Disables the move constructor

    // Constructor: Initializes from a std::string and removes spaces
    Expr(std::string s) : left(nullptr), right(nullptr), op(0) {
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
        std::istringstream str(s);
        left = new Expr(str, SUMS);
    }

    // Constructor: Deep copy an expression
    Expr(const Expr& expr) 
        : value(expr.value), op(expr.op), name(expr.name) {
        left = expr.left ? new Expr(*expr.left) : nullptr;
        right = expr.right ? new Expr(*expr.right) : nullptr;
    }

    // Constructor: An empty expression
    Expr() : left(nullptr), right(nullptr), value(0.0), op(0) {}

    // Destructor: Clean up nodes
    ~Expr() {
        delete left;
        delete right;
    }

    Expr& operator=(const Expr& other) {
        if (this != &other) { // Self-assignment check
            delete left;
            delete right;

            value = other.value;
            op = other.op;
            name = other.name;

            left = other.left ? new Expr(*other.left) : nullptr;
            right = other.right ? new Expr(*other.right) : nullptr;
        }
        return *this;
    }

    // Evaluate the expression with given variable substitutions
    T operator()(const std::map<std::string, T>& vars) const {
        // Debugging:
        // if (std::string("+-*/^").find(op) != std::string::npos) {
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
                if (left->right == nullptr)
                    throw std::invalid_argument("Error: Function '" + name + "' expects two arguments.");
                return funcs2[name]((*left->left)(vars), (*left->right)(vars));
            }
            if (vars.find(name) != vars.end()) {
                return vars.at(name);
            }
            throw std::invalid_argument("Error: Variable '" + name + "' is undefined.");
        }

        return left ? (*left)(vars) : value;
    }
};