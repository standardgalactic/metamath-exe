/*****************************************************************************/
/*        Copyright (C) 2011  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*
mmvstr.h - VMS-BASIC variable length string library routines header
This is an emulation of the string functions available in VMS BASIC.
*/

/******************************************************************************

Variable-length string handler
------------------------------

     This collection of string-handling functions emulate most of the
string functions of VMS BASIC.  The objects manipulated by these
functions are strings of a special type called 'vstring' which have no
pre-defined upper length limit but are dynamically allocated and
deallocated as needed.  To use the vstring functions within a program,
all vstrings must be initially set to the null string when declared or
before first used, for example:

        vstring string1 = "";
        vstring stringArray[] = {"", "", ""};

        vstring bigArray[100][10]; /- Must be initialized before using -/
        int i, j;
        for (i = 0; i < 100; i++)
          for (j = 0; j < 10; j++)
            bigArray[i][j] = ""; /- Initialize -/


     After initialization, vstrings should be assigned with the 'let(&'
function only; for example the statements

        let(&string1, "abc");
        let(&string1, string2);
        let(&string1, left(string2, 3));

all assign the second argument to 'string1'.  The 'let(&' function must
_not_ be used to initialize a vstring the first time.

     Any local vstrings in a function must be deallocated before returning
from the function, otherwise there will be memory leakage and eventual
memory overflow.  To deallocate, assign the vstring to "" with 'let(&':

        void abc(void) {
          vstring xyz = "";
          ...
          let(&xyz, "");
        }

     The 'cat' function emulates the '+' concatenation operator in BASIC.
It has a variable number of arguments, and the last argument should always
be NULL.  For example,

        let(&string1,cat("abc","def",NULL));

assigns "abcdef" to 'string1'.  Warning: 0 will work instead of NULL on the
VAX but not on the Macintosh, so always use NULL.

     All other functions are generally used exactly like their BASIC
equivalents.  For example, the BASIC statement

        let string1$=left$("def",len(right$("xxx",2)))+"ghi"+string2$

is emulated in c as

        let(&string1,cat(left("def",len(right("xxx",2))),"ghi",string2,NULL));

Note that ANSII c does not allow "$" as part of an identifier
name, so the names in c have had the "$" suffix removed.

     The string arguments of the vstring functions may be either standard c
strings or vstrings (except that the first argument of the 'let(&' function
must be a vstring).  The standard c string functions may use vstrings or
vstring functions as their string arguments, as long as the vstring variable
itself (which is a char * pointer) is not modified and no attempt is made to
increase the length of a vstring.  Caution must be excercised when
assigning standard c string pointers to vstrings or the results of
vstring functions, as the memory space may be deallocated when the
'let(&' function is next executed.  For example,

        char *stdstr; /- A standard c string pointer -/
         ...
        stdstr=left("abc",2);

will assign "ab" to 'stdstr', but this assignment will be lost when the
next 'let(&' function is executed.  To be safe, use 'strcpy':

        char stdstr1[80]; /- A fixed length standard c string -/
         ...
        strcpy(stdstr1,left("abc",2));

Here, of course, the user must ensure that the string copied to 'stdstr1'
does not exceed 79 characters in length.

     The vstring functions allocate temporary memory whenever they are called.
This temporary memory is deallocated whenever a 'let(&' assignment is
made.  The user should be aware of this when using vstring functions
outside of 'let(&' assignments; for example

        for (i=0; i<10000; i++)
          print2("%s\n",left(string1,70));

will allocate another 70 bytes or so of memory each pass through the loop.
If necessary, dummy 'let(&' assignments can be made periodically to clear
this temporary memory:

        for (i=0; i<10000; i++)
          {
          print2("%s\n",left(string1,70));
          let(&dummy,"");
          }

It should be noted that the 'linput' function assigns its target string
with 'let(&' and thus has the same effect as 'let(&'.

******************************************************************************/


#ifndef METAMATH_MMVSTR_H_
#define METAMATH_MMVSTR_H_

#include <stdio.h>

/* A vstring is like a C string, but it contains a control block allowing
for memory allocation. New vstrings should always be constructed from the
`vstringdef` macro. */
typedef char* vstring;

