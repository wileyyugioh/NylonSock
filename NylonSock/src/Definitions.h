//
//  Definitions.h
//  NylonSock
//
//  Created by Wiley Yu(AN) on 1/26/16.
//  Copyright © 2016 Wiley Yu. All rights reserved.
//

#ifndef Definitions_h

#if defined(unix) || defined(__unix__) || defined(__unix)
#define PLAT_UNIX

#elif defined(__APPLE__)
#define PLAT_APPLE

#elif defined(_WIN32)
#define PLAT_WIN
#endif

#if defined(PLAT_UNIX) || defined(PLAT_APPLE)
#define UNIX_HEADER
#endif

#define Definitions_h


#endif /* Definitions_h */
