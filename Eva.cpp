/*
 * Eva Interpreter
 */
#include <iostream>
#include <cassert>

/*
 * Expr Node
 *
 * val - Simplest form of expression.
 * subExpr - Complex expression that can be evaluated.
 */
template <typename T>
struct Expr {
  T val;
  std::vector<Expr*> subExpr;

 public:
  T getVal();
  void setVal(T val);

  std::vector<Expr<T>*> getSubExpr();

  static bool isNumber(Expr<T>* expr);
  static bool isString(Expr<T>* expr);

  explicit Expr(T val, std::vector<Expr*> subExpr = {});
};

// Checks if the expression value is an integral type for self-evaluation.
template<typename T>
bool Expr<T>::isNumber(Expr<T>* expr) {
  if (!expr->getSubExpr().empty())
    return false;
  return std::is_integral<T>::value;
}

// Checks if the expression value is a string type for self-evaluation.
template<typename T>
bool Expr<T>::isString(Expr<T>* expr) {
  if (!expr->getSubExpr().empty())
    return false;
  return std::is_same<decltype(expr->getVal()), std::string>::value;
}

// Get the value of the expression.
template<typename T>
T Expr<T>::getVal() {
  return val;
}

// Set the value of the expression.
template<typename T>
void Expr<T>::setVal(T val) {
  this->val = val;
}

// Get the sub expression for this expression.
template<typename T>
std::vector<Expr<T>*> Expr<T>::getSubExpr() {
  return subExpr;
}

// Expr constructor with an optional parameter for sub expression.
template<typename T>
Expr<T>::Expr(T val, std::vector<Expr *> subExpr) {
  this->val = val;
  this->subExpr = subExpr;
}

class Eva {
 public:

  // Evaluates result for a complex expression.
  template<typename T>
  T eval(Expr<T>* expr);
};

// Recursively checks the simplest form of expression
// for each complex expression and applys the binary operator
// between to find the result.
template<typename T>
T Eva::eval(Expr<T>* expr) {
  try {
    if (Expr<T>::isNumber(expr))
      return expr->getVal();
    if (Expr<T>::isString(expr))
      return expr->getVal();

    if (!expr->getSubExpr().empty()) {
      for (auto &x: expr->getSubExpr()) {

      }
    }
  } catch (...) {
    std::cerr << "Unimplemented: " << expr << "\n";
  }
}

// Temporary Tests
int main() {
  Eva eva;

  auto *numberExpr = new Expr<int>(1);
  assert(eva.eval(numberExpr) == 1);

  auto *stringExpr = new Expr<std::string>("1");
  assert(eva.eval(stringExpr) == "1");

  std::cout << "All assertions passed!";
}