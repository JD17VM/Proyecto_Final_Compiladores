include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

void printUsageInfo()
{
    cout << "Usage:\n";
    cout << "1. Input Markdown file name\n";
    cout << "2. Output file name\n";
}

int readLine(const string &fileBuffer, string &lineBuffer, int offset)
{
    int length = 0;
    while (fileBuffer[offset + length] != '\n')
    {
        ++length;
    }
    ++length; // include '\n'
    lineBuffer.assign(fileBuffer.substr(offset, length));
    return length;
}

int eatLeadingSpaces(string &line)
{
    int result = 0;
    while (line[result] == ' ')
    {
        ++result;
    }
    return result;
}

void indentFile(ofstream &file, int level)
{
    for (int i = 0; i < level; ++i)
    {
        file << "   ";
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Too few arguments provided.\n";
        printUsageInfo();
        return -1;
    }
    else if (argc > 3)
    {
        cout << "Too many arguments provided.\n";
        printUsageInfo();
        return -1;
    }

    string inputFileName(argv[1]);
    string archivoSalidaName(argv[2]);

    ifstream inputFile(inputFileName);
    if (!inputFile.is_open())
    {
        cout << "Failed to open file: " << inputFileName << endl;
        return -1;
    }

    inputFile.seekg(0, ios::end);
    int tamanio = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    if (tamanio == 0)
    {
        cout << "Input file is empty.\n";
        inputFile.close();
        return -1;
    }

    string archivoEntrada(tamanio, '\0');
    inputFile.read(&archivoEntrada[0], tamanio);
    inputFile.close();

    ofstream archivoSalida(archivoSalidaName);
    if (!archivoSalida.is_open())
    {
        cout << "Failed to create file: " << archivoSalidaName << endl;
        return -1;
    }

    string lineBuffer(200, '\0');
    int fileCursor = 0;
    int lineLength = 0;
    int lineCount = 0;
    int currentIndent = 0;
    int previousIndent = -1;
    bool isInList = false;
    bool isUsingBullets = true;
    int openedLists = 0;

    archivoSalida << "\\documentclass{article} \n\\usepackage[utf8]{inputenc} \n\\title{} \n\\date\n"
               << "\\begin{document} \n\n";

    while (fileCursor < tamanio)
    {
        lineLength = readLine(archivoEntrada, lineBuffer, fileCursor);
        fileCursor += lineLength;
        lineBuffer[lineLength - 1] = '\0';
        ++lineCount;

        int lineIndent = eatLeadingSpaces(lineBuffer);
        currentIndent = lineIndent / 4;

        bool skipLine = false;
        int charIndex = 0;
        while (lineBuffer[charIndex] && !skipLine)
        {
            char currentChar = lineBuffer[charIndex];
            switch (currentChar)
            {
            case '\n':
                if (lineLength == 1 && isInList)
                {
                    for (int i = 0; i < openedLists + 1; ++i)
                    {
                        indentFile(archivoSalida, openedLists - i);
                        archivoSalida << (isUsingBullets ? "\\end{itemize}\n\n" : "\\end{enumerate}\n\n");
                    }
                    isInList = false;
                    openedLists = 0;
                }
                else
                {
                    archivoSalida << currentChar;
                }
                break;
            case '#':
                skipLine = true;
                if (lineIndent == 0)
                {
                    lineBuffer[lineLength - 1] = '\0';
                    if (lineBuffer[charIndex + 1] == '#' && lineBuffer[charIndex + 2] == '#' &&

                        (lineBuffer[charIndex + 3] == ' ' || (lineBuffer[charIndex + 3] == '*' &&
                                                              lineBuffer[charIndex + 4] == ' ')))
                    {
                        bool isUnnumbered = lineBuffer[charIndex + 3] == '*';
                        archivoSalida << "\\subsubsection%s{%s}\n"
                                   << (isUnnumbered ? "*" : "") << &lineBuffer[charIndex + 4 + isUnnumbered];
                    }
                    else if (lineBuffer[charIndex + 1] == '#' &&
                             (lineBuffer[charIndex + 2] == ' ' || (lineBuffer[charIndex + 2] == '*' &&
                                                                   lineBuffer[charIndex + 3] == ' ')))
                    {
                        bool isUnnumbered = lineBuffer[charIndex + 2] == '*';
                        archivoSalida << "\\subsection%s{%s}\n"
                                   << (isUnnumbered ? "*" : "") << &lineBuffer[charIndex + 3 + isUnnumbered];
                    }
                    else if (lineBuffer[charIndex + 1] == ' ' || (lineBuffer[charIndex + 1] == '*' &&
                                                                  lineBuffer[charIndex + 2] == ' '))
                    {
                        bool isUnnumbered = lineBuffer[charIndex + 1] == '*';
                        archivoSalida << "\\section%s{%s}\n"
                                   << (isUnnumbered ? "*" : "") << &lineBuffer[charIndex + 2 + isUnnumbered];
                    }
                }
                else
                {
                    skipLine = false;
                    lineBuffer[lineLength - 1] = '\n';
                    archivoSalida << currentChar;
                }
                break;
            case '-':
                if (currentIndent == lineIndent / 4 && lineBuffer[charIndex + 1] != '-')
                {
                    isUsingBullets = true;
                    if (lineBuffer[charIndex + 1] == '&')
                    {
                        isUsingBullets = false;
                        ++charIndex;
                    }

                    if (!isInList)
                    {
                        indentFile(archivoSalida, currentIndent);
                        archivoSalida << (isUsingBullets ? "\\begin{itemize}\n" : "\\begin{enumerate}\n");
                        isInList = true;
                        archivoSalida << "\\item" << &lineBuffer[charIndex + 1];
                        skipLine = true;
                    }
                    else
                    {
                        if (currentIndent > previousIndent)
                        {
                            indentFile(archivoSalida, currentIndent);
                            archivoSalida << (isUsingBullets ? "\\begin{itemize}\n" : "\\begin{enumerate}\n");
                            ++openedLists;
                        }
                        else if (currentIndent < previousIndent)
                        {
                            indentFile(archivoSalida, previousIndent);
                            archivoSalida << (isUsingBullets ? "\\end{itemize}\n" : "\\end{enumerate}\n");
                            --openedLists;
                        }
                        indentFile(archivoSalida, currentIndent);
                        archivoSalida << "\\item" << &lineBuffer[charIndex + 1];
                        skipLine = true;
                    }
                }
                else if (lineBuffer[charIndex + 1] == '-')
                {
                    archivoSalida << "\\textbf{";
                    charIndex += 2;
                    while (!(lineBuffer[charIndex] == '-' && lineBuffer[charIndex + 1] == '-'))
                    {
                        archivoSalida << lineBuffer[charIndex];
                        ++charIndex;
                    }
                    archivoSalida << "}";
                    ++charIndex;
                }
                else
                {
                    archivoSalida << currentChar;
                }
                break;
            case '_':
                if (charIndex == 0 || lineBuffer[charIndex - 1] == ' ')
                {
                    archivoSalida << "\\emph{";
                    ++charIndex;
                    while (lineBuffer[charIndex] != '_')
                    {
                        archivoSalida << lineBuffer[charIndex];
                        ++charIndex;
                    }
                    archivoSalida << "}";
                }
                else
                {
                    archivoSalida << currentChar;
                }
                break;
            case '@':
                if (charIndex == 0)
                {
                    archivoSalida << "\\begin{figure}[h]\n\\centering\\n\\";

                    char nameBuffer[128] = {0};
                    ++charIndex;
                    int start = charIndex;
                    while (lineBuffer[charIndex] != ' ')
                    {
                        archivoSalida << lineBuffer[charIndex];
                        nameBuffer[charIndex - start] = lineBuffer[charIndex];
                        ++charIndex;
                    }
                    ++charIndex;
                    lineBuffer[lineLength - 1] = '\0';
            archivoSalida << "}\n\\includegraphics
            (width=\\textwidth]{
                        % s}\n\\label{
                        % s}\n\\end{
                        figure}\n"
                         << &lineBuffer[charIndex] << nameBuffer << endl;
            skipLine = true;
                }
                else
                {
                    archivoSalida << "\\ref{";
                    ++charIndex;
                    while (lineBuffer[charIndex] == '_' ||
                           (lineBuffer[charIndex] >= 'a' && lineBuffer[charIndex] <= 'z'))
                    {
                        archivoSalida << lineBuffer[charIndex];
                        ++charIndex;
                    }
                    archivoSalida << "}%c" << lineBuffer[charIndex] << endl;
                }
                break;
            case '`':
                // Code block handling will go here
                if (charIndex == 0)
                {
                    // Inline code block
                    archivoSalida << "\\texttt{";
                    ++charIndex;
                    while (lineBuffer[charIndex] != '`')
                    {
                        archivoSalida << lineBuffer[charIndex];
                        ++charIndex;
                    }
                    archivoSalida << "}" << endl;
                    skipLine = true;
                }
                else
                {
                    // Fenced code block
                    archivoSalida << "```" << endl;
                    ++charIndex;
                    while (lineBuffer[charIndex] != '`' || lineBuffer[charIndex + 1] != '`')
                    {
                        archivoSalida << lineBuffer[charIndex];
                        ++charIndex;
                    }
                    archivoSalida << "```" << endl;
                    skipLine = true;
                    charIndex += 2;
                }
                break;
            default:
                archivoSalida << currentChar;
                break;
            }
            ++charIndex;
        }
        previousIndent = currentIndent;
    }

    archivoSalida << "\n\n\\end{document} " << endl;
    cout << "File " << archivoSalidaName << " generated successfully!\n";
    return 0;
}
