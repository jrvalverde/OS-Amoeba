/*	@(#)fold.c	1.1	91/04/24 17:34:45 */
/*  fold - folds long lines		Author:Terrence W. Holm */

/*  Usage:  fold  [ -width ]  [ file ... ]  */

#include <stdio.h>
#include <stdlib.h>

#define  TAB		8
#define  DEFAULT_WIDTH  80


main( argc, argv )
  int   argc;
  char *argv[];

  {
  int  width = DEFAULT_WIDTH;
  int  i;
  FILE *f;

  if ( argc > 1  &&  argv[1][0] == '-' )
    {
    if ( (width = atoi( &argv[1][1] )) <= 0 )
	{
	fprintf( stderr, "Bad number for fold\n" );
  	exit( 1 );
	}

    --argc;
    ++argv;
    }

  if ( argc == 1 )
    Fold( stdin, width );
  else
    for ( i = 1;  i < argc;  ++i )
      {
      if ( (f = fopen( argv[i], "r" )) == NULL )
        {
        perror( argv[i] );
        exit( 1 );
        }

      Fold( f, width );
      fclose( f );
      }

  exit( 0 );
  }




int column = 0;		/*  Current column, retained between files  */


Fold( f, width )
  FILE *f;
  int  width;

  {
  int c;

  while ( (c = getc( f )) != EOF )
    {
    if ( c == '\t' )
      column = ( column / TAB + 1 ) * TAB;
    else if ( c == '\b' )
      column = column > 0 ? column - 1 : 0;
    else if ( c == '\n'  ||  c == '\r' )
      column = 0;
    else
      ++column;

    if ( column > width )
      {
      putchar( '\n' );
      column = c == '\t' ? TAB : 1;
      }

    putchar( c );
    }
  }
