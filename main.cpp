#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/utils.h"

void PrintArgumentsInfo()
{
    printf( "The arguments should be:\n" );
    printf( "1. Name of the Markdown file to convert\n" );
    printf( "2. Output file name\n" );
}

int ReadLine( char *fileBuffer, char *lineBuffer, int fileOffset )
{
    int length = 0;
    while ( fileBuffer[ fileOffset + length ] != '\n' )
    {
        ++length;
    }

    ++length; // include '\n'
    memcpy( lineBuffer, &fileBuffer[ fileOffset ], length );
    return length;
}

int EatLeadingSpaces( char *line )
{
    int result = 0;
    while ( line[ result ] == ' ' )
    {
        ++result;
    }
    return result;
}

inline void IndentFile( FILE *file, int level )
{
    for ( int i = 0; i < level; ++i )
    {
        fprintf( file, "    " );
    }
}

int main( int argc, char **argv )
{
    if ( argc < 3 )
    {
        printf( "Provided too few arguments.\n" );
        PrintArgumentsInfo();
        return -1;
    }
    else if ( argc > 3 )
    {
        printf( "Provided too many arguments.\n" );
        PrintArgumentsInfo();
        return -1;
    }

    char *inputFileName = argv[ 1 ];
    char *outputFileName = argv[ 2 ];

    FILE *inputFile;
    fopen_s( &inputFile, inputFileName, "rt" );

    if ( !inputFile )
    {
        printf( "Failed to open file: %s.\n", inputFileName );
        return -1;
    }

    fseek( inputFile, 0, SEEK_END );
    int inputFileSize = ( int ) ftell( inputFile );
    fseek( inputFile, 0, SEEK_SET );

    if ( inputFileSize == 0 )
    {
        printf( "Input file size is 0.\n" );
        fclose( inputFile );
        return -1;
    }

    char *inputFileText = ( char * ) malloc( ( inputFileSize + 1 ) * sizeof( char ) );
    defer { free( inputFileText ); };
    int countRead = ( int ) fread( inputFileText, sizeof( char ), inputFileSize, inputFile );

    if ( countRead < inputFileSize )
    {
        inputFileSize = countRead;
        inputFileText = ( char * ) realloc( inputFileText, countRead + 1 );
    }

    inputFileText[ countRead ] = '\0';
    fclose( inputFile );

    FILE *outputFile;
    fopen_s( &outputFile, outputFileName, "w" );

    if ( !outputFile )
    {
        printf( "Failed to create file: %s\n", outputFileName );
        return -1;
    }

    defer { fclose( outputFile ); };

    char *lineBuffer = ( char * ) malloc( 200 * sizeof( char ) );
    defer { free( lineBuffer ); };

    int inputFileCursor = 0;
    int lineLength = -1;
    int lineCount = 0;
    int currentIndentationLevel = 0;
    int previousIndentationLevel = -1;
    bool listOpened = false;
    bool isItemize = true;
    int openedLists = 0;

    fprintf( outputFile, "\\documentclass{article} \n\\usepackage[utf8]{inputenc} \n\\title{} \n\\date\n" );
    fprintf( outputFile, "\\begin{document} \n\n" );
    while ( inputFileCursor < inputFileSize )
    {
        lineLength = ReadLine( inputFileText, lineBuffer, inputFileCursor );
        inputFileCursor += lineLength;
        lineBuffer[ lineLength ] = '\0';
        ++lineCount;
        // printf( "%s", lineBuffer );

        int lineCursor = EatLeadingSpaces( lineBuffer );
        currentIndentationLevel = lineCursor / 4;
        // printf( "Line: %d, leading spaces: %d\n", lineCount, lineCursor );
        bool skipLine = false;
        while ( lineBuffer[ lineCursor ] && !skipLine )
        {
            char currentChar = lineBuffer[ lineCursor ];
            skipLine = false;
            switch ( currentChar )
            {
                case '\n':
                {
                    if ( lineLength == 1 && listOpened )
                    {
                        for ( int i = 0; i < openedLists + 1; ++i )
                        {
                            IndentFile( outputFile, openedLists - i );
                            fprintf( outputFile, isItemize ? "\\end{itemize}\n\n" : "\\end{enumerate}\n\n" );
                        }
                        listOpened = false;
                        openedLists = 0;
                    }
                    else
                    {
                        fprintf( outputFile, "%c", currentChar );
                    }
                }
                break;

                case '#':
                {
                    skipLine = true;
                    if ( lineCursor == 0 )
                    {
                        lineBuffer[ lineLength - 1 ] = '\0';
                        if ( lineBuffer[ lineCursor + 1 ] == '#' && lineBuffer[ lineCursor + 2 ] == '#' &&
                             ( lineBuffer[ lineCursor + 3 ] == ' ' || ( lineBuffer[ lineCursor + 3 ] == '*' &&
                                                                        lineBuffer[ lineCursor + 4 ] == ' ' ) ) )
                        {
                            bool unnumbered = lineBuffer[ lineCursor + 3 ] == '*';
                            fprintf( outputFile, "\\subsubsection%s{%s}\n", unnumbered ? "*" : "", &lineBuffer[ lineCursor + 4 + unnumbered ] );
                        }
                        else if ( lineBuffer[ lineCursor + 1 ] == '#' &&
                                  ( lineBuffer[ lineCursor + 2 ] == ' ' || ( lineBuffer[ lineCursor + 2 ] == '*' &&
                                                                             lineBuffer[ lineCursor + 3 ] == ' ' ) ) )
                        {
                            bool unnumbered = lineBuffer[ lineCursor + 2 ] == '*';
                            fprintf( outputFile, "\\subsection%s{%s}\n", unnumbered ? "*" : "", &lineBuffer[ lineCursor + 3 + unnumbered ] );
                        }
                        else if ( lineBuffer[ lineCursor + 1 ] == ' ' || ( lineBuffer[ lineCursor + 1 ] == '*' &&
                                                                           lineBuffer[ lineCursor + 2 ] == ' ' ) )
                        {
                            bool unnumbered = lineBuffer[ lineCursor + 1 ] == '*';
                            fprintf( outputFile, "\\section%s{%s}\n", unnumbered ? "*" : "", &lineBuffer[ lineCursor + 2 + unnumbered ] );
                        }
                    }
                    else
                    {
                        skipLine = false;
                        lineBuffer[ lineLength - 1 ] = '\n';
                        fprintf( outputFile, "%c", currentChar );
                    }
                }
                break;

                case '-':
                {
                    if ( currentIndentationLevel == lineCursor / 4 && lineBuffer[ lineCursor + 1 ] != '-' ) // first character in line
                    {
                        isItemize = true;
                        if ( lineBuffer[ lineCursor + 1 ] == '&' )
                        {
                            isItemize = false;
                            ++lineCursor;
                        }

                        if ( !listOpened )
                        {
                            IndentFile( outputFile, currentIndentationLevel );
                            fprintf( outputFile, isItemize ? "\\begin{itemize}\n" : "\\begin{enumerate}\n" );
                            listOpened = true;
                            fprintf( outputFile, "\\item%s", &lineBuffer[ lineCursor + 1 ] );
                            skipLine = true;
                        }
                        else
                        {
                            if ( currentIndentationLevel > previousIndentationLevel )
                            {
                                IndentFile( outputFile, currentIndentationLevel );
                                fprintf( outputFile, isItemize ? "\\begin{itemize}\n" : "\\begin{enumerate}\n" );
                                ++openedLists;
                            }
                            else if ( currentIndentationLevel < previousIndentationLevel )
                            {
                                IndentFile( outputFile, previousIndentationLevel );
                                fprintf( outputFile, isItemize ? "\\end{itemize}\n" : "\\end{enumerate}\n" );
                                --openedLists;
                            }
                            IndentFile( outputFile, currentIndentationLevel );
                            fprintf( outputFile, "\\item%s", &lineBuffer[ lineCursor + 1 ] );
                            skipLine = true;
                        }
                    }
                    else if ( lineBuffer[ lineCursor + 1 ] == '-' )
                    {
                        fprintf( outputFile, "\\textbf{" );
                        lineCursor += 2;
                        while ( !( lineBuffer[ lineCursor ] == '-' && lineBuffer[ lineCursor + 1 ] == '-' ) )
                        {
                            fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                            ++lineCursor;
                        }
                        fprintf( outputFile, "}" );
                        ++lineCursor;
                    }
                    else
                    {
                        fprintf( outputFile, "%c", currentChar );
                    }
                }
                break;

                case '_':
                {
                    if ( lineCursor == 0 || lineBuffer[ lineCursor - 1 ] == ' ' )
                    {
                        fprintf( outputFile, "\\emph{" );
                        ++lineCursor;
                        while ( lineBuffer[ lineCursor ] != '_' )
                        {
                            fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                            ++lineCursor;
                        }
                        fprintf( outputFile, "}" );
                    }
                    else
                    {
                        fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                    }
                }
                break;

                case '@':
                {
                    if ( lineCursor == 0 )
                    {
                        fprintf( outputFile, "\\begin{figure}[h]\n\\centering\n\\" );

                        char nameBuffer[ 128 ] = { 0 };
                        ++lineCursor;
                        int start = lineCursor;
                        while ( lineBuffer[ lineCursor ] != ' ' )
                        {
                            fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                            nameBuffer[ lineCursor - start ] = lineBuffer[ lineCursor ];
                            ++lineCursor;
                        }
                        ++lineCursor;
                        lineBuffer[ lineLength - 1 ] = '\0';
                        fprintf( outputFile, "}\n\\includegraphics[width=\\textwidth]{%s}\n\\label{%s}\n\\end{figure}\n",
                                 &lineBuffer[ lineCursor ], nameBuffer );
                        skipLine = true;
                    }
                    else
                    {
                        fprintf( outputFile, "\\ref{" );
                        ++lineCursor;
                        while ( lineBuffer[ lineCursor ] == '_' ||
                                ( lineBuffer[ lineCursor ] >= 'a' && lineBuffer[ lineCursor <= 'z' ] ) )
                        {
                            fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                            ++lineCursor;
                        }
                        fprintf( outputFile, "}%c", lineBuffer[ lineCursor ] );
                    }
                }
                break;

                case '`':
                    if ( lineCursor == 0 )

                default:
                {
                    fprintf( outputFile, "%c", currentChar );
                }
                break;
            }

            ++lineCursor;
        }
        previousIndentationLevel = currentIndentationLevel;
    }

    fprintf( outputFile, "\n\n\\end{document} " );
    printf( "File %s generated successfully!\n", outputFileName );
    return 0;
}
