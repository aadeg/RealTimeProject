#ifndef _UTILS_H_
#define _UTILS_H_

// #include <stdio.h>

// CHECK(expr, err_expr) runs expr and checks the returned value.
// If expr return a value not equal to 0, an error message is printed
// in stderr and err_expr is executed
//
// Credits: adaptation of code found at https://stackoverflow.com/a/6933170
/*
#define CHECK(expr, err_expr) { \
	int err = (expr); \
	if (err) { \
		fprintf(stderr, "Runtime error: %s returned %d at %s:%d", #expr, err, __FILE__, __LINE__); \
		err_expr; \
	} \
} \
*/

#define SUCCESS 0
#define ERROR_GENERIC -1

#endif