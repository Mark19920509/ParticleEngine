
#ifndef UTILITY
#define UTILITY

#include <iostream> 
#include <sstream> 

//#ifndef RELEASE
    #define DEBUG_OUT( s ) {std::wostringstream os_;    os_ << s;   OutputDebugStringW( os_.str().c_str() );} 
//#else //RELEASE
//    #define DEBUG_OUT( s )
//#endif //RELEASE

#define UNUSED_PARAMETER( p ) {  p; }


#endif // UTILITY