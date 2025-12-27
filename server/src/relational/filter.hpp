//this is a filter operation that filters the 
//scanner/joiner output to a specific predicate
#include "operations.hpp"


class Filter : public Operator {

public:
    Filter();


    bool next() override;

};