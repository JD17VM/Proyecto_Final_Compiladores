#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "utils.h"

void PrintArgumentsInfo()
{
    std::cout << "Los argumentos que se deben poner son:\n";
    std::cout << "1. Nombre del archivo MD\n";
    std::cout << "2. Nombre del archivo de Salida\n";
}

size_t ReadLine(const std::string &fileBuffer, std::string &lineBuffer, size_t fileOffset)
{
    size_t length = 0;
    while (fileOffset + length < fileBuffer.size() && fileBuffer[fileOffset + length] != '\n')
    {
        ++length;
    }
    lineBuffer = fileBuffer.substr(fileOffset, length);
    return length + 1; // include '\n'
}

int EatLeadingSpaces(const std::string &line)
{
    int result = 0;
    while (result < line.size() && line[result] == ' ')
    {
        ++result;
    }
    return result;
}

void IndentFile(std::ofstream &file, int level)
{
    for (int i = 0; i < level; ++i)
    {
        file << "    ";
    }
}

void ProcessMarkdown(std::ofstream &outputFile, const std::string &line, int &lineCursor)
{
    while (lineCursor < line.length())
    {
        if (line[lineCursor] == '*' && line[lineCursor + 1] == '*')
        {
            outputFile << "\\textbf{";
            lineCursor += 2;
            while (lineCursor < line.length() && !(line[lineCursor] == '*' && line[lineCursor + 1] == '*'))
            {
                outputFile << line[lineCursor];
                ++lineCursor;
            }
            outputFile << "}";
            lineCursor += 2;
        }
        else if (line[lineCursor] == '*')
        {
            outputFile << "\\textit{";
            ++lineCursor;
            while (lineCursor < line.length() && line[lineCursor] != '*')
            {
                outputFile << line[lineCursor];
                ++lineCursor;
            }
            outputFile << "}";
            ++lineCursor;
        }
        else
        {
            outputFile << line[lineCursor];
            ++lineCursor;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Provided too few arguments.\n";
        PrintArgumentsInfo();
        return -1;
    }
    else if (argc > 3)
    {
        std::cerr << "Provided too many arguments.\n";
        PrintArgumentsInfo();
        return -1;
    }

    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];

    std::ifstream inputFile(inputFileName);
    if (!inputFile)
    {
        std::cerr << "Failed to open file: " << inputFileName << ".\n";
        return -1;
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string inputFileText = buffer.str();
    inputFile.close();

    if (inputFileText.empty())
    {
        std::cerr << "Input file size is 0.\n";
        return -1;
    }

    std::ofstream outputFile(outputFileName);
    if (!outputFile)
    {
        std::cerr << "Failed to create file: " << outputFileName << "\n";
        return -1;
    }

    defer { outputFile.close(); };

    size_t inputFileCursor = 0;
    int currentIndentationLevel = 0;
    int previousIndentationLevel = -1;
    bool listOpened = false;
    bool isItemize = true;
    int openedLists = 0;

    while (inputFileCursor < inputFileText.size())
    {
        std::string line;
        size_t lineLength = ReadLine(inputFileText, line, inputFileCursor);
        inputFileCursor += lineLength;

        if (line.empty())
        {
            outputFile << "\n";
            continue;
        }

        int lineCursor = EatLeadingSpaces(line);
        currentIndentationLevel = lineCursor / 4;
        bool skipLine = false;

        while (lineCursor < line.length() && !skipLine)
        {
            char currentChar = line[lineCursor];
            skipLine = false;
            switch (currentChar)
            {
                case '#':
                {
                    skipLine = true;
                    if (lineCursor == 0)
                    {
                        if (line[lineCursor + 1] == '#' && line[lineCursor + 2] == '#' &&
                            (line[lineCursor + 3] == ' ' || (line[lineCursor + 3] == '*' && line[lineCursor + 4] == ' ')))
                        {
                            bool unnumbered = line[lineCursor + 3] == '*';
                            outputFile << "\\subsubsection" << (unnumbered ? "*" : "") << "{" << line.substr(lineCursor + 4 + unnumbered) << "}\n";
                        }
                        else if (line[lineCursor + 1] == '#' &&
                                 (line[lineCursor + 2] == ' ' || (line[lineCursor + 2] == '*' && line[lineCursor + 3] == ' ')))
                        {
                            bool unnumbered = line[lineCursor + 2] == '*';
                            outputFile << "\\subsection" << (unnumbered ? "*" : "") << "{" << line.substr(lineCursor + 3 + unnumbered) << "}\n";
                        }
                        else if (line[lineCursor + 1] == ' ' || (line[lineCursor + 1] == '*' && line[lineCursor + 2] == ' '))
                        {
                            bool unnumbered = line[lineCursor + 1] == '*';
                            outputFile << "\\section" << (unnumbered ? "*" : "") << "{" << line.substr(lineCursor + 2 + unnumbered) << "}\n";
                        }
                    }
                    else
                    {
                        skipLine = false;
                        outputFile << currentChar;
                    }
                }
                break;

                case '*':
                {
                    if (currentIndentationLevel == lineCursor / 4 && line[lineCursor + 1] != '*') // first character in line
                    {
                        isItemize = true;
                        if (line[lineCursor + 1] == '&')
                        {
                            isItemize = false;
                            ++lineCursor;
                        }

                        if (!listOpened)
                        {
                            IndentFile(outputFile, currentIndentationLevel);
                            outputFile << (isItemize ? "\\begin{itemize}\n" : "\\begin{enumerate}\n");
                            listOpened = true;
                            outputFile << "\\item ";
                            lineCursor++;
                            ProcessMarkdown(outputFile, line, lineCursor);
                            outputFile << "\n";
                            skipLine = true;
                        }
                        else
                        {
                            if (currentIndentationLevel > previousIndentationLevel)
                            {
                                IndentFile(outputFile, currentIndentationLevel);
                                outputFile << (isItemize ? "\\begin{itemize}\n" : "\\begin{enumerate}\n");
                                ++openedLists;
                            }
                            else if (currentIndentationLevel < previousIndentationLevel)
                            {
                                IndentFile(outputFile, previousIndentationLevel);
                                outputFile << (isItemize ? "\\end{itemize}\n" : "\\end{enumerate}\n");
                                --openedLists;
                            }
                            IndentFile(outputFile, currentIndentationLevel);
                            outputFile << "\\item ";
                            lineCursor++;
                            ProcessMarkdown(outputFile, line, lineCursor);
                            outputFile << "\n";
                            skipLine = true;
                        }
                    }
                    else
                    {
                        ProcessMarkdown(outputFile, line, lineCursor);
                    }
                }
                break;

                case '_':
                {
                    if (lineCursor == 0 || line[lineCursor - 1] == ' ')
                    {
                        outputFile << "\\emph{";
                        ++lineCursor;
                        while (lineCursor < line.length() && line[lineCursor] != '_')
                        {
                            outputFile << line[lineCursor];
                            ++lineCursor;
                        }
                        outputFile << "}";
                        ++lineCursor;
                    }
                    else
                    {
                        outputFile << currentChar;
                    }
                }
                break;

                case '@':
                {
                    if (lineCursor == 0)
                    {
                        outputFile << "\\begin{figure}[h]\n\\centering\n\\includegraphics[width=\\textwidth]{";

                        std::string nameBuffer;
                        ++lineCursor;
                        int start = lineCursor;
                        while (lineCursor < line.length() && line[lineCursor] != ' ')
                        {
                            outputFile << line[lineCursor];
                            nameBuffer += line[lineCursor];
                            ++lineCursor;
                        }
                        ++lineCursor;
                        outputFile << "}\n\\caption{" << line.substr(lineCursor) << "}\n\\label{" << nameBuffer << "}\n\\end{figure}\n";
                        skipLine = true;
                    }
                    else
                    {
                        outputFile << "\\ref{";
                        ++lineCursor;
                        while (lineCursor < line.length() && (line[lineCursor] == '_' ||
                               (line[lineCursor] >= 'a' && line[lineCursor] <= 'z')))
                        {
                            outputFile << line[lineCursor];
                            ++lineCursor;
                        }
                        outputFile << "}" << line[lineCursor];
                    }
                }
                break;

                default:
                {
                    outputFile << currentChar;
                }
                break;
            }

            ++lineCursor;
        }
        previousIndentationLevel = currentIndentationLevel;
    }

    // Close any open lists
    if (listOpened)
    {
        for (int i = 0; i < openedLists + 1; ++i)
        {
            IndentFile(outputFile, openedLists - i);
            outputFile << (isItemize ? "\\end{itemize}\n\n" : "\\end{enumerate}\n\n");
        }
    }

    std::cout << "File " << outputFileName << " generated successfully!\n";
    return 0;
}
