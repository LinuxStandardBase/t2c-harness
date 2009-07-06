// This header file is provided only to make the transition of the tests 
// to newer versions of T2C easier. 
//
// The tests should include neither t2c.h nor t2c_api.h or t2c_tet_support.h
// explicitly but it is sometimes necessary to use TRACE/TRACE0 in some 
// auxiliary source files. These files should include t2c_api.h to be 
// compatible with T2C v2 or later.

#ifndef _T2C_API_H_INCLUDED_
#define _T2C_API_H_INCLUDED_

#include "t2c.h"
#include "t2c_tet_support.h"
    
#endif //_T2C_API_H_INCLUDED_
