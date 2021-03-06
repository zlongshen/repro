// Purpose: Try to address issue with needing to explicitly overload Eigen::VectorXd AutoDiffVecXd

// Alejandro mentioned this here: https://forum.kde.org/viewtopic.php?f=74&t=133499&p=358803

/* <example from="drake-distro:5729940:drake/solvers/constraint.h:89">
// If trying to templatize these, errors about ambiguity occur
  void Eval(const Eigen::Ref<const Eigen::VectorXd>& x, ...) const ...
  void Eval(const Eigen::Ref<const AutoDiffVecXd>& x, ...) const ...
   </example> */

#include <iostream>
#include <string>
using std::cout;
using std::endl;
using std::string;


/* <snippet from="drake-distro:5729940:drake/common/eigen_autodiff_types.h"> */
#include <Eigen/Dense>
#include <unsupported/Eigen/AutoDiff>

/// An autodiff variable with `num_vars` partials.
template <int num_vars>
using AutoDiffd = Eigen::AutoDiffScalar<Eigen::Matrix<double, num_vars, 1> >;

/// A vector of `rows` autodiff variables, each with `num_vars` partials.
template <int num_vars, int rows>
using AutoDiffVecd = Eigen::Matrix<AutoDiffd<num_vars>, rows, 1>;

/// A dynamic-sized vector of autodiff variables, each with a dynamic-sized
/// vector of partials.
typedef AutoDiffVecd<Eigen::Dynamic, Eigen::Dynamic> AutoDiffVecXd;
/* </snippet> */


auto vec_ref_explicit(const Eigen::Ref<const Eigen::VectorXd> &x) {
    return string("Eigen::VectorXd");
}
auto vec_ref_explicit(const Eigen::Ref<const AutoDiffVecXd> &x) {
    return string("AutoDiffVecXd");
}

template<typename Vector>
struct vec_trait { Vector invalid; };
template<>
struct vec_trait<Eigen::VectorXd> {
    static const char* name() { return "Eigen::VectorXd"; };
};
template<>
struct vec_trait<AutoDiffVecXd> {
    static const char* name() { return "AutoDiffVecXd"; };
};

// Does not work
template<typename Vector>
auto vec_ref_template_ref(const Eigen::Ref<const Vector> &x) {
    return string("Ref ") + vec_trait<Vector>::name();
}
// template<>
// auto vec_ref_template<Eigen::VectorXd>(const Eigen::Ref<const Eigen::VectorXd> &x);
/*
// Does not work
template<typename Derived>
auto vec_ref_template(const Eigen::Ref<const Eigen::DenseBase<Derived>> &x) {
    return string("templated ") + vec_trait<Eigen::DenseBase<Derived>>::name;
}
*/
// Works, but not worthwhile?
template<typename Derived>
auto vec_ref_template(const Eigen::MatrixBase<Derived> &x) {
    // https://forum.kde.org/viewtopic.php?f=74&t=117572&p=313833
    typedef typename std::remove_cv<typename std::remove_reference<decltype(x.derived())>::type>::type Vector;
    return vec_ref_template_ref<Vector>(x);
}

#define PRINT(x) #x ": " << (x) << endl

int main() {
    Eigen::VectorXd x;
    AutoDiffVecXd x_taylor;

    cout
        << PRINT(vec_ref_explicit(x))
        << PRINT(vec_ref_explicit(x_taylor))
        << PRINT(vec_ref_template(x))
        << PRINT(vec_ref_template(x_taylor));
        // << PRINT(vec_ref_template(x_taylor));
    return 0;
}