/* A vstring is allocated in temporary storage. These strings will be deallocated
  after the next call to `let`.

  A `temp_vstring` should never be used as the first argument of a `let`.
  This code is INCORRECT:

    temp_vstring foo = left("foobar", 3);
    let(&foo, "bar"); // this will cause a double free

  It is okay (and quite common) to use a temp_vstring as the second argument,
  however. It is best not to hold on to the value, though, because the `let`
  will free it. This code is INCORRECT:

    vstringdef(x);
    temp_vstring foobar = cat("foo", "bar");
    let(&x, foobar); // frees foobar
    let(&x, foobar); // dangling reference

  There is a converse problem when `temp_vstring`s are used without `let`:

    for (int i = 0; i < 100000; i++) {
      vstringdef(x);
      if (strlen(space(i)) == 99999) break;
    }

  We don't need to deallocate the string returned by `space()` directly,
  because it returns a `temp_vstring`, but because there is no `let` in
  this function, we end up allocating a bunch of temporaries and
  effectively get a memory leak. (There is space for about 100
  temporaries so this loop will cause a crash.) To solve this problem,
  we can either use a dummy `let()` statement in the loop, or call
  `freeTempAlloc` directly:

    for (int i = 0; i < 100000; i++) {
      vstringdef(x);
      if (strlen(space(i)) == 99999) break;
      freeTempAlloc();
    }

*/
typedef vstring temp_vstring;

#define vstringdef(x) vstring x = ""

/* Emulation of BASIC string assignment */
/* This function must ALWAYS be called to make assignment to
   a vstring in order for the memory cleanup routines, etc.
   to work properly.  A new vstring should be initialized to "" (the empty string),
   and the 'vstringdef' macro handles creation of such variables. */
/* `source` must not point into `target` (but this is unlikely to arise if
   `source` is calculated using `temp_vstring` operations from `target`). */
void let(vstring *target, const char *source);

/* Emulation of BASIC string concatenation - last argument MUST be NULL */
/* vstring cat(vstring string1, ..., stringN, NULL); */
/* e.g. 'let(&abc, cat("Hello", " ", left("worldx", 5), "!", NULL);' */
/* Also the first string must not be `NULL`, i.e. `cat(NULL)` alone is invalid. */
temp_vstring cat(const char * string1, ...);

/* Emulation of BASIC linput (line input) statement; returns NULL if EOF */
/* Note that linput assigns target string with let(&target,...) */
  /*
    BASIC:  linput "what";a$
    c:      linput(NULL,"what?",&a);

    BASIC:  linput #1,a$                        (error trap on EOF)
    c:      if (!linput(file1,NULL,&a)) break;  (break on EOF)

  */
/* returns whether a (possibly empty) line was successfully read */
int linput(FILE *stream, const char *ask, vstring *target);

/* Emulation of BASIC string functions */
/* Indices are 1-based */
temp_vstring seg(const char *sin, long p1, long p2);
temp_vstring mid(const char *sin, long p, long l);
temp_vstring left(const char *sin, long n);
temp_vstring right(const char *sin, long n);
temp_vstring edit(const char *sin, long control);
temp_vstring space(long n);
temp_vstring string(long n, char c);
temp_vstring chr(long n);
temp_vstring xlate(const char *sin, const char *table);
temp_vstring date(void);
temp_vstring time_(void);
temp_vstring num(double x);
temp_vstring num1(double x);
temp_vstring str(double x);
long len(const char *s);
long instr(long start, const char *string1, const char *string2);
long rinstr(const char *string1, const char *string2);
long ascii_(const char *c);
double val(const char *s);

/* Emulation of Progress 4GL string functions */
temp_vstring entry(long element, const char *list);
long lookup(const char *expression, const char *list);
long numEntries(const char *list);
long entryPosition(long element, const char *list);

/* Routines may/may not be written (lowest priority):
vstring place$(vstring sout);
vstring prod$(vstring sout);
vstring quo$(vstring sout);
*/

/******* Special purpose routines for better
      memory allocation (use with caution) *******/

extern long g_tempAllocStackTop;   /* Top of stack for tempAlloc functon */
extern long g_startTempAllocStack; /* Where to start freeing temporary allocation
    when let() is called (normally 0, except for nested vstring functions) */

/* Make string have temporary allocation to be released by next let().
  This function effectively changes the type of `s`
  from `vstring` (an owned pointer) to `temp_vstring` (a temporary to be
  freed by the next `let()`). See `temp_vstring` for information on what
  you can do with temporary strings.
  In particular, after makeTempAlloc() is called, the vstring may NOT be
  assigned again with let(). */
void makeTempAlloc(vstring s);    /* Make string have temporary allocation to be
                                    released by next let() */
#endif /* METAMATH_MMVSTR_H_ */