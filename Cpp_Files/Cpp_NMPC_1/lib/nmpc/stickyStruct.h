//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// stickyStruct.h
//
// Code generation for function 'stickyStruct'
//

#ifndef STICKYSTRUCT_H
#define STICKYSTRUCT_H

// Include files
#include "anonymous_function.h"
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Definitions
namespace coder {
namespace internal {
class stickyStruct {
public:
  b_anonymous_function value;
};

class b_stickyStruct {
public:
  anonymous_function value;
  stickyStruct next;
};

class c_stickyStruct {
public:
  b_stickyStruct next;
};

class d_stickyStruct {
public:
  c_stickyStruct next;
};

class e_stickyStruct {
public:
  d_stickyStruct next;
};

class f_stickyStruct {
public:
  e_stickyStruct next;
};

class g_stickyStruct {
public:
  f_stickyStruct next;
};

class h_stickyStruct {
public:
  g_stickyStruct next;
};

class i_stickyStruct {
public:
  h_stickyStruct next;
};

} // namespace internal
} // namespace coder

#endif
// End of code generation (stickyStruct.h)
