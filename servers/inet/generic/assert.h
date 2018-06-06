
#ifndef INET_ASSERT_H
#define INET_ASSERT_H

#if !NDEBUG

void bad_assertion(char *file, int line, char *what);
void bad_compare(char *file, int line, int lhs, char *what, int rhs);

#define mol_assert(x)	((void)(!(x) ? bad_assertion(__FILE__, __LINE__, \
			#x),0 : 0))
#define compare(a,t,b)	(!((a) t (b)) ? bad_compare(__FILE__, __LINE__, \
				(a), #a " " #t " " #b, (b)) : (void) 0)

#else /* NDEBUG */

#define mol_assert(x)		0
#define compare(a,t,b)		0

#endif /* NDEBUG */

#endif /* INET_ASSERT_H */

