#ifndef FEATURES_RTREE_CLASSIFIER_H
#define FEATURES_RTREE_CLASSIFIER_H

#include "calonder_descriptor/randomized_tree.h"
#include "calonder_descriptor/signature.h"
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector_sparse.hpp>

namespace ublas = boost::numeric::ublas;

namespace features {

class RTreeClassifier
{
public:
  static const int DEFAULT_TREES = 50;
    
  RTreeClassifier();

  void train(std::vector<BaseKeypoint> const& base_set, Rng &rng,
             int num_trees = RTreeClassifier::DEFAULT_TREES,
             int depth = RandomizedTree::DEFAULT_DEPTH,
             int views = RandomizedTree::DEFAULT_VIEWS,
             size_t reduced_num_dim = RandomizedTree::DEFAULT_REDUCED_NUM_DIM);
  void train(std::vector<BaseKeypoint> const& base_set,
             Rng &rng, PatchGenerator &make_patch,
             int num_trees = RTreeClassifier::DEFAULT_TREES,
             int depth = RandomizedTree::DEFAULT_DEPTH,
             int views = RandomizedTree::DEFAULT_VIEWS,
             size_t reduced_num_dim = RandomizedTree::DEFAULT_REDUCED_NUM_DIM);

  DenseSignature getSignature(IplImage* patch);
  SparseSignature getSparseSignature(IplImage* patch) const;   // deprecated, to disappear soon
   
  inline int classes() { return classes_; }
  inline int original_num_classes() { return original_num_classes_; }
  
  float threshold() { return threshold_; }
  void setThreshold(float thres) { threshold_ = thres; }
  // TODO: make threshold arg independent of the number of classes and trees.
  
  void read(const char* file_name);
  void read(std::istream &is);
  void write(const char* file_name) const;
  void write(std::ostream &os) const;

private:
  int classes_;
  int original_num_classes_;
  float threshold_;
  std::vector<RandomizedTree> trees_;
};

} // namespace features

#endif
