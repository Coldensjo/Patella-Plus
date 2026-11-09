/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */

/*
 * Tibia client version (Patella): Henry Spencer's regexp of 1986, as
 * compiled into Tibia.exe 7.72 (regcomp 0x50fa40, regexec 0x50f360; used by
 * Status.cpp for the link check of copied text). The client's changes are
 * marked "Client change". regsub.c and regerror.c are not part of the client.
 */
#ifndef REGEXP_H
#define REGEXP_H

#define NSUBEXP  10
typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

/*
 * Client change: prototypes, usable from the client's C++ code, instead of
 * the K&R declarations
 *	extern regexp *regcomp();
 *	extern int regexec();
 *	extern void regsub();
 *	extern void regerror();
 * regsub is not compiled into the client, and regerror is regexp.c's own
 * (it calls regerrorhandler, which the client never sets).
 */
#ifdef __cplusplus
extern "C" {
#endif
extern regexp *regcomp(char *exp);
extern int regexec(regexp *prog, char *string);
extern void (*regerrorhandler)(char *s);	/* 0x71f034 */
#ifdef __cplusplus
}
#endif

#endif
